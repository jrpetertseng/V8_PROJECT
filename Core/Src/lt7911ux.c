#include <string.h>
#include "main.h"
#include "lt7911ux.h"
#include "lt7911_fw.h"
#include <stdbool.h>
#include "usbd_cdc_if.h"

//void LT7911_ModeSwitch(int mode);
#if ENABLE_LT7911_USB_DEBUG
extern void usb_printf( const char *format, ...);
#endif

/* This is the i2c bus in J5x project, defined in main.c */
extern I2C_HandleTypeDef hi2c3;

extern const unsigned char LT7911_FW[LT7911_DEMO_FW_LENGTH];
uint8_t key_lt7911_hdcp[LT7911_HDCP_KEY_LENGTH];
//unsigned char compare_bin[LT7911_DEMO_FW_LENGTH] = { 0x0 };
unsigned char compare_bin[LT7911_DEMO_FW_LENGTH+32] = { 0x0 };
static uint32_t lt7911uxc_get_crc(uint8_t *upgradeData, uint32_t len);
void LT7911_WriteCRC( void);
#define Flash_flag 1

uint8_t FW_CRC = 0;

void LT7911_Init( void)
{
    /* Reset LT7911 */
    HAL_GPIO_WritePin( LT7911_RSTN_GPIO_Port, LT7911_RSTN_Pin, GPIO_PIN_RESET);
    HAL_Delay( 12);
    /* Release LT7911 into working mode. */
    HAL_GPIO_WritePin( LT7911_RSTN_GPIO_Port, LT7911_RSTN_Pin, GPIO_PIN_SET);
    HAL_Delay( 12);

}

//void LT7911_ModeSwitch()
//{
//    uint8_t mode;
//
////    mode = HAL_GPIO_ReadPin( SW_KEY_2D3D_GPIO_Port, SW_KEY_2D3D_Pin);
//
////    HAL_GPIO_WritePin( SW_BRG_2D3D_GPIO_Port, SW_BRG_2D3D_Pin, mode);
//    HAL_Delay(12);
//
//}

HAL_StatusTypeDef HDMI_WriteI2C_Byte( uint16_t addr, uint8_t buf)
{
    HAL_StatusTypeDef nRet = HAL_I2C_Mem_Write( &hi2c3, (LT7911_DEV_ADDR << 1),
            addr, I2C_MEMADD_SIZE_8BIT, &buf, 1, LT7911_TIMEOUT);

#if ENABLE_LT7911_USB_DEBUG
    if(HAL_OK != nRet)
    {
        usb_printf( "err %s: [%d]\n", __FUNCTION__, __LINE__);
    }
#endif

    return nRet;
}

HAL_StatusTypeDef HDMI_WriteI2C_ByteN( uint16_t addr, uint8_t *buf,
        uint16_t size)
{
    HAL_StatusTypeDef nRet = HAL_I2C_Mem_Write( &hi2c3, (LT7911_DEV_ADDR << 1),
            addr, I2C_MEMADD_SIZE_8BIT, buf, size, LT7911_TIMEOUT);

#if ENABLE_LT7911_USB_DEBUG
    if(HAL_OK != nRet)
    {
        usb_printf( "err %s: [%d]\n", __FUNCTION__, __LINE__);
    }
#endif

    return nRet;
}

uint8_t HDMI_ReadI2C_Byte( uint16_t addr)
{
    //GPIO_PinState state;
    uint8_t data;

    if(HAL_OK
            != HAL_I2C_Mem_Read( &hi2c3, (LT7911_DEV_ADDR << 1), addr,
                    I2C_MEMADD_SIZE_8BIT, &data, 1, LT7911_TIMEOUT))
    {
        data = 0x00;
#if ENABLE_LT7911_USB_DEBUG
        usb_printf( "err %s: [%d]\n", __FUNCTION__, __LINE__);
#endif
    }

    return data;
}


HAL_StatusTypeDef HDMI_ReadI2C_ByteN( uint16_t addr, uint8_t *buf,
        uint16_t size)
{
    return HAL_I2C_Mem_Read( &hi2c3, (LT7911_DEV_ADDR << 1), addr,
            I2C_MEMADD_SIZE_8BIT, buf, size, LT7911_TIMEOUT);
}

