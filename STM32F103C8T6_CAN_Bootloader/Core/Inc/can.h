/*
 * can.h
 *
 *  Created on: 9 Sept 2026
 *      Author: Harshit Singh
 */

#ifndef CAN_H
#define CAN_H
#include <stdint.h>

typedef struct {
	uint16_t id;
	uint8_t len;
	uint8_t data[8];
} can_frame_t;

void can_init(void);
int can_send(const can_frame_t *f);
int can_receive(can_frame_t *f);

#endif
