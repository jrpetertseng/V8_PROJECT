/*
 * rpr0521.h
 *
 *  Created on: Jan 12, 2023
 *      Author: User
 */

#ifndef INC_RPR0521_H_
#define INC_RPR0521_H_

#include "stm32f7xx_hal.h"

#define RPR0521_ADDRESS             0x38

#define PRP0521_MAX_DELAY           1000

#define SYSTEM_CONTROL              0x40
#define SYSTEM_CONTROL_VALUE        0x0A

#define MODE_CONTROL                0x41
#define MODE_CONTROL_DEFAULT_VALUE  0x00
#define MODE_CONTROL_VALUE          0x43 //ALS:standby, PS:100ms

#define ALS_PS_CONTROL              0x42
#define ALS_PS_CONTROL_VALUE        0x02

#define PS_CONTROL                  0x43
#define PS_CONTROL_VALUE            0x00 //0x01

#define PS_DATA_LSB                 0x44

#define PS_DATA_MSB                 0x45

#define ALS_DATA0_LSB               0x46

#define ALS_DATA0_MSB               0x47

#define ALS_DATA1_LSB               0x48

#define ALS_DATA1_MSB               0x49

#define INTERRUPT                   0x4A
#define INTERRUPT_DEFAULT_VALUE     0x00
#define INTERRUPT_VALUE             0x9D //0xAD

#define PS_TH_LSB                   0x4B
#define PS_TH_LSB_VALUE             0xFF

#define PS_TH_MSB                   0x4C
#define PS_TH_MSB_VALUE             0x0F

#define PS_TL_LSB                   0x4D
#define PS_TL_LSB_VALUE             0x00

#define PS_TL_MSB                   0x4E
#define PS_TL_MSB_VALUE             0x00

#define ALS_DATA0_TH_LSB            0x4F
#define ALS_DATA0_TH_LSB_VALUE      0xFF

#define ALS_DATA0_TH_MSB            0x50
#define ALS_DATA0_TH_MSB_VALUE      0xFF

#define ALS_DATA0_TL_LSB            0x51
#define ALS_DATA0_TL_LSB_VALUE      0x00

#define ALS_DATA0_TL_MSB            0x52
#define ALS_DATA0_TL_MSB_VALUE      0x00

#define PS_OFFSET_LSB               0x53
#define PS_OFFSET_LSB_VALUE         0x00

#define PS_OFFSET_MSB               0x54
#define PS_OFFSET_MSB_VALUE         0x00

#define MANUFACT_ID                 0x92
#define MANUFACT_ID_VALUE           0xE0

extern I2C_HandleTypeDef hi2c1;

#define PS_SENSOR_BUS               (&hi2c1)

HAL_StatusTypeDef RPR0521_Init(void);
void RPR0521_SetUp(void);
HAL_StatusTypeDef PS_Enable(void);
uint16_t RPR0521_ReadPS(void);
#endif /* INC_RPR0521_H_ */
