#ifndef _PARAMDIC_H
#define _PARAMDIC_H

typedef struct
{
  unsigned char key;
  long value;
} keyparam;

int ParamDicReadFromFlash();   // ������� ������� ���������� �� flash
void ParamDicWriteToFlash(void); // �������� ������� ���������� �� flash
int ParamDicGetParam(unsigned char key, long *value); // �������� �������� ��������� �� ����� (�������� ��������� �� ��������� value)
unsigned long GetParam(unsigned char key); // �������� �������� ��������� (������� ���������� �������� ���������)
int ParamDicSetParam(unsigned char key, unsigned long value); // ���������� �������� ���������
int ParamDicRemoveParam (unsigned char key); // ������� �������� �� �������
void ParamDicConvertToFUDPBuff(unsigned char*FUDPBuff, int *FUDPBuffLen); // ������������� ������� � �����, ������������ �� CAN

#endif