// lt7911uxc_t lt7911uxc_CheckIDTable[] = 
// {
//     {0x86,0xff,0xe1,0x00},
//     {0x86,0x00,0x02,0xFF},
// 	{0x00,0x00,0x00,0x00},
// };


uint16_t LT7911_ReadChipId( void)  //59 CRC address
{
//    uint8_t bA000=0, bA001=0;
    uint16_t nChipId = 0;
    uint8_t buf[2];
    //EnableI2C
    HDMI_WriteI2C_Byte( 0xFF, 0xE0);
    HDMI_WriteI2C_Byte( 0xEE, 0x01);

    //CheckID
    HDMI_WriteI2C_Byte( 0xFF, 0xE1);
    HAL_Delay( 2);
    HDMI_ReadI2C_ByteN( 0x00, buf, 2);

    nChipId = ((((uint16_t)buf[0]) << 8) & 0xFF00)
            | (((uint16_t)buf[1]) & 0x00FF);

    return nChipId;
}

void LT7911_Config( void)
{
    HDMI_WriteI2C_Byte( 0xFF, 0xE0);
    HDMI_WriteI2C_Byte( 0xEE, 0x01);

    HDMI_WriteI2C_Byte( 0x5E, 0xC1);
    HDMI_WriteI2C_Byte( 0x58, 0x00);
    HDMI_WriteI2C_Byte( 0x59, 0x50);
    HDMI_WriteI2C_Byte( 0x5A, 0x10);
    HDMI_WriteI2C_Byte( 0x5A, 0x00);
    HDMI_WriteI2C_Byte( 0x58, 0x21);
}

/* config
86 ff e0 00
86 ee 01 00
86 5e c1 00
86 58 00 00
86 59 50 00
86 5a 10 00
86 5a 00 00
86 58 21 00
*/


HAL_StatusTypeDef LT7911_ReadHDCPKey( uint8_t *pKey)
{
    uint8_t bRead;
    uint8_t addr[3] = { 0, 0, 0 };
    uint8_t byPageReadData[256];
    long lReadAddr;
    int nPage, i;
    int nPageReadLen;

    HAL_StatusTypeDef bRet;

    HDMI_WriteI2C_Byte( 0xFF, 0x80);
    HDMI_WriteI2C_Byte( 0xEE, 0x01);

    // reset fifo
    HDMI_WriteI2C_Byte( 0xFF, 0x90);

    HAL_Delay( 2);
    bRead = HDMI_ReadI2C_Byte( 0x02);
    bRead &= 0xDF;
    HDMI_WriteI2C_Byte( 0x02, bRead);
    bRead |= 0x20;
    HDMI_WriteI2C_Byte( 0x02, bRead);

    // wren enable
    HDMI_WriteI2C_Byte( 0xFF, 0x80);
    HDMI_WriteI2C_Byte( 0x5A, 0x86);
    HDMI_WriteI2C_Byte( 0x5A, 0x82);

    lReadAddr = 0x6000; //Key starting Address in the Flash is 0x6000
    addr[0] = (lReadAddr & 0xFF0000) >> 16;
    addr[1] = (lReadAddr & 0xFF00) >> 8;
    addr[2] = lReadAddr & 0xFF;

    // The are a total of 286 bytes in the key , read up to 16 bytes at a time
    nPage = LT7911_HDCP_KEY_LENGTH / 16;
    if(LT7911_HDCP_KEY_LENGTH % 16 != 0)
    {
        ++nPage;
    }

    // read npage times 16 bytes
    for( i = 0; i < nPage; ++i)
    {
        // Set the read address
        HDMI_WriteI2C_Byte( 0x5E, 0x60 | 0xF);
        HDMI_WriteI2C_Byte( 0x5A, 0xA2);
        HDMI_WriteI2C_Byte( 0x5A, 0x82);
        HDMI_WriteI2C_Byte( 0x5B, addr[0]);
        HDMI_WriteI2C_Byte( 0x5C, addr[1]);
        HDMI_WriteI2C_Byte( 0x5D, addr[2]);
        HDMI_WriteI2C_Byte( 0x5A, 0x92);
        HDMI_WriteI2C_Byte( 0x5A, 0x82);
        HDMI_WriteI2C_Byte( 0x58, 0x01);

        nPageReadLen = 16;

        //There is no 16 bytes in the last time , calculate the length
        if((LT7911_HDCP_KEY_LENGTH - (i * 16)) < 16)
        {
            //This nPageReadLen is the last length
            nPageReadLen = LT7911_HDCP_KEY_LENGTH - i * 16;
        }

        //Read key data from register 0x5F and save them
        memset( byPageReadData, 0, 256);
        HAL_Delay( 2);
        bRet = HDMI_ReadI2C_ByteN( 0x5F, byPageReadData, nPageReadLen);
#if ENABLE_LT7911_USB_DEBUG
        if(HAL_OK == bRet)
        {
            /* Display the key. */
            usb_printf(
                    "key[%d]: [%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X]\n",
                    i, ((int)byPageReadData[0]) & 0xFF,
                    ((int)byPageReadData[1]) & 0xFF,
                    ((int)byPageReadData[2]) & 0xFF,
                    ((int)byPageReadData[3]) & 0xFF,
                    ((int)byPageReadData[4]) & 0xFF,
                    ((int)byPageReadData[5]) & 0xFF,
                    ((int)byPageReadData[6]) & 0xFF,
                    ((int)byPageReadData[7]) & 0xFF,
                    ((int)byPageReadData[8]) & 0xFF,
                    ((int)byPageReadData[9]) & 0xFF,
                    ((int)byPageReadData[10]) & 0xFF,
                    ((int)byPageReadData[11]) & 0xFF,
                    ((int)byPageReadData[12]) & 0xFF,
                    ((int)byPageReadData[13]) & 0xFF,
                    ((int)byPageReadData[14]) & 0xFF,
                    ((int)byPageReadData[15]) & 0xFF);
            memcpy( pKey + (i * 16), byPageReadData, nPageReadLen);
        }
        else
        {
            /* Error */
            usb_printf( "err %s: [%d]\n", __FUNCTION__, __LINE__);
        }
#endif
        if(HAL_OK != bRet)
        {
            /* Always return error. */
            return HAL_ERROR;
        }

        // The read address is increased by 16 each time
        lReadAddr += nPageReadLen;
        addr[0] = (lReadAddr & 0xFF0000) >> 16;
        addr[1] = (lReadAddr & 0xFF00) >> 8;
        addr[2] = lReadAddr & 0xFF;
    }

    return HAL_OK;
}

