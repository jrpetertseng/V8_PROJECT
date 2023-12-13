#include <string.h>
#include "main.h"
#include "lt7911.h"

extern I2C_HandleTypeDef hi2c3;

static HAL_StatusTypeDef WriteI2C_Byte( uint16_t addr, uint8_t buf);
static HAL_StatusTypeDef ReadI2C_Byte( uint16_t addr, uint8_t *buf, uint16_t size);

static HAL_StatusTypeDef WriteI2C_Byte( uint16_t addr, uint8_t buf)
{
    uint8_t ret;
    ret =  HAL_I2C_Mem_Write( &hi2c3, (LT7911_DEV_ADDR << 1), addr, I2C_MEMADD_SIZE_8BIT, &buf, 1, LT7911_TIMEOUT);
    return ret;
}

static HAL_StatusTypeDef ReadI2C_Byte( uint16_t addr, uint8_t *buf, uint16_t size)
{
    uint8_t ret;
    ret = HAL_I2C_Mem_Read( &hi2c3, (LT7911_DEV_ADDR << 1), addr, I2C_MEMADD_SIZE_8BIT, buf, size, LT7911_TIMEOUT);
    return ret;
}

void LT7911_Mode_Switch(uint8_t value)
{
    WriteI2C_Byte( 0xFF, 0xE0);
    WriteI2C_Byte( 0xB1, value);
}

void GetBridgeVersion(uint8_t *brgFW)
{
    WriteI2C_Byte( 0xFF, 0xE0); //0xE0
    ReadI2C_Byte( 0x80, brgFW, sizeof(brgFW));

}
