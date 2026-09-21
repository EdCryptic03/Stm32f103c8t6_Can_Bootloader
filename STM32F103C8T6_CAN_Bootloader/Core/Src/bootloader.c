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

#define CORE_CLOCK_HZ 8000000U
#define BOOT_WINDOW_MS 10000U

#define HEADER_BASE 0x08004000UL
#define APP_BASE 0x08004400UL
#define APP_END 0x08010000UL
#define APP_MAGIC 0xB00710ADUL


volatile uint32_t g_write_addr;
volatile uint32_t g_image_len;
volatile uint32_t g_bytes_recv;

#define APP_DESC ((const volatile app_desc_t *)HEADER_BASE)

typedef void (*app_entry_t)(void);

typedef struct{
	uint32_t magic;
	uint32_t length;
	uint32_t crc;
} app_desc_t;

typedef enum {
	ST_IDLE, ST_CONNECTED, ST_ERASED
} bl_state_t;

static bl_state_t g_state = ST_IDLE;


/*=====================================================================================
 * We need a proper boot window with SysTick as the busy-count window is fragile
 * SysTick is a 24-bit down-counter tool built into the Cortex-M core.
 * We're going to set it to tick every 1ms and count real milliseconds, so "1.5s" is
 * exactly 1.5s regardless of compiler or optimization.
 * ==================================================================================*/
static void systick_start_1ms(void){

	SysTick->LOAD = (CORE_CLOCK_HZ / 1000U) - 1U;
	SysTick->VAL = 0U;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
}

static int boot_window(uint32_t ms, can_frame_t *rx){
	systick_start_1ms();
	uint32_t elapsed = 0;
	while (elapsed < ms){
		if(can_receive(rx) == 0)
			return 1;
		if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
			elapsed++;
	}
	return 0;
}

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
	SysTick->CTRL = 0;
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
 * Standard CRC-32 , Working with the Standard CRC peripheral of ST32F1 seires
 * Reference Manual RM0008 (CRC Calculation Unit)
 * ===================================================== */
static uint32_t crc32_compute(const uint8_t *data, uint32_t len){

		RCC->AHBENR |= RCC_AHBENR_CRCEN;
		CRC->CR = CRC_CR_RESET;  //CRC_DR reset offset : 0xFFFFFFFF
		for(uint32_t i=0;i<len;i += 4){
			CRC->DR = *(const uint32_t *)(data + i);
			}
		return CRC->DR;
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
 *
 * Tightened each case in bl_handle_frame
 * ================================================================ */

void bl_handle_frame(const can_frame_t *f){
	if(f->id == BL_ID_CMD){
		switch(f->data[0]){  // Byte 0 :opcode

		case BL_CMD_CONNECT:{
			uint32_t len = rd_u32(&f->data[1]);
			if(len == 0 || len > (APP_END - APP_BASE)){
				bl_respond(BL_NACK);
				break;
			}
			g_image_len = len;
			g_write_addr = APP_BASE;
			g_bytes_recv = 0;
			g_state = ST_CONNECTED;
			bl_respond(BL_ACK);
			break;
		}

		case BL_CMD_ERASE:
			if(g_state != ST_CONNECTED) {
				bl_respond(BL_NACK);
				break; // Must be CONNECTED FIRST
			}
			flash_unlock();
			if(flash_erase_region(HEADER_BASE, APP_END) == FLASH_OK){
				g_state = ST_ERASED;
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
		/* Adding new conditions before starting the existing BL_ID_DATA branch
		 * First condition checks if there is any DATA before ERASE
		 * Second condition checks if the data received plus the new frame is more
		 * than what the host promised.
		 * Third condition checks if flash address plus the new frame is more than the
		 * address of the app region */
		if(g_state != ST_ERASED){
			bl_respond(BL_NACK);
			return;
		}
		if(g_bytes_recv + f->len > g_image_len){
			bl_respond(BL_NACK);
			return;
		}
		if(g_write_addr + f->len > APP_END){
			bl_respond(BL_NACK);
			return;
		}

		// Continuing the BL_ID_DATA branch
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
	/*1. We check if a host wants to talk. For that we create a knocking window.
	 * If the host knocks the window , we service it.
	 * 2. If no host wants to talk (quiet window), we check if the app is valid or not
	 * using app_is_valid() and jump to the application using jump_to_application().
	 * Otherwise , the bootloader. */

	can_frame_t rx;

	if(boot_window(BOOT_WINDOW_MS, &rx)){
		bl_handle_frame(&rx);
	} else if (app_is_valid()){
		jump_to_application();
	}

	while(1){
		if (can_receive(&rx) == 0)
			bl_handle_frame(&rx);
	}
}



