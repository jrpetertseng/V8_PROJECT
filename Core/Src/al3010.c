/*
 * al3010.c
 *
 *  Created on: 2022年8月25日
 *      Author: User
 */

#include "al3010.h"
#include "i2c.h"
#include <string.h>
#include "usbd_custom_hid_if_imu.h"
#include "usb.h"

uint32_t ambientLight = 0;
static JQueueMessage_t imuReport;

#define ALS_REPORT_LENGTH       7

static char seiko_als_STREAM_1[ALS_REPORT_LENGTH] =
    { 0x0A, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00 };

static HAL_StatusTypeDef AL3010_ReadRegisters(uint16_t reg, uint8_t *data, uint16_t length);
static HAL_StatusTypeDef AL3010_WriteRegister(uint16_t reg, uint8_t *data);

HAL_StatusTypeDef AL3010_Init(void)
{
    uint8_t data;
    uint8_t errNum = 0;
    HAL_StatusTypeDef status;

    data = AL3010_SYS_CONF_DEFAULT_VALUE;
    status = AL3010_WriteRegister(AL3010_SYS_CONF, &data);
    if(status != HAL_OK)
    {
        errNum++;
    }

    data = AL3010_ALS_CONF_DEFAULT_VALUE;
    status = AL3010_WriteRegister(AL3010_ALS_CONF, &data);
    if(status != HAL_OK)
    {
        errNum++;
    }

    data = AL3010_THLL_DEFAULT_VALUE;
    status = AL3010_WriteRegister(AL3010_THLL, &data);
    if(status != HAL_OK)
    {
        errNum++;
    }

    data = AL3010_THLH_DEFAULT_VALUE;
    status = AL3010_WriteRegister(AL3010_THLH, &data);
    if(status != HAL_OK)
    {
        errNum++;
    }

    data = AL3010_THHL_DEFAULT_VALUE;
    status = AL3010_WriteRegister(AL3010_THHL, &data);
    if(status != HAL_OK)
    {
        errNum++;
    }

    data = AL3010_THHH_DEFAULT_VALUE;
    status = AL3010_WriteRegister(AL3010_THHH, &data);
    if(status != HAL_OK)
    {
        errNum++;
    }

    data = 0x00;
    status = AL3010_ReadRegisters(AL3010_SYS_CONF, &data, 1);
    if(data != AL3010_SYS_CONF_DEFAULT_VALUE)
    {
        errNum++;
    }
    status = AL3010_ReadRegisters(AL3010_ALS_CONF, &data, 1);
    if(data != AL3010_ALS_CONF_DEFAULT_VALUE)
    {
        errNum++;
    }

    if(errNum != 0)
    {
        return HAL_ERROR;
    }
    return HAL_OK;
}

HAL_StatusTypeDef AL3010_ReadData(void)
{
    uint8_t regData[2];
    uint16_t tmp;
    SCALES_TYPE scals;
    HAL_StatusTypeDef status;

    status = AL3010_ReadRegisters(AL3010_OUT_ALS_REG, regData, AL3010_OUT_ALS_LEN);
    tmp = (((uint16_t)regData[1] << 8) & 0xFF00)
               | ((uint16_t)regData[0] & 0x00FF);

    scals.GAIN_SCALE = AL3010_GAIN_SCALE_2;

    ambientLight = (uint32_t) (tmp * scals.GAIN_SCALE / 10000);

    return status;

}

HAL_StatusTypeDef AL3010_ReadData_ISR(void)
{
    uint8_t regData[2];
    uint16_t tmp;
    SCALES_TYPE scals;
    HAL_StatusTypeDef status;

    status = AL3010_ReadRegisters(AL3010_INT_STATUS, regData, 1);
    if (status != HAL_OK)
    {
        return HAL_ERROR;
    }

    status = AL3010_ReadRegisters(AL3010_OUT_ALS_REG, regData, AL3010_OUT_ALS_LEN);
    tmp = (((uint16_t)regData[1] << 8) & 0xFF00)
               | ((uint16_t)regData[0] & 0x00FF);

    scals.GAIN_SCALE = AL3010_GAIN_SCALE_2;

    ambientLight = (uint32_t) (tmp * scals.GAIN_SCALE / 10000);

    return status;

}

void ALS_SendReport_FS(void)
{
    imuReport.type               = USB_HID_IMU_INPUT_REPORT;
    imuReport.data.imuReport.len = ALS_REPORT_LENGTH;

    memcpy( seiko_als_STREAM_1+3, &ambientLight, sizeof(int32_t));
    memcpy( imuReport.data.imuReport.report, seiko_als_STREAM_1,
    		imuReport.data.imuReport.len);
    usbSendMessage( &imuReport);
}

void ALS_SendFAKE_FS(void)
{
    imuReport.type               = USB_HID_IMU_INPUT_REPORT;
    imuReport.data.imuReport.len = ALS_REPORT_LENGTH;

//    memcpy( seiko_als_STREAM_1+3, 100, sizeof(int32_t));
    seiko_als_STREAM_1[3] = 0x64;
    memcpy( imuReport.data.imuReport.report, seiko_als_STREAM_1,
            imuReport.data.imuReport.len);
    usbSendMessage( &imuReport);
}

static HAL_StatusTypeDef AL3010_WriteRegister(uint16_t reg, uint8_t *data)
{
    uint8_t ret;

    ret = HAL_I2C_Mem_Write(ALS_SENSOR_BUS, (uint16_t) (AL3010_SENSOR_ADDRESS_GND << 1),
        reg, I2C_MEMADD_SIZE_8BIT, data, 1, AL3010_MAX_DELAY);
    return ret;
}

static HAL_StatusTypeDef AL3010_ReadRegisters(uint16_t reg, uint8_t *data, uint16_t length)
{
    uint8_t ret;

    ret = HAL_I2C_Mem_Read(ALS_SENSOR_BUS, (uint16_t) (AL3010_SENSOR_ADDRESS_GND << 1),
        reg, I2C_MEMADD_SIZE_8BIT, data, length, AL3010_MAX_DELAY);
    return ret;
}


