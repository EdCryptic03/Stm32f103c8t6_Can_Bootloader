/*
 * bootloader.c
 *
 *  Created on: 11 Sept 2026
 *      Author: Harshit Singh
 */

#include "can.h"
#include "bootloader.h"
#include "flash.h"
#include "stm32f1xx.h"

#define APP_BASE 0x08004400UL
#define APP_END 0x08010000UL

volatile uint32_t g_write_addr;
volatile uint32_t g_image_len;
volatile uint32_t g_bytes_recv;

typedef void (*app_entry_t)(void);

/*=====================================================
 * FUNCTION TO SEND ONE-BYTE ACK/NACK BACK TO THE HOST
 * ====================================================*/
static void bl_respond(uint8_t status){

	can_frame_t r;
	r.id = BL_ID_RESP;
	r.len = 1;
	r.data[0] = status;
	can_send(&r);
}

/* ============================================================
 * FUNCTION TO REBUILD 32 BIT NUMBER FROM 4 BYTES (IMAGE LENGTH)
 * =============================================================
 */
static uint32_t rd_u32(const uint8_t *p){

	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
			((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}


static void jump_to_application(void){
	uint32_t app_stack = *(volatile uint32_t *)(APP_BASE);
		uint32_t app_reset = *(volatile uint32_t *)(APP_BASE + 4U);

		__disable_irq();
		SCB->VTOR = APP_BASE;
		__set_MSP(app_stack);

		app_entry_t app_entry = (app_entry_t)app_reset;
		__enable_irq();
		app_entry();
	}

/*=================================================================
 * g_image_len = total image size
 * g_write_addr = start of the app
 * g_bytes_recv = set to zero because we didn't write anything yet
 * ================================================================ */

void bl_handle_frame(const can_frame_t *f){
	if(f->id == BL_ID_CMD){
		switch(f->data[0]){  // Byte 0 :opcode

		case BL_CMD_CONNECT:
			g_image_len = rd_u32(&f->data[1]);
			g_write_addr = APP_BASE;
			g_bytes_recv = 0;
			bl_respond(BL_ACK);
			break;

		case BL_CMD_ERASE:
			flash_unlock();
			if(flash_erase_region(APP_BASE, APP_END) == FLASH_OK){
				bl_respond(BL_ACK);
			} else {
				bl_respond(BL_NACK);
			}
			flash_lock();
			break;

		case BL_CMD_END:
			if(g_bytes_recv == g_image_len){
				bl_respond(BL_ACK);
			} else {
				bl_respond(BL_NACK);
			}
			break;

		case BL_CMD_GO:
			bl_respond(BL_ACK);
			jump_to_application();
			break;

		default: // in case of unknown command
			bl_respond(BL_NACK);
			break;
		}
	}
	else if(f->id == BL_ID_DATA) {
		flash_unlock();
		flash_status_t st = flash_program(g_write_addr, f->data, f->len);
		flash_lock();
		if(st == FLASH_OK){
			g_write_addr += f->len;
			g_bytes_recv += f->len; // counting what we stored
			bl_respond(BL_ACK);
		} else {
			bl_respond(BL_NACK);
		}
	}
}


