#ifndef _ISOTP_H
#define _ISOTP_H
#include "globals.h"
enum state {WAIT_FIRST_FRAME, RECEIVE_MSG};

//int ISOTPSendMsg(unsigned char* msgBuff, int buffLen);
int ISOTPSendMsg1(unsigned char* hdrBuff, unsigned char hdrLen, unsigned long /*unsigned char __farflash*/ msgBuff, int buffLen, char memtype);
void ISOTPSendSingleFrame(unsigned char* msgBuff, char buffLen);
void ISOTPReceiveSingleFrame(unsigned char* frameBuff, char *frameBuffLen);
char ISOTPReceiveMessage(unsigned char* blockBuff, int *msgLen, void (*ProcessBuffer)(unsigned char lastBlock, unsigned char bytesReceived));



void wdt_enable();
#endif