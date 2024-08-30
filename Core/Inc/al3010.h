/*
 * al3010.h
 *
 *  Created on: 2022年8月25日
 *      Author: User
 */
#ifndef INC_AL3010_H_
#define INC_AL3010_H_

#include "stm32f7xx_hal.h"

#define AL3010_MAX_DELAY                        100

// I2C addresses of the sensor
#define AL3010_SENSOR_ADDRESS_FLOAT             0x1E
#define AL3010_SENSOR_ADDRESS_VDD               0x1D
#define AL3010_SENSOR_ADDRESS_GND               0x1C

#define AL3010_SUPPORTED_FREQUENCIES            {1, 2, 4, 8, 16, 32, 64, 128, 256}

#define AL3010_SUPPORTED_SCALES                 {11872, 2968, 742 , 186}

#define AL3010_GAIN_SCALE_1                     11872	/* 1.1872 lux/count */
#define AL3010_GAIN_SCALE_2                     2968	/* 0.2968 lux/count */
#define AL3010_GAIN_SCALE_3                     742		/* 0.0742 lux/count */
#define AL3010_GAIN_SCALE_4                     186		/* 0.0186 lux/count */

typedef struct scale
{
    int16_t GAIN_SCALE;
} SCALES_TYPE;

//SYS_CONF
#define AL3010_SYS_CONF                         0x00
#define AL3010_SYS_CONF_DEFAULT_VALUE           0x01

#define SYS_MODE_OFFSET                         0
#define SYS_MODE_LEN                            3
#define AL3010_SYS_MODE_ALS_ONCE                (5 << SYS_MODE_OFFSET)
#define AL3010_SYS_MODE_ALS_EN                  (1 << SYS_MODE_OFFSET)
#define AL3010_SYS_MODE_SHUTDOWN                (0 << SYS_MODE_OFFSET)
#define AL3010_SYS_MODE_SOFTRESET               (4 << SYS_MODE_OFFSET)

//ALS_CONF
#define AL3010_ALS_CONF                         0x10
#define AL3010_ALS_CONF_DEFAULT_VALUE           0x13
/*
	ALS Configuration Register (0x10)
	Default value: 0x00

	Bits 6:4 - Gain (Ambient light detectable range)
	Default: 000 (0 ~ 77806 lux)
	000: 0 ~ 77806 lux (Default)
	001: 0 ~ 19452 lux <----------
	010: 0 ~ 4863 lux
	011: 0 ~ 1216 lux

	Bits 1:0 - Interrupt Filter (ALS Interrupt trigger condition)
	Default: 00 (1 conversion time)
	00: 1 conversion time (Default)
	01: 4 conversion times
	10: 8 conversion times
	11: 16 conversion times <----------
*/

#define GAIN_OFFSET                             4
#define GAIN_LEN                                3
#define AL3010_GAIN_GAINX1                      (0 << GAIN_OFFSET)
#define AL3010_GAIN_GAINX4                      (1 << GAIN_OFFSET)
#define AL3010_GAIN_GAINX16                     (2 << GAIN_OFFSET)
#define AL3010_GAIN_GAINX64                     (3 << GAIN_OFFSET)

#define INT_FILTER_OFFSET                       0
#define INT_FILTER_LEN                          2
#define AL3010_INT_FILTER_AFTER_1               (0 << INT_FILTER_OFFSET)
#define AL3010_INT_FILTER_AFTER_4               (1 << INT_FILTER_OFFSET)
#define AL3010_INT_FILTER_AFTER_8               (2 << INT_FILTER_OFFSET)
#define AL3010_INT_FILTER_AFTER_16              (3 << INT_FILTER_OFFSET)

//THLL
#define AL3010_THLL                             0x1A
#define AL3010_THLL_DEFAULT_VALUE               0xFF

//THLH
#define AL3010_THLH                             0x1B
#define AL3010_THLH_DEFAULT_VALUE               0xFF

//THHL
#define AL3010_THHL                             0x1C
#define AL3010_THHL_DEFAULT_VALUE               0x00

//THHH
#define AL3010_THHH                             0x1D
#define AL3010_THHH_DEFAULT_VALUE               0x00

//INT_STATUS
#define AL3010_INT_STATUS                       0x01
#define AL3010_INT_STATUS_DEFAULT_VALUE         0x00
#define ALS_INT_OFFSET                          0
#define ALS_INT_LEN                             1
#define AL3010_ALS_INT_NO_TRIGGER               (0 << ALS_INT_OFFSET)
#define AL3010_ALS_INT_ALS_TRIGGER              (1 << ALS_INT_OFFSET)

// Define output registers
#define AL3010_OUT_ALS_REG                      0x0C
#define AL3010_OUT_ALS_LEN                      2

extern I2C_HandleTypeDef hi2c1;

#define ALS_SENSOR_BUS                          (&hi2c1)


// ********************************
// * FUNCTION FORWARD DECLARATION *
// ********************************
HAL_StatusTypeDef AL3010_Init(void);
void ALS_SendReport_HS(void);
HAL_StatusTypeDef AL3010_ReadData(void);
HAL_StatusTypeDef AL3010_ReadData_ISR(void);

#endif /* INC_AL3010_H_ */
