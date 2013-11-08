
#include "isotp.h"
#include "CAN.h"
#include "FIFO.h"
#include "intrinsics.h"
#include "iocan128.h"
#include "flash.h"

#define SEPARATION_TIME 0

extern CANFIFO CANMsgBuff;


/*----------------------- ��������������� �������   -------------------------*/

/* ������������ ������ Flow Control*/
void MakeFlowControlPacket(unsigned char* buff, unsigned char blocksize, int septime)
{
  buff[0] = (0x03 << 4);
  buff[1] = blocksize;
  buff[2] = septime;
  for(char i=3; i<8; i++) buff[i] = 0;
}


/* ����� ������ Flow Control */
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
int ISOTPSendMsg1(unsigned char* hdrBuff, unsigned char hdrLen, unsigned char __farflash* msgBuff, int buffLen)
/* ������� �������� ������ msgBuff ������ buffLen �� ISO TP*/
{
  
  int sndBytesCnt; // ������� ������������ ����
  unsigned char blocksize;    // Block Size
  unsigned char MsgsBeforeFC=0;
  char sepTime;    // Separation Time
  unsigned char  outbuff[8]; // �����, ������� ����� ������������ ��� �������� CAN ���������
  char idx = 1;
  
  
  outbuff[0] = 0x10;         // � ���� 4-7 ����� ��� 1 (FirstFrame)
  outbuff[0] |=  (unsigned char)((buffLen +hdrLen)>>8);// ���� 0-3 - ������� 4 ���� ����� ������
  outbuff[1]  =  (unsigned char)(buffLen+hdrLen);// 2-� ���� CAN-������ - ��������� ���� ����� ������ 
  for (int i=0; i<6; i++)
    if (i>=hdrLen)
     outbuff[i+2] = msgBuff[i-hdrLen];
    else
     outbuff[i+2] = hdrBuff[i]; 
 
  Write_CAN_buff(outbuff, 8, 0x200); // ���������� First Frame � CAN
  
  __delay_cycles(100000);
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
          outbuff[i] = msgBuff[sndBytesCnt - hdrLen];
        else
          outbuff[i] = hdrBuff[sndBytesCnt];
        sndBytesCnt++;
      }
    
    outbuff[0] = 0x20 + idx;
    Write_CAN_buff(outbuff, 8, 0x200); // ���������� Consecutive Frame � CAN
    MsgsBeforeFC--;
    __delay_cycles(100000);
    
    idx++;
    idx&=0x0F;
    //if (idx==0) idx = 1;
  }
  return 0; 
}




/* �������� ��������� */
/*
int ISOTPSendMsg(unsigned char* msgBuff, int buffLen)
// ������� �������� ������ msgBuff ������ buffLen �� ISO TP
{
  
  int sndBytesCnt; // ������� ������������ ����
  unsigned char blocksize;    // Block Size
  unsigned char MsgsBeforeFC=0;
  char sepTime;    // Separation Time
  unsigned char  outbuff[8]; // �����, ������� ����� ������������ ��� �������� CAN ���������
  char idx = 1;
  
  
  outbuff[0] = 0x10;         // � ���� 4-7 ����� ��� 1 (FirstFrame)
  outbuff[0] |=  (unsigned char)(buffLen>>8);// ���� 0-3 - ������� 4 ���� ����� ������
  outbuff[1]  =  (unsigned char)buffLen;// 2-� ���� CAN-������ - ��������� ���� ����� ������ 
  for (int i=0; i<6; i++)
     outbuff[i+2] = msgBuff[i];
 
  Write_CAN_buff(outbuff, 8, 0x200); // ���������� First Frame � CAN
  
  __delay_cycles(100000);
  sndBytesCnt = 6;
  
  while (sndBytesCnt<buffLen) 
  {
    if (MsgsBeforeFC==0)
    {
       ReceiveFlowControl(&blocksize, &sepTime);
       MsgsBeforeFC = blocksize;
       
    }
    outbuff[0] = 0x20 + idx;
    for (char i=1; i<8; i++)
      {
        outbuff[i] = msgBuff[sndBytesCnt];
        sndBytesCnt++;
      }
    Write_CAN_buff(outbuff, 8, 0x200); // ���������� Consecutive Frame � CAN
    MsgsBeforeFC--;
    __delay_cycles(1000000);
    
    idx++;
    idx&=0x0F;
    //if (idx==0) idx = 1;
  }
  return 0; 
}
*/
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
  Write_CAN_buff(outbuff, 8, 0x200); // ���������� Single Frame
}

