#ifndef _FIFO_H
#define _FIFO_H

#define FIFO_LEN 128 // ����� �������

typedef struct       // ��������� CAN - ������� �������
{
  unsigned char MsgBody[8]; // CAN-��������� (�������� 8 ����)
  unsigned int  ID;         // CAN-�������������
  unsigned char Len;        // ���������� ���� CAN-���������
} CANMesStruc;

typedef struct       // ������� ��������� (��������� �����)
{
  unsigned char tail;   // ������ 1-�� �������� �������
  unsigned char head;   // ������ ���������� �������� �������
  CANMesStruc Buff[FIFO_LEN]; // ������-�������
} CANFIFO;


void FIFO_init(CANFIFO *Buff);             // ������������� �������
unsigned char FIFO_isEmpty(CANFIFO *Buff); // ����� �� �������?
void FIFO_PushToBuff(CANFIFO *Buff, CANMesStruc *CANMsg); // ��������� � �������
unsigned char FIFO_PullFromBuff(CANFIFO *Buff, CANMesStruc *CANMsg); // �������

#endif