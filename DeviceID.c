#include "deviceid.h"
#include "globals.h"

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
