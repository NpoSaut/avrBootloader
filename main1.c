
#include "globals.h"
#include "defines.h"
#include "flash.h"
#include "iocan128.h"
#include "FIFO.h"
#include "intrinsics.h"
//#include "CRC16.h"
#include "CAN.h"
#include "isotp.h"


unsigned int times_ms[TIMES_COUNT]; // ������ ��������� �������

char a;
char flag;


void wdt_disable()			/* Turn off WDT */		
{ 	
	WDTCR = (1<<WDCE)|(1<<WDE);
	WDTCR = 0x00;
}

extern CANFIFO CANMsgBuff;

void (*CallProgram)( void ) = 0x0000; //��������� �� ������ ���������������� ���������
//long address;
//unsigned int ms_counter=0;
//unsigned int ms_counter1=0;
typedef struct
  {
    
    unsigned long blockID;
    unsigned char modif;
    unsigned char softwareModuleNum;
    unsigned char channel;
    unsigned long blockSN;
    
    
  } SysIDStruc;

SysIDStruc SysID;
SysIDStruc IDFromCAN;

  unsigned char keyCnt;
  unsigned char fileCnt;

unsigned char state;

void PutIDToCANBuff(SysIDStruc *ID,  unsigned char *Buff);

#pragma inline = forced
void Timer_init()
{

  //    ������������� ������ 1			?????
  //	������������ ���� ����� � ���������� 100 �c ��� ������������ ��������� � CAN
    TCCR1A = 0;				// ����� ������ ��� � ����.OCR1A
	TCCR1B = (1<<WGM12)|(1<<CS12);    // �������� ��������������� ������� ��/256
	OCR1A = 4688; 			// �������� Compare Register A (����������.����.)
  //	outp(OCR1B, 460); 			// �������� Compare Register B (����� Compare)
	TCNT1 = 0;        		// �������� (��������)  TCNT1 
	//	TIMSK |= (1<<OCIE1A);  // ��������� ���������� ������� 1 �� "���������� �"	
	
  //   ������������� ������ 2: ��������� ����� � �������� 1 �� ��� ...

 	TCCR2A = (1<<WGM21)|(1<<CS22)|(1<<CS21);     //  ����� ������ CTC, �������.1/256, ���. PB4(OC2A) ��������


	OCR2A  = 46; 			// �������� Compare Register A  
	ASSR = 0;		// ����. ������.������ �������2 � ����. ��������� �����.
	TCNT2  = 0;        		// �������� (��������)  TCNT0
					// �������� ���������� �� "����������" (1<<OCIE0A) �����

        TIMSK1 = 0; // 
        TIMSK2 = (1<<OCIE2A);  // ��������� ���������� �� �������2
}


typedef struct
{
  //unsigned char fnameLen; // ����� ����� �����
  unsigned long addr;     // ����� � ������
  unsigned long fsize;    // ������ ����� (����� ����� ������) 
  unsigned int fCRC;     // ����������� ����� ��� �����
} filerecord;

filerecord fileTable[5];


unsigned char fileTableBuff[255];

typedef struct
{
  unsigned char key;
  long value;
} keyparam;

keyparam keyparamTable[64];

int FileTableCheckCRC(filerecord *fileTable, unsigned char fileCnt, SysIDStruc* SysID);

/*
#pragma vector = TIMER1_COMPA_vect
__interrupt void TIMER1A_interrupt(void)
{   
}
*/



unsigned int crc_ccitt_update (unsigned int crc, unsigned char data)
    {
        data ^= LOBYTE(crc);
        data ^= data << 4;

        return ((((unsigned int)data << 8) | HIBYTE(crc)) ^ (unsigned char)(data >> 4) 
                ^ ((unsigned int)data << 3));
    }

// ----------- ����� ----------------------

int ParamDicReadFromFlash(keyparam *keyTable, unsigned char *keyCnt)
{
  /* ������� ��������� �� Flash-������ ����������� ������� ������� � ��������� ���
  � ������ �� ��������� keyTable, � ���������� ��������� ������� - �� ��������� keyCnt */
  
  *keyCnt = *((unsigned char __farflash*)PARAMLIST_FLASHADDR);
  if (*keyCnt == 0xFF) // ���� � ������ 0xFF, �������, ��� ������� ����
  {
      *keyCnt = 0;
     // return -1; // ������� �� ���������
  }
  
  *keyCnt+=CONST_KEYS_CONUT; // ���������� ���������� ���������� �������
  keyTable[0].key   = BOOTLOADER_TYPE_KEY; // ��������� � ������� ���������� ���������� ��������
  keyTable[0].value = BOOTLOADER_TYPE; 
  keyTable[1].key   = BOOTLOADER_VERSION_KEY;   
  keyTable[1].value = BOOTLOADER_VERSION;
  keyTable[2].key   = BOOTLOADER_FS_ENABLE_KEY;    
  keyTable[2].value = BOOTLOADER_FS_ENABLE;
  keyTable[3].key   = BOOTLOADER_PROTOCOL_VER_KEY;    
  keyTable[3].value = BOOTLOADER_PROTOCOL_VER;
  keyTable[4].key   = BOOTLOADER_OLD_PROTOCOL_VER_KEY;    
  keyTable[4].value = BOOTLOADER_OLD_PROTOCOL_VER;
    
  for (int i=CONST_KEYS_CONUT; i<*keyCnt; i++)
  {
    unsigned int offset = (i-CONST_KEYS_CONUT)*5;
    keyTable[i].key   = *((unsigned char __farflash*)(PARAMLIST_FLASHADDR+1+offset));
    keyTable[i].value = *((long __farflash*)(PARAMLIST_FLASHADDR+2+offset));
  }
  return 0;
}

