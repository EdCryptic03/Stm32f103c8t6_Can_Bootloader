#include <stm32f1xx.h>
#include "can.h"
#include "bootloader.h"


#define APP_ADDRESS 0x08004400UL

int main(void){

	RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
	GPIOC->CRH &= ~(0xFU << 20);
	GPIOC->CRH |= (0x2U << 20);
	GPIOC->ODR &= ~(1U << 13);

	can_init();


	can_frame_t tx;
	tx.id = 0x123;
	tx.len = 2;
	tx.data[0] = 0xDE;
	tx.data[1] = 0xAD;

	while(1){
		can_send(&tx);
		GPIOC->ODR ^= (1U << 13);
		for(volatile int i = 0;i<200000;i++) {}
	}

//
////	can_init();
////	can_frame_t rx;
////	while(1){
////		if(can_receive(&rx) == 0){
////			bl_handle_frame(&rx);
//		}
//	}
}

