#ifndef _PARAMDIC_H
#define _PARAMDIC_H

typedef struct
{
  unsigned char key;
  long value;
} keyparam;

int ParamDicReadFromFlash();   // Считать словарь параметров из flash
void ParamDicWriteToFlash(void); // Записать словарь параметров во flash
int ParamDicGetParam(unsigned char key, long *value); // Получить значение параметра по ключу (значение запишется по указателю value)
unsigned long GetParam(unsigned char key); // Получить значение параметра (функция возвращает значение параметра)
int ParamDicSetParam(unsigned char key, unsigned long value); // Установить значение параметра
int ParamDicRemoveParam (unsigned char key); // Удалить параметр из словаря
void ParamDicConvertToFUDPBuff(unsigned char*FUDPBuff, int *FUDPBuffLen); // Преобразовать словарь в буфер, отправляемый по CAN

#endif