void ParamDicWriteToFlash(keyparam *keyTable, unsigned char keyCnt)
{
  /* ������� ���������� �� flash-������ ����������� ������� �������, ������������� � ������ �� ��������� keyTable. 
  keyCnt - ���������� ������� */
    unsigned char tmpFlashBuff[PAGESIZE]; // �������� ������ - ��������� ������ ���  ������ �� Flash
    tmpFlashBuff[0] = keyCnt - CONST_KEYS_CONUT; // 0-� ����. - ���-�� �������. ���������� �������� � ������� ������� ������� �� �����, ������� �������� �� ����������
    unsigned int buffIdx = 1;   // ����� �� ��������� �� ����������� �������
    for (int i=CONST_KEYS_CONUT; i<keyCnt; i++) // � ������ ������� ��������� ���������� ��������. ����� �� ����������, �������� � i = 3
    {// ��������� �������� ������, ������� ����� ������� (1 ����) � �������� ������� (4 �����)
      tmpFlashBuff[buffIdx++] = keyTable[i].key; 
      *((unsigned long*)(tmpFlashBuff+buffIdx)) = keyTable[i].value;
      buffIdx+=4;
    }
    for (int i=buffIdx; i<PAGESIZE; i++)
      tmpFlashBuff[i] = 0xFF; // �������� ���������������� ����� �������� ������ 255
    ErasePage(PARAMLIST_FLASHADDR); // ������ �������� ����� �������
    WriteFlashPage(PARAMLIST_FLASHADDR, tmpFlashBuff); // �����
    return;
}

int ParamDicGetParam(unsigned char key, long *value)
{
/* ������� �������� �������� �������� � ������ key ����� ��������� value.
  ���� �������� �������, ������� ���������� 0, � ��������� ������ - (-1) */
  switch(key) // ������� ���������, �� ����������� �� ���� ���������� ���������
  {
    case BOOTLOADER_TYPE_KEY:
        {
          *value = BOOTLOADER_TYPE;          
          return 0;
        }
    case BOOTLOADER_VERSION_KEY:
        {
          *value = BOOTLOADER_VERSION;
          return 0;
        }
    case BOOTLOADER_FS_ENABLE_KEY:
        {
          *value = BOOTLOADER_FS_ENABLE;
          return 0;
        }
    case BOOTLOADER_PROTOCOL_VER_KEY:
        {
          *value = BOOTLOADER_PROTOCOL_VER;
          return 0;
        }
    case BOOTLOADER_OLD_PROTOCOL_VER_KEY:
        {
          *value = BOOTLOADER_OLD_PROTOCOL_VER;
          return 0;
        }
  }  
  // ���� ���� �� ����� ����� ���������� �������, �� �������� ��� ����� � ������� ������� ���������� �� Flash
  unsigned char keyCnt = *((unsigned char __farflash*)PARAMLIST_FLASHADDR);
  if (keyCnt==0xFF) return -1;
  for (int i=1; i<=(keyCnt*5); i+=5)
  {
    if (*((unsigned char __farflash*)(PARAMLIST_FLASHADDR+i))==key)
    {
      *value = *((unsigned long __farflash*)(PARAMLIST_FLASHADDR+i+1));
      return 0;
    }
  }
  *value = 0;
  return -1;
}


#pragma location="GETPARAMFUNC" 
unsigned long GetParam(unsigned char key)
{
  long val=0;
  if (!ParamDicGetParam(key, &val))
  {
    return val;
  }
  return 0;
}
#pragma required = GetParam // ������ ����������� �� ���������� �������������� �������

int ParamDicSetParam(keyparam *keyTable, unsigned char *keyCnt, unsigned char key, unsigned long value)
{
    for (int i=0; i<*keyCnt; i++) // ���� ���� ����� ��� ���������
    {
      if (keyTable[i].key == key) // �����
      {
        keyTable[i].value = value; // ��������
        return 0; // �����
      }
    }
    
    if ((*keyCnt)<MAX_KEYS_COUNT) // ����� ��������� �� ����� - ������� �����
    {
       keyTable[*keyCnt].key   = key;
       keyTable[*keyCnt].value = value; 
       (*keyCnt)++;
       return 0;
    }
    else
        return 2; // ��������� ������������ ���������� ������  
}

