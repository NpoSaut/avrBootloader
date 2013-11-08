
#include "isotp.h"
#include "CAN.h"
#include "FIFO.h"
#include "intrinsics.h"
#include "iocan128.h"
#include "flash.h"

#define SEPARATION_TIME 0

extern CANFIFO CANMsgBuff;


/*----------------------- ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ   -------------------------*/

/* ФОРМИРОВАНИЕ ФРЕЙМА Flow Control*/
void MakeFlowControlPacket(unsigned char* buff, unsigned char blocksize, int septime)
{
  buff[0] = (0x03 << 4);
  buff[1] = blocksize;
  buff[2] = septime;
  for(char i=3; i<8; i++) buff[i] = 0;
}


/* ПРИЕМ ФРЕЙМА Flow Control */
int ReceiveFlowControl(unsigned char* blocksize, char *septime)
{
  CANMesStruc CANMsg; 
  char FC_Received = 0;      // Флаг "принят кадр Flow Control"
  while (!FC_Received) //Ожидаем кадр Flow Control
  {
     while (!FIFO_isEmpty(&CANMsgBuff))
     {
        FIFO_PullFromBuff(&CANMsgBuff, &CANMsg); //Достаем из буфера сообщение 
        if (((CANMsg.MsgBody[0] & 0xF0)>>4) == 3) // Дождались кадра Flow Control
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

/*------------ ОСНОВНЫЕ ФУНКЦИИ, ИСПОЛЬЗУЕМЫЕ ВО ВНЕШНЕЙ ПРОГРАММЕ ---------- */

/* ОТПРАВКА СООБЩЕНИЯ */
int ISOTPSendMsg1(unsigned char* hdrBuff, unsigned char hdrLen, unsigned char __farflash* msgBuff, int buffLen)
/* Функция отправки буфера msgBuff длиной buffLen по ISO TP*/
{
  
  int sndBytesCnt; // Счетчик отправленных байт
  unsigned char blocksize;    // Block Size
  unsigned char MsgsBeforeFC=0;
  char sepTime;    // Separation Time
  unsigned char  outbuff[8]; // Буфер, который будем использовать для отправки CAN сообщений
  char idx = 1;
  
  
  outbuff[0] = 0x10;         // В биты 4-7 пишем код 1 (FirstFrame)
  outbuff[0] |=  (unsigned char)((buffLen +hdrLen)>>8);// Биты 0-3 - старшие 4 бита длины данных
  outbuff[1]  =  (unsigned char)(buffLen+hdrLen);// 2-й байт CAN-фрейма - остальные биты длины данных 
  for (int i=0; i<6; i++)
    if (i>=hdrLen)
     outbuff[i+2] = msgBuff[i-hdrLen];
    else
     outbuff[i+2] = hdrBuff[i]; 
 
  Write_CAN_buff(outbuff, 8, 0x200); // Отправляем First Frame в CAN
  
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
    Write_CAN_buff(outbuff, 8, 0x200); // Отправляем Consecutive Frame в CAN
    MsgsBeforeFC--;
    __delay_cycles(100000);
    
    idx++;
    idx&=0x0F;
    //if (idx==0) idx = 1;
  }
  return 0; 
}




/* ОТПРАВКА СООБЩЕНИЯ */
/*
int ISOTPSendMsg(unsigned char* msgBuff, int buffLen)
// Функция отправки буфера msgBuff длиной buffLen по ISO TP
{
  
  int sndBytesCnt; // Счетчик отправленных байт
  unsigned char blocksize;    // Block Size
  unsigned char MsgsBeforeFC=0;
  char sepTime;    // Separation Time
  unsigned char  outbuff[8]; // Буфер, который будем использовать для отправки CAN сообщений
  char idx = 1;
  
  
  outbuff[0] = 0x10;         // В биты 4-7 пишем код 1 (FirstFrame)
  outbuff[0] |=  (unsigned char)(buffLen>>8);// Биты 0-3 - старшие 4 бита длины данных
  outbuff[1]  =  (unsigned char)buffLen;// 2-й байт CAN-фрейма - остальные биты длины данных 
  for (int i=0; i<6; i++)
     outbuff[i+2] = msgBuff[i];
 
  Write_CAN_buff(outbuff, 8, 0x200); // Отправляем First Frame в CAN
  
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
    Write_CAN_buff(outbuff, 8, 0x200); // Отправляем Consecutive Frame в CAN
    MsgsBeforeFC--;
    __delay_cycles(1000000);
    
    idx++;
    idx&=0x0F;
    //if (idx==0) idx = 1;
  }
  return 0; 
}
*/
/* ОТПРАВКА ОДИНОЧНОГО КАДРА Single Frame */
void ISOTPSendSingleFrame(unsigned char* msgBuff, char buffLen)
{
  /* Функция отправки Single Frame по ISO TP. 
     msgBuff - отправляемые данные, buffLen - длина (1-7) */
  unsigned char outbuff[8]; // Буфер для отправляемого CAN-сообщения
  outbuff[0] = (buffLen & 0xF); // биты 0-3 - длина данных, биты 4-7 - код типа сообщ. Здесь код равен 0 (Single Frame)
  for (int i=0; i<(buffLen & 0xF); i++)
  {
    outbuff[i+1] = msgBuff[i];
  }
  Write_CAN_buff(outbuff, 8, 0x200); // Отправляем Single Frame
}

void ISOTPReceiveSingleFrame(unsigned char* frameBuff, char *frameBuffLen)
{
  CANMesStruc CANMsg;
  for(;;)
  {
     while (!FIFO_isEmpty(&CANMsgBuff))
     {
       FIFO_PullFromBuff(&CANMsgBuff, &CANMsg);
       if ((CANMsg.MsgBody[0] & 0xF0)==0) // Приняли Single frame
       {
         *frameBuffLen = CANMsg.MsgBody[0] & 0x0F; // Длина Single Frame
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
//  unsigned char blockBuff[256]; // Основной буфер для хранения данных, принятых по ISO TP 
  unsigned char overageBuff[8]; // Дополнительный буфер
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
        
        FIFO_PullFromBuff(&CANMsgBuff, &CANMsg); //Достаем из буфера сообщение 
        switch (state)  //и обрабатываем в зависимости от состояния, в котором находимся
        {
          
          case WAIT_FIRST_FRAME: //Ожидание First Frame
               PORTE = 0xFA;
               if ((CANMsg.MsgBody[0] & 0xF0)== 0) // Приняли Single frame
                   {
                     *msgLen = CANMsg.MsgBody[0] & 0x0F; // Длина Single Frame
                     for (char i=0; i<(CANMsg.MsgBody[0] & 0x0F); i++)
                       blockBuff[i] = CANMsg.MsgBody[i+1];
                     ProcessBuffer(1);
                     return;
                   }     
               
               if ((CANMsg.MsgBody[0] & 0xF0) == 0x10) //Дождались First Frame
                  {
                    *msgLen=(CANMsg.MsgBody[0] &0xF)<<8; 
                    *msgLen+=CANMsg.MsgBody[1];         //Извлекли длину всего сообщения
                    CANFrameCnt = 1;
                    msgBufPos  = 0;
                    rcvByteCnt  = 0;
                    for (unsigned char i=2; i<8; i++)
                    {
                      blockBuff[msgBufPos] = CANMsg.MsgBody[i];
                      msgBufPos++;
                      rcvByteCnt++;
                    }
                    blocklen = 36; /* Запрашивать будем блок длиной в 36 сообщений. Это 36*7=252 байта информации 
                                     (при этом 6 байт уже приняли через First Frame). Итого - 258 байт.
                                      256 байт пойдут в основной буфер, 2 - в дополнительный*/
                    MakeFlowControlPacket(a, blocklen, SEPARATION_TIME); // Формируем Flow Control
                    Write_CAN_buff(a, 8, 0x200);           // Отправляем Flow Control
                    state = RECEIVE_MSG; //Переходим в состояние приема сообщения
                  }
              break;
               
               
          case RECEIVE_MSG: //Прием сообщения
            
                for (unsigned char i=1; i<8; i++)
                {

                    if (msgBufPos<256)
                      blockBuff[msgBufPos] = CANMsg.MsgBody[i]; // Заполняем массив на 256 байт
                    else
                      overageBuff[msgBufPos-256] = CANMsg.MsgBody[i]; // остаток блока скидываем во временный буфер
                    msgBufPos++;  
                    rcvByteCnt++;
                }
                
                if (msgBufPos>256)                // Если приняли больше 256 байт, то используется доп. буфер
                  overageBuffLen = msgBufPos-256; // Считаем длину дополнительного буфера
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
                   // После заполнения буфера вызываем callback-функцию его обработки
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