HAL_StatusTypeDef LT7911_WriteHDCPKey( uint8_t *pKey)
{
    uint8_t bRead;
    uint8_t addr[3] = { 0x00, 0x60, 0x00 };
    long lWriteAddr, lStartAddr;
    //long    lEndAddr;
    int nPage, j;

    HAL_StatusTypeDef bRet;

    HDMI_WriteI2C_Byte( 0xFF, 0x80);
    HDMI_WriteI2C_Byte( 0xEE, 0x01);

    // reset fifo
    HDMI_WriteI2C_Byte( 0xFF, 0x90);
    HAL_Delay( 2);
    bRead = HDMI_ReadI2C_Byte( 0x02);
    bRead &= 0xDF;
    HDMI_WriteI2C_Byte( 0x02, bRead);
    bRead |= 0x20;
    HDMI_WriteI2C_Byte( 0x02, bRead);

    addr[0] = 0x00;
    addr[1] = 0x60;
    addr[2] = 0x00;

    HDMI_WriteI2C_Byte( 0xFF, 0x80);
    HDMI_WriteI2C_Byte( 0x5A, 0x86);
    HDMI_WriteI2C_Byte( 0x5A, 0x82);

    lWriteAddr = 0x6000;
    lStartAddr = lWriteAddr;
    //lEndAddr   = lWriteAddr;

    nPage = LT7911_HDCP_KEY_LENGTH / 16;
    if((LT7911_HDCP_KEY_LENGTH % 16) != 0)
    {
        ++nPage;
    }

    for( j = 0; j < nPage; ++j)
    {
        HDMI_WriteI2C_Byte( 0x5A, 0x86);
        HDMI_WriteI2C_Byte( 0x5A, 0x82);

        HDMI_WriteI2C_Byte( 0x5E, 0xE0 | 0xF);
        HDMI_WriteI2C_Byte( 0x5A, 0xA2);
        HDMI_WriteI2C_Byte( 0x5A, 0x82);

        HDMI_WriteI2C_Byte( 0x58, 0x01);

        bRet = HDMI_WriteI2C_ByteN( 0x59, pKey + (j * 16), 16);
        if(HAL_OK != bRet)
        {
            /* Error */
#if ENABLE_LT7911_USB_DEBUG
            usb_printf( "%s: [%d] [%d]\n", __FUNCTION__, __LINE__, j);
#endif
            return HAL_ERROR;
        }

        HDMI_WriteI2C_Byte( 0x5B, addr[0]);
        HDMI_WriteI2C_Byte( 0x5C, addr[1]);
        HDMI_WriteI2C_Byte( 0x5D, addr[2]);
        HDMI_WriteI2C_Byte( 0x5E, 0xE0);
        HDMI_WriteI2C_Byte( 0x5A, 0x92);
        HDMI_WriteI2C_Byte( 0x5A, 0x82);

        lStartAddr += 16;
        addr[0] = (lStartAddr & 0xFF0000) >> 16;
        addr[1] = (lStartAddr & 0xFF00) >> 8;
        addr[2] = lStartAddr & 0xFF;
    }

    HDMI_WriteI2C_Byte( 0x5A, 0x8A);
    HDMI_WriteI2C_Byte( 0x5A, 0x82);

    return HAL_OK;
}