int ParamDicRemoveParam(keyparam *keyTable, unsigned char *keyCnt, unsigned char key)
{
    for (int i=0; i<*keyCnt; i++) // ���� ���� ��������� ����
    {
      if (keyTable[i].key == key) // �����
      {
        for (int j=i; j<*keyCnt-1; j++)
          keyTable[j] = keyTable[j+1];
        (*keyCnt)--;
        return 0;
      }
    } 
    return 2; // �������� �� ������
}

void ParamDicConvertToFUDPBuff(keyparam *keyTable, unsigned char keyCnt, unsigned char*FUDPBuff, int *FUDPBuffLen)
{
  int buffIdx=1;
  for (char i=0; i<keyCnt; i++)
  {
     FUDPBuff[buffIdx++] = (unsigned char) keyTable[i].key;
     *((unsigned long*)(FUDPBuff+buffIdx)) = keyTable[i].value;
     buffIdx+=4;
  }
  *FUDPBuffLen = buffIdx;
}

unsigned int CalculateCRCForFlashRegion(unsigned long offset, unsigned long len)
{
  unsigned int crc = 0xFFFF;
  for (unsigned long pos=offset; pos<offset+len; pos++)
  {
    crc = crc_ccitt_update(crc, *((unsigned char __farflash*)pos)/*ReadFlashByte(pos)*/);
  }
  return crc; 
}

int FileTableCheckCRC(filerecord *fileTable, unsigned char fileCnt, SysIDStruc* SysID)
{
  unsigned char msgBuff[7];
  if (!fileCnt) return -1;
  char result = 0;
  for (char i=0; i<fileCnt; i++)
  {
    if (CalculateCRCForFlashRegion(fileTable[i].addr, fileTable[i].fsize)!=fileTable[i].fCRC)
    {
      msgBuff[0] = 0xFF;
      PutIDToCANBuff(SysID, msgBuff);
      /*msgBuff[1] = SysID->systemID;
      msgBuff[2] = SysID->blockID;
      msgBuff[3] = SysID->blockMdf;
      msgBuff[4] = i;*/
      ISOTPSendSingleFrame(msgBuff, 7);
      result = -1;
    }
  }
  return result;
}

#pragma inline = forced
int FileTableReadFromFlash(filerecord *fileTable, unsigned char *fileCnt)
{
 // *fileCnt = ReadFlashByte(FILETABLE_FLASHADDR);
  *fileCnt =  *((unsigned char __farflash*)FILETABLE_FLASHADDR);
  if (*fileCnt == 255) 
  {
    *fileCnt = 0;
    return -1;
  }
  for (int i = 0; i<*fileCnt; i++)
  {
    unsigned char offset = 12*i;

    fileTable[i].addr  = *((unsigned long __farflash*)(FILETABLE_FLASHADDR+1+offset));
    fileTable[i].fsize = *((unsigned long __farflash*)(FILETABLE_FLASHADDR+5+offset));
    fileTable[i].fCRC  = *((unsigned int __farflash*)(FILETABLE_FLASHADDR+9+offset));    
  }
  return 0; 
}

void FileTableWriteToFlash(filerecord *fileTable, unsigned char fileCnt)
{
  unsigned char tmpFlashBuff[PAGESIZE];
  tmpFlashBuff[0] = fileCnt;
  unsigned char buffIdx = 1;
  for (char i=0; i<fileCnt; i++)
  {
    
    *((unsigned long*)(tmpFlashBuff+buffIdx))   = fileTable[i].addr;
    *((unsigned long*)(tmpFlashBuff+buffIdx+4)) = fileTable[i].fsize;
    *((unsigned long*)(tmpFlashBuff+buffIdx+8)) = fileTable[i].fCRC;
    buffIdx+=12;
    
  }
  for (int i=buffIdx; i<256; i++)
      tmpFlashBuff[i] = 0xFF;
  ErasePage(FILETABLE_FLASHADDR);
  WriteFlashPage(FILETABLE_FLASHADDR, tmpFlashBuff);
  return;
}

int FindFileByAddr(filerecord *fileTable, unsigned long addr, unsigned char fileCnt)
{
  for (int i=0; i<fileCnt; i++)
  {
    if (fileTable[i].addr == addr) return i;
  }
  return -1;
}

unsigned long Hex2long(unsigned char *hexStr)
{
  unsigned long result = 0;
  if ((hexStr[1] == 'e') || (hexStr[1] == 'E'))
    result = 0x1000000;
  for (int i=3; i<=hexStr[0]; i++)
    if ((hexStr[i]>='0') && (hexStr[i]<='9'))
    {
      result += ((unsigned long)(hexStr[i]-0x30)) << ((hexStr[0]-i)<<2); // ����������� ����� ����� �� ��� �����
    }
      else
            if ((hexStr[i]>='A') && (hexStr[i]<='F'))         
            {
              result += ((unsigned long)(hexStr[i]-0x37)) << ((hexStr[0]-i)<<2);
            }
              else
                    if ((hexStr[i]>='a') && (hexStr[i]<='f'))         
                    {
                      result += ((unsigned long)(hexStr[i]-0x57)) << ((hexStr[0]-i)<<2);
                    }
   return result;
}

