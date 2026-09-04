#include <stm32f1xx.h>

int main(void){

	RCC -> APB2ENR |= RCC_APB2ENR_IOPCEN;

	GPIOC -> CRH &= ~(0xFU << 20);
	GPIOC -> CRH |= (0x2U << 20);

	while(1){

		GPIOC -> ODR ^= (1U << 13);
		for(volatile uint32_t i = 0; i < 300000; i++);
	}


}