void LT7911_BlockErase( void)
{
    HDMI_WriteI2C_Byte( 0xFF, 0xE0);
    HDMI_WriteI2C_Byte( 0xEE, 0x01);

    HDMI_WriteI2C_Byte( 0x5A, 0x04);
    HDMI_WriteI2C_Byte( 0x5A, 0x00);

    HDMI_WriteI2C_Byte( 0x5B, 0x00);
    HDMI_WriteI2C_Byte( 0x5C, 0x00);
    HDMI_WriteI2C_Byte( 0x5D, 0x00);

    HDMI_WriteI2C_Byte( 0x5A, 0x01);
    HDMI_WriteI2C_Byte( 0x5A, 0x00);
    HAL_Delay(500); 

    HDMI_WriteI2C_Byte( 0xFF, 0xE0);
    HDMI_WriteI2C_Byte( 0xEE, 0x01);

    HDMI_WriteI2C_Byte( 0x5A, 0x04);
    HDMI_WriteI2C_Byte( 0x5A, 0x00);

    HDMI_WriteI2C_Byte( 0x5B, 0x00);
    HDMI_WriteI2C_Byte( 0x5C, 0x80);
    HDMI_WriteI2C_Byte( 0x5D, 0x00);

    HDMI_WriteI2C_Byte( 0x5A, 0x01);
    HDMI_WriteI2C_Byte( 0x5A, 0x00);
    HAL_Delay(500);

}

/*
86 5a 04 00
86 5a 00 00
86 5b 00 00 // flash address[23:16]
86 5c 00 00 // flash address[15:8]
86 5d 00 00 // flash address[7:0]
86 5a 01 00 // half-block erase (32KB)
86 5a 00 00
// delay 500ms
86 5a 04 00
86 5a 00 00
86 5b 00 00 // flash address[23:16]
86 5c 80 00 // flash address[15:8]
86 5d 00 00 // flash address[7:0]
86 5a 01 00 // half-block erase (32KB)
86 5a 00 00
// delay 500ms
  */

