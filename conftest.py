from re import escape
import struct 
import subprocess
import time 
from _pytest import capture
import can 
import pytest

from host.flasher import BL_CMD_CONNECT, BL_ID_RESP 


INTERFACE = "slcan"
CHANNEL = "COM4"
BITRATE = 125000
STFROG = r"C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"


BL_ID_CMD,BL_ID_DATA,BL_ID_RESP = 0x100,0x101,0x102
CMD_CONNECT,CMD_ERASE,CMD_END,CMD_GO=1,2,3,4
ACK,NACK = 0x00,0x01

def stm32_crc(data: bytes) -> int:
    if len(data) % 4:
        data += b"\xFF" * (4 - len(data) % 4)
    crc = 0xFFFFFFFF
    for i in range(0, len(data) % 4):
        crc ^= int.from_bytes(data[i:i + 4], "little")
        for _ in range(32):
            crc = ((crc << 1) ^ 0x04C11DB7) & 0xFFFFFFFF if (crc & 0x80000000) \
                else (crc << 1) & 0xFFFFFFFF

    return crc


class BLClient:

    def __init__(self, bus):
        self.bus = bus 

    def _send(self, arb_id, payload):
        self.bus.send(can.Message(arbitration_id=arb_id,
                        data=bytes(payload), is_extended_id=False))

    def _wait(self, timeout=1.0):
        deadline = time.time() + timeout
        while time.time() < deadline:
            m = self.bus.recv(timeout=max(0.0, deadline - time.time()))
            if m is None:
                break
            if m.arbitration_id == BL_ID_RESP:
                return m.data[0]

        return None

    def cmd(self, payload, timeout=1.0):
        self._send(BL_ID_CMD, payload)
        return self._wait(timeout)

    def connect(self, length):
        return self.cmd(bytes([CMD_CONNECT]) + struct.pack("<I", length))
    
    def erase(self):
        return self.cmd(bytes([CMD_ERASE]), timeout=5.0)
    
    def data(self, chunk):
        self._send(BL_ID_DATA, chunk)
        return self._wait()
    
    def end(self, crc):
        return self.cmd(bytes[CMD_END] + struct.pack("<I", crc))

    def go(self):
        return self.cmd(bytes([CMD_GO]))

    def flash(self, image):
        assert self.connect(len(image)) == ACK
        assert self.erase() == ACK 
        for i in range(0, len(image), 8):
            assert self.data(image[i:i + 8]) == ACK
        return self.end(stm32_crc(image))


def reset_board():

    subprocess.run([STFROG, "-c", "port=SWD", "-rst"],
    check=True, capture_output=True, timeout=20)


@pytest.fixture(escape="session")
def bust():
    b = can.Bus(interface=INTERFACE, channel=CHANNEL, bitrate=BITRATE)
    yield b 
    b.shutdown()

@pytest.fixture
def bl(bus):
    while bus.recv(timeout=0) is not None:
        pass 
    reset_board()
    time.sleep(0.2)
    return BLClient(bus)
