/*
 * can.c
 *
 *  Created on: 9 Sept 2026
 *      Author: Harshit Singh
 */

#include "can.h"
#include "stm32f1xx.h"

void can_init(void){

	// Clock Settings
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;
	RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;

	// GPIO Settings
	GPIOA->CRH &= ~((0xFU << 12) | (0xFU << 16));
	GPIOA->CRH |= (0x8U << 12);
	GPIOA->ODR |= (1U << 11);
	GPIOA->CRH |= (0xBU << 16);

	// CAN Settings (Activating CAN)
	CAN1->MCR |= CAN_MCR_INRQ;
	CAN1->MCR &= ~CAN_MCR_SLEEP;
	while(!(CAN1->MSR & CAN_MSR_INAK)) {}


	CAN1->BTR = (3U << CAN_BTR_BRP_Pos)
				| (12U << CAN_BTR_TS1_Pos)
				| (1U << CAN_BTR_TS2_Pos)
				| (0U << CAN_BTR_SJW_Pos)
				| CAN_BTR_LBKM;


	// Filter settings to receive everything in FIFO1

	CAN1->FMR |= CAN_FMR_FINIT;
	CAN1->FA1R &= ~(1U << 0);
	CAN1->FS1R |= (1U << 0);
	CAN1->FM1R &= ~(1U << 0);
	CAN1->sFilterRegister[0].FR1 = 0;
	CAN1->sFilterRegister[0].FR2 = 0;
	CAN1->FFA1R &= ~(1U << 0);
	CAN1->FA1R |= (1U << 0);
	CAN1->FMR &= ~CAN_FMR_FINIT;

	CAN1->MCR &= ~CAN_MCR_INRQ;
	while (CAN1->MSR & CAN_MSR_INAK) {}

}

int can_send(const can_frame_t *f){

	if((CAN1->TSR & CAN_TSR_TME0) == 0) {
		return -1;
	}

	CAN1->sTxMailBox[0].TIR = ((uint32_t)f->id << 21);

	CAN1->sTxMailBox[0].TDTR = ((uint32_t)f->len & 0xF);

	CAN1->sTxMailBox[0].TDLR = ((uint32_t)f -> data[0])
								| ((uint32_t)f -> data[1] << 8)
								| ((uint32_t)f -> data[2] << 16)
								| ((uint32_t)f -> data[3] << 24);

	CAN1->sTxMailBox[0].TDHR = ((uint32_t)f -> data[0]) |
								((uint32_t)f -> data[1] << 8)
								| ((uint32_t)f -> data[2] << 16)
								| ((uint32_t)f -> data[3] << 24);

	CAN1->sTxMailBox[0].TIR |= CAN_TI0R_TXRQ;
	return 0;

}

int can_receive(can_frame_t *f){

	if((CAN1->RF0R & CAN_RF0R_FMP0) == 0){
		return -1;
	}

	f->id = (uint16_t)((CAN1->sFIFOMailBox[0].RIR >> 21)&0x7FF);
	f->len = (uint16_t)(CAN1->sFIFOMailBox[0].RDTR & 0x0F);

	uint32_t lo = CAN1->sFIFOMailBox[0].RDLR;
	uint32_t hi = CAN1->sFIFOMailBox[0].RDHR;

	f->data[0] = (uint8_t)(lo);
	f->data[1] = (uint8_t)(lo >> 8);
	f->data[2] = (uint8_t)(lo >> 16);
	f->data[3] = (uint8_t)(lo >> 24);
	f->data[4] = (uint8_t)(hi);
	f->data[5] = (uint8_t)(hi >> 8);
	f->data[6] = (uint8_t)(hi >> 16);
	f->data[7] = (uint8_t)(hi >> 24);

	CAN1->RF0R |= CAN_RF0R_RFOM0;
	return 0;
}




