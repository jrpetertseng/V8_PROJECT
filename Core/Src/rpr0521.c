/*
 * rpr0521.c
 *
 *  Created on: Jan 12, 2023
 *      Author: User
 */

#include "rpr0521.h"
#include "i2c.h"

static HAL_StatusTypeDef WriteRegister(uint16_t reg, uint8_t *value);
static HAL_StatusTypeDef ReadRegister(uint16_t reg, uint8_t *value);

HAL_StatusTypeDef RPR0521_Init(void)
{
    uint8_t data = 0x00;
    uint8_t errNum = 0;
    HAL_StatusTypeDef status;

    status = ReadRegister(MANUFACT_ID , &data);
    if(status != HAL_OK)
    {
        return HAL_ERROR;
    }
    if(data != MANUFACT_ID_VALUE)
    {
        return HAL_ERROR;
    }

    data = SYSTEM_CONTROL_VALUE;
    status = WriteRegister(SYSTEM_CONTROL, &data);
    if(status != HAL_OK)
    {
        errNum++;
    }

    data = MODE_CONTROL_DEFAULT_VALUE;
    status = WriteRegister(MODE_CONTROL, &data);
    if(status != HAL_OK)
    {
        errNum++;
    }

    data = ALS_PS_CONTROL_VALUE;
    status = WriteRegister(ALS_PS_CONTROL, &data);
    if(status != HAL_OK)
    {
        errNum++;
    }

    data = PS_CONTROL_VALUE;
    status = WriteRegister(PS_CONTROL, &data);
    if(status != HAL_OK)
    {
        errNum++;
    }

    data = INTERRUPT_DEFAULT_VALUE;
    status = WriteRegister(INTERRUPT, &data);
    if(status != HAL_OK)
    {
        errNum++;
    }

    data = PS_TH_LSB_VALUE;
    status = WriteRegister(PS_TH_LSB, &data);
    if(status != HAL_OK)
    {
        errNum++;
    }

    data = PS_TH_MSB_VALUE;
    status = WriteRegister(PS_TH_MSB, &data);
    if(status != HAL_OK)
    {
        errNum++;
    }

    data = PS_TL_LSB_VALUE;
    status = WriteRegister(PS_TL_LSB, &data);
    if(status != HAL_OK)
    {
        errNum++;
    }

    data = PS_TL_MSB_VALUE;
    status = WriteRegister(PS_TL_MSB, &data);
    if(status != HAL_OK)
    {
        errNum++;
    }

    data = ALS_DATA0_TH_LSB_VALUE;
    status = WriteRegister(ALS_DATA0_TH_LSB, &data);
    if(status != HAL_OK)
    {
        errNum++;
    }

    data = ALS_DATA0_TH_MSB_VALUE;
    status = WriteRegister(ALS_DATA0_TH_MSB, &data);
    if(status != HAL_OK)
    {
        errNum++;
    }

    data = ALS_DATA0_TL_LSB_VALUE;
    status = WriteRegister(ALS_DATA0_TL_LSB, &data);
    if(status != HAL_OK)
    {
        errNum++;
    }

    data = ALS_DATA0_TL_MSB_VALUE;
    status = WriteRegister(ALS_DATA0_TL_MSB, &data);
    if(status != HAL_OK)
    {
        errNum++;
    }

    data = PS_OFFSET_LSB_VALUE;
    status = WriteRegister(PS_OFFSET_LSB, &data);
    if(status != HAL_OK)
    {
        errNum++;
    }

    data = PS_OFFSET_MSB_VALUE;
    status = WriteRegister(PS_OFFSET_MSB, &data);
    if(status != HAL_OK)
    {
        errNum++;
    }

    if(errNum != 0)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

void RPR0521_SetUp(void)
{
    uint8_t data = 0x00;

    data = MODE_CONTROL_VALUE;
    WriteRegister(MODE_CONTROL, &data);
    data = INTERRUPT_VALUE;
    WriteRegister(INTERRUPT, &data);
}

uint16_t RPR0521_ReadPS(void)
{
    uint8_t LOW_BYTE;
    uint8_t HIGH_BYTE;
    uint16_t p_threshold;

    HAL_StatusTypeDef ret = 0;

    ret += ReadRegister(PS_DATA_MSB, &HIGH_BYTE);
    ret += ReadRegister(PS_DATA_LSB, &LOW_BYTE);
    p_threshold = ((((uint16_t)HIGH_BYTE) << 8) & 0xFF00)
                          | (((uint16_t)LOW_BYTE) & 0x00FF);

    return p_threshold;
}

static HAL_StatusTypeDef WriteRegister(uint16_t reg, uint8_t *value)
{
    uint8_t ret;

    ret = HAL_I2C_Mem_Write(PS_SENSOR_BUS, (uint16_t) (RPR0521_ADDRESS << 1),
        reg, I2C_MEMADD_SIZE_8BIT, value, 1, PRP0521_MAX_DELAY);
    return ret;
}

static HAL_StatusTypeDef ReadRegister(uint16_t reg, uint8_t *value)
{
    uint8_t ret;

    ret = HAL_I2C_Mem_Read(PS_SENSOR_BUS, (uint16_t) (RPR0521_ADDRESS << 1),
        reg, I2C_MEMADD_SIZE_8BIT, value, 1, PRP0521_MAX_DELAY);
    return ret;
}