void LT7911_WriteFirmware( uint8_t *byWriteData, uint32_t DataLen)
{
//    uint8_t bRead;
    uint8_t addr[3] = { 0, 0, 0 };
    long lWriteAddr, lStartAddr;
    int nPage, j;

    HAL_StatusTypeDef bRet;

    HDMI_WriteI2C_Byte( 0xFE, 0xE0);
    HDMI_WriteI2C_Byte( 0xEE, 0x01);


    // Firmware data starting address is 0x00
    lWriteAddr = 0;
    lStartAddr = lWriteAddr;

    //The are a total of Datalen bytes in the firmware , read up to 16 bytes at a time
    nPage = DataLen / 32;
    if(0x0 != (DataLen % 32))
    {
        ++nPage;
    }
    addr[0] = (lStartAddr & 0xFF0000) >> 16;
    addr[1] = (lStartAddr & 0xFF00) >> 8;
    addr[2] = lStartAddr & 0xFF;
    // write npage times 16 bytes
    for( j = 0; j < nPage; ++j)
    {
        //wren
          HDMI_WriteI2C_Byte( 0xFF, 0xE1);
          HDMI_WriteI2C_Byte( 0x03, 0x2E);
          HDMI_WriteI2C_Byte( 0x03, 0xEE);

          HDMI_WriteI2C_Byte( 0xFF, 0xE0);
          HDMI_WriteI2C_Byte( 0x5A, 0x04);
          HDMI_WriteI2C_Byte( 0x5A, 0x00);

          //i2c data to fifo
          HDMI_WriteI2C_Byte( 0x5E, 0xDF);
          HDMI_WriteI2C_Byte( 0x5A, 0x20);
          HDMI_WriteI2C_Byte( 0x5A, 0x00);

          HDMI_WriteI2C_Byte( 0x58, 0x21);

        bRet = HDMI_WriteI2C_ByteN( 0x59, byWriteData + (j * 32), 32);
#if 1
        if(HAL_OK != bRet)
        {
            /* Error */
            usb_printf( "%s: [%d] [%d]\n", __FUNCTION__, __LINE__, j);
//            HAL_Delay(1);
        }
#else
		(void)bRet;
#endif

        HDMI_WriteI2C_Byte( 0x5B, addr[0]);
        HDMI_WriteI2C_Byte( 0x5C, addr[1]);
        HDMI_WriteI2C_Byte( 0x5D, addr[2]);
        HDMI_WriteI2C_Byte( 0x5A, 0x10);
        HDMI_WriteI2C_Byte( 0x5A, 0x00);

        lStartAddr += 32;
        addr[0] = (lStartAddr & 0xFF0000) >> 16;
        addr[1] = (lStartAddr & 0xFF00) >> 8;
        addr[2] = lStartAddr & 0xFF;
    }

    LT7911_WriteCRC();
    //wrdi
    HDMI_WriteI2C_Byte( 0x5A, 0x08);
    HDMI_WriteI2C_Byte( 0x5A, 0x00);
}

void LT7911_ReadbackFirmware( uint8_t *byReadData, uint32_t WriteDataLen)
{
//    uint8_t bRead;
    uint8_t addr[3] = { 0, 0, 0 };
    uint8_t byPageReadData[32];
    long lReadAddr;
    int nPage, i;
    int nPageReadLen;

    HAL_StatusTypeDef bRet;

    LT7911_Config();
    lReadAddr = 0;

    addr[0] = (lReadAddr & 0xFF0000) >> 16;
    addr[1] = (lReadAddr & 0xFF00) >> 8;
    addr[2] = lReadAddr & 0xFF;

    //WriteDatalen is firmware length, read up to 32 bytes at a time
    nPage = WriteDataLen / 32;
    if((WriteDataLen % 32) != 0)
    {
        ++nPage;
    }

    for( i = 0; i < nPage; ++i)
    {
        // flash to fifo1
        HDMI_WriteI2C_Byte( 0x5e, 0x5f);
        HDMI_WriteI2C_Byte( 0x5a, 0x20);
        HDMI_WriteI2C_Byte( 0x5a, 0x00);

        HDMI_WriteI2C_Byte( 0x5B, addr[0]);
        HDMI_WriteI2C_Byte( 0x5C, addr[1]);
        HDMI_WriteI2C_Byte( 0x5D, addr[2]);

        // flash to fifo2
        HDMI_WriteI2C_Byte( 0x5A, 0x10);
        HDMI_WriteI2C_Byte( 0x5A, 0x00);

        HDMI_WriteI2C_Byte( 0x58, 0x21);

        nPageReadLen = 32; //16
        if((WriteDataLen - (i * 32)) < 32)
        {
            nPageReadLen = WriteDataLen - (i * 32);
        }

        memset( byPageReadData, 0, 32); //16*8=128

        bRet = HDMI_ReadI2C_ByteN( 0x5F, byPageReadData, nPageReadLen);
        if(HAL_OK == bRet)
        {
            /* Move the data into user buffer. */
            memcpy( byReadData + (i * 32), byPageReadData, nPageReadLen);
//            usb_printf("byPageReadData: 0x%04X \r\n", byPageReadData);
        }
        else usb_printf("read page error");

        // The read address is increased by 16 each time
        lReadAddr += nPageReadLen;
        addr[0] = (lReadAddr & 0xFF0000) >> 16;
        addr[1] = (lReadAddr & 0xFF00) >> 8;
        addr[2] = lReadAddr & 0xFF;
    }
    //wrdi
    HDMI_WriteI2C_Byte( 0x5A, 0x08);
    HDMI_WriteI2C_Byte( 0x5A, 0x00);
}

