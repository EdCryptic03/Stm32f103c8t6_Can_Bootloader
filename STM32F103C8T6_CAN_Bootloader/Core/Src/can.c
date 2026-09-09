/*
 * can.c
 *
 *  Created on: 9 Sept 2026
 *      Author: Harshit Singh
 */

#include "can.h"
#include "stm32f1xx.h"

void can_init(void){

	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;
	RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;

	GPIOA->CRH &= ~((0xFU << 12) | (0xFU << 16));
	GPIOA->CRH |= (0x8U << 12);
	GPIOA->ODR |= (1U << 11);
	GPIOA->CRH |= (0xBU << 16);

	CAN1->MCR |= CAN_MCR_INRQ;
	CAN1->MCR &= ~CAN_MCR_SLEEP;
	while(!(CAN1->MSR & CAN_MSR_INAK)) {}




}
