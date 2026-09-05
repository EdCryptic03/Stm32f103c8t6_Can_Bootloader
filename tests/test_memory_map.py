import pytest 
from host import memory_map as mm 

def test_flash_region():

    total = mm.BOOTLOADER_SIZE + mm.APP_HEADER_SIZE + mm.APP_SIZE
    assert total == mm.FLASH_SIZE

def test_app_region():
    assert mm.APP_BASE == mm.APP_HEADER_END

def test_app_region():
    assert mm.is_in_app_region(mm.APP_BASE) == True
    assert mm.is_in_app_region(mm.FLASH_END) == False
    assert mm.is_in_app_region(mm.APP_HEADER_BASE) == False 
    assert mm.is_in_app_region(mm.BOOTLOADER_BASE) == False 
    assert mm.is_in_app_region(0x08010000) == False

def test_page_of_known_addresses():
    assert mm.page_of(mm.FLASH_BASE) == 0
    assert mm.page_of(mm.APP_HEADER_BASE) == 16
    assert mm.page_of(mm.APP_BASE) == 17
    assert mm.page_of(0x0800FC00) == 63


def test_page_of_unknown_addresses():
    with pytest.raises(ValueError):
        mm.page_of(0x20000000)
