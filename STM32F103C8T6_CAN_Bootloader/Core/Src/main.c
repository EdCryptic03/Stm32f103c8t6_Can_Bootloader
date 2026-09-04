#include <stm32f1xx.h>

#define APP_ADDRESS 0x08004400UL

typedef void (*app_entry_t)(void);

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

int main(void){

	jump_to_application();
	while(1) { }
}
