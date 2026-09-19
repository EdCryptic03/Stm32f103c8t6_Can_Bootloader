#include <stm32f1xx.h>
#include "can.h"
#include "bootloader.h"


#define APP_ADDRESS 0x08004400UL

int main(void){

	can_init();
	bl_run(); // boot window , either app or bootloader
	while(1){
	}
}

