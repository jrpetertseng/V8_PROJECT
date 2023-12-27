/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "i2s.h"
#include "spi.h"
#include "tim.h"
#include "usb_device.h"
//#include "usb_otg_hs.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <math.h>
#include "stm32f7xx_hal_tim.h"
#include "debug_defs.h"
#include "cmd_engine.h"
#include "bno080.h"

#include "usbd_composite.h"
#include "usbd_cdc.h"
#include "usbd_cdc_devctlr.h"
#include "usbd_customhid.h"
#include "usbd_customhid_imu.h"
//#include "usbd_customhid_als.h"
#include "usbd_custom_hid_if.h"
#include "usbd_custom_hid_if_imu.h"
//#include "usbd_custom_hid_if_als.h"
#include "usbd_audio_if.h"

  #ifdef PRINTF_VIA_CDC_ENABLED
#include "usbd_cdc_if.h"
  #endif
#include "pingpong_buf.h"
#include "flash_rw_process.h"
#include "ecx343.h"
#include "rpr0521.h"
#include "al3010.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c3;
extern USBD_HandleTypeDef hUsbDeviceFS;
extern osThreadId_t ALSensorTaskHandle;
extern osThreadId_t PSTaskHandle;

uint32_t nBno08xGpioInts;
uint32_t nAlsGpioInts;
uint32_t nPsGpioInts;
uint32_t nIMUHIDUsbOuts;
uint32_t nDMAAudioInts;
uint32_t nTaskAudioInts;
extern uint16_t p_threshold;
extern int16_t data_i2s[AUDIO_IN_PACKET*_PACK_SIZE];

/*new PingPong Buffer*/
int16_t pingpong_1[AUDIO_IN_PACKET/2];
int16_t pingpong_2[AUDIO_IN_PACKET/2];
PingPongBuffer_t pingPong;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */
static void MPU_Config(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void boot_UserDFU(void)
{
    USBD_Stop(&hUsbDeviceFS);
    HAL_Delay(100);
    USBD_DeInit(&hUsbDeviceFS);
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

bool ADC_to_MIC(void **process_buf)
{
    if(PingPongBuffer_GetReadBuf(&pingPong, process_buf))
    {
        return true;
    }else return false;
}

/* ENABLE_CDC_ENGINEERING_TEST */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();
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
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_I2C3_Init();
  MX_I2S3_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_SPI4_Init();
  MX_TIM13_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();
//  MX_USB_OTG_HS_PCD_Init();

  /* USER CODE BEGIN 2 */
  nBno08xGpioInts   = 0;
  nAlsGpioInts      = 0;
  nPsGpioInts       = 0;
  nIMUHIDUsbOuts    = 0;

  /* Calculate the step of our sin wave */
#if ENABLE_MIS
  PingPongBuffer_Init(&pingPong, pingpong_1, pingpong_2);
  if (HAL_I2S_Receive_DMA(&hi2s3, (uint16_t *)data_i2s, sizeof(data_i2s)/2) != HAL_OK) {
       Error_Handler();
  }
#endif
//  HAL_GPIO_WritePin(ALS_RST_GPIO_Port, ALS_RST_Pin, GPIO_PIN_SET);
//  HAL_GPIO_WritePin(TOF_EN_GPIO_Port, TOF_EN_Pin, GPIO_PIN_SET);
//  HAL_GPIO_WritePin(CAM_RST_GPIO_Port, CAM_RST_Pin, GPIO_PIN_SET);
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();

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
  RCC_OscInitStruct.PLL.PLLM = 24;
  RCC_OscInitStruct.PLL.PLLN = 432;
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
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV4;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2C1|RCC_PERIPHCLK_I2C3
                              |RCC_PERIPHCLK_I2S|RCC_PERIPHCLK_CLK48;
  PeriphClkInitStruct.PLLI2S.PLLI2SN = 192;
  PeriphClkInitStruct.PLLI2S.PLLI2SR = 2;
  PeriphClkInitStruct.PLLI2S.PLLI2SQ = 2;
  PeriphClkInitStruct.PLLI2SDivQ = 1;
  PeriphClkInitStruct.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
  PeriphClkInitStruct.I2c3ClockSelection = RCC_I2C3CLKSOURCE_PCLK1;
  PeriphClkInitStruct.I2sClockSelection = RCC_I2SCLKSOURCE_PLLI2S;
  PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLL;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/**
  * @brief  EXTI line detection callback.
  * @param  GPIO_Pin Specifies the port pin connected to corresponding EqXTI line.
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(IMU_INTN_Pin == GPIO_Pin)
    {
        SH2_GPIO_EXTI_Callback( GPIO_Pin);
        nBno08xGpioInts += 1;
    }
#if ENABLE_ALS
    else if(ALS_INT_Pin == GPIO_Pin)
    {
        osThreadFlagsSet(ALSensorTaskHandle, 0x02);
        nAlsGpioInts += 1;
    }
#endif
#if ENABLE_PS
    else if(PS_INT_Pin == GPIO_Pin)
    {
        osThreadFlagsSet(PSTaskHandle, 0x01);

        nPsGpioInts += 1;
    }
#endif
#if ENABLE_TOF
    else if(GPIO_Pin == TOF_INT_Pin)
    {
//        nTofGpioInts_1 += 1;
        isrToFTaskTrigger();
    }
#endif
}
#if 0
void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  void *pinpong_ptr;
  PingPongBuffer_GetWriteBuf(&pingPong, &pinpong_ptr);
  for (uint8_t i = 0; i < sizeof(data_i2s)/4 ; i++) {
          *((uint16_t *)pinpong_ptr+i) = (data_i2s[i*2]+1600)*2 ;
  }
  PingPongBuffer_SetWriteDone(&pingPong);

//    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//    xSemaphoreGiveFromISR(micLock, &xHigherPriorityTaskWoken);
//    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

}
#endif
// redirect printf() to USB CDC_DEVCTLR
PUTCHAR_PROTOTYPE
{
#ifdef PRINTF_VIA_CDC_ENABLED
  #if USE_USB_TX_TASK
    usbDebugChars( &ch, 1);
  #else
    #error "CAN NOT use direct USB TX because freeRTOS is ON!!!"
  #endif
#endif
    return ch;
}
/* USER CODE END 4 */

/* MPU Configuration */

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
/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM10 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM10) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
