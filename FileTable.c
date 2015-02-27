#include "FileTable.h"
#include "globals.h"
#include "defines.h"
#include "flash.h"
#include "ISOTP.h"

typedef struct
{
  //unsigned char fnameLen; // Длина имени файла
  unsigned long addr;     // Адрес в памяти
  unsigned long fsize;    // Размер файла (длина блока данных) 
  unsigned int fCRC;     // Контрольная сумма для файла
} filerecord;

filerecord fileTable[5];
unsigned char fileCnt;

unsigned char fileTableBuff[255];




unsigned int crc_ccitt_update (unsigned int crc, unsigned char data)
    {
        data ^= LOBYTE(crc);
        data ^= data << 4;

        return ((((unsigned int)data << 8) | HIBYTE(crc)) ^ (unsigned char)(data >> 4) 
                ^ ((unsigned int)data << 3));
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

int FileTableCheckCRC(SysIDStruc* SysID)
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


void FileTableClear()
{
  fileCnt = 0;
}

//#pragma inline = forced
int FileTableReadFromFlash()
{
 // *fileCnt = ReadFlashByte(FILETABLE_FLASHADDR);
  fileCnt =  *((unsigned char __farflash*)FILETABLE_FLASHADDR);
  if (fileCnt == 255) 
  {
    fileCnt = 0;
    return -1;
  }
  for (int i = 0; i<fileCnt; i++)
  {
    unsigned char offset = 12*i;

    fileTable[i].addr  = *((unsigned long __farflash*)(FILETABLE_FLASHADDR+1+offset));
    fileTable[i].fsize = *((unsigned long __farflash*)(FILETABLE_FLASHADDR+5+offset));
    fileTable[i].fCRC  = *((unsigned int __farflash*)(FILETABLE_FLASHADDR+9+offset));    
  }
  return 0; 
}

void FileTableWriteToFlash()
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

int FindFileByAddr(unsigned long addr)
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
      result += ((unsigned long)(hexStr[i]-0x30)) << ((hexStr[0]-i)<<2); // Вытаскиваем адрес файла из его имени
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
int FileTableAddEntry(unsigned long addr, unsigned long size, unsigned int CRC )
{

  unsigned int newFilePageAddr = addr/PAGESIZE; // Адрес нового блока пересчитываем из номера байта в номер страницы Flash
  unsigned int newFilePageCnt =  size/PAGESIZE + (size%PAGESIZE!=0);  
  if (fileCnt<MAX_FILE_COUNT) // Проверяем, не превышено ли максимальное кол-во файлов
  {
    for (char i=0; i<fileCnt; i++) // Проверяем, не возникло ли пересечений адресов
    {
      if (addr<0x1000000) // Единица в седьмом разряде частью адреса не является. Это признак размещения блока в EEPROM
      {
        if (fileTable[i].addr >= 0x1000000) continue;
        unsigned int existFilePageAddr = fileTable[i].addr / PAGESIZE; // Адреса сущ. блока пересчитываем из номера байта в номер страницы Flash
        unsigned int existFilePageCnt = fileTable[i].fsize/PAGESIZE + (fileTable[i].fsize%PAGESIZE!=0);
        // Проверяем все возможные варианты пересечения создаваемого файла с уже существующими и выхода за пределы допустимой области
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
    
     fileTable[fileCnt].addr  = addr;
     fileTable[fileCnt].fsize = size;
     fileTable[fileCnt].fCRC  = CRC;
     fileCnt++;
    return 0; // Всё ок. Новая запись добавлена в таблицу
  }
  else return 2;
  
}

//#pragma inline = forced
int FileTableRemoveEntry(unsigned long removeAddr)
{
   for (char i=0; i<fileCnt; i++) 
    {
      if (fileTable[i].addr == removeAddr)
      {
        if (removeAddr<0x1000000)
        {
          for (long addr=(removeAddr-removeAddr%PAGESIZE); addr<(removeAddr+fileTable[i].fsize); addr+=256)
            ErasePage(addr);
        }        
        for (char j=i+1; j<fileCnt; j++)
          fileTable[j-1] = fileTable[j];
          fileCnt--;

        return 0; // Запись удалена
      }
    }
   return 1; // Дошли до конца таблицы, так ничего и не найдя.
}


void FileTableConvertToFUDPBuff(unsigned char*FUDPBuff, int *FUDPBuffLen)
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
       if (fl) // Флаг (ведущие нули закончились)
        {
          FUDPBuff[buffIdx++] = (s<0xA)?(s+0x30):(s+0x57);
        }
     }
     if (!fl) // Если число оказалось равным 0, то формируем строку из одного символа 0x30
     {
       FUDPBuff[buffIdx++] = 3;   // Длина - 3 символ
       FUDPBuff[buffIdx++] = 'f';
       FUDPBuff[buffIdx++] = '/';
       FUDPBuff[buffIdx++] = 0x30; // Символ "0"
     }
     
     *((unsigned long*)(FUDPBuff+buffIdx)) = fileTable[i].fsize;
     *((unsigned long*)(FUDPBuff+buffIdx+4)) = CalculateCRCForFlashRegion(fileTable[i].addr, fileTable[i].fsize);//fileTable[i].fCRC;
     buffIdx+=8;
  }
  *FUDPBuffLen = buffIdx;
}
// ----------------------------------------
