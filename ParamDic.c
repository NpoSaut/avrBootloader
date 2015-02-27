#include "paramdic.h"
#include "flash.h"
#include "globals.h"


keyparam keyparamTable[64];
unsigned char keyCnt;


int ParamDicReadFromFlash()
{
  /* ������� ��������� �� Flash-������ ����������� ������� ������� � ��������� ���
  � ������ �� ��������� keyTable, � ���������� ��������� ������� - �� ��������� keyCnt */
  
  keyCnt = *((unsigned char __farflash*)PARAMLIST_FLASHADDR);
  if (keyCnt == 0xFF) // ���� � ������ 0xFF, �������, ��� ������� ����
  {
      keyCnt = 0;
     // return -1; // ������� �� ���������
  }
  
  keyCnt+=CONST_KEYS_CONUT; // ���������� ���������� ���������� �������
  keyparamTable[0].key   = BOOTLOADER_TYPE_KEY; // ��������� � ������� ���������� ���������� ��������
  keyparamTable[0].value = BOOTLOADER_TYPE; 
  keyparamTable[1].key   = BOOTLOADER_VERSION_KEY;   
  keyparamTable[1].value = BOOTLOADER_VERSION;
  keyparamTable[2].key   = BOOTLOADER_FS_ENABLE_KEY;    
  keyparamTable[2].value = BOOTLOADER_FS_ENABLE;
  keyparamTable[3].key   = BOOTLOADER_PROTOCOL_VER_KEY;    
  keyparamTable[3].value = BOOTLOADER_PROTOCOL_VER;
  keyparamTable[4].key   = BOOTLOADER_OLD_PROTOCOL_VER_KEY;    
  keyparamTable[4].value = BOOTLOADER_OLD_PROTOCOL_VER;
    
  for (int i=CONST_KEYS_CONUT; i<keyCnt; i++)
  {
    unsigned int offset = (i-CONST_KEYS_CONUT)*5;
    keyparamTable[i].key   = *((unsigned char __farflash*)(PARAMLIST_FLASHADDR+1+offset));
    keyparamTable[i].value = *((long __farflash*)(PARAMLIST_FLASHADDR+2+offset));
  }
  return 0;
}

void ParamDicWriteToFlash(void)
{
  /* ������� ���������� �� flash-������ ����������� ������� �������. 
  */
    unsigned char tmpFlashBuff[PAGESIZE]; // �������� ������ - ��������� ������ ���  ������ �� Flash
    tmpFlashBuff[0] = keyCnt - CONST_KEYS_CONUT; // 0-� ����. - ���-�� �������. ���������� �������� � ������� ������� ������� �� �����, ������� �������� �� ����������
    unsigned int buffIdx = 1;   // ����� �� ��������� �� ����������� �������
    for (int i=CONST_KEYS_CONUT; i<keyCnt; i++) // � ������ ������� ��������� ���������� ��������. ����� �� ����������, �������� � i = 3
    {// ��������� �������� ������, ������� ����� ������� (1 ����) � �������� ������� (4 �����)
      tmpFlashBuff[buffIdx++] = keyparamTable[i].key; 
      *((unsigned long*)(tmpFlashBuff+buffIdx)) = keyparamTable[i].value;
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

int ParamDicSetParam(unsigned char key, unsigned long value)
{
    for (int i=0; i<keyCnt; i++) // ���� ���� ����� ��� ���������
    {
      if (keyparamTable[i].key == key) // �����
      {
        keyparamTable[i].value = value; // ��������
        return 0; // �����
      }
    }
    
    if (keyCnt<MAX_KEYS_COUNT) // ����� ��������� �� ����� - ������� �����
    {
       keyparamTable[keyCnt].key   = key;
       keyparamTable[keyCnt].value = value; 
       keyCnt++;
       return 0;
    }
    else
        return 2; // ��������� ������������ ���������� ������  
}

int ParamDicRemoveParam (unsigned char key)
{
    for (int i=0; i<keyCnt; i++) // ���� ���� ��������� ����
    {
      if (keyparamTable[i].key == key) // �����
      {
        for (int j=i; j<keyCnt-1; j++)
          keyparamTable[j] = keyparamTable[j+1];
          keyCnt--;
        return 0;
      }
    } 
    return 2; // �������� �� ������
}

void ParamDicConvertToFUDPBuff(unsigned char*FUDPBuff, int *FUDPBuffLen)
{
  int buffIdx=1;
  for (char i=0; i<keyCnt; i++)
  {
     FUDPBuff[buffIdx++] = (unsigned char) keyparamTable[i].key;
     *((unsigned long*)(FUDPBuff+buffIdx)) = keyparamTable[i].value;
     buffIdx+=4;
  }
  *FUDPBuffLen = buffIdx;
}
