/*
 * flash.h
 *
 *  Created on: 7 Sept 2026
 *      Author: Harshit Singh
 */

#ifndef FLASH_H
#define FLASH_H

#include <stdint.h>

typedef enum {

	FLASH_OK = 0,
	FLASH_ERR_RANGE,
	FLASH_ERR_PROG,
	FLASH_ERR_WRP
} flash_status_t;

void flash_unlock(void);
void flash_lock(void);
flash_status_t flash_erase_page(uint32_t page_addr);
flash_status_t flash_halfword_program(uint32_t addr, uint16_t data);
flash_status_t flash_erase_region(uint32_t start_addr, uint32_t end_addr);
flash_status_t flash_program(uint32_t len, const uint8_t *data, uint32_t addr);




#endif /* INC_FLASH_H_ */
