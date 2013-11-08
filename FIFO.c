#include "FIFO.h"
#include "iocan128.h"

void FIFO_init(CANFIFO *Buff)
{
  Buff->head = 0;
  Buff->tail = 0;
}

unsigned char FIFO_isEmpty(CANFIFO *Buff)
{
  return (Buff->head == Buff->tail);
}


void FIFO_PushToBuff(CANFIFO *Buff, CANMesStruc *CANMsg)
{
  Buff->Buff[Buff->head++] = *CANMsg;
  Buff->head%=FIFO_LEN;
  if (Buff->head == Buff->tail) 
    {
      Buff->tail++;
      Buff->tail%=FIFO_LEN;
    }
  return;
}

unsigned char FIFO_PullFromBuff(CANFIFO *Buff, CANMesStruc *CANMsg)
{
  if (!FIFO_isEmpty(Buff))
  {
  
    *CANMsg = Buff->Buff[Buff->tail++];
    Buff->tail%=FIFO_LEN;    
    return 1;
  }

    return 0;
}

