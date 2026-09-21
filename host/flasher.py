""" CAN bootloader flasher for STM32F103C8T6.
Working protocol: CONNECT -> ERASE -> DATA.... -> END -> GO"""

import sys 
import time 
import struct 
import can 
import zlib

INTERFACE = "slcan"
CHANNEL = "COM4" # Set according to your device manager
BITRATE = 125000 # I've chosen this as default , it can be increased though


BL_ID_CMD = 0x100
BL_ID_DATA = 0x101
BL_ID_RESP = 0x102

BL_CMD_CONNECT = 0x01
BL_CMD_ERASE = 0x02
BL_CMD_END = 0x03
BL_CMD_GO = 0x04

BL_ACK = 0x00
BL_NACK = 0x01

ACK_TIMEOUT = 1.0 
ERASE_TIMEOUT = 5.0 

def wait_for_ack(bus, timeout):
    deadline = time.time() + timeout
    while time.time() < deadline:
        msg = bus.recv(timeout=deadline - time.time())
        if msg is None:
            break
        if msg.arbitration_id == BL_ID_RESP:
            return msg.data[0] == BL_ACK;
    return False


def send_and_wait(bus, arb_id, payload, timeout=ACK_TIMEOUT):
    msg = can.Message(arbitration_id = arb_id,
                    data=bytes(payload),
                    is_extended_id=False)
    
    bus.send(msg)
    return wait_for_ack(bus,timeout)

# I'm sending CONNECT over and over until the bootloader answers (catching the boot window)
def connect_with_retry(bus , length, timeout=10.0):
    payload = [BL_CMD_CONNECT] + list(struct.pack("<I", length))
    print("Knocking for bootloader, reset the board now...", flush=True)
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            bus.send(can.Message(arbitration_id=BL_ID_CMD, data=payload, is_extended_id=False))
        except can.CanError:
            pass
        msg = bus.recv(timeout=0.1)
        if msg is not None and msg.arbitration_id == BL_ID_RESP and msg.data[0] == BL_ACK:
            print("Connected")
            return True
        print(".", end="", flush=True)
    print(" timed out")
    return False


def flash_firmware(bus, image):

    if len(image) % 8 != 0:
        image += b"\xFF" * (8 - len(image) % 8)
    length = len(image)
    print(f"Image: {length} bytes ({length // 8} frames)")

    print("CONNECT....", end=" ", flush=True)
    if not connect_with_retry(bus, length):
        return fail("bootloader didn't respond : Reset not within boot window perhaps..")
    print("ACK")


    print("ERASE....", end=" ", flush=True)
    if not send_and_wait(bus, BL_ID_CMD, [BL_CMD_ERASE] + list(struct.pack("<I", length))):
        return fail("no ACK to CONNECT")
    print("ACK")


    print("DATA....", end=" ", flush=True)
    for i in range(0, length, 8):
        chunk = image[i:i + 8]
        if not send_and_wait(bus, BL_ID_DATA, chunk):
            return fail(f"no ACK at offset {i}")
        print(".", end="", flush=True)
    print("done")

    print("END...", end=" ", flush=True)
    crc = zlib.crc32(image) & 0xFFFFFFFF
    if not send_and_wait(bus, BL_ID_CMD, [BL_CMD_END] + list(struct.pack("<I", crc))):
        return fail("END rejected : CRC mismatch or incomplete image")
    print(f"ACK (crc=0x{crc:08X})")


    print("GO....", end=" ", flush=True)
    if not send_and_wait(bus, BL_ID_CMD, [BL_CMD_GO]):
        return fail("no ACK to GO")
    print("ACK - Application Launched")
    return True


def fail(reason):
    print(f"\nFAILED: {reason}")
    return False

def main():
    if len(sys.argv) != 2:
        print("usage: python flasher.py firmware.bin")
        sys.exit(1)
    
    with open(sys.argv[1], "rb") as f:
        image = f.read()
    
    bus = can.Bus(interface=INTERFACE, channel=CHANNEL, bitrate=BITRATE)
    try:
        ok = flash_firmware(bus, image)
    finally:
        bus.shutdown()
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
