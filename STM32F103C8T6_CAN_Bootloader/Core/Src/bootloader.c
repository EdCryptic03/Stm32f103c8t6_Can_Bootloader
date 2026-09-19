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

#define HEADER_BASE 0x08004000UL
#define APP_BASE 0x08004400UL
#define APP_END 0x08010000UL
#define APP_MAGIC 0xB00710ADUL


volatile uint32_t g_write_addr;
volatile uint32_t g_image_len;
volatile uint32_t g_bytes_recv;

#define BOOT_WINDOW 2000000U

#define APP_DESC ((const volatile app_desc_t *)HEADER_BASE)

typedef void (*app_entry_t)(void);

typedef struct{
	uint32_t magic;
	uint32_t length;
	uint32_t crc;
} app_desc_t;


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



/*=====================================================
 * Standard CRC-32 , Currently matching with Python's zlib.crc32.
 * ===================================================== */
static uint32_t crc32_compute(const uint8_t *data, uint32_t len){

	uint32_t crc = 0xFFFFFFFFU;
	for(uint32_t i = 0;i < len; i++){
		crc ^= data[i];
		for(int b = 0;b < 8;b++){
			if(crc & 1U){
				crc = (crc >> 1) ^ 0xEDB88320U;
			} else {
				crc >>=1;
			}
		}
	}
	return crc ^ 0xFFFFFFFFU;
}

/*=============================================
 * BUILDING APP VALIDATION
 * ===========================================*/

static int app_is_valid(void){

	if(APP_DESC->magic != APP_MAGIC)
		return 0;

	uint32_t len = APP_DESC->length;
	if(len == 0 || len > (APP_END - APP_BASE))
		return 0;


	uint32_t sp = *(const uint32_t *)APP_BASE; // Initial Stack pointer
	uint32_t reset = *(const uint32_t *)(APP_BASE + 4U);
	if(sp < 0x20000000U || sp > 0x20005000U)
	return 0;
	if(reset < APP_BASE || reset >= APP_END)
	return 0;
	if((reset & 1U) == 0)
	return 0;

	if(crc32_compute((const uint8_t *)APP_BASE, len) != APP_DESC->crc)
		return 0;


	return 1;
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
			if(flash_erase_region(HEADER_BASE, APP_END) == FLASH_OK){
				bl_respond(BL_ACK);
			} else {
				bl_respond(BL_NACK);
			}
			flash_lock();
			break;

		case BL_CMD_END:
			uint32_t host_crc = rd_u32(&f->data[1]);
			uint32_t calc_crc = crc32_compute((const uint8_t *)APP_BASE, g_image_len);
			if(g_bytes_recv == g_image_len && host_crc == calc_crc){
				app_desc_t desc = {APP_MAGIC, g_image_len, calc_crc};
				flash_unlock();
				flash_program(HEADER_BASE, (const uint8_t *)&desc, sizeof(desc));
				flash_lock();
				bl_respond(BL_ACK);
			} else {
				bl_respond(BL_NACK);
			}
			break;

		case BL_CMD_GO:
			if(app_is_valid()){ // Checks if app is valid first otherwise it refuses to jump
				bl_respond(BL_ACK);
				jump_to_application();
			} else {
				bl_respond(BL_NACK);
			}
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

/* ++++++++++++++++++++++++++
 * Bootloader window Function
 * +++++++++++++++++++++++++++
 */
void bl_run(void){
	/*1. First we check if a host wants to talk to us.
	 * If it does then we stay and listen to the host that wants to update
	 * and become the bootloader.
	 * 2. If no host wants to talk , we check if the app is valid
	 * using app_is_valid() and jump to the application using jump_to_application() */

	can_frame_t rx;
	int stay = 0;

	for(uint32_t i = 0; i<BOOT_WINDOW;i++){
		if(can_receive(&rx) == 0){
			bl_handle_frame(&rx);
			stay = 1;
			break;
		}
	}


	if(!stay && app_is_valid())
		jump_to_application();


	while(1){
		if(can_receive(&rx) == 0)
			bl_handle_frame(&rx);
	}
}



