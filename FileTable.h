#ifndef _FILETABLE_H
#define _FILETABLE_H

#include "deviceID.h" 

void FileTableClear();
int FileTableCheckCRC(SysIDStruc* SysID);
void FileTableClear();
void FileTableWriteToFlash();
int FileTableReadFromFlash();
int FindFileByAddr(unsigned long addr);
unsigned long Hex2long(unsigned char *hexStr);
int FileTableAddEntry(unsigned long addr, unsigned long size, unsigned int CRC );
int FileTableRemoveEntry(unsigned long removeAddr);
void FileTableConvertToFUDPBuff(unsigned char*FUDPBuff, int *FUDPBuffLen);

#endif