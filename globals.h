#include "FIFO.h"

#ifndef _GLOBALS_H
#define _GLOBALS_H

#define	HIBYTE(a) (unsigned char)(a>>8)
#define LOBYTE(a) (unsigned char)(a & 0xff)

/* Глобальные определения */

#define TIMES_COUNT 10


/* Задержка на ожидание команды "Начало программирования". Если бутлоадер, находясь
в режиме программирования, в течение этого времени не получает команд от 
программирующего устройства, то передает управление основной программе*/
#define PROG_CONDITION_DELAY_IDX 1        
#define PROG_CONDITION_DELAY_PRESET 10000 

#define ISOTP_TIMEOUT_IDX 2
#define ISOTP_TIMEOUT_PRESET 2000

/* Следующие два параметра определяют область ключей, доступных для ихменения
программирующим устройством */
#define REMOTE_CHANGEABLE_KEY_AREA_LO 1 
#define REMOTE_CHANGEABLE_KEY_AREA_HI 127


#define SYS_ID_KEY    128

#define BLOCK_ID_KEY               129
#define MODIF_KEY                  134
#define CHANNEL_KEY                133
#define BLOCK_SN_KEY               131
#define SOFTWARE_MODULE_NUM_KEY    130

  /*      
    unsigned int  blockID;
    unsigned char modif;
    unsigned char softwareModuleNum;
    unsigned char channel;
    unsigned int  blockSN;
*/
    
/* Постоянные свойства */
#define BOOTLOADER_TYPE_KEY       192 // Ключ свойства "Тип загрузчика"
#define BOOTLOADER_TYPE      2 // Значение свойства "Тип загрузчика"

#define BOOTLOADER_VERSION_KEY    193 // Ключ свойства "Версия загрузчика"
#define BOOTLOADER_VERSION   1    // Значение свойства "Версия загрузчика"

#define BOOTLOADER_FS_ENABLE_KEY  194 // Ключ свойства "Наличие файловой системы"
#define BOOTLOADER_FS_ENABLE 0    // Значение свойства "Наличие файловой системы"

#define BOOTLOADER_PROTOCOL_VER_KEY  195 // Ключ свойства "Версия протокола обновления ПО"
#define BOOTLOADER_PROTOCOL_VER 2    // Значение свойства "Версия протокола обновления ПО"

#define BOOTLOADER_OLD_PROTOCOL_VER_KEY  196 // Ключ свойства "Самая старая из поддерживаемых версий протокола обновления ПО"
#define BOOTLOADER_OLD_PROTOCOL_VER 2    // Значение свойства "Самая старая из поддерживаемых версий протокола обновления ПО"

#define FILETABLE_FLASHADDR 0x1DE00 // Начальный адрес Flash таблицы размещения блоков информации
#define PARAMLIST_FLASHADDR 0x1DF00 // Начальный адрес Flash словаря параметров
#define EEPROM_SIZE         0x1000

#define MAX_FILE_COUNT   5
#define MAX_KEYS_COUNT   50
#define CONST_KEYS_CONUT 5

// Идентификаторы команд программирования по ISO TP
#define PROG_INIT               0x01 // Переход в режим программирования
#define PROG_STATUS             0x02 // Инициализация соединения (отправка словаря параметров)
#define PROG_LIST_RQ            0x03 // Сообщение запроса списка файлов
#define PROG_LIST               0x04 // Сообщение списка файлов
#define PROG_READ_RQ            0x05 // 
#define PROG_READ               0x06
#define PROG_RM                 0x07
#define PROG_RM_ACK             0x08
#define PROG_CREATE             0x09
#define PROG_CREATE_ACK         0x0A
#define PROG_WRITE              0x0B
#define PROG_WRITE_ACK          0x0C
#define PROG_MR_PROPER          0x0D
#define PROG_MR_PROPER_ACK      0x0E
#define PARAM_SET_RQ            0x0F
#define PARAM_SET_ACK           0x10
#define PARAM_RM_RQ             0x11
#define PARAM_RM_ACK            0x12
#define PROG_SUBMIT             0x13
#define PROG_SUBMIT_ACK         0x14
#define         SUBMIT              0x00
#define             SUBMIT_ACK_OK          0x00
#define             SUBMIT_ACK_FAIL        0x01
#define         CANCEL              0x01
#define             CANCEL_ACK_OK          0x02
#define             CANCEL_ACK_FAIL        0x03
#define BROADCAST_ANSWER        0x00

#endif