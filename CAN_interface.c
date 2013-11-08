/*
  процедуры инициализации CAN-контроллера
  и процедуры обмена через CAN-интерфейс
*/

#include <iocan128.h>
#include "intrinsics.h"
#include "types.h"
#include "glob_defs.h"


void CAN_init()
{
		//	Инициализация CAN
        CANGCON = (1<<SWRES);	// программный перезапуск CAN
          /*   ldi  R16,00000001b /
             sts  CANGCON,R16 */
        CANGIE = 0x00;      // запрет ВСЕХ прерываний от CAN
          /*   ldi R16,00000000b
             sts CANGIE,R16  */
        CANIE1 = 0x00;     	// запрет прерываний объектов (Mob) 8-14
          /*   ldi R16,0
             sts CANIE1,R16  */
        CANIE2 = 0x00;		//  запрет прерываний объектов (Mob) 0-7     
           /*  ldi R16,00000000b
             sts CANIE2,R16  */
		
/*#if (SPEED_CAN == 100)
        CANBT1 = 0x0A; //0x04   //	настройки для скорости 100 кбит/сек
        CANBT2 = 0x0E; //0x0C
        CANBT3 = 0x4B; //0x37
#else  */  		
        CANBT1 = 0x04; 
        CANBT2 = 0x0C;
        CANBT3 = 0x37;
/*#endif  */
		
             //----настройка MOb0-Mob3---------------------
	for (BYTE i=0; i<4; i++)
	{
		CANPAGE = (i<<4);		// выбор MOb0 (Page Mob Register)
			// настройка Identifier Mask Registers
			// Ставим маску (фильтрация сообщ.) на 10 разрядов IDTAG + RTR		
		CANIDM4 = (1<<RTRMSK);		// |(1<<IDEMSK);  
		CANIDM3 = 0;
		CANIDM2 = 0;	//0xE0,	//	0xA0
		CANIDM1 = 0;	//0xFF;

        CANIDT4 = 0;
		CANIDT3 = 0;
		CANIDT2 = 0;
        CANIDT1 = 0;
		// Принимать будем все пакеты и фильтровать на ходу
	/*					// Без маски	
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
        CANCDMOB = (1<<CONMOB1)|(1<<DLC3); 	// Настройка Mob на прием, 8 байт DLC
					// RPLV=0 - no automatic reply; IDE=0 - CAN standard rev 2A
           /*  ldi R16,10001000b /// разрешение приёма, длина данных 8 байт
             sts CANCDMOB,R16  */
	}	
        CANIDM4 = (1<<RTRMSK);		// |(1<<IDEMSK);  // настройка Identifier Mask Registers               
		CANIDM3 = 0;
		CANIDM2 = 0;
		CANIDM1 = 0;

        CANIDT4 = 0;
		CANIDT3 = 0;
		CANIDT2 = 0;
		CANIDT1 = 0;
             //Запрет объектов сообщений 4-14
		for (BYTE i=4; i<15; i++)
		{
		  	CANPAGE = (i<<4);
			CANCDMOB = 0;
		} 
             
        CANGCON = (1<<ENASTB);	// разрешение работы CAN
          /*  ldi  R16,00000010b 
            sts  CANGCON,R16  */
         
}			
