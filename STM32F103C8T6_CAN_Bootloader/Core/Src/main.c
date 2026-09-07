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
// _______________________________________________End of debugger function

int main(void){

//	flash_test();
	jump_to_application();
	while(1) { }
}
