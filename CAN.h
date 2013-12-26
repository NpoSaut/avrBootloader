#ifndef _CAN_H
#define _CAN_H

void CAN_init();
void Tmpp();
//int Read_CAN_mess();
void Write_CAN_buff(unsigned char *Buff, unsigned char BuffLen, unsigned long CAN_ID);

#define FU_INIT (unsigned int)(0x66A8 >> 5)
#define FU_PROG (unsigned int)(0x66C8 >> 5)
#define FU_DEV  (unsigned int)(0x66E8 >> 5)

#endif