void LT7911_WriteCRC( void)
{
//    uint8_t bRead;
    uint8_t addr[3] = { 0, 0, 0 };
    long lWriteAddr, lStartAddr;
    // HDMI_WriteI2C_Byte( 0xFE, 0xE0);
    // HDMI_WriteI2C_Byte( 0xEE, 0x01);

    //wren
    HDMI_WriteI2C_Byte( 0xFF, 0xE1);
    HDMI_WriteI2C_Byte( 0x03, 0x2E);
    HDMI_WriteI2C_Byte( 0x03, 0xEE);

    HDMI_WriteI2C_Byte( 0xFF, 0xE0);
    HDMI_WriteI2C_Byte( 0x5A, 0x04);
    HDMI_WriteI2C_Byte( 0x5A, 0x00);

    //i2c data to fifo
    HDMI_WriteI2C_Byte( 0x5E, 0xDF);
    HDMI_WriteI2C_Byte( 0x5A, 0x20);
    HDMI_WriteI2C_Byte( 0x5A, 0x00);

    HDMI_WriteI2C_Byte( 0x58, 0x21);


    //write crc data
    lWriteAddr = 65535;
    lStartAddr = lWriteAddr;

    HDMI_WriteI2C_Byte( 0x59, FW_CRC);

    addr[0] = (lStartAddr & 0xFF0000) >> 16;
    addr[1] = (lStartAddr & 0xFF00) >> 8;
    addr[2] = lStartAddr & 0xFF;

        HDMI_WriteI2C_Byte( 0x5B, addr[0]);
        HDMI_WriteI2C_Byte( 0x5C, addr[1]);
        HDMI_WriteI2C_Byte( 0x5D, addr[2]);
        HDMI_WriteI2C_Byte( 0x5A, 0x10);
        HDMI_WriteI2C_Byte( 0x5A, 0x00);
}

