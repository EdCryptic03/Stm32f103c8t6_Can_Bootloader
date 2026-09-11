HOST		    BOOTLOADER
| CONNECT (len) ----> | prepare, write-pr = APP_BASE 
| <----- ACK ----->   | 
| ERASE ------->      | erase pages 17-63
(flash_erase_region)

| <-----ACK------> | (takes a moment: ACK When done) 
|  DATA [8 bytes] -----> | write 8 bytes 

(flash_program) 
| <----- ACK ------> | .....ready for next 
| DATA[8 bytes] -----> | write, advance ptr
| <------ ACK -----> | 

| .....repeat until all 'len' bytes sent.....
| END (crc32) -----> | compute CRC, compare 

| <------ ACK/NACK ----> | match ? accept : reject 
| GO   -------> | stamp header magic 

jump_to_applicatio()
