#ifndef _DEVICEID_H
#define _DEVICEID_H

typedef struct
  {
    unsigned long blockID;
    unsigned char modif;
    unsigned char softwareModuleNum;
    unsigned char channel;
    unsigned long blockSN;
  } SysIDStruc;

char ExtractIDFromCANBuff(SysIDStruc *ID,  unsigned char *Buff);
void PutIDToCANBuff(SysIDStruc *ID,  unsigned char *Buff);

#endif