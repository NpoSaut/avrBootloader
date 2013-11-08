/*
  ��������� ������������� CAN-�����������
  � ��������� ������ ����� CAN-���������
*/

#include <iocan128.h>
#include "intrinsics.h"
#include "types.h"
#include "glob_defs.h"


void CAN_init()
{
		//	������������� CAN
        CANGCON = (1<<SWRES);	// ����������� ���������� CAN
          /*   ldi  R16,00000001b /
             sts  CANGCON,R16 */
        CANGIE = 0x00;      // ������ ���� ���������� �� CAN
          /*   ldi R16,00000000b
             sts CANGIE,R16  */
        CANIE1 = 0x00;     	// ������ ���������� �������� (Mob) 8-14
          /*   ldi R16,0
             sts CANIE1,R16  */
        CANIE2 = 0x00;		//  ������ ���������� �������� (Mob) 0-7     
           /*  ldi R16,00000000b
             sts CANIE2,R16  */
		
/*#if (SPEED_CAN == 100)
        CANBT1 = 0x0A; //0x04   //	��������� ��� �������� 100 ����/���
        CANBT2 = 0x0E; //0x0C
        CANBT3 = 0x4B; //0x37
#else  */  		
        CANBT1 = 0x04; 
        CANBT2 = 0x0C;
        CANBT3 = 0x37;
/*#endif  */
		
             //----��������� MOb0-Mob3---------------------
	for (BYTE i=0; i<4; i++)
	{
		CANPAGE = (i<<4);		// ����� MOb0 (Page Mob Register)
			// ��������� Identifier Mask Registers
			// ������ ����� (���������� �����.) �� 10 �������� IDTAG + RTR		
		CANIDM4 = (1<<RTRMSK);		// |(1<<IDEMSK);  
		CANIDM3 = 0;
		CANIDM2 = 0;	//0xE0,	//	0xA0
		CANIDM1 = 0;	//0xFF;

        CANIDT4 = 0;
		CANIDT3 = 0;
		CANIDT2 = 0;
        CANIDT1 = 0;
		// ��������� ����� ��� ������ � ����������� �� ����
	/*					// ��� �����	
		if ( NumChip == First )
		{
			CANIDT2 = (BYTE)(ID_CANMES_1A<<5);
			CANIDT1 = (BYTE)((ID_CANMES_1A & 0x7ff)>>3);
		}
 		else
		{
		  	CANIDT2 = (BYTE)(ID_CANMES_1B<<5);
			CANIDT1 = (BYTE)((ID_CANMES_1B & 0x7ff)>>3);
		}
	*/	
        CANCDMOB = (1<<CONMOB1)|(1<<DLC3); 	// ��������� Mob �� �����, 8 ���� DLC
					// RPLV=0 - no automatic reply; IDE=0 - CAN standard rev 2A
           /*  ldi R16,10001000b /// ���������� �����, ����� ������ 8 ����
             sts CANCDMOB,R16  */
	}	
        CANIDM4 = (1<<RTRMSK);		// |(1<<IDEMSK);  // ��������� Identifier Mask Registers               
		CANIDM3 = 0;
		CANIDM2 = 0;
		CANIDM1 = 0;

        CANIDT4 = 0;
		CANIDT3 = 0;
		CANIDT2 = 0;
		CANIDT1 = 0;
             //������ �������� ��������� 4-14
		for (BYTE i=4; i<15; i++)
		{
		  	CANPAGE = (i<<4);
			CANCDMOB = 0;
		} 
             
        CANGCON = (1<<ENASTB);	// ���������� ������ CAN
          /*  ldi  R16,00000010b 
            sts  CANGCON,R16  */
         
}			