//#pragma inline = forced
int FileTableAddEntry(filerecord *fileTable, unsigned char* fileCnt, unsigned long addr, unsigned long size, unsigned int CRC )
{

  unsigned int newFilePageAddr = addr/PAGESIZE; // ����� ������ ����� ������������� �� ������ ����� � ����� �������� Flash
  unsigned int newFilePageCnt =  size/PAGESIZE + (size%PAGESIZE!=0);  
  if (*fileCnt<MAX_FILE_COUNT) // ���������, �� ��������� �� ������������ ���-�� ������
  {
    for (char i=0; i<*fileCnt; i++) // ���������, �� �������� �� ����������� �������
    {
      if (addr<0x1000000) // ������� � ������� ������� ������ ������ �� ��������. ��� ������� ���������� ����� � EEPROM
      {
        if (fileTable[i].addr >= 0x1000000) continue;
        unsigned int existFilePageAddr = fileTable[i].addr / PAGESIZE; // ������ ���. ����� ������������� �� ������ ����� � ����� �������� Flash
        unsigned int existFilePageCnt = fileTable[i].fsize/PAGESIZE + (fileTable[i].fsize%PAGESIZE!=0);
        // ��������� ��� ��������� �������� ����������� ������������ ����� � ��� ������������� � ������ �� ������� ���������� �������
        if ((newFilePageAddr >= existFilePageAddr) && (newFilePageAddr  <= existFilePageAddr + existFilePageCnt - 1) || 
            (newFilePageAddr + newFilePageCnt - 1 >= existFilePageAddr) && (newFilePageAddr  + newFilePageCnt - 1  <= existFilePageAddr + existFilePageCnt - 1) ||
            (newFilePageAddr < existFilePageAddr) && (newFilePageAddr  + newFilePageCnt - 1  > existFilePageAddr + existFilePageCnt - 1) ||
            (newFilePageAddr  + newFilePageCnt - 1 >= FILETABLE_FLASHADDR/PAGESIZE + (FILETABLE_FLASHADDR%PAGESIZE!=0)))
          return 3;
      }
      else
      {
        if (fileTable[i].addr < 0x1000000) continue;
        else
           if ((addr >= fileTable[i].addr) && (addr  <= fileTable[i].addr + fileTable[i].fsize - 1) ||
              (addr + size - 1 >= fileTable[i].addr) && (addr + size - 1  <= fileTable[i].addr + fileTable[i].fsize - 1) ||
              (addr < fileTable[i].addr) && (addr + size - 1  > fileTable[i].addr + fileTable[i].fsize - 1) ||
              (addr + size - 1 > EEPROM_SIZE))
           return 3;
      }
    }
    
     fileTable[*fileCnt].addr  = addr;
     fileTable[*fileCnt].fsize = size;
     fileTable[*fileCnt].fCRC  = CRC;
     (*fileCnt)++;
    return 0; // �� ��. ����� ������ ��������� � �������
  }
  else return 2;
  
}

//#pragma inline = forced
int FileTableRemoveEntry(filerecord *fileTable, unsigned char* fileCnt, unsigned long removeAddr)
{
   for (char i=0; i<*fileCnt; i++) 
    {
      if (fileTable[i].addr == removeAddr)
      {
        if (removeAddr<0x1000000)
        {
          for (long addr=(removeAddr-removeAddr%PAGESIZE); addr<(removeAddr+fileTable[i].fsize); addr+=256)
            ErasePage(addr);
        }        
        for (char j=i+1; j<*fileCnt; j++)
          fileTable[j-1] = fileTable[j];
        (*fileCnt)--;

        return 0; // ������ �������
      }
    }
   return 1; // ����� �� ����� �������, ��� ������ � �� �����.
}


void FileTableConvertToFUDPBuff(filerecord *fileTable, unsigned char fileCnt, unsigned char*FUDPBuff, int *FUDPBuffLen)
{
  int buffIdx=1;
  for (char i=0; i<fileCnt; i++)
  {
     char folder = (fileTable[i].addr & 0x01000000)?'e':'f';
     unsigned char fl = 0;
     for (int j=2; j<8; j++)
     {
       char s = (char)(fileTable[i].addr >> 28-j*4) & 0x0F;
       if ((s>0) && !fl)
        {
          fl = 1;
          FUDPBuff[buffIdx++] = (8-j)+2;
          FUDPBuff[buffIdx++] = folder;
          FUDPBuff[buffIdx++] = '/';
        }
       if (fl) // ���� (������� ���� �����������)
        {
          FUDPBuff[buffIdx++] = (s<0xA)?(s+0x30):(s+0x57);
        }
     }
     if (!fl) // ���� ����� ��������� ������ 0, �� ��������� ������ �� ������ ������� 0x30
     {
       FUDPBuff[buffIdx++] = 3;   // ����� - 3 ������
       FUDPBuff[buffIdx++] = 'f';
       FUDPBuff[buffIdx++] = '/';
       FUDPBuff[buffIdx++] = 0x30; // ������ "0"
     }
     
     *((unsigned long*)(FUDPBuff+buffIdx)) = fileTable[i].fsize;
     *((unsigned long*)(FUDPBuff+buffIdx+4)) = CalculateCRCForFlashRegion(fileTable[i].addr, fileTable[i].fsize);//fileTable[i].fCRC;
     buffIdx+=8;
  }
  *FUDPBuffLen = buffIdx;
}
// ----------------------------------------


