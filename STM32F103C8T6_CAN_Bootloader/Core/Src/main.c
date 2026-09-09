#include <stm32f1xx.h>
#include "flash.h"
#include "can.h"

#define APP_ADDRESS 0x08004400UL


volatile int g_send_status;
volatile int g_recv_status;
volatile uint16_t g_rx_id;
volatile int g_can_test_pass;

typedef void (*app_entry_t)(void);


static void jump_to_application(void){

	uint32_t app_stack = *(volatile uint32_t *)(APP_ADDRESS);
	uint32_t app_reset = *(volatile uint32_t *)(APP_ADDRESS + 4U);

	__disable_irq();
	SCB->VTOR = APP_ADDRESS;
	__set_MSP(app_stack);

	app_entry_t app_entry = (app_entry_t)app_reset;
	__enable_irq();
	app_entry();
}

static void can_loopback_test(void){

	can_init();
	can_frame_t tx;
	tx.id = 0x123;
	tx.len = 4;
	tx.data[0] = 0xDE;
	tx.data[1] = 0xAD;
	tx.data[2] = 0xBE;
	tx.data[3] = 0xEF;

	g_send_status = can_send(&tx);

	can_frame_t rx;
	g_recv_status = -1;
	for(volatile uint32_t t = 0;t < 1000000U;t++){
		if(can_receive(&rx) == 0){
			g_recv_status = 0;
			break;
		}
	}

	if(g_recv_status == 0) {
		g_rx_id = rx.id;
		g_can_test_pass = (rx.id == 0x123 && rx.len == 4 &&
							rx.data[0] == 0xDE && rx.data[1] == 0xAD
							&& rx.data[2] == 0xBE && rx.data[3] == 0xEF);

	}



}

int main(void){
//	jump_to_application();
	can_loopback_test();
	while(1) { }
}
