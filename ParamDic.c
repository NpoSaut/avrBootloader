#include "paramdic.h"
#include "flash.h"
#include "globals.h"


keyparam keyparamTable[64];
unsigned char keyCnt;


int ParamDicReadFromFlash()
{
  /* Функция считывает из Flash-памяти контроллера словарь свойств и размещает его
  в буфере по указателю keyTable, а количество считанных свойств - по указателю keyCnt */
  
  keyCnt = *((unsigned char __farflash*)PARAMLIST_FLASHADDR);
  if (keyCnt == 0xFF) // если в ячейке 0xFF, считаем, что словарь пуст
  {
      keyCnt = 0;
     // return -1; // Таблица не заполнена
  }
  
  keyCnt+=CONST_KEYS_CONUT; // Прибавляем количество постоянных свойств
  keyparamTable[0].key   = BOOTLOADER_TYPE_KEY; // Добавляем в таблицу параметров постоянные свойства
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
  /* Функция записывает во flash-память контроллера словарь свойств. 
  */
    unsigned char tmpFlashBuff[PAGESIZE]; // Байтовый массив - временный буффер для  записи на Flash
    tmpFlashBuff[0] = keyCnt - CONST_KEYS_CONUT; // 0-й элем. - кол-во свойств. Постоянные свойства в словаре свойств хранить не будем, поэтому вычитаем их количество
    unsigned int buffIdx = 1;   // Встаём на следующий за количеством элемент
    for (int i=CONST_KEYS_CONUT; i<keyCnt; i++) // В начале таблицы находятся постоянные свойства. Чтобы их пропустить, начинаем с i = 3
    {// Заполняем байтовый массив, чередуя ключи свойств (1 байт) и значения свойств (4 байта)
      tmpFlashBuff[buffIdx++] = keyparamTable[i].key; 
      *((unsigned long*)(tmpFlashBuff+buffIdx)) = keyparamTable[i].value;
      buffIdx+=4;
    }
    for (int i=buffIdx; i<PAGESIZE; i++)
      tmpFlashBuff[i] = 0xFF; // Заполним неиспользованную часть страницы числом 255
    ErasePage(PARAMLIST_FLASHADDR); // Чистим страницу перед записью
    WriteFlashPage(PARAMLIST_FLASHADDR, tmpFlashBuff); // Пишем
    return;
}

int ParamDicGetParam(unsigned char key, long *value)
{
/* Функция передает значение свойства с ключем key через указатель value.
  Если свойство найдено, функция возвращает 0, в противном случае - (-1) */
  switch(key) // Сначала проверяем, не принадлежит ли ключ постоянным свойствам
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
  // Если ключ не нашли среди постоянных свойств, то пытаемся его найти в области словаря параметров на Flash
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
#pragma required = GetParam // Просим оптимизатор не выпиливать неиспользуемую функцию

int ParamDicSetParam(unsigned char key, unsigned long value)
{
    for (int i=0; i<keyCnt; i++) // Ищем ключ среди уже имеющихся
    {
      if (keyparamTable[i].key == key) // Нашли
      {
        keyparamTable[i].value = value; // Изменили
        return 0; // Вышли
      }
    }
    
    if (keyCnt<MAX_KEYS_COUNT) // Среди имеющихся не нашли - создаем новый
    {
       keyparamTable[keyCnt].key   = key;
       keyparamTable[keyCnt].value = value; 
       keyCnt++;
       return 0;
    }
    else
        return 2; // Превысили максимальное количество ключей  
}

int ParamDicRemoveParam (unsigned char key)
{
    for (int i=0; i<keyCnt; i++) // Ищем ключ удаляемый ключ
    {
      if (keyparamTable[i].key == key) // Нашли
      {
        for (int j=i; j<keyCnt-1; j++)
          keyparamTable[j] = keyparamTable[j+1];
          keyCnt--;
        return 0;
      }
    } 
    return 2; // Параметр не найден
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