unsigned char msgBuff[255];
int msgBuffLen;
unsigned char msgBuffOut[255];
int msgBuffOutLen = 0;//!!!

unsigned char flashBuff[255];
unsigned char flashBuffIdx;

unsigned long flashStartAddr = 0;
unsigned long eepromAddr = 0;

unsigned char progstate = 0;

unsigned long fileStartAddr;
unsigned char flashPageOffset;

void StartProgram()
{
  //
  long value, blockNum;

  int result = ParamDicGetParam(131, &value);
  if ((result==-1) || (value==0))
  {
    blockNum = (unsigned long)(*(unsigned char __flash*)0x10A) + 
               ((unsigned long)(*(unsigned char __flash*)0x109) << 8);
    ParamDicSetParam(keyparamTable, &keyCnt, 131, blockNum);
    ParamDicWriteToFlash(keyparamTable, keyCnt); 
  }
  // ������� �� ����� � ��������� �������� ��������� 
  CANGIE = 0; // ��������� ���������� �� CAN
  TIMSK1 = 0; // ��������� ���������� �� �������-�������� 1
  TIMSK2 = 0; // ��������� ���������� �� �������-�������� 2
  MCUCR =  (1<<IVCE); // ��������� ������������ �������� ����������
  MCUCR = 0;          // ������� ���������� - � ������ flash
 // DDRE = 0x00;        // ��������� ���� E �� ����
  wdt_disable();
  *((unsigned int *) &GPIOR1) = (unsigned int)GetParam;
  //PORTC = 0x30;
  CallProgram();      // ������ ���������������� ������
}    


#pragma vector = TIMER2_COMP_vect
__interrupt void TIMER2_interrup(void)
{
  
    __disable_interrupt();
    for (char i=0; i<TIMES_COUNT; i++)
    if (times_ms[i]<60000) times_ms[i]++;
    if (times_ms[PROG_CONDITION_DELAY_IDX] >= PROG_CONDITION_DELAY_PRESET)
    {

      times_ms[PROG_CONDITION_DELAY_IDX] = 0;
      if (FileTableCheckCRC(fileTable, fileCnt, &SysID) == 0)
          StartProgram();   
      else
      {
          WDTCR |= (1<<WDCE)|(1<<WDE);
          WDTCR = (1<<WDE)|(1<<WDP2)|(1<<WDP1);	// ���������� ������ ����-���� ~1,0 �
          for(;;)
              asm("nop");
      }
    }
    __enable_interrupt();
    return;
}



char ExtractIDFromCANBuff(SysIDStruc *ID,  unsigned char *Buff)
{
  if (Buff[1]!=PROG_INIT) return 0;
  ID->blockID = ((unsigned int) Buff[2] << 4) + (Buff[3] >> 4) ;
  ID->modif = Buff[3] & 0x0F;
  ID->softwareModuleNum = Buff[4];  
  ID->channel = Buff[5]>>4;
  ID->blockSN = ((unsigned long)(Buff[5]&0x0F) << 16) | ((unsigned long)Buff[6]<<8) | Buff[7];
  return 1;
}

void PutIDToCANBuff(SysIDStruc *ID,  unsigned char *Buff)
{

  Buff[1] = ID->blockID>>4;
  Buff[2] = ((ID->blockID & 0x0f) <<4) | (ID->modif & 0x0f);
  Buff[3] = ID->softwareModuleNum;
  Buff[4] = ((ID->channel & 0x0F )<< 4)|(ID->blockSN>>16);
  Buff[5] = ID->blockSN>>8;
  Buff[6] = ID->blockSN;
}

