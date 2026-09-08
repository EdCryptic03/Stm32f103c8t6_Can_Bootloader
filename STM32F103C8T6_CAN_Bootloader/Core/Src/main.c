#include <stm32f1xx.h>
#include "flash.h"

#define APP_ADDRESS 0x08004400UL

#define TEST_ADDR 0x0800FC00U

typedef void (*app_entry_t)(void);

// Declaring test global variables. I've used g in the naming so its easy to recognize
volatile flash_status_t g_erase_status;
volatile uint16_t g_after_erase;
volatile flash_status_t g_prog_status;
volatile uint16_t g_after_prog;
volatile int g_flash_test_pass;

volatile flash_status_t g_erase2_status;
volatile flash_status_t g_prog2_status;
volatile int g_buffer_test_pass;

static const uint8_t test_buf[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};



static void jump_to_application(void){

	uint32_t app_stack = *(volatile uint32_t *)(APP_ADDRESS);
	uint32_t app_reset = *(volatile uint32_t *)(APP_ADDRESS + 4U);

	__disable_irq();
	SCB->VTOR;
	__set_MSP(app_stack);

	app_entry_t app_entry = (app_entry_t)app_reset;
	__enable_irq();
	app_entry();
}

/* Self-Debugger test function to verify FPEC unlock sequence, page erase,half-word program,
 * re-lock, with an address guard that refuses the bootloader's own pages (0-15)
 */
static void flash_test(void){

	flash_unlock();
	g_erase_status = flash_erase_page(TEST_ADDR);
	g_after_erase = *(volatile uint16_t *)TEST_ADDR;
	g_prog_status = flash_halfword_program(TEST_ADDR, 0xABCD);
	g_after_prog = *(volatile uint16_t *)TEST_ADDR;
	flash_lock();

	if(g_erase_status == FLASH_OK && g_after_erase == 0xFFFF && g_prog_status == FLASH_OK
			&& g_after_prog == 0XABCD){
		g_flash_test_pass = 1;
	}
}
// _______________________________________________End of test debugger function

static void flash_buffer_test(void){

	flash_unlock();
	g_erase2_status = flash_erase_region(TEST_ADDR, TEST_ADDR+FLASH_PAGE_SIZE);
	g_prog2_status = flash_program(sizeof(test_buf), test_buf, TEST_ADDR);
	flash_lock();

	if(g_erase2_status == FLASH_OK && g_prog2_status == FLASH_OK){
		 g_buffer_test_pass = 1;
	}

	for(uint32_t i = 0; i < sizeof(test_buf);i++){
		if(*(volatile uint8_t *)(TEST_ADDR + i) != test_buf[i]){
			g_buffer_test_pass = 0;
		}
	}
}

int main(void){

//	flash_test();
//	jump_to_application();
	flash_buffer_test();
	while(1) { }
}
