/*
 * flash_rw_process.c
 *
 *  Created on: 2022年12月14日
 *      Author: User
 */

#include "flash_rw_process.h"
#include "string.h"
#include "stdio.h"

static uint32_t GetSector(uint32_t Address);

// If using STM32H7Ax/BX Series, uncomment the line below
#define FLASHWORD         4

// If using any other STM32H7 Series, uncomment the line below
//#define FLASHWORD       8

/* Some Controllers like STM32H7Ax have 128 sectors. It's not possible to write each one of them here.
   You can come up with easier ways to set the sector numbers. FOR EXAMPLE

static uint32_t GetSector(uint32_t Address)
{
  uint16_t address = Address-0x08000000;
  int mentissa = address/8192;  // Each Sector is 8 KB

  return mentissa;
}

*/

void float2Bytes(uint8_t *ftoa_bytes_temp, float float_variable) {
    union {
        float a;
        uint8_t bytes[4];
    } thing;

    thing.a = float_variable;

    for (uint8_t i = 0; i < 4; i++) {
        ftoa_bytes_temp[i] = thing.bytes[i];
    }
}

float Bytes2float(uint8_t *ftoa_bytes_temp) {
    union {
        float a;
        uint8_t bytes[4];
    } thing;

    for (uint8_t i = 0; i < 4; i++) {
        thing.bytes[i] = ftoa_bytes_temp[i];
    }

    return thing.a;
}

/* The DATA to be written here MUST be according to the List Shown Below

For EXAMPLE:- For H74x/5x, a single data must be 8 numbers of 32 bits word
If you try to write a single 32 bit word, it will automatically write 0's for the rest 7

*          - 256 bits for STM32H74x/5X devices (8x 32bits words)
*          - 128 bits for STM32H7Ax/BX devices (4x 32bits words)
*          - 256 bits for STM32H72x/3X devices (8x 32bits words)
*
*/

uint32_t Flash_Write_Data(uint32_t StartSectorAddress, uint32_t *data, uint16_t numberofwords) {
    static FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SECTORError;

    /* Unlock the Flash to enable the flash control register access */
    HAL_FLASH_Unlock();

    /* Erase the user Flash area */
    /* Get the number of sector to erase from 1st sector */

    uint32_t StartSector = GetSector(StartSectorAddress);
    uint32_t EndSectorAddress = StartSectorAddress + numberofwords * 4;
    uint32_t EndSector = GetSector(EndSectorAddress);

    /* Fill EraseInit structure */
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    EraseInitStruct.Sector = StartSector;
    // The the proper BANK to erase the Sector
    // if (StartSectorAddress < 0x08100000)
    //     EraseInitStruct.Banks     = FLASH_BANK_1;
    // else EraseInitStruct.Banks    = FLASH_BANK_2;

    // EraseInitStruct.NbSectors     = (EndSector - StartSector) + 1;
    EraseInitStruct.NbSectors = (EndSector - StartSector) + 1;

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError) != HAL_OK) {
        return HAL_FLASH_GetError();
    }

    /* Program the user Flash area 8 WORDS at a time
     * (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) */

    while (numberofwords > 0) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, StartSectorAddress, (uint32_t)*data) == HAL_OK) {
            StartSectorAddress += FLASHWORD;
            numberofwords -= 1;
            data += 1;
        } else {
            /* Error occurred while writing data in Flash memory */
            HAL_FLASH_Lock();
            // return HAL_FLASH_GetError ();
            return 1;
        }
    }

    /* Lock the Flash to disable the flash control register access */
    HAL_FLASH_Lock();

    return 0;
}

void Flash_Read_Data(uint32_t StartSectorAddress, uint32_t *data, uint16_t numberofwords) {
    while (numberofwords--) {
        *data = *(__IO uint32_t *)StartSectorAddress;
        StartSectorAddress += 4;
        data++;
    }
}

void Convert_To_Str(uint32_t *Data, char *Buf) {
    int numberofbytes = ((strlen((char *)Data) / 4) + ((strlen((char *)Data) % 4) != 0)) * 4;

    for (int i = 0; i < numberofbytes; i++) {
        Buf[i] = Data[i / 4] >> (8 * (i % 4));
    }
}

void Flash_Write_NUM(uint32_t StartSectorAddress, float Num) {
    uint8_t bytes_temp[4];

    float2Bytes(bytes_temp, Num);
    Flash_Write_Data(StartSectorAddress, (uint32_t *)bytes_temp, 1);
}

float Flash_Read_NUM(uint32_t StartSectorAddress) {
    uint8_t buffer[4];

    Flash_Read_Data(StartSectorAddress, (uint32_t *)buffer, 1);
    return Bytes2float(buffer);
}

/**
  * @brief  Gets the sector of a given address
  * @param  Address Address of the FLASH Memory
  * @retval The sector of a given address
  */
static uint32_t GetSector(uint32_t Address) {
    uint32_t sector = 0;

    if ((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0)) {
        sector = FLASH_SECTOR_0;
    } else if ((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1)) {
        sector = FLASH_SECTOR_1;
    } else if ((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2)) {
        sector = FLASH_SECTOR_2;
    } else if ((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3)) {
        sector = FLASH_SECTOR_3;
    } else if ((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4)) {
        sector = FLASH_SECTOR_4;
    } else if ((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5)) {
        sector = FLASH_SECTOR_5;
    } else if ((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6)) {
        sector = FLASH_SECTOR_6;
    } else {
        sector = FLASH_SECTOR_7;
    }

    return sector;
}