/* Callback ������� ��������� ��������� �� ISO-TP ������ ������ */
void ProcessBuffer(unsigned char lastBlock, unsigned char bytesReceived) 
{
  unsigned long addr;
  unsigned long size;
  unsigned long CRC;
  
//  long offset = 0;
  unsigned char key;
  unsigned long value;
  times_ms[PROG_CONDITION_DELAY_IDX] = 0;
  switch (progstate)
  {
    case 0:                    // ���������� �� ����������������
            break;
    case 1:                    // ����� ����������������
            switch (msgBuff[0])
            {
                case PROG_CREATE:                   
                    addr = Hex2long(msgBuff+1);
                    size = *((unsigned long*)(msgBuff+msgBuff[1]+2));
                    CRC =  *((unsigned long*)(msgBuff+msgBuff[1]+6));        
                    msgBuffOut[0] = PROG_CREATE_ACK;
                    msgBuffOut[1] = FileTableAddEntry(fileTable, &fileCnt, addr, size, CRC );
                    ErasePage(FILETABLE_FLASHADDR);
                    FileTableWriteToFlash(fileTable, fileCnt);
                    ISOTPSendSingleFrame(msgBuffOut, 2);
                    break;
                case PROG_WRITE:  
                    fileStartAddr = Hex2long(msgBuff+1);
                    if (FindFileByAddr(fileTable, fileStartAddr, fileCnt)!=-1)
                    {                            
                      if (fileStartAddr<0x1000000) // 
                      {
                        flashStartAddr = fileStartAddr + *((unsigned long*)(msgBuff+2+msgBuff[1]));//offset;   // ��������� ����� flash ������, � �������� ������ ������
                        flashPageOffset = flashStartAddr & 0xFF;   // �������, ��������� ���� ����� ������� �� ������ �������� flash		// !!!! �������� �� PAGE_SIZE !!!!
                        for (int i=flashPageOffset; i<256; i++)
                          flashBuff[i] = 0xFF;
                        
                        flashBuffIdx = flashPageOffset;
                        for (int i=6+msgBuff[1]; i<bytesReceived; i++)
                        {
                                flashBuff[flashBuffIdx] = msgBuff[i];
                                if (flashBuffIdx < 255)
                                        {
                                                flashBuffIdx++;
                                        }
                                else
                                        {
                                                flashBuffIdx=0;
                                                WriteFlashPage((flashStartAddr- flashPageOffset), flashBuff);
                                                flashStartAddr+=256;
                                                
                                        }
                        }
                       
                        if (!lastBlock) progstate = 2;
                                   else
                                   {
                                      WriteFlashPage((flashStartAddr- flashPageOffset), flashBuff);
                                      msgBuffOut[0] = PROG_WRITE_ACK;
                                      msgBuffOut[1] = 0;
                                      ISOTPSendSingleFrame(msgBuffOut, 2);
                                   }
                      }
                      else
                      {
                        eepromAddr = fileStartAddr + *((unsigned long*)(msgBuff+2+msgBuff[1]));//offset;
                        for (int i=6+msgBuff[1]; i<bytesReceived; i++)
                        {
                          unsigned char __eeprom *eepromPtr;
                          eepromPtr = ( unsigned char __eeprom *)(eepromAddr++);
                          *eepromPtr = msgBuff[i];
                        }
                        if (!lastBlock)
                            progstate = 3;
                          else
                          {
                            msgBuffOut[0] = PROG_WRITE_ACK;
                            msgBuffOut[1] = 0;
                            ISOTPSendSingleFrame(msgBuffOut, 2);
                          }
                      }
                    }
                    break;
                case PROG_LIST_RQ:
                    msgBuffOut[0] = PROG_LIST;
                    FileTableConvertToFUDPBuff(fileTable, fileCnt, msgBuffOut, &msgBuffOutLen);
                    ISOTPSendMsg1(msgBuffOut, msgBuffOutLen, 0, 0, 0);
                    break;
                case PROG_READ_RQ:
                    msgBuffOut[0] = PROG_READ;
                    fileStartAddr = Hex2long(msgBuff+1);
                    //unsigned int buffLen;                   
                    if (FindFileByAddr(fileTable, fileStartAddr, fileCnt)!=-1)
                    {  
                        msgBuffOutLen= *((unsigned int*)(msgBuff+6+msgBuff[1]));
                        msgBuffOut[1] = 0;
                        flashStartAddr = fileStartAddr + *((unsigned long*)(msgBuff+2+msgBuff[1]));//offset;
                        ISOTPSendMsg1(msgBuffOut,2, flashStartAddr, msgBuffOutLen, (fileStartAddr>=0x1000000));                                  
                     } else
                     {
                       msgBuffOut[1] = 1;
                       ISOTPSendSingleFrame(msgBuffOut, 2);
                     }
                     break;
		case PROG_RM:							// �������� ����� ������
                    msgBuffOut[0] = PROG_RM_ACK;
                    addr = Hex2long(msgBuff+1);
	            msgBuffOut[1] = FileTableRemoveEntry(fileTable, &fileCnt, addr);	
                    ISOTPSendSingleFrame(msgBuffOut, 2);
                    FileTableWriteToFlash(fileTable, fileCnt);
                    
                    break;
		case PROG_MR_PROPER:					// �������� ���� ������ ��������
                    EraseChip();
                    msgBuffOut[0] = PROG_MR_PROPER_ACK;
                    fileCnt = 0;
                    ISOTPSendSingleFrame(msgBuffOut, 2);
                    break;
                case PARAM_SET_RQ:
                    msgBuffOut[0] = PARAM_SET_ACK;
                    key   = msgBuff[1];
                    value = *((unsigned long*)(msgBuff+2));
                    if ((key >= REMOTE_CHANGEABLE_KEY_AREA_LO) && (key <= REMOTE_CHANGEABLE_KEY_AREA_HI))
                    {
                       msgBuffOut[1] = ParamDicSetParam(keyparamTable, &keyCnt, key, value);
                    } else
                       msgBuffOut[1] = 1;
                    if (!msgBuffOut[1]) ParamDicWriteToFlash(keyparamTable, keyCnt); 
                    ISOTPSendSingleFrame(msgBuffOut, 2);
                    break;
                case PARAM_RM_RQ:
                    msgBuffOut[0] = PARAM_RM_ACK;
                    key   = msgBuff[1];
                    if ((key >= REMOTE_CHANGEABLE_KEY_AREA_LO) && (key <= REMOTE_CHANGEABLE_KEY_AREA_HI))
                    {
                      msgBuffOut[1] = ParamDicRemoveParam(keyparamTable, &keyCnt, key);
                    } else
                      msgBuffOut[1] = 1;
                    if (!msgBuffOut[1]) ParamDicWriteToFlash(keyparamTable, keyCnt); 
                    ISOTPSendSingleFrame(msgBuffOut, 2);
                    break;
                case PROG_INIT:                                      // ���� ���������� � ������ ���������������� � ������� ������� PROG_INIT, 
                   /* if (!((msgBuff[1] == SysID.systemID) &&          // ����������� �� ���, �� ���������������
                    (msgBuff[2] == SysID.blockID) &&
                    (msgBuff[3] == (unsigned char)(SysID.blockMdf>>8)) &&
                    (msgBuff[4] == (unsigned char)SysID.blockMdf) &&
                    (msgBuff[5] == 0xF2) &&
                    (msgBuff[6] == 0x5B)))*/
                     ExtractIDFromCANBuff(&IDFromCAN, msgBuff);
                     if (!
                         (((IDFromCAN.blockID == SysID.blockID)||(IDFromCAN.blockID==0)) &&
                          ((IDFromCAN.modif == SysID.modif)||(IDFromCAN.modif == 0)) &&
                          ((IDFromCAN.channel == SysID.channel)||(IDFromCAN.channel == 0)) &&
                          ((IDFromCAN.blockSN == SysID.blockSN)||(IDFromCAN.blockSN == 0)) &&
                          ((IDFromCAN.softwareModuleNum == SysID.softwareModuleNum)||(IDFromCAN.softwareModuleNum == 0)))
                         )
                      {
                        WDTCR |= (1<<WDCE)|(1<<WDE);
                        WDTCR = (1<<WDE)|(1<<WDP2)|(1<<WDP1);	// ���������� ������ ����-���� ~1,0 �
                        for(;;)
                         asm("nop");
                      }
                    break;
                case PROG_SUBMIT:
                    msgBuffOut[0] = PROG_SUBMIT_ACK;
                    switch (msgBuff[1])
                    {
                    case SUBMIT: // ��������� ���������
                      {
                        if (FileTableCheckCRC(fileTable, fileCnt, &SysID) == 0)
                        {
                          msgBuffOut[1] = SUBMIT_ACK_OK; // CRC �������
                          ISOTPSendSingleFrame(msgBuffOut, 2);
                          StartProgram(); 
                        }
                        else
                        {
                          msgBuffOut[1] = SUBMIT_ACK_FAIL; // ���-�� ����� �� ���
                          ISOTPSendSingleFrame(msgBuffOut, 2);
                          WDTCR |= (1<<WDCE)|(1<<WDE);
                          WDTCR = (1<<WDE)|(1<<WDP2)|(1<<WDP1);	// ���������� ������ ����-���� ~1,0 �
                          for(;;)
                             asm("nop");
                        }
                        break;
                      }
                    case CANCEL: // �������� ���������
                      {
                          msgBuffOut[1] = CANCEL_ACK_FAIL; // ����������� ������ ��������� ����������
                          ISOTPSendSingleFrame(msgBuffOut, 2);
                          break;
                      }
                    }
                    break;
            }
            break;
    case 2:                 // ������ flash-������
          for (int i=0; i<bytesReceived; i++)
            {
                    flashBuff[flashBuffIdx] = msgBuff[i];
                    if ((flashBuffIdx < 255))
                            {
                                    flashBuffIdx++;
                            }
                    else
                            {
                                    flashBuffIdx=0;
                                    WriteFlashPage(flashStartAddr, flashBuff);
                                    flashStartAddr+=256;                                                          
                            }
            }			
           if (lastBlock) 
            {
              for (int i=flashBuffIdx; i<256; i++)
                flashBuff[i] = 0xFF;
              WriteFlashPage(flashStartAddr, flashBuff); 
              msgBuffOut[0] = PROG_WRITE_ACK;
              msgBuffOut[1] = 0;
              ISOTPSendSingleFrame(msgBuffOut, 2);
              progstate = 1;
            }
           break;
    case 3:               // ������ EEPROM
          for (int i=0; i<bytesReceived; i++)
          {
            unsigned char __eeprom *eepromPtr;
            eepromPtr = (unsigned char __eeprom *)(eepromAddr++);
            *eepromPtr = msgBuff[i];
          }
          if (lastBlock) 
          {
             msgBuffOut[0] = PROG_WRITE_ACK;
             msgBuffOut[1] = 0;
             ISOTPSendSingleFrame(msgBuffOut, 2);
             progstate = 1;
          }
          break;    
  }
  return;
}


