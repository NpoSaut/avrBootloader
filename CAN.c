
#include "CAN.h"
#include "FIFO.h"
#include "intrinsics.h"


//#include "globals.h"
/*
  процедуры инициализации CAN-контроллера
  и процедуры обмена через CAN-интерфейс
	*** исправление в ф-ции обработки сообщения в части корректного обращения к 
		данным в очереди ч-з CANPAGE (09-12)
*/

#include <iocan128.h>


#define	HIBYTE(a) (unsigned char)(a>>8)
#define LOBYTE(a) (unsigned char)(a & 0xff)

#define SPEED_CAN  100


CANFIFO CANMsgBuff;
int test = 0;

CANMesStruc CanMsg;
 
void CAN_init()
{
    DDRD  = (1<<DDB3)|(1<<DDB5); 
            //	Инициализация CAN
    CANGCON = (1<<SWRES);	// программный перезапуск CAN     
    CANGIE = 0x00;      // запрет ВСЕХ прерываний от CAN
    CANIE1 = 0x00;     	// запрет прерываний объектов (Mob) 8-14
    CANIE2 = 0x00;		//  запрет прерываний объектов (Mob) 0-7     


    CANBT1 = 0x0A;   //	настройки для скорости 100 кбит/сек
    CANBT2 = 0x0E;
    CANBT3 = 0x4B; 
    
         //----настройка MOb0-Mob3---------------------
    for (unsigned char i=0; i<4; i++)
    {
            CANPAGE = (i<<4);		// выбор MOb0 (Page Mob Register)
     CANIDM4 = (1<<RTRMSK);		// |(1<<IDEMSK);  // настройка Identifier Mask Registers               
            CANIDM3 = 0;
            CANIDM2 = 0x80;
            CANIDM1 = 0xFF;

    CANIDT4 = 0;
            CANIDT3 = 0;
            CANIDT2 =          FU_PROG << 5;
            CANIDT1 = /*0x7E0*/FU_PROG >> 3;               
            // Ставим маску (фильтрация сообщ.) на 10 разрядов IDTAG + RTR	
            // SetMaskIDTAG();		
    CANCDMOB = (1<<CONMOB1)|(1<<DLC3); 	// Настройка Mob на прием, 8 байт DLC
                                    // RPLV=0 - no automatic reply; IDE=0 - CAN standard rev 2A
   
    }	
    

         //	Запрет объектов сообщений 4-14
            for (unsigned char i=4; i<15; i++)
            {
                    CANPAGE = (i<<4);
                    CANCDMOB = 0;
            } 

    CANGCON = (1<<ENASTB);	// разрешение работы CAN
    CANGIE = (1<<ENIT)|(1<<ENRX)|(1<<ENRX)|(1<<ENBOFF); // Разрешаем прерывание CAN по приему кадра
    CANIE2 = (1<<ENMOB0)|(1<<ENMOB1)|(1<<ENMOB2)|(1<<ENMOB3); // Разрешение прерываний по мобам 1-4         
}




void Write_CAN_buff(unsigned char *Buff, unsigned char BuffLen, unsigned long CAN_ID)
{

      //CANSTMOB = 0;
        __disable_interrupt();
       CANPAGE = 0x10;
       asm("nop\n");
       
       asm("nop\n");
       CANIDT4 = 0;		// RTRTAG = 0; RB0TAG = 0
       CANIDT3 = 0;
       CANIDT2 = (LOBYTE(CAN_ID)<<5);	// 3 мл.бит ID : bits 2..0
       CANIDT1 = LOBYTE(CAN_ID>>3); //  8 ст.бит ID : bits 10..3
       for (int i=0; i<BuffLen; i++)
       {
          asm("nop\n");
          CANMSG = Buff[i];
       }
       
       CANSTMOB = 0x00; 	// Сбрасываем регистр статуса сообщения
       CANGIE = 0; // Запрещаем прерывания от CAN
      // __delay_cycles(10000);
       CANCDMOB = BuffLen|(1<<CONMOB0);//|(1<<DLC1);		// Разрешение передачи, CAN-A (IDE=0), 2 байт DLC
       while (!(CANSTMOB & (1<<TXOK))){
         asm("nop");
       }
        CANGIE = (1<<ENIT)|(1<<ENRX); // Разрешаем прерывание CAN по приему кадра
      // __delay_cycles(10000);      
             __enable_interrupt();
}


#pragma vector = CANIT_vect
__interrupt void CAN_interrupt(void)
{
      // FIFO_init(CANMsgBuff);
     __disable_interrupt();
     // PORTE = 0x10;               
      unsigned int Temp =0;
      for (unsigned char j=0; j<4; j++)
    {
         
         CANPAGE = (j<<4);		// выбор MObx на прием
		 asm("nop\n");		//__no_operation; 
         unsigned char StatusCAN = CANSTMOB;
         // Проверяем ящики на прием сообщений
		if ((StatusCAN &(1<<RXOK)) == 0)	continue;	// return;
		  
		// CANSTMOB=0;
		// Проверяем на ошибки приема
		if (StatusCAN & ((1<<SERR)|(1<<CERR)|(1<<FERR)) )	goto reinstal;
	
		
		// Проверка принятого значения RTR	
		if (CANIDT4 & (1<<RTRTAG)) 	goto reinstal;
		
                
                    // Добавляем в сообщение в буфер FIFO 
                    Temp = (CANIDT1*8);
                    Temp += (CANIDT2>>5);
                if ((Temp == FU_INIT /*0x7E0*/) || (Temp == FU_PROG/*0x7E1*/))
                {
                    //CANPAGE = (j<<4)|(1<<AINC);  
                
                    CanMsg.ID  = Temp;
                    CanMsg.Len =  CANCDMOB & 0x0F;
                    for (unsigned char ii=0; ii<CanMsg.Len; ii++)
                    {
                      CANPAGE = (j<<4)|(ii); 
                      CanMsg.MsgBody[ii] = CANMSG;
                      asm("nop");
                    }
                    FIFO_PushToBuff(&CANMsgBuff, &CanMsg);  
                    
                
                }
reinstal:		
		CANPAGE = (j<<4);	//	=0;
		// SetMaskIDTAG();
					// Повторная настройка Mob на прием, 8 байт DLC
                CANIDM4 = (1<<RTRMSK);		// |(1<<IDEMSK);  // настройка Identifier Mask Registers               
                CANIDM3 = 0;
                CANIDM2 = 0x80;
                CANIDM1 = 0xFF;

                CANIDT4 = 0;
                CANIDT3 = 0;
                CANIDT2 =          FU_PROG << 5;
                CANIDT1 = /*0x7E0*/FU_PROG >> 3;          
		CANCDMOB = (1<<CONMOB1)|(1<<DLC3); 	
		CANSTMOB &= ~(0x3E); 	// Снятие признака приема сообщения (1<<RXOK) & flag_errors
	}	
  __enable_interrupt();
}
