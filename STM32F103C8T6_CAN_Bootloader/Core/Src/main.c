#include <stm32f1xx.h>
#include "can.h"
#include "bootloader.h"


#define APP_ADDRESS 0x08004400UL

int main(void){

	can_init();
	can_frame_t rx;
	while(1){
		if(can_receive(&rx) == 0){
			bl_handle_frame(&rx);
		}
	}
}

