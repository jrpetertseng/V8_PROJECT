/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
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

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
extern void boot_UserDFU(void);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define TOF_RST_Pin GPIO_PIN_10
#define TOF_RST_GPIO_Port GPIOC
#define IMU_RST_Pin GPIO_PIN_8
#define IMU_RST_GPIO_Port GPIOB
#define LT7911_CFG_SDA_Pin GPIO_PIN_9
#define LT7911_CFG_SDA_GPIO_Port GPIOC
#define LT7911_CFG_SCL_Pin GPIO_PIN_8
#define LT7911_CFG_SCL_GPIO_Port GPIOA
#define CAM_RST_Pin GPIO_PIN_11
#define CAM_RST_GPIO_Port GPIOB
#define ALS_RST_Pin GPIO_PIN_2
#define ALS_RST_GPIO_Port GPIOA
#define LT7911_INT_Pin GPIO_PIN_1
#define LT7911_INT_GPIO_Port GPIOA
#define LT7911_RSTN_Pin GPIO_PIN_2
#define LT7911_RSTN_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */
void delay_us(uint32_t udelay);
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