void ISOTPReceiveSingleFrame(unsigned char* frameBuff, char *frameBuffLen)
{
  CANMesStruc CANMsg;
  for(;;)
  {
     while (!FIFO_isEmpty(&CANMsgBuff))
     {
       FIFO_PullFromBuff(&CANMsgBuff, &CANMsg);
       if ((CANMsg.MsgBody[0] & 0xF0)==0) // ������� Single frame
       {
         *frameBuffLen = CANMsg.MsgBody[0] & 0x0F; // ����� Single Frame
         for (char i=0; i<(CANMsg.MsgBody[0] & 0x0F); i++)
           frameBuff[i] = CANMsg.MsgBody[i+1];
         return;
       }
     }
  }
}

void ISOTPReceiveMessage(unsigned char* blockBuff, int *msgLen, void (*ProcessBuffer)(unsigned char lastBlock))
{
 
  CANMesStruc CANMsg; 
  unsigned char a[8] = {0,0,0,0,0,0,0,0};
  unsigned char blocklen;
  unsigned char state;
//  unsigned char blockBuff[256]; // �������� ����� ��� �������� ������, �������� �� ISO TP 
  unsigned char overageBuff[8]; // �������������� �����
  char overageBuffLen = 0;
  
  state = WAIT_FIRST_FRAME;
  
  unsigned char CANFrameCnt;
  int msgBufPos;
  int rcvByteCnt;

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
               PORTE = 0xFA;
               if ((CANMsg.MsgBody[0] & 0xF0)== 0) // ������� Single frame
                   {
                     *msgLen = CANMsg.MsgBody[0] & 0x0F; // ����� Single Frame
                     for (char i=0; i<(CANMsg.MsgBody[0] & 0x0F); i++)
                       blockBuff[i] = CANMsg.MsgBody[i+1];
                     ProcessBuffer(1);
                     return;
                   }     
               
               if ((CANMsg.MsgBody[0] & 0xF0) == 0x10) //��������� First Frame
                  {
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
                    blocklen = 36; /* ����������� ����� ���� ������ � 36 ���������. ��� 36*7=252 ����� ���������� 
                                     (��� ���� 6 ���� ��� ������� ����� First Frame). ����� - 258 ����.
                                      256 ���� ������ � �������� �����, 2 - � ��������������*/
                    MakeFlowControlPacket(a, blocklen, SEPARATION_TIME); // ��������� Flow Control
                    Write_CAN_buff(a, 8, 0x200);           // ���������� Flow Control
                    state = RECEIVE_MSG; //��������� � ��������� ������ ���������
                  }
              break;
               
               
          case RECEIVE_MSG: //����� ���������
            
                for (unsigned char i=1; i<8; i++)
                {

                    if (msgBufPos<256)
                      blockBuff[msgBufPos] = CANMsg.MsgBody[i]; // ��������� ������ �� 256 ����
                    else
                      overageBuff[msgBufPos-256] = CANMsg.MsgBody[i]; // ������� ����� ��������� �� ��������� �����
                    msgBufPos++;  
                    rcvByteCnt++;
                }
                
                if (msgBufPos>256)                // ���� ������� ������ 256 ����, �� ������������ ���. �����
                  overageBuffLen = msgBufPos-256; // ������� ����� ��������������� ������
                else
                  overageBuffLen = 0;
                
                if ((CANFrameCnt==0) || (rcvByteCnt>=*msgLen))
                {
                    if (252+overageBuffLen<256)
                      blocklen = 37;
                    else
                      blocklen = 36;
                    __delay_cycles(1000000);

                   // __delay_cycles(100000);
                   // ����� ���������� ������ �������� callback-������� ��� ���������
                   if (rcvByteCnt>=*msgLen)
                   {
                    ProcessBuffer(1); 
                    PORTE = 2 << 2;
                   }
                   else
                   {
                    ProcessBuffer(0);
                    PORTE = 1 << 2;
                    MakeFlowControlPacket(a, blocklen, SEPARATION_TIME);
                    Write_CAN_buff(a, 8, 0x200);
                   }
                 //   WriteFlashPage(flashPageStartAddr, blockBuff);
                  //  flashPageStartAddr+=256;
                    
                    msgBufPos = 0;
                    
                    while (msgBufPos<overageBuffLen)
                    {
                      blockBuff[msgBufPos] = overageBuff[msgBufPos];
                      msgBufPos++;
                    }
                }

                CANFrameCnt++;
                CANFrameCnt%=blocklen;
                if (rcvByteCnt>=*msgLen) 
                  {
                  //  PORTE = 0xAA;
                    return;
                    
                  }
               
        }
   
      }
  }  
}
