import struct 
from conftest import ACK,NACK,stm32_crc


def make_valid_app(n=32):
    img = bytearray(struct.pack("<II", 0x20005000, 0x08004401))
    img += bytes(range(n-8))
    if len(img) % 8:
        img += b"\xFF" * (8 - len(img) % 8)
    return bytes(img)



def test_happy_path_and_acks(bl):
    assert bl.flash(make_valid_app()) == ACK 


def test_wrong_crc(bl):
    img = make_valid_app()
    assert bl.connect(len(img)) == ACK
    assert bl.erase() == ACK
    for i in range(0, len(img), 8):
        assert bl.data(img[i:i + 4]) == ACK
    assert bl.end(stm32_crc(img) ^ 0x1) == NACK 



def test_incomplete_image(bl):
    img = make_valid_app(32)
    assert bl.connect(len(img)) == ACK 
    assert bl.erase() == ACK 
    assert bl.data(img[:8]) == ACK 
    assert bl.end(stm32_crc(img)) == NACK 


def test_data_before_connect(bl):
    assert bl.data(bytes(8)) == NACK 

def test_data_before_erase(bl):
    assert bl.connect(32) == ACK 
    assert bl.data(bytes(8)) == NACK 

def test_connect_zero_length(bl):
    assert bl.connect(0) == NACK 


def test_data_overflow(bl):
    assert bl.connect(8) == ACK 
    assert bl.erase() == ACK 
    assert bl.data(bytes(8)) == ACK 
    assert bl.data(bytes(0)) == NACK 

def test_go_refuse(bl):
    bl.flash(bytes(range(32)))
    assert bl.go() == NACK