/*=================================Test ALL Flash========================================*/
void LT7911_ReadbackAllFlash( uint8_t *byReadData)
{
    uint8_t addr[3] = { 0, 0, 0 };
    uint8_t byPageReadData[32] = {0};
//    uint8_t byReadData[32] = {0};
    long lReadAddr;
    int nPage, i;
    int nPageReadLen;

    HAL_StatusTypeDef bRet;

    LT7911_Config();
    lReadAddr = 0;
    uint32_t WriteDataLen = 65536;
    addr[0] = (lReadAddr & 0xFF0000) >> 16;
    addr[1] = (lReadAddr & 0xFF00) >> 8;
    addr[2] = lReadAddr & 0xFF;

    //WriteDatalen is firmware length, read up to 32 bytes at a time
    nPage = WriteDataLen / 32;
    if((WriteDataLen % 32) != 0)
    {
        ++nPage;
    }

    for( i = 0; i < nPage; ++i)
    {
        // flash to fifo1
        HDMI_WriteI2C_Byte( 0x5e, 0x5f);
        HDMI_WriteI2C_Byte( 0x5a, 0x20);
        HDMI_WriteI2C_Byte( 0x5a, 0x00);

        HDMI_WriteI2C_Byte( 0x5B, addr[0]);
        HDMI_WriteI2C_Byte( 0x5C, addr[1]);
        HDMI_WriteI2C_Byte( 0x5D, addr[2]);

        // flash to fifo2
        HDMI_WriteI2C_Byte( 0x5A, 0x10);
        HDMI_WriteI2C_Byte( 0x5A, 0x00);

        HDMI_WriteI2C_Byte( 0x58, 0x21);

        nPageReadLen = 32; //16
        if((WriteDataLen - (i * 32)) < 32)
        {
            nPageReadLen = WriteDataLen - (i * 32);
        }

        memset( byPageReadData, 0, 32); //16*8=128

        bRet = HDMI_ReadI2C_ByteN( 0x5F, byPageReadData, nPageReadLen);
        if(HAL_OK == bRet)
        {
            HAL_Delay(1);
            for (int j=0; j<8;j++){
            usb_printf("0x%2X, 0x%2X,  0x%2X, 0x%2X\r\n",
                    byPageReadData[j*4], byPageReadData[j*4+1],
                    byPageReadData[j*4+2],byPageReadData[j*4+3]);
            }
//            memcpy( byReadData + (i * 32), byPageReadData, nPageReadLen);
            /* Move the data into user buffer. */
//            memset( byReadData, 0x00, 32);
//            memcpy( byReadData, LT7911_FW + (i * 32), nPageReadLen);
//            if (memcmp( byReadData, byPageReadData, nPageReadLen))
//            {
////                HAL_Delay(1);
//                usb_printf("Data net match: Flash:0x%4X, FW: 0x%4X\r\n",
//                        byPageReadData, byReadData);
//            }
//            usb_printf("byPageReadData: 0x%04X \r\n", byPageReadData);
        }
        else usb_printf("read page error\r\n");

        // The read address is increased by 16 each time
        lReadAddr += nPageReadLen;
        addr[0] = (lReadAddr & 0xFF0000) >> 16;
        addr[1] = (lReadAddr & 0xFF00) >> 8;
        addr[2] = lReadAddr & 0xFF;
    }
    //wrdi
    HDMI_WriteI2C_Byte( 0x5A, 0x08);
    HDMI_WriteI2C_Byte( 0x5A, 0x00);
//    for( i = 0; i < nPage; ++i)
//    {
//
//        usb_printf("0x%2X, 0x%2X, 0x%2X, 0x%2x \r\n", *(byReadData + i*32),
//                *(byReadData + (i*32)+8), *(byReadData + (i*32)+16), *(byReadData + (i*32)+24));
//        HAL_Delay(1);
//    }
    usb_printf("ReadbackAllFlash Done \r\n");
}


void lt7911_firmware_update_init( void)
{
    uint8_t run_version[4];
    do
    {
        usb_printf( "lt7911_firmware_update init.. \r\n");
        HAL_Delay(5000);
        usb_printf( "lt7911_firmware_update init start \r\n");
        HAL_Delay(1000);
        lt7911uxc_version(run_version);
        HAL_Delay(10);
        uint16_t chipId = LT7911_ReadChipId();
        FW_CRC = lt7911uxc_get_crc(LT7911_FW, LT7911_DEMO_FW_LENGTH);
        usb_printf( "FW_CRC:[%02X]\r\n", FW_CRC);
        // Step1 : Read chip ID to check I2C connection
        if(LT7911_CHIP_ID != chipId)
        {
            /* Chip id mismatched, skip the update init procedure. */
            usb_printf( "Err chip id:[%04X]\r\n", chipId);
            break;
        }
        usb_printf( "Rd chip id:[%04X]\r\n", chipId);

        // Step2 : Initial Settings
        LT7911_Config();

        // Step3 : Read the hdcp key which is stored in the flash and save it.
        // memset( key_lt7911_hdcp, 0x0, LT7911_HDCP_KEY_LENGTH);
        // if(HAL_OK != LT7911_ReadHDCPKey( key_lt7911_hdcp))
        // {
        //     /* OMG, error, S.O.S.!!! */
        //     usb_printf( "Rd HDCP key err\n");
        //     break;
        // }

//         Step4 Block Erase, we have no way back after this.
#if Flash_flag
        LT7911_BlockErase();

//         Step5 : Write the firmware data into flash
//         Note the buffer length MUST be multiple of 32, and the empty buffer
//         MUST be 0xFF


        memset( compare_bin, 0xFF, LT7911_DEMO_FW_LENGTH+32);
        usb_printf( "Flash Bridge FW \r\n");
        memcpy( compare_bin, LT7911_FW,
        LT7911_DEMO_FW_LENGTH);
        LT7911_WriteFirmware( (uint8_t*)compare_bin, LT7911_DEMO_FW_LENGTH);

        // Step6 : Read the data in the flash and compare it with original data
        usb_printf( "Read Bridge FW \r\n");

        memset( compare_bin, 0x00, LT7911_DEMO_FW_LENGTH);
        LT7911_ReadbackFirmware( compare_bin, LT7911_DEMO_FW_LENGTH);

        if(memcmp( compare_bin, LT7911_FW,
        LT7911_DEMO_FW_LENGTH))
        {
            /* FLASH content is inconsistent. */
            usb_printf( "err FW W then R!!!\n");
            HAL_Delay(100);
        }
        else
        {
            /* FLASH content is consistent. */
            usb_printf( "FW W then R success.\n");
            HAL_Delay(100);
        }
        usb_printf( "LT7911 update finished\n");
#else
        memset( compare_bin, 0x00, 65536);
        LT7911_ReadbackAllFlash(compare_bin);
#endif

    } while(0);
}


