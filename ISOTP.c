
#include "isotp.h"
#include "CAN.h"
#include "FIFO.h"
#include "intrinsics.h"
#include "iocan128.h"
#include "flash.h"


#define SEPARATION_TIME 0
#define OUT_MSG_DESC 0x7E2

extern CANFIFO CANMsgBuff;
extern unsigned int times_ms[TIMES_COUNT];

/*----------------------- ��������������� �������   -------------------------*/

void wdt_enable()			/* Configure & run WDT */
{
	WDTCR |= (1<<WDCE)|(1<<WDE);
	WDTCR = (1<<WDE)|(1<<WDP2)|(1<<WDP1);	// ���������� ������ ����-���� ~2,0 �
}

/* ������������ ������ Flow Control*/

#pragma inline = forced
void MakeFlowControlPacket(unsigned char* buff, char flag, unsigned char blocksize, int septime)
{
  buff[0] = (0x30 + flag);
  buff[1] = blocksize;
  buff[2] = septime;
  for(char i=3; i<8; i++) buff[i] = 0;
}


/* ����� ������ Flow Control */
#pragma inline = forced
int ReceiveFlowControl(unsigned char* blocksize, char *septime)
{
  CANMesStruc CANMsg; 
  char FC_Received = 0;      // ���� "������ ���� Flow Control"
  while (!FC_Received) //������� ���� Flow Control
  {
     while (!FIFO_isEmpty(&CANMsgBuff))
     {
        FIFO_PullFromBuff(&CANMsgBuff, &CANMsg); //������� �� ������ ��������� 
        if (((CANMsg.MsgBody[0] & 0xF0)>>4) == 3) // ��������� ����� Flow Control
        {
            switch (CANMsg.MsgBody[0] &0x0F)
            {
              case 0:                   // Clear to send
                      *blocksize = CANMsg.MsgBody[1];
                      *septime   = CANMsg.MsgBody[2];
                      FC_Received = 1;
                      break;
              case 1:                   // Wait
                      break;
              case 2:                   // Overflow/Abort
                      return -1;
            }
            break;
        }
     }  
  }
  return 0;
}

/*------------ �������� �������, ������������ �� ������� ��������� ---------- */

/* �������� ��������� */
int ISOTPSendMsg1(unsigned char* hdrBuff, unsigned char hdrLen, unsigned long msgBuff, int buffLen, char memType)
/* ������� �������� ������ msgBuff ������ buffLen �� ISO TP*/
{
  if ((hdrLen<8)&&(buffLen==0))
  {
     ISOTPSendSingleFrame(hdrBuff, hdrLen);
     return 0;
  }
  //unsigned char __farflash* flashPtr;
  int sndBytesCnt; // ������� ������������ ����
  unsigned char blocksize;    // Block Size
  unsigned char MsgsBeforeFC=0;
  char sepTime;    // Separation Time
  unsigned char  outbuff[8]; // �����, ������� ����� ������������ ��� �������� CAN ���������
  char idx = 1;
  
  
  outbuff[0] = 0x10;         // � ���� 4-7 ����� ��� 1 (FirstFrame)
  outbuff[0] |=  (unsigned char)((buffLen + hdrLen)>>8);// ���� 0-3 - ������� 4 ���� ����� ������
  outbuff[1]  =  (unsigned char)(buffLen + hdrLen);// 2-� ���� CAN-������ - ��������� ���� ����� ������ 
  for (int i=0; i<6; i++)
    if (i>=hdrLen)
    {
     //flashPtr = (unsigned char __farflash*)(*msgBuff+i+hdrLen);
     if (memType)
       outbuff[i+2] = *((unsigned char __eeprom*)(msgBuff+i-hdrLen));//msgBuff[i-hdrLen];
      else
       outbuff[i+2] = *((unsigned char __farflash*)(msgBuff+i-hdrLen));//msgBuff[i-hdrLen];
    }
    else
     outbuff[i+2] = hdrBuff[i]; 
 
  Write_CAN_buff(outbuff, 8, OUT_MSG_DESC); // ���������� First Frame � CAN
  sndBytesCnt = 6;
  
  while (sndBytesCnt<buffLen+hdrLen) 
  {
    if (MsgsBeforeFC==0)
    {
       ReceiveFlowControl(&blocksize, &sepTime);
       MsgsBeforeFC = blocksize;
       
    }
    for (char i=1; i<8; i++)
      {
        if (sndBytesCnt>=hdrLen)
        {
         // flashPtr = *((unsigned char __farflash*)(*msgBuff+sndBytesCnt - hdrLen);
         if (memType)
          outbuff[i] =  *((unsigned char __eeprom*)(msgBuff+sndBytesCnt - hdrLen));//msgBuff[sndBytesCnt - hdrLen];
         else
          outbuff[i] =  *((unsigned char __farflash*)(msgBuff+sndBytesCnt - hdrLen));//msgBuff[sndBytesCnt - hdrLen];
        }
        else
          outbuff[i] = hdrBuff[sndBytesCnt];
        sndBytesCnt++;
      }
    
    outbuff[0] = 0x20 + idx;
    Write_CAN_buff(outbuff, 8, OUT_MSG_DESC); // ���������� Consecutive Frame � CAN
    MsgsBeforeFC--;
    idx++;
    idx&=0x0F;

  }
  return 0; 
}



