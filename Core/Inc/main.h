/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include "cmsis_os.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
extern uint32_t nBno08xGpioInts;
extern uint32_t nAlsGpioInts;
extern uint32_t nPsGpioInts;
extern uint32_t nIMUHIDUsbOuts;
extern uint32_t nUsbAudioInts;
extern uint32_t nDMAAudioInts;
extern uint32_t nTaskAudioInts;
extern uint8_t interruptTofEnable;
//extern uint32_t nTofGpioInts_1;
extern void (* tof_callback)(void);
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

#define USE_USB_TX_TASK			1


extern void usb_printf(const char *format, ...);
extern void boot_UserDFU(void);
extern void isrToFTaskTrigger( void);
extern void McuReset(void);

// ref: https://stackoverflow.com/questions/49820288/stm32-printf-over-usb-cdc
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE* f)
#endif /* __GNUC__ */
PUTCHAR_PROTOTYPE;

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define PNL_3V3_EN_Pin GPIO_PIN_1
#define PNL_3V3_EN_GPIO_Port GPIOD
#define VBUS_5V_PNL_FLG_Pin GPIO_PIN_5
#define VBUS_5V_PNL_FLG_GPIO_Port GPIOD
#define MIC_CK_Pin GPIO_PIN_3
#define MIC_CK_GPIO_Port GPIOB
#define TOF_SYNC_Pin GPIO_PIN_3
#define TOF_SYNC_GPIO_Port GPIOE
#define OTG_FS_DP_Pin GPIO_PIN_12
#define OTG_FS_DP_GPIO_Port GPIOA
#define MIC_WS_Pin GPIO_PIN_15
#define MIC_WS_GPIO_Port GPIOA
#define PNL_1V8_EN_Pin GPIO_PIN_0
#define PNL_1V8_EN_GPIO_Port GPIOD
#define VBUS_5V_SYS_FLG_Pin GPIO_PIN_4
#define VBUS_5V_SYS_FLG_GPIO_Port GPIOD
#define LT7911_INT_B_Pin GPIO_PIN_4
#define LT7911_INT_B_GPIO_Port GPIOB
#define PS_ALS_SDA_Pin GPIO_PIN_7
#define PS_ALS_SDA_GPIO_Port GPIOB
#define TOF_INT_Pin GPIO_PIN_1
#define TOF_INT_GPIO_Port GPIOE
#define TOF_INT_EXTI_IRQn EXTI1_IRQn
#define PNL_L_MOSI_Pin GPIO_PIN_6
#define PNL_L_MOSI_GPIO_Port GPIOE
#define OTG_FS_DM_Pin GPIO_PIN_11
#define OTG_FS_DM_GPIO_Port GPIOA
#define OTG_FS_VBUS_Pin GPIO_PIN_9
#define OTG_FS_VBUS_GPIO_Port GPIOA
#define VBUS_5V_PNL_EN_Pin GPIO_PIN_3
#define VBUS_5V_PNL_EN_GPIO_Port GPIOD
#define MIC_SD_Pin GPIO_PIN_5
#define MIC_SD_GPIO_Port GPIOB
#define IMU_RST_Pin GPIO_PIN_8
#define IMU_RST_GPIO_Port GPIOB
#define PNL_L_SCK_Pin GPIO_PIN_2
#define PNL_L_SCK_GPIO_Port GPIOE
#define LT7911_CFG_SDA_Pin GPIO_PIN_9
#define LT7911_CFG_SDA_GPIO_Port GPIOC
#define ALS_INT_Pin GPIO_PIN_8
#define ALS_INT_GPIO_Port GPIOC
#define ALS_INT_EXTI_IRQn EXTI9_5_IRQn
#define LT7911_CFG_SCL_Pin GPIO_PIN_8
#define LT7911_CFG_SCL_GPIO_Port GPIOA
#define IMU_NRST_Pin GPIO_PIN_7
#define IMU_NRST_GPIO_Port GPIOC
#define PS_ALS_SCL_Pin GPIO_PIN_6
#define PS_ALS_SCL_GPIO_Port GPIOB
#define PNL_L_NSS_Pin GPIO_PIN_4
#define PNL_L_NSS_GPIO_Port GPIOE
#define IMU_WAKE_Pin GPIO_PIN_6
#define IMU_WAKE_GPIO_Port GPIOC
#define MAX98360A_GPIO_Pin GPIO_PIN_15
#define MAX98360A_GPIO_GPIO_Port GPIOD
#define PNL_6V6_N_EN_Pin GPIO_PIN_2
#define PNL_6V6_N_EN_GPIO_Port GPIOD
#define AUDIO_DET_Pin GPIO_PIN_0
#define AUDIO_DET_GPIO_Port GPIOE
#define PNL_L_MISO_Pin GPIO_PIN_5
#define PNL_L_MISO_GPIO_Port GPIOE
#define SW_KEY_2D3D_Pin GPIO_PIN_14
#define SW_KEY_2D3D_GPIO_Port GPIOD
#define PNL_VOL_MINUS_Pin GPIO_PIN_12
#define PNL_VOL_MINUS_GPIO_Port GPIOD
#define PNL_VOL_PLUS_Pin GPIO_PIN_11
#define PNL_VOL_PLUS_GPIO_Port GPIOD
#define PNL_R_XCLR_Pin GPIO_PIN_0
#define PNL_R_XCLR_GPIO_Port GPIOB
#define IMU_SCK_Pin GPIO_PIN_5
#define IMU_SCK_GPIO_Port GPIOA
#define PNL_R_MOSI_Pin GPIO_PIN_3
#define PNL_R_MOSI_GPIO_Port GPIOC
#define PS_INT_Pin GPIO_PIN_0
#define PS_INT_GPIO_Port GPIOC
#define PS_INT_EXTI_IRQn EXTI0_IRQn
#define PNL_L_XCLR_Pin GPIO_PIN_1
#define PNL_L_XCLR_GPIO_Port GPIOB
#define IMU_MISO_Pin GPIO_PIN_6
#define IMU_MISO_GPIO_Port GPIOA
#define IMU_NSS_Pin GPIO_PIN_4
#define IMU_NSS_GPIO_Port GPIOA
#define PNL_R_TEMP_Pin GPIO_PIN_0
#define PNL_R_TEMP_GPIO_Port GPIOA
#define PNL_R_SCK_Pin GPIO_PIN_13
#define PNL_R_SCK_GPIO_Port GPIOB
#define CAM_RST_Pin GPIO_PIN_11
#define CAM_RST_GPIO_Port GPIOB
#define TOF_LPN_Pin GPIO_PIN_8
#define TOF_LPN_GPIO_Port GPIOE
#define IMU_INTN_Pin GPIO_PIN_4
#define IMU_INTN_GPIO_Port GPIOC
#define IMU_INTN_EXTI_IRQn EXTI4_IRQn
#define SW_BRG_2D3D_Pin GPIO_PIN_3
#define SW_BRG_2D3D_GPIO_Port GPIOA
#define ALS_RST_Pin GPIO_PIN_2
#define ALS_RST_GPIO_Port GPIOA
#define PNL_L_TEMP_Pin GPIO_PIN_1
#define PNL_L_TEMP_GPIO_Port GPIOC
#define PNL_R_NSS_Pin GPIO_PIN_12
#define PNL_R_NSS_GPIO_Port GPIOB
#define TOF_EN_Pin GPIO_PIN_7
#define TOF_EN_GPIO_Port GPIOE
#define IMU_BOOTN_Pin GPIO_PIN_5
#define IMU_BOOTN_GPIO_Port GPIOC
#define LT7911_INT_Pin GPIO_PIN_1
#define LT7911_INT_GPIO_Port GPIOA
#define PNL_R_MISO_Pin GPIO_PIN_2
#define PNL_R_MISO_GPIO_Port GPIOC
#define SPI_I2C_N_Pin GPIO_PIN_9
#define SPI_I2C_N_GPIO_Port GPIOE
#define LT7911_RSTN_Pin GPIO_PIN_2
#define LT7911_RSTN_GPIO_Port GPIOB
#define IMU_MOSI_Pin GPIO_PIN_7
#define IMU_MOSI_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

#define V_MAJOR 0
#define V_MINOR 1
#define V_PATCH 1
/* Model Code Rules:
 *  1st character: J, Jorjin
 *  2nd character: 8, J8 series
 *  3rd character: L,
 */
#define MODEL_SUFFIX "J8L"
//#define PRINTF_VIA_CDC_ENABLED

void resample_linear(uint16_t *pbuf, uint32_t* tmp );
int16_t median_calc(int16_t arr[], int size);
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
