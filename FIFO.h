#ifndef _FIFO_H
#define _FIFO_H

#define FIFO_LEN 128 // Длина очереди

typedef struct       // Сообщение CAN - элемент очереди
{
  unsigned char MsgBody[8]; // CAN-сообщение (максимум 8 байт)
  unsigned int  ID;         // CAN-идентификатор
  unsigned char Len;        // Количество байт CAN-сообщения
} CANMesStruc;

typedef struct       // Очередь сообщений (кольцевой буфер)
{
  unsigned char tail;   // Индекс 1-го элемента очереди
  unsigned char head;   // Индекс последнего элемента очереди
  CANMesStruc Buff[FIFO_LEN]; // Массив-очередь
} CANFIFO;


void FIFO_init(CANFIFO *Buff);             // Инициализация очереди
unsigned char FIFO_isEmpty(CANFIFO *Buff); // Пуста ли очередь?
void FIFO_PushToBuff(CANFIFO *Buff, CANMesStruc *CANMsg); // Поместить в очередь
unsigned char FIFO_PullFromBuff(CANFIFO *Buff, CANMesStruc *CANMsg); // Извлечь

#endif