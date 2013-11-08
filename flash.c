#include "flash.h"
#include "intrinsics.h"
#include "defines.h"

/*
  Функции для самопрограммирования Flash-памяти программ
*/


void EraseChip()
{
#pragma diag_suppress=Pe1053 // Suppress warning for conversion from long-type address to flash ptr.
    for(long address = 0; address <= APP_END; address += PAGESIZE)
    { 
        #ifdef __HAS_RAMPZ__
        RAMPZ = (unsigned char)(address >> 16);
        #endif
        _SPM_ERASE(address);
        while( SPMCSR & (1<<SPMEN) ); // Wait until Flash write completed
    }
#pragma diag_default=Pe1053 // Back to default.
}    

void ErasePage(long address)
{
    #pragma diag_suppress=Pe1053 // Suppress warning for conversion from long-type address to flash ptr.
     #ifdef __HAS_RAMPZ__
    RAMPZ = (unsigned char)(address >> 16);
    #endif
    _SPM_ERASE(address);
    while( SPMCSR & (1<<SPMEN) ); // Wait until Flash write completed 
    #pragma diag_default=Pe1053 // Back to default.
}

void FlashFlush(unsigned long flashStartAdr){
#pragma diag_suppress=Pe1053 // Suppress warning for conversion from long-type address to flash ptr.
  #ifdef __HAS_RAMPZ__
  unsigned char RAMPZ_TMP = RAMPZ;
  RAMPZ = (unsigned char)(flashStartAdr >> 16);
  #endif
 /* _SPM_ERASE(flashStartAdr);
  while( SPMCSR & (1<<SPMEN) ); // Wait until Flash write completed */
  _SPM_PAGEWRITE(flashStartAdr);
  while( SPMCSR & (1<<SPMEN) ); // Wait until Flash write completed
  #ifdef RWWSRE
  __DataToR0ByteToSPMCR_SPM( 0, (unsigned char)(1<<RWWSRE)|(1<<SPMEN)); // Enable RWW
  #endif
  RAMPZ = RAMPZ_TMP;
#pragma diag_default=Pe1053 // Back to default.
}


unsigned char WriteFlashPage(unsigned long flashStartAdr, unsigned char *dataPage)
{
  unsigned int index;
  //unsigned char eepromInterruptSettings;
    __disable_interrupt();
    //eepromInterruptSettings = EECR & (1<<EERIE); // Stores EEPROM interrupt mask
    EECR &= ~(1<<EERIE);                    // Disable EEPROM interrupt
    while(EECR & (1<<EEWE));                // Wait if ongoing EEPROM write
    for(index = 0; index < PAGESIZE; index+=2)
    { 
        _SPM_FILLTEMP(index, (unsigned int)dataPage[index]+((unsigned int)dataPage[index+1] << 8));
 //     *((unsigned int)(dataPage + index))
  //     _SPM_FILLTEMP(index, 0xAAAA);
    }
    
    FlashFlush(flashStartAdr);         // Writes to Flash
      __enable_interrupt();
  /*  EECR |= eepromInterruptSettings;        // Restore EEPROM interrupt mask*/
    return 1;                            // Return TRUE if address
                                            // valid for writing

}

/*
unsigned char ReadFlashByte(unsigned long flashStartAdr)
{
   unsigned char __farflash* FlashBytePtr;
   FlashBytePtr = (unsigned char __farflash*)flashStartAdr;
   return *FlashBytePtr;
} 

unsigned long ReadFlashLong(unsigned long flashStartAdr)
{
   unsigned long __farflash* FlashLongPtr;
   FlashLongPtr = (unsigned long __farflash*)flashStartAdr;
   return *FlashLongPtr;
} 
*/
void ReadFlashPage(unsigned long flashStartAdr, unsigned char *PageBuf)
{
  for (int i = 0; i<PAGESIZE; i++)
  {
    *(PageBuf+i) = *((unsigned char __farflash*)(flashStartAdr+i));//ReadFlashByte(i);
  }
} 