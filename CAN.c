
#include "CAN.h"
#include "FIFO.h"
#include "intrinsics.h"


//#include "globals.h"
/*
  ��������� ������������� CAN-�����������
  � ��������� ������ ����� CAN-���������
	*** ����������� � �-��� ��������� ��������� � ����� ����������� ��������� � 
		������ � ������� �-� CANPAGE (09-12)
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
            //	������������� CAN
    CANGCON = (1<<SWRES);	// ����������� ���������� CAN     
    CANGIE = 0x00;      // ������ ���� ���������� �� CAN
    CANIE1 = 0x00;     	// ������ ���������� �������� (Mob) 8-14
    CANIE2 = 0x00;		//  ������ ���������� �������� (Mob) 0-7     


    CANBT1 = 0x0A;   //	��������� ��� �������� 100 ����/���
    CANBT2 = 0x0E;
    CANBT3 = 0x4B; 
    
         //----��������� MOb0-Mob3---------------------
    for (unsigned char i=0; i<4; i++)
    {
            CANPAGE = (i<<4);		// ����� MOb0 (Page Mob Register)
     CANIDM4 = (1<<RTRMSK);		// |(1<<IDEMSK);  // ��������� Identifier Mask Registers               
            CANIDM3 = 0;
            CANIDM2 = 0x80;
            CANIDM1 = 0xFF;

    CANIDT4 = 0;
            CANIDT3 = 0;
            CANIDT2 =          FU_PROG << 5;
            CANIDT1 = /*0x7E0*/FU_PROG >> 3;               
            // ������ ����� (���������� �����.) �� 10 �������� IDTAG + RTR	
            // SetMaskIDTAG();		
    CANCDMOB = (1<<CONMOB1)|(1<<DLC3); 	// ��������� Mob �� �����, 8 ���� DLC
                                    // RPLV=0 - no automatic reply; IDE=0 - CAN standard rev 2A
   
    }	
    

         //	������ �������� ��������� 4-14
            for (unsigned char i=4; i<15; i++)
            {
                    CANPAGE = (i<<4);
                    CANCDMOB = 0;
            } 

    CANGCON = (1<<ENASTB);	// ���������� ������ CAN
    CANGIE = (1<<ENIT)|(1<<ENRX)|(1<<ENRX)|(1<<ENBOFF); // ��������� ���������� CAN �� ������ �����
    CANIE2 = (1<<ENMOB0)|(1<<ENMOB1)|(1<<ENMOB2)|(1<<ENMOB3); // ���������� ���������� �� ����� 1-4         
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
       CANIDT2 = (LOBYTE(CAN_ID)<<5);	// 3 ��.��� ID : bits 2..0
       CANIDT1 = LOBYTE(CAN_ID>>3); //  8 ��.��� ID : bits 10..3
       for (int i=0; i<BuffLen; i++)
       {
          asm("nop\n");
          CANMSG = Buff[i];
       }
       
       CANSTMOB = 0x00; 	// ���������� ������� ������� ���������
       CANGIE = 0; // ��������� ���������� �� CAN
      // __delay_cycles(10000);
       CANCDMOB = BuffLen|(1<<CONMOB0);//|(1<<DLC1);		// ���������� ��������, CAN-A (IDE=0), 2 ���� DLC
       while (!(CANSTMOB & (1<<TXOK))){
         asm("nop");
       }
        CANGIE = (1<<ENIT)|(1<<ENRX); // ��������� ���������� CAN �� ������ �����
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
         
         CANPAGE = (j<<4);		// ����� MObx �� �����
		 asm("nop\n");		//__no_operation; 
         unsigned char StatusCAN = CANSTMOB;
         // ��������� ����� �� ����� ���������
		if ((StatusCAN &(1<<RXOK)) == 0)	continue;	// return;
		  
		// CANSTMOB=0;
		// ��������� �� ������ ������
		if (StatusCAN & ((1<<SERR)|(1<<CERR)|(1<<FERR)) )	goto reinstal;
	
		
		// �������� ��������� �������� RTR	
		if (CANIDT4 & (1<<RTRTAG)) 	goto reinstal;
		
                
                    // ��������� � ��������� � ����� FIFO 
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
					// ��������� ��������� Mob �� �����, 8 ���� DLC
                CANIDM4 = (1<<RTRMSK);		// |(1<<IDEMSK);  // ��������� Identifier Mask Registers               
                CANIDM3 = 0;
                CANIDM2 = 0x80;
                CANIDM1 = 0xFF;

                CANIDT4 = 0;
                CANIDT3 = 0;
                CANIDT2 =          FU_PROG << 5;
                CANIDT1 = /*0x7E0*/FU_PROG >> 3;          
		CANCDMOB = (1<<CONMOB1)|(1<<DLC3); 	
		CANSTMOB &= ~(0x3E); 	// ������ �������� ������ ��������� (1<<RXOK) & flag_errors
	}	
  __enable_interrupt();
}
