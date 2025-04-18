/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "i2c.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_core.h"
#include "lt7911ux.h"
#include "usbd_cdc_if.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SCAN_I2C    0
#define I2C_BUS     (&hi2c3)
#define KETBOARD_TEST 0
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
uint16_t i2s_data[32];
uint16_t sample_data[16];
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t pnl_ready = 0;
uint8_t als_ready = 0;
uint8_t ps_ready  = 0;
extern USBD_HandleTypeDef hUsbDeviceHS;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void MPU_Config(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void boot_UserDFU(void)
{
    USBD_Stop(&hUsbDeviceHS);
    HAL_Delay(100);
    USBD_DeInit(&hUsbDeviceHS);
    HAL_Delay(100);

    SCB_DisableICache();
    SCB_DisableDCache();
    *((unsigned long *)0x20017FF0) = 0xDEADBEEF; // 256KB STM32F723

    __DSB();
    __ISB();

    __DSB();

    NVIC_SystemReset();
    /* Should never reach!!! */
    while(1);
}

void delay_us(uint32_t udelay)
{
  uint32_t startval = SysTick->VAL;
  uint32_t tickn = HAL_GetTick();
  uint32_t delay_ticks = (udelay * (SystemCoreClock/1000000U))/8U;

  while ((SysTick->VAL - startval) < delay_ticks)
  {
    if (HAL_GetTick() != tickn)
    {
      tickn = HAL_GetTick();
      startval = SysTick->VAL;
    }
  }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  #if SCAN_I2C
  uint8_t result;
  uint16_t deviceAddr;
  #endif

  #if KETBOARD_TEST
  uint8_t test_1 = 0;
  uint8_t test_2 = 0;
  uint8_t test_3 = 0;
  uint8_t test_4 = 0;
  uint8_t test_5 = 0;
  uint8_t test_6 = 0;
  uint8_t key_pressed = 0;
  uint32_t last_time = 0;
  #endif
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USB_DEVICE_Init();
  MX_I2C3_Init();
  /* USER CODE BEGIN 2 */
  HAL_GPIO_WritePin(ALS_RST_GPIO_Port, ALS_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(100);
  HAL_GPIO_WritePin(TOF_RST_GPIO_Port, TOF_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(100);
  HAL_GPIO_WritePin(IMU_RST_GPIO_Port, IMU_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(100);
  HAL_GPIO_WritePin(CAM_RST_GPIO_Port, CAM_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(100);

  /*al3010 init*/
//  AL3010_Init();
//  HAL_Delay(10);
  /*rpr0521 init*/
//  RPR0521_Init();
//  HAL_Delay(10);
//  RPR0521_SetUp();
//  HAL_Delay(10);
  /*ecx343en init*/
//  ECX343EN_Init();
//  HAL_Delay(10);
//  HAL_GPIO_WritePin(LT7911_RSTN_GPIO_Port, LT7911_RSTN_Pin, GPIO_PIN_SET);
//  HAL_Delay(100);
//  ECX343EN_Run();
//  HAL_Delay(10);

  LT7911_Init();
//  uint16_t nChipId = LT7911_ReadChipId();
  lt7911_firmware_update_init();
  HAL_Delay(2000);
//  if (HAL_I2S_Receive_DMA(&hi2s3, (uint16_t *)i2s_data, sizeof(i2s_data)/2) != HAL_OK) {
//       Error_Handler();
//  }
  boot_UserDFU();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
//      if (als_ready)
//      {
//          AL3010_ReadData();
//      }
//      else if (ps_ready)
//      {
//          RPR0521_ReadPS();
//      }

//      p_threshold = RPR0521_ReadPS();
//      on_flag = (p_threshold > 1000) ? 1 : 0;
//      if (on_flag && pnl_flag)
//      {
//          pnl_flag = 0;
//          ECX343EN_PowerOff();
//
//      }
//      else if (!on_flag && !pnl_flag)
//      {
//          pnl_flag = 1;
//          ECX343EN_PowerOn();
//      }
//      AL3010_ReadData();


//    ECX343EN_Inversion(0x88);
//    HAL_Delay(1000);
//    ECX343EN_Inversion(0x84);
//    HAL_Delay(1000);
//    ECX343EN_Inversion(0x80);
//    HAL_Delay(1000);
//    ECX343EN_ArbitraryLuminanceH(0x01);
//    HAL_Delay(10);
//    ECX343EN_ArbitraryLuminanceL(0xF4);
//    HAL_Delay(1000);
//    ECX343EN_ArbitraryLuminanceH(0x00);
//    HAL_Delay(10);
//    ECX343EN_ArbitraryLuminanceL(0x64);
//    HAL_Delay(1000);
//    ECX343EN_OrbitH(0x30);
//    HAL_Delay(1000);
//    ECX343EN_OrbitH(0x10);
//    HAL_Delay(1000);
//    ECX343EN_OrbitV(0x30);
//    HAL_Delay(1000);
//    ECX343EN_OrbitV(0x10);
//    HAL_Delay(1000);

    #if KETBOARD_TEST
    if(HAL_GPIO_ReadPin(TEST1_GPIO_Port, TEST1_Pin) == GPIO_PIN_RESET)
    {
        if (!key_pressed) {
          uint32_t current_time = HAL_GetTick();
          if ((current_time - last_time) > 200) {
              test_1++;
            last_time = current_time;
          }
          key_pressed = 1;
        }
        HAL_Delay(50);
      } else {
        key_pressed = 0;
      }

    else if(HAL_GPIO_ReadPin(TEST2_GPIO_Port, TEST2_Pin) == GPIO_PIN_RESET)
    {
        if (!key_pressed) {
          uint32_t current_time = HAL_GetTick();
          if ((current_time - last_time) > 200) {
              test_2++;
            last_time = current_time;
          }
          key_pressed = 1;
        }
        HAL_Delay(50);
      } else {
        key_pressed = 0;
      }

    else if(HAL_GPIO_ReadPin(TEST3_GPIO_Port, TEST3_Pin) == GPIO_PIN_RESET)
    {
        if (!key_pressed) {
          uint32_t current_time = HAL_GetTick();
          if ((current_time - last_time) > 200) {
              test_3++;
            last_time = current_time;
          }
          key_pressed = 1;
        }
        HAL_Delay(50);
      } else {
        key_pressed = 0;
      }

    else if(HAL_GPIO_ReadPin(TEST4_GPIO_Port, TEST4_Pin) == GPIO_PIN_RESET)
    {
        if (!key_pressed) {
          uint32_t current_time = HAL_GetTick();
          if ((current_time - last_time) > 200) {
              test_4++;
            last_time = current_time;
          }
          key_pressed = 1;
        }
        HAL_Delay(50);
      } else {
        key_pressed = 0;
      }

    else if(HAL_GPIO_ReadPin(TEST5_GPIO_Port, TEST5_Pin) == GPIO_PIN_RESET)
    {
        if (!key_pressed) {
          uint32_t current_time = HAL_GetTick();
          if ((current_time - last_time) > 200) {
              test_5++;
            last_time = current_time;
          }
          key_pressed = 1;
        }
        HAL_Delay(50);
      } else {
        key_pressed = 0;
      }

    else if(HAL_GPIO_ReadPin(TEST6_GPIO_Port, TEST6_Pin) == GPIO_PIN_RESET)
    {
        if (!key_pressed) {
          uint32_t current_time = HAL_GetTick();
          if ((current_time - last_time) > 200) {
              test_6++;
            last_time = current_time;
          }
          key_pressed = 1;
        }
        HAL_Delay(50);
      } else {
        key_pressed = 0;
      }
    #endif

    #if SCAN_I2C
    for (uint16_t i=1; i<=128; i++)
    {
      result = HAL_I2C_IsDeviceReady(I2C_BUS, (i<<1), 3, 5);
      if (result == HAL_OK)
      {
          deviceAddr = (i<<1);
      }
      HAL_Delay(5);
    }
    #endif
//    HAL_Delay(10);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 12;
  RCC_OscInitStruct.PLL.PLLN = 216;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();
  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_HFNMI_PRIVDEF);

}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
