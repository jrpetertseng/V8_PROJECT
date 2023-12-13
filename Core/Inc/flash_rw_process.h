/*
 * flash_rw_process.h
 *
 *  Created on: 2022年12月14日
 *      Author: User
 */

#ifndef INC_FLASH_RW_PROCESS_H_
#define INC_FLASH_RW_PROCESS_H_


#define APP1_START (0x08040000)         //Origin + Bootloader size (64kB)
#define APP2_START (0x08060000)         //Origin + Bootloader size (64kB) + App1 Bank (320kB)
#define FLASH_BANK_SIZE (0X20000)       //128K
#define FLASH_PAGE_SIZE_USER (0x400)    //1kB


#include "main.h"
#include "usbd_cdc_if.h"


#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbytes */

uint32_t Flash_Write_Data (uint32_t StartSectorAddress, uint32_t *data, uint16_t numberofwords);
void Flash_Read_Data (uint32_t StartSectorAddress, uint32_t *data, uint16_t numberofwords);
void Convert_To_Str (uint32_t *Data, char *Buf);

void Flash_Write_NUM (uint32_t StartSectorAddress, float Num);
float Flash_Read_NUM (uint32_t StartSectorAddress);

typedef struct _ECX343_DATA
{
    //ECX343_PARAMETER_DATA
    uint32_t  uLCD_INVL;
    uint32_t  uLCD_INVR;
    uint32_t  uLCD_LUXL;
    uint32_t  uLCD_LUXR;
    uint32_t  uLCD_HORBL;
    uint32_t  uLCD_HORBR;
    uint32_t  uLCD_VORBL;
    uint32_t  uLCD_VORBR;
    uint32_t  uLCD_MODE; //0:60/2d, 1:120/2d, 2:60/3d, 3:120/3d
    uint32_t  checksum;
    uint32_t  uLCD_DEVICEL[5];
    uint32_t  uLCD_DEVICER[5];
}__packed ECX343_DATA;

typedef enum {
    CheckSum_FAIL,
    CheckSum_OK,
} CheckSum_status;

#endif /* INC_FLASH_RW_PROCESS_H_ */