__C_task void main(void)
{
  
  __disable_interrupt();
  //DDRE = 0xFC;
  //PORTE = 0x08;
  //DDRC = (3<<4);
  times_ms[PROG_CONDITION_DELAY_IDX] = 0;

  MCUCR = (1<<IVCE);            
  MCUCR = (1<<IVSEL);    // ������� ���������� - � ������ Boot Loader 
  CAN_init();
  //*((unsigned int *) &GPIOR1) = (unsigned int)ParamDicGetParam;
  //a = PIND;
 
  //wdt_enable();
  	//WDTCR = (1<<WDCE)|(1<<WDE);
	WDTCR = 0x00;
 // wdt_disable(); // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  
  FIFO_init(&CANMsgBuff);
  Timer_init(); 

  __enable_interrupt();
  fileCnt = 0;
  __delay_cycles(1000000);

  FileTableReadFromFlash(fileTable, &fileCnt);
  ParamDicReadFromFlash(keyparamTable, &keyCnt);
  
  long value; 

  ParamDicGetParam(BLOCK_ID_KEY , &value);
  SysID.blockID = (unsigned int)value;
  ParamDicGetParam(MODIF_KEY, &value);
  SysID.modif = value;
  ParamDicGetParam(CHANNEL_KEY, &value);
  SysID.channel = value;  
  ParamDicGetParam(BLOCK_SN_KEY , &value);
  SysID.blockSN = value;  
  ParamDicGetParam(SOFTWARE_MODULE_NUM_KEY, &value); 
  SysID.softwareModuleNum = value;  
  
  CANMesStruc CANMsg;  
 // unsigned int tmp;

  //unsigned char msgBuff[8];  
  progstate = 0;
  
  while (!progstate) // � ����� ������� ������� �� �������� ����������
  {
   /* if (CANSTMOB & (1<<BOFF))
      PORTE = 0x08;
    PORTE = 0x40;*/
    while (!FIFO_isEmpty(&CANMsgBuff) && !progstate)
     {
       FIFO_PullFromBuff(&CANMsgBuff, &CANMsg); //������� �� FIFO ��������� 
       
       if (ExtractIDFromCANBuff(&IDFromCAN, CANMsg.MsgBody ) )
       { 
          /* PORTC = 0x20;
                   msgBuffOut[0] = 0x15;
                   PutIDToCANBuff(&IDFromCAN, msgBuffOut);
                   ISOTPSendSingleFrame(msgBuffOut, 7);*/
                   
         char fl = 0;
            if (IDFromCAN.blockID == 0) fl = 1;
             else if (IDFromCAN.blockID != SysID.blockID) continue;
            if (IDFromCAN.modif == 0) fl = 1;
	     else if (IDFromCAN.modif != SysID.modif) continue;
            if (IDFromCAN.channel ==0) fl = 1;
	     else if (IDFromCAN.channel != SysID.channel) continue;
            if (IDFromCAN.blockSN ==0) fl = 1;
             else if (IDFromCAN.blockSN != SysID.blockSN) continue;
            if (IDFromCAN.softwareModuleNum == 0) fl = 1;
             else if (IDFromCAN.softwareModuleNum != SysID.softwareModuleNum) continue;
            
               FIFO_init(&CANMsgBuff);
               if (!fl)        
               {
                // PORTE = 0x80;
                __delay_cycles(500000); 
                times_ms[PROG_CONDITION_DELAY_IDX] = 0; 
                int len;
                msgBuffOut[0] = PROG_STATUS;
                ParamDicConvertToFUDPBuff (keyparamTable, keyCnt, msgBuffOut, &len); 
                ISOTPSendMsg1(msgBuffOut, len, 0, 0, 0);
                progstate = 1; // ���� ������� �������������� �������, ��������� � ��������� 1
                
                break;
               } else
                 //if ((CANMsg.MsgBody[6] == 0) && (CANMsg.MsgBody[7] ==0))
                 {
                   msgBuffOut[0] = BROADCAST_ANSWER;
                   PutIDToCANBuff(&SysID, msgBuffOut);
                   ISOTPSendSingleFrame(msgBuffOut, 7);
                  }
       }  
     }
  }
  for(;;) // � ����� ������������ ���������� ���������
  {   
    // PORTE = 0xC0;
     if (ISOTPReceiveMessage(msgBuff, &msgBuffLen, ProcessBuffer)==1)
     {
       FIFO_init(&CANMsgBuff);
       progstate = 1;
     }
     __watchdog_reset();
  }
  
  
}