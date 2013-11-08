#ifndef _CAN_H
#define _CAN_H

void CAN_init();
void Tmpp();
//int Read_CAN_mess();
void Write_CAN_buff(unsigned char *Buff, unsigned char BuffLen, unsigned long CAN_ID);



#endif