/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    i2c.c
  * @brief   This file provides code for the configuration
  *          of the I2C instances.
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
/* Includes ------------------------------------------------------------------*/
#include "i2c.h"

/* USER CODE BEGIN 0 */
#include "debug_defs.h"
/* USER CODE END 0 */

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c3;

/* I2C1 init function */
void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
#if ENABLE_TOF_15HZ
  hi2c1.Init.Timing = 0x00200922; //0x6000030D //0x00200922 (1M)
#else
  hi2c1.Init.Timing = 0x6000030D;
#endif
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}
/* I2C3 init function */
void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.Timing = 0x20404768;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspInit 0 */

  /* USER CODE END I2C1_MspInit 0 */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C1 GPIO Configuration
    PB7     ------> I2C1_SDA
    PB6     ------> I2C1_SCL
    */
    GPIO_InitStruct.Pin = PS_ALS_SDA_Pin|PS_ALS_SCL_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2C1 clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();

    /* I2C1 interrupt Init */
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 5, 0); //5 ,modify into 4(highest) to test if still crash.
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
  /* USER CODE BEGIN I2C1_MspInit 1 */

  /* USER CODE END I2C1_MspInit 1 */
  }
  else if(i2cHandle->Instance==I2C3)
  {
  /* USER CODE BEGIN I2C3_MspInit 0 */

  /* USER CODE END I2C3_MspInit 0 */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**I2C3 GPIO Configuration
    PC9     ------> I2C3_SDA
    PA8     ------> I2C3_SCL
    */
    GPIO_InitStruct.Pin = LT7911_CFG_SDA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
    HAL_GPIO_Init(LT7911_CFG_SDA_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LT7911_CFG_SCL_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
    HAL_GPIO_Init(LT7911_CFG_SCL_GPIO_Port, &GPIO_InitStruct);

    /* I2C3 clock enable */
    __HAL_RCC_I2C3_CLK_ENABLE();
  /* USER CODE BEGIN I2C3_MspInit 1 */

  /* USER CODE END I2C3_MspInit 1 */
  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{

  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspDeInit 0 */

  /* USER CODE END I2C1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C1_CLK_DISABLE();

    /**I2C1 GPIO Configuration
    PB7     ------> I2C1_SDA
    PB6     ------> I2C1_SCL
    */
    HAL_GPIO_DeInit(PS_ALS_SDA_GPIO_Port, PS_ALS_SDA_Pin);

    HAL_GPIO_DeInit(PS_ALS_SCL_GPIO_Port, PS_ALS_SCL_Pin);

//    GPIO_InitTypeDef GPIO_InitStruct = {0};
//    GPIO_InitStruct.Pin = PS_ALS_SDA_Pin | PS_ALS_SCL_Pin;
//    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;  // 開漏輸出
//    GPIO_InitStruct.Pull = GPIO_NOPULL;
//    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
//    HAL_GPIO_Init(PS_ALS_SCL_GPIO_Port, &GPIO_InitStruct);
//
//    for (int i = 0; i < 10; i++) {
////        HAL_GPIO_WritePin(PS_ALS_SCL_GPIO_Port, PS_ALS_SDA_Pin, GPIO_PIN_SET);
//        HAL_GPIO_WritePin(PS_ALS_SCL_GPIO_Port, PS_ALS_SCL_Pin, GPIO_PIN_SET);
//        osDelay(1);
////        HAL_GPIO_WritePin(PS_ALS_SCL_GPIO_Port, PS_ALS_SDA_Pin, GPIO_PIN_RESET);
//        HAL_GPIO_WritePin(PS_ALS_SCL_GPIO_Port, PS_ALS_SCL_Pin, GPIO_PIN_RESET);
//        osDelay(1);
//    }
//
//    // 將管腳恢復為I2C功能
//    GPIO_InitStruct.Pin = PS_ALS_SDA_Pin | PS_ALS_SCL_Pin;
//    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
//    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;  // 根據實際使用的I2C號碼調整
//    HAL_GPIO_Init(PS_ALS_SCL_GPIO_Port, &GPIO_InitStruct);
//
//    HAL_GPIO_DeInit(PS_ALS_SDA_GPIO_Port, PS_ALS_SDA_Pin);
//    HAL_GPIO_DeInit(PS_ALS_SCL_GPIO_Port, PS_ALS_SCL_Pin);
    /* I2C1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
  /* USER CODE BEGIN I2C1_MspDeInit 1 */

  /* USER CODE END I2C1_MspDeInit 1 */
  }
  else if(i2cHandle->Instance==I2C3)
  {
  /* USER CODE BEGIN I2C3_MspDeInit 0 */

  /* USER CODE END I2C3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C3_CLK_DISABLE();

    /**I2C3 GPIO Configuration
    PC9     ------> I2C3_SDA
    PA8     ------> I2C3_SCL
    */
    HAL_GPIO_DeInit(LT7911_CFG_SDA_GPIO_Port, LT7911_CFG_SDA_Pin);

    HAL_GPIO_DeInit(LT7911_CFG_SCL_GPIO_Port, LT7911_CFG_SCL_Pin);

  /* USER CODE BEGIN I2C3_MspDeInit 1 */

  /* USER CODE END I2C3_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
void I2C1_SoftwareReset(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 將I2C管腳配置為GPIO
    HAL_GPIO_DeInit(PS_ALS_SDA_GPIO_Port, PS_ALS_SDA_Pin | PS_ALS_SCL_Pin);  // 假設I2C使用的是GPIOB的PIN6(SCL)和PIN7(SDA)

    GPIO_InitStruct.Pin = PS_ALS_SDA_Pin | PS_ALS_SCL_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;  // 開漏輸出
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PS_ALS_SDA_GPIO_Port, &GPIO_InitStruct);

    // 生成I2C時鐘脈衝
    for (int i = 0; i < 10; i++) {
        HAL_GPIO_WritePin(PS_ALS_SDA_GPIO_Port, PS_ALS_SCL_Pin, GPIO_PIN_SET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(PS_ALS_SDA_GPIO_Port, PS_ALS_SCL_Pin, GPIO_PIN_RESET);
        HAL_Delay(1);
    }

    // 將管腳恢復為I2C功能
    GPIO_InitStruct.Pin = PS_ALS_SDA_Pin | PS_ALS_SCL_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;  // 根據實際使用的I2C號碼調整
    HAL_GPIO_Init(PS_ALS_SDA_GPIO_Port, &GPIO_InitStruct);

    // 重新初始化I2C
    HAL_I2C_DeInit(&hi2c1);  // 假設使用的是hi2c1
    HAL_I2C_Init(&hi2c1);
}
/* USER CODE END 1 */