/* �������� ���������� ����� Single Frame */
void ISOTPSendSingleFrame(unsigned char* msgBuff, char buffLen)
{
  /* ������� �������� Single Frame �� ISO TP. 
     msgBuff - ������������ ������, buffLen - ����� (1-7) */
  unsigned char outbuff[8]; // ����� ��� ������������� CAN-���������
  outbuff[0] = (buffLen & 0xF); // ���� 0-3 - ����� ������, ���� 4-7 - ��� ���� �����. ����� ��� ����� 0 (Single Frame)
  for (int i=0; i<(buffLen & 0xF); i++)
  {
    outbuff[i+1] = msgBuff[i];
  }
  Write_CAN_buff(outbuff, 8, OUT_MSG_DESC); // ���������� Single Frame
}

char ISOTPReceiveMessage(unsigned char* blockBuff, int *msgLen, void (*ProcessBuffer)(unsigned char lastBlock, unsigned char bytesReceived))
{
 
  CANMesStruc CANMsg; 
  unsigned char a[8] = {0,0,0,0,0,0,0,0};
  unsigned char blocklen;
  unsigned char state;
//  unsigned char blockBuff[256]; // �������� ����� ��� �������� ������, �������� �� ISO TP 

  
  state = WAIT_FIRST_FRAME;
  
  unsigned char CANFrameCnt;
  int msgBufPos;
  int rcvByteCnt;
  char msgIdx;

  for (;;)
  {
  // if (ms_counter>1000) state = WAIT_FIRST_FRAME;
  // PORTE = CANMsgBuff.head;
   
    while (!FIFO_isEmpty(&CANMsgBuff))
     {
        
        FIFO_PullFromBuff(&CANMsgBuff, &CANMsg); //������� �� ������ ��������� 
        switch (state)  //� ������������ � ����������� �� ���������, � ������� ���������
        {
          
          case WAIT_FIRST_FRAME: //�������� First Frame
             //  PORTE = 0xFA;
               if ((CANMsg.MsgBody[0] & 0xF0)== 0) // ������� Single frame
                   {
                     times_ms[PROG_CONDITION_DELAY_IDX] = 0;
                     *msgLen = CANMsg.MsgBody[0] & 0x0F; // ����� Single Frame
                     for (char i=0; i<(CANMsg.MsgBody[0] & 0x0F); i++)
                       blockBuff[i] = CANMsg.MsgBody[i+1];
                     ProcessBuffer(1, CANMsg.MsgBody[0]);
                     return 0;
                   }     
               
               if ((CANMsg.MsgBody[0] & 0xF0) == 0x10) //��������� First Frame
                  {
                    times_ms[PROG_CONDITION_DELAY_IDX] = 0;
                    *msgLen=(CANMsg.MsgBody[0] &0xF)<<8; 
                    *msgLen+=CANMsg.MsgBody[1];         //�������� ����� ����� ���������
                    CANFrameCnt = 1;
                    msgBufPos  = 0;
                    rcvByteCnt  = 0;
                    
                    for (unsigned char i=2; i<8; i++)
                    {
                      blockBuff[msgBufPos] = CANMsg.MsgBody[i];
                      msgBufPos++;
                      rcvByteCnt++;
                    }
                    blocklen = 35; 
                    MakeFlowControlPacket(a, 0, blocklen, SEPARATION_TIME); // ��������� Flow Control
                    FIFO_init(&CANMsgBuff);
                    Write_CAN_buff(a, 8, OUT_MSG_DESC);           // ���������� Flow Control
                    state = RECEIVE_MSG; //��������� � ��������� ������ ���������
                    times_ms[ISOTP_TIMEOUT_IDX] = 0;
                    msgIdx = 0x0;
                  }
              break;
               
               
          case RECEIVE_MSG: //����� ���������
                times_ms[ISOTP_TIMEOUT_IDX] = 0;
                times_ms[PROG_CONDITION_DELAY_IDX] = 0;
                if ((CANMsg.MsgBody[0]&0x0F) != ((msgIdx + 1)&0x0F))
                {
                    MakeFlowControlPacket(a, 2, 0, 0); // ��������� Flow Control
                    Write_CAN_buff(a, 8, OUT_MSG_DESC);           // ���������� Flow Control
                    /*wdt_enable();
                    for(;;) asm("nop");*/
                    return 1;
                }
                
                msgIdx = (msgIdx+1) & 0x0F;
                for (unsigned char i=1; i<8; i++)
                {   
                    blockBuff[msgBufPos] = CANMsg.MsgBody[i]; // ��������� ������ �� 256 ����  
                    msgBufPos++;  
                    rcvByteCnt++;
                    if (rcvByteCnt>=*msgLen) break;
                }
     
                if ((CANFrameCnt==0) || (rcvByteCnt>=*msgLen))
                {

                   blocklen = 36; // ����������� ������ �� 36 ������� ( 36*7=252 ����� )
                   // ����� ���������� ������ �������� callback-������� ��� ���������
                   /*for (int i=msgBufPos; i<256; i++)
                      blockBuff[i] = 0xFF;*/
                   if (rcvByteCnt>=*msgLen)
                   {
                    ProcessBuffer(1, msgBufPos); 
                   }
                   else
                   {
                    ProcessBuffer(0, msgBufPos);
                    MakeFlowControlPacket(a, 0, blocklen, SEPARATION_TIME);
                    Write_CAN_buff(a, 8, OUT_MSG_DESC);
                   }
                    msgBufPos = 0;
                   /* for (int i=msgBufPos; i<256; i++)
                      blockBuff[i] = 0xFF;*/
        
                }

                CANFrameCnt++;
                CANFrameCnt%=blocklen;
                if (rcvByteCnt>=*msgLen) 
                  {
                    return 0;              
                  }
               
        }
   
      }
    
    if ((state==RECEIVE_MSG) && (times_ms[ISOTP_TIMEOUT_IDX]>ISOTP_TIMEOUT_PRESET))
    {
        MakeFlowControlPacket(a, 2, 0, 0); // ��������� Flow Control
        Write_CAN_buff(a, 8, OUT_MSG_DESC);           // ���������� Flow Control
       /* wdt_enable();
        for(;;) asm("nop");*/
         return 1;
    }
    
  }  
}
