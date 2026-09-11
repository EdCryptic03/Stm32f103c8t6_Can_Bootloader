/*
 * bootloader.h
 *
 *  Created on: 11 Sept 2026
 *      Author: Harshit Singh
 */

#ifndef BOOTLOADER_H
#define BOOTLOADER_H
#include "can.h"

#define BL_ID_CMD 0x100
#define BL_ID_DATA 0x101
#define BL_ID_RESP 0x102

#define BL_CMD_CONNECT 0x01
#define BL_CMD_ERASE 0x02
#define BL_CMD_END 0x03
#define BL_CMD_GO 0x04

#define BL_ACK 0x00
#define BL_NACK 0x01

void bl_handle_frame(const can_frame_t *f);



#endif /* INC_BOOTLOADER_H_ */