typedef struct
{
    uint8_t Width;
    uint32_t  Poly;
    uint32_t  CrcInit;
    uint32_t  XorOut;
    bool RefIn;
    bool RefOut;
}CrcInfoTypeS;


/**
 *******************************************************************************
 * @brief   bit reverse
 * @param   [in] inVal	- in data
 * @param   [in] bits   - inverse bit
 * @return  out data
 * @note
 *******************************************************************************
 */
unsigned int BitsReverse(uint32_t inVal, uint8_t bits)
{
    uint32_t outVal = 0;
    uint8_t i;

    for(i=0; i<bits; i++)
    {
        if(inVal & (1 << i)) outVal |= 1 << (bits - 1 - i);
    }

    return outVal;
}

/**
 *******************************************************************************
 * @brief   get crc value
 * @param   [in] type   - CRC config
 * @param   [in] *buf   - data buffer
 * @param   [in] bufLen - data len
 * @return  crc value
 * @note
 *******************************************************************************
 */
unsigned int GetCRC(CrcInfoTypeS type, uint8_t *buf, uint32_t bufLen)
{
    uint8_t width  = type.Width;
    uint32_t  poly   = type.Poly;
    uint32_t  crc    = type.CrcInit;
    uint32_t  xorout = type.XorOut;
    bool refin  = type.RefIn;
    bool refout = type.RefOut;
    uint8_t n;
    uint32_t  bits;
    uint32_t  data;
    uint8_t i;

    n    =  (width < 8) ? 0 : (width-8);
    crc  =  (width < 8) ? (crc<<(8-width)) : crc;
    bits =  (width < 8) ? 0x80 : (1 << (width-1));
    poly =  (width < 8) ? (poly<<(8-width)) : poly;
    while(bufLen--)
    {
        data = *(buf++);
        if(refin == true)
            data = BitsReverse(data, 8);
        crc ^= (data << n);
        for(i=0; i<8; i++)
        {
            if(crc & bits)
            {
                crc = (crc << 1) ^ poly;
            }
            else
            {
                crc = crc << 1;
            }
        }
    }
    crc = (width<8) ? (crc>>(8-width)) : crc;
    if(refout == true)
        crc = BitsReverse(crc, width);
    crc ^= xorout;

    return (crc & ((2<<(width-1)) - 1));
}

static uint32_t lt7911uxc_get_crc(uint8_t *upgradeData, uint32_t len)
{
#define LT7911UXC_FW_AREA_SIZE              (64 * 1024)
    CrcInfoTypeS type =
    {
        .Width = 8,
        .Poly  = 0x31,
        .CrcInit = 0,
        .XorOut = 0,
        .RefOut = false,
        .RefIn = false,
    };
    uint32_t crc_size = LT7911UXC_FW_AREA_SIZE - 1;
    uint8_t default_val = 0xFF;

    type.CrcInit = GetCRC(type, upgradeData, len);

    crc_size -= len;
    while(crc_size--)
    {
        type.CrcInit = GetCRC(type, &default_val, 1);
    }
    return type.CrcInit;
}
void lt7911uxc_version(uint8_t *run_version)
{

    HDMI_WriteI2C_Byte( 0xFF, 0xE0); //0xE0

    HAL_Delay(2);
    HDMI_ReadI2C_ByteN( 0x80, run_version, 4);

}
