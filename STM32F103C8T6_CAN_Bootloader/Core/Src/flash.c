/*
 * flash.c
 *
 *  Created on: 7 Sept 2026
 *      Author: Harshit Singh
 */


#include "flash.h"
#include "stm32f1xx.h"

// Keys for unlocking FPEC (FLash Program/Erase Controller)
//#define FLASH_KEY1 0x45670123U
//#define FLASH_KEY2 0xCDEF89ABU

#define WRITABLE_START 0x08004000U
#define FLASH_END_ADDR 0x08010000U

#define FLASH_PAGE_SIZE 1024U

static void flash_wait_busy(void){

	while(FLASH->SR & FLASH_SR_BSY) {}
}


/* Functions for unlocking/locking FPEC using the flash control register
 * and the magic keys
 */
void flash_unlock(void){

	if(FLASH->CR & FLASH_CR_LOCK) {
		FLASH->KEYR = FLASH_KEY1;
		FLASH->KEYR = FLASH_KEY2;

	}
}

void flash_lock(void){

	FLASH->CR |= FLASH_CR_LOCK;
}

// End of the locking/unlocking functions
// ____________________________________

// Function for erasing page using reference manual procedure
flash_status_t flash_erase_page(uint32_t page_addr){

	if(page_addr < WRITABLE_START || page_addr >= FLASH_END_ADDR){
		return FLASH_ERR_RANGE;
	}

	flash_wait_busy();
	FLASH->CR |= FLASH_CR_PER;
	FLASH->AR = page_addr;
	FLASH->CR |= FLASH_CR_STRT;
	flash_wait_busy();
	FLASH->CR &= ~FLASH_CR_PER;

	if(FLASH->SR & FLASH_SR_WRPRTERR){
		FLASH->SR = FLASH_SR_WRPRTERR;
		return FLASH_ERR_WRP;
	}

	return FLASH_OK;
}

/* Function for writing the 16 bit or half word data into the flash
 * using reference manual procedure
*/
flash_status_t flash_halfword_program(uint32_t addr, uint16_t data){

	if(addr < WRITABLE_START || addr >= FLASH_END_ADDR){
		return FLASH_ERR_RANGE;
	}

	flash_wait_busy();
	FLASH->CR |= FLASH_CR_PG;
	*(volatile uint16_t*)addr = data;
	flash_wait_busy();
	FLASH->CR &= ~FLASH_CR_PG;

	if(FLASH->SR & FLASH_SR_PGERR){
		FLASH->SR = FLASH_SR_PGERR;
		return FLASH_ERR_PROG;
	}
	return FLASH_OK;
}

/*Function to erase the page region */
flash_status_t flash_erase_region(uint32_t start_addr, uint32_t end_addr){

	for(uint32_t addr = start_addr; addr < end_addr; addr += FLASH_PAGE_SIZE){
		flash_status_t status = flash_erase_page(addr);
			if(status != FLASH_OK){
				return status;
			}
	}

	return FLASH_OK;
}

/*buffer function to program a buffer into the flash memory in 16-bit halfwords */
flash_status_t flash_program(uint32_t addr, const uint8_t *data, uint32_t len){

	for(uint32_t i = 0; i < len; i+=2){
		uint16_t hw;
		if(i + 1 < len){
			hw = data[i] | (data[i+1] << 8);
		} else {
			hw = data[i] | (0xFF << 8);
		}
		flash_status_t status = flash_halfword_program(addr+i, hw);
		if(status != FLASH_OK){
			return status;
		}
	}
	return FLASH_OK;
}



