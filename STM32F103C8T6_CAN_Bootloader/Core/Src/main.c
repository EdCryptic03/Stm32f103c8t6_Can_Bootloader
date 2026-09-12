#include <stm32f1xx.h>
#include "can.h"
#include "bootloader.h"


#define APP_ADDRESS 0x08004400UL

volatile int g_proto_test_pass;

static void protocol_test(void){

	can_init();
	can_frame_t f;

	f.id = BL_ID_CMD;
	f.len = 5;
	f.data[0] = BL_CMD_CONNECT;
	f.data[1] = 8;
	f.data[2] = 0;
	f.data[3] = 0;
	f.data[4] = 0;
	bl_handle_frame(&f);

	f.id = BL_ID_CMD;
	f.len = 1;
	f.data[0] = BL_CMD_ERASE;
	bl_handle_frame(&f);


	f.id = BL_ID_DATA;
	f.len = 8;
	for(int i = 0;i<8;i++)
	{
		f.data[i] = 0xA0 + i;
	}
	bl_handle_frame(&f);

	int ok = 1;
	for(int i = 0;i<8;i++){
		if(*(volatile uint8_t *)(APP_ADDRESS + i) != (0xA0 + i)){
			ok = 0;
		}
	}

	g_proto_test_pass = ok;
}


int main(void){

//	can_init();
//	can_frame_t rx;
	protocol_test();
	while(1){
//		if(can_receive(&rx) == 0){
//			bl_handle_frame(&rx);
		}
	}

