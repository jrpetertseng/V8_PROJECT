/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "timers.h"
#include "debug_defs.h"
#include "cmd_engine.h"
#include "semphr.h"
#include "portmacro.h"

#include "usbd_composite.h"
#include "usbd_cdc.h"
#include "usbd_cdc_devctlr.h"
#include "usbd_customhid.h"
#include "usbd_customhid_imu.h"
#include "usbd_customhid_als.h"
#include "usbd_custom_hid_if.h"
#include "usbd_custom_hid_if_imu.h"
#include "usbd_custom_hid_if_als.h"

//#include "lt7911d.h"
#include "al3010.h"
#include "ecx343.h"
#include "rpr0521.h"
#include "lt7911.h"

#include "usb.h"
#include "bno080.h"

#include "i2s.h"
#include "pingpong_buf.h"
#include "vl53l8cx_api.h"
#include "button_handling.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

#if ENABLE_CDC_ENGINEERING_TEST
static JQueueMessage_t report_usb;
#endif
typedef StaticTask_t osStaticThreadDef_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* We accept the ce loop to run every 5ms. */
#define CE_LOOP_MS          5
#define VOLUME              4
#define I2C_BUS             (&hi2c1)
#define SPK_TICK            9000
#define PROXIMITY_THRESHOLD 10
#define BRIGHTNESS_CHANGE_THRESHOLD  10
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern I2C_HandleTypeDef  hi2c1;
extern I2C_HandleTypeDef  hi2c3;
extern USBD_HandleTypeDef hUsbDeviceHS;
extern TIM_HandleTypeDef  htim13;
extern USBD_AUDIO_HandleTypeDef hUACMic;
extern PingPongBuffer_t pingPong;
extern uint8_t readCount;
extern uint32_t nUsbAudioInts;
extern uint32_t nUsbAudioIntsNotReady;
extern uint32_t light;
extern ECX343_DATA ecx343_current_data;
extern bool command_flag;
extern float temperatureLeft;
extern float temperatureRight;
extern float smoothed_left;
extern float smoothed_right;
extern uint16_t current_brightness[2];

enum PowerState current_state = POWER_OFF;
uint8_t DebugSwitch = 0;
uint8_t AutoBrightness = 0;
int16_t data_i2s[AUDIO_IN_PACKET*_PACK_SIZE];
int16_t average_volume = 1430; //1430
uint8_t buttonEvent;

//bool medianFlag = true;
//int16_t median_buf[AUDIO_IN_PACKET/2];
//extern uint32_t _tmp[AUDIO_IN_PACKET/2];
void *pinpong_ptr;

/* Definitions for cmdToFTask */
#if ENABLE_CMD
osThreadId_t cmdToFTaskHandle;
static uint32_t cmdToFTaskBuffer[ 1536 ];
osStaticThreadDef_t cmdToFTaskControlBlock;
const osThreadAttr_t cmdToFTask_attributes = {
  .name = "cmdToFTask",
  .cb_mem = &cmdToFTaskControlBlock,
  .cb_size = sizeof(cmdToFTaskControlBlock),
  .stack_mem = &cmdToFTaskBuffer[0],
  .stack_size = sizeof(cmdToFTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal, //osPriorityBelowNormal
};
#endif
#if ENABLE_OLD_TOF
osThreadId_t isrToFTaskHandle;
static uint32_t isrToFTaskBuffer[ 256 ];
osStaticThreadDef_t isrToFTaskControlBlock;
const osThreadAttr_t isrToFTask_attributes = {
  .name = "isrToFTask",
  .cb_mem = &isrToFTaskControlBlock,
  .cb_size = sizeof(isrToFTaskControlBlock),
  .stack_mem = &isrToFTaskBuffer[0],
  .stack_size = sizeof(isrToFTaskBuffer),
  .priority = (osPriority_t) osPriorityISR,
};
#endif

#if ENABLE_TOF
static void ToFTask(void * argument);
osThreadId_t ToFTaskHandle;
static uint32_t ToFTaskBuffer[ 1536 ]; //512 //1536
osStaticThreadDef_t ToFTaskControlBlock;
const osThreadAttr_t ToFTask_attributes = {
  .name = "ToFTask",
  .cb_mem = &ToFTaskControlBlock,
  .cb_size = sizeof(ToFTaskControlBlock),
  .stack_mem = &ToFTaskBuffer[0],
  .stack_size = sizeof(ToFTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal, //osPriorityNormal
};
#endif

  #if ENABLE_CDC_DEVCTLR_LOAD_PRINT
//#error
static void UsbCpuLoadTask(void * argument);

static uint32_t usbCpuLoadTaskBuffer[ 512 ];
osStaticThreadDef_t usbCpuLoadTaskControlBlock;
const osThreadAttr_t usbCpuLoadTask_attributes = {
  .name = "usbCpuLoadTask",
  .cb_mem = &usbCpuLoadTaskControlBlock,
  .cb_size = sizeof(usbCpuLoadTaskControlBlock),
  .stack_mem = &usbCpuLoadTaskBuffer[0],
  .stack_size = sizeof(usbCpuLoadTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};

  #endif

//ckhsu for imu bno08x
static void ImuSensorTask(void * argument);
#if ENABLE_IMU
static uint32_t ImuSensorTaskBuffer[ 1024 ];
osStaticThreadDef_t ImuSensorTaskControlBlock;
const osThreadAttr_t ImuSensorTask_attributes = {
  .name = "ImuSensorTask",
  .cb_mem = &ImuSensorTaskControlBlock,
  .cb_size = sizeof(ImuSensorTaskControlBlock),
  .stack_mem = &ImuSensorTaskBuffer[0],
  .stack_size = sizeof(ImuSensorTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};
//ckhsu end bno08x
#endif

osThreadId_t MainTaskHandle;
uint32_t MainTaskBuffer[ 512 ];
osStaticThreadDef_t MainTaskControlBlock;
const osThreadAttr_t MainTask_attributes = {
  .name = "MainTask",
  .cb_mem = &MainTaskControlBlock,
  .cb_size = sizeof(MainTaskControlBlock),
  .stack_mem = &MainTaskBuffer[0],
  .stack_size = sizeof(MainTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal, //osPriorityAboveNormal
};

#if ENABLE_ALS
osThreadId_t ALSensorTaskHandle;
uint32_t ALSensorTaskBuffer[ 256 ]; //512
osStaticThreadDef_t ALSensorTaskControlBlock;
const osThreadAttr_t ALSensorTask_attributes = {
  .name = "ALSensorTask",
  .cb_mem = &ALSensorTaskControlBlock,
  .cb_size = sizeof(ALSensorTaskControlBlock),
  .stack_mem = &ALSensorTaskBuffer[0],
  .stack_size = sizeof(ALSensorTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};
#endif
/* Definitions for usbTxTask */
osThreadId_t usbTxTaskHandle;
uint32_t usbTxTaskBuffer[ 1024 ]; //512
osStaticThreadDef_t usbTxTaskControlBlock;
const osThreadAttr_t usbTxTask_attributes = {
  .name = "usbTxTask",
  .cb_mem = &usbTxTaskControlBlock,
  .cb_size = sizeof(usbTxTaskControlBlock),
  .stack_mem = &usbTxTaskBuffer[0],
  .stack_size = sizeof(usbTxTaskBuffer),
  .priority = (osPriority_t) osPriorityAboveNormal,
};

#if ENABLE_SCAN_I2C
static void I2CScanTask(void * argument);
osThreadId_t I2CScanTaskHandle;
static uint32_t I2CScanTaskBuffer[ 512 ];
osStaticThreadDef_t I2CScanTaskControlBlock;
const osThreadAttr_t I2CScanTask_attributes = {
  .name = "I2CScanTask",
  .cb_mem = &I2CScanTaskControlBlock,
  .cb_size = sizeof(I2CScanTaskControlBlock),
  .stack_mem = &I2CScanTaskBuffer[0],
  .stack_size = sizeof(I2CScanTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};
#endif
#if ENABLE_PS
osThreadId_t PSTaskHandle;
uint32_t PSensorTaskBuffer[ 512 ]; //512
osStaticThreadDef_t PSensorTaskControlBlock;
const osThreadAttr_t PSensorTask_attributes = {
  .name = "PSensorTask",
  .cb_mem = &PSensorTaskControlBlock,
  .cb_size = sizeof(PSensorTaskControlBlock),
  .stack_mem = &PSensorTaskBuffer[0],
  .stack_size = sizeof(PSensorTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};
#endif
/* Definitions for micRxTask */
//osThreadId_t micRxTaskHandle;
//uint32_t micRxTaskBuffer[ 512 ];
//osStaticThreadDef_t micRxTaskControlBlock;
//const osThreadAttr_t micRxTask_attributes = {
//  .name = "micRxTask",
//  .cb_mem = &micRxTaskControlBlock,
//  .cb_size = sizeof(micRxTaskControlBlock),
//  .stack_mem = &micRxTaskBuffer[0],
//  .stack_size = sizeof(micRxTaskBuffer),
//  .priority = (osPriority_t) osPriorityNormal,  //osPriorityHigh
//};

/* Definitions for flashRxTxTask */
//osThreadId_t flashRxTxTaskHandle;
//uint32_t flashRxTxTaskBuffer[ 512 ];
//osStaticThreadDef_t flashRxTxTaskControlBlock;
//const osThreadAttr_t flashRxTxTask_attributes = {
//  .name = "flashRxTxTask",
//  .cb_mem = &flashRxTxTaskControlBlock,
//  .cb_size = sizeof(flashRxTxTaskControlBlock),
//  .stack_mem = &flashRxTxTaskBuffer[0],
//  .stack_size = sizeof(flashRxTxTaskBuffer),
//  .priority = (osPriority_t) osPriorityNormal,
//};
#if ENABLE_ADC
static void ADCTask(void * argument);
osThreadId_t ADCTaskHandle;
static uint32_t ADCTaskBuffer[ 512 ];
osStaticThreadDef_t ADCTaskControlBlock;
const osThreadAttr_t ADCTask_attributes = {
  .name = "ADCTask",
  .cb_mem = &ADCTaskControlBlock,
  .cb_size = sizeof(ADCTaskControlBlock),
  .stack_mem = &ADCTaskBuffer[0],
  .stack_size = sizeof(ADCTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal, //osPriorityAboveNormal
};
#endif
/* USER CODE END Variables */


/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* ckhsu, I am lazy to modify the CE loop, so we try to use */
/* a timed semaphore to reduce the load of it.              */

/* An array to hold handles to the created timers. */
  #if 0
static TimerHandle_t     ceTimers;
  #endif
static SemaphoreHandle_t ceLock;
static int               nExecs_CmdToF;
static SemaphoreHandle_t isrToFLock;
static int               nExecs_IsrToF;
static SemaphoreHandle_t I2C1_Lock;

SemaphoreHandle_t isrALSLock = NULL;
SemaphoreHandle_t isrPSLock = NULL;

float readVoltageAndCalculate(uint8_t voltage, uint8_t panel, ADC_HandleTypeDef *hadc);

void MainTask(void * argument);
void ALSensorTask(void * argument);
void ADCTask(void * argument);

void CmdToFTask(void *argument);
void IsrToFTask(void *argument);
void PnlTask(void *argument);
void UsbTxTask(void *argument);
void MicRxTask(void *argument);
void PSensorTask(void *argument);

/* USER CODE END FunctionPrototypes */


extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */
/* USER CODE BEGIN 1 */

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);


volatile unsigned long ulHighFrequencyTimerTicks;

/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void)
{
  ulHighFrequencyTimerTicks = 0;
  HAL_TIM_Base_Start_IT(&htim13);
}

__weak unsigned long getRunTimeCounterValue(void)
{
  return ulHighFrequencyTimerTicks;
}

  #if 0
static void ceTimerCallback( TimerHandle_t xTimer)
{
    /* Optionally do something if the pxTimer parameter is NULL. */
    configASSERT( xTimer );

    xSemaphoreGive( ceLock);
}
  #endif

void isrToFTaskTrigger( void)
{
	static BaseType_t xHigherPriorityTaskWoken;

	xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR( isrToFLock, &xHigherPriorityTaskWoken);
    
    /* If xHigherPriorityTaskWoken was set to true we should yield. */
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken);
}
static inline void i2c1TxBlock( void)
{

    xSemaphoreTake(I2C1_Lock, portMAX_DELAY);
}
static inline void i2c1TxUnblock( void)
{
    xSemaphoreGive(I2C1_Lock);
}

/* USER CODE END 1 */
/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */

  /* USER CODE BEGIN RTOS_THREADS */
  usbTxTaskHandle = osThreadNew(UsbTxTask, NULL, &usbTxTask_attributes);

  /* creation of micRxTask */

//  MainTaskHandle = osThreadNew(MainTask, NULL, &MainTask_attributes);


#if ENABLE_MIS
//  micRxTaskHandle = osThreadNew(MicRxTask, NULL, &micRxTask_attributes);
#endif

//#if ENABLE_ALS
//  ALSensorTaskHandle = osThreadNew(ALSensorTask, NULL, &ALSensorTask_attributes);
//
//#endif
//
//#if ENABLE_ADC
//  ADCTaskHandle = osThreadNew(ADCTask, NULL, &ADCTask_attributes);
//#endif
//
//#if ENABLE_PS
//  PSTaskHandle = osThreadNew(PSensorTask, NULL, &PSensorTask_attributes);
//#endif

  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_UsbTxTask */
/**
  * @brief  Function implementing the usbTxTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_UsbTxTask */
void UsbTxTask(void *argument)
{
  /* USER CODE BEGIN Pre UsbTxTask */
  // This task is responsible to create all other tasks.
    HAL_GPIO_WritePin(ALS_RST_GPIO_Port, ALS_RST_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(CAM_RST_GPIO_Port, CAM_RST_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TOF_EN_GPIO_Port, TOF_EN_Pin, GPIO_PIN_SET);
    osDelay(500);
  usbInit();
  /* USER CODE END Pre UsbTxTask */
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN UsbTxTask */

  /* creation of cmdToFTask */
#if ENABLE_CMD
  cmdToFTaskHandle = osThreadNew( CmdToFTask, NULL, &cmdToFTask_attributes);
#endif
#if ENABLE_OLD_TOF
  isrToFLock       = xSemaphoreCreateBinary();
  isrToFTaskHandle = osThreadNew( IsrToFTask, NULL, &isrToFTask_attributes);

  //cmdToFTaskHandle = osThreadNew( CmdToFTask, NULL, &cmdToFTask_attributes);
#endif
  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  #if ENABLE_CDC_DEVCTLR_LOAD_PRINT
    //#error "CPU load task"
    osThreadNew( UsbCpuLoadTask, NULL, &usbCpuLoadTask_attributes);
  #endif
#if ENABLE_IMU
  osThreadNew( ImuSensorTask, NULL, &ImuSensorTask_attributes);
#endif
  I2C1_Lock = xSemaphoreCreateBinary();
  if (I2C1_Lock != NULL) {
          xSemaphoreGive(I2C1_Lock);
  }
  MainTaskHandle = osThreadNew(MainTask, NULL, &MainTask_attributes);
  /* This is in fact a never leave loop. */
  usbLoop();

  /* USER CODE END UsbTxTask */
}

/* USER CODE BEGIN Header_CmdToFTask */
/**
* @brief Function implementing the cmdToFTask thread.
* @param argument: Not used
* @retval None
*/

#define ALS_REPORT_TIME_IN_MS       200
#define TIME_2_SEND_ALS_TICK        (ALS_REPORT_TIME_IN_MS/CE_LOOP_MS)

void IsrToFTask(void *argument)
{
    nExecs_IsrToF = 0;
    usb_waitUntilInited();

  /* Infinite loop */
    while(1)
    {
        // Wait until isr triggered.
        xSemaphoreTake( isrToFLock, portMAX_DELAY);
        /* Do ToF callback. */
        nExecs_IsrToF += 1;
        if(tof_callback)
        {
            tof_callback();
        }
    }
}



/* USER CODE END Header_CmdToFTask */
void CmdToFTask(void *argument)
{
  /* USER CODE BEGIN CmdToFTask */
    int nALSTickCount;
  #if ENABLE_CDC_ENGINEERING_TEST
    int nCount;
    int nSeikoImuRptIdx = 0;
    int nIterCount;

#define TEST_SEIKO_REPORTS      4
    const char seiko_imu_STREAM_0[TEST_SEIKO_REPORTS][15] =
    {
        { 0x02, 0x02, 0x04, 0x00, 0xEA, 0x0B, 0x00, 0xE0, 0xBF, 0x13, 0xFA, 0x80, 0x29, 0x74, 0x00 },
        { 0x02, 0x02, 0x04, 0x08, 0x5B, 0x0B, 0x00, 0xA8, 0x2F, 0x0B, 0xFA, 0x57, 0xFF, 0x70, 0x00 },

        { 0x02, 0x02, 0x04, 0xA7, 0x19, 0x0C, 0x00, 0xE8, 0xAC, 0x0C, 0xFA, 0x07, 0xA0, 0x70, 0x00 },
        { 0x02, 0x02, 0x04, 0x58, 0xBA, 0x0B, 0x00, 0xA8, 0x2F, 0x0B, 0xFA, 0x00, 0x2F, 0x71, 0x00 },
    };
  #endif

  #if ENABLE_CDC_ENGINEERING_TEST
    nIterCount = 0;
    HAL_Delay(2000);
  #endif

    nExecs_CmdToF = 0x0;
    nALSTickCount = 0;
    ceLock   = xSemaphoreCreateBinary();
  #if 0
    ceTimers = xTimerCreate ( "Timer",        /* Just a text name, not used by the RTOS kernel. */
                              (CE_LOOP_MS),   /* The timer period in ticks, must be greater than 0. */
                              pdTRUE,         /* The timers will auto-reload themselves when they expire. */
                              (void *)0,      /* The ID is used to store a count of the number of times the timer has expired, which is initialised to 0. */
                              ceTimerCallback /* Each timer calls the same callback when it expires. */
                            );

    if(NULL == ceTimers)
    {
        /* The timer was not created. */
    }
    else
    {
        /* Start the timer.  No block time is specified, and
        even if one was it would be ignored because the RTOS
        scheduler has not yet been started. */
        if(pdPASS != xTimerStart( ceTimers, 0))
        {
            /* The timer could not be set into the Active state. */
        }
    }
  #endif

    usb_waitUntilInited();

  /* Infinite loop */
    for(;;)
    {
        xSemaphoreTake( ceLock, CE_LOOP_MS);
        nExecs_CmdToF += 1;
        nALSTickCount += 1;

  #if ENABLE_CDC_ENGINEERING_TEST
        // ckhsu: Test send for debug purpose.
#define MAX_RETRY_HID       100

        if(bHidTest)
        {
            nIterCount += 1;
#define TIME_2_SEND_IMU_TICK        (1000/CE_LOOP_MS)
            if(TIME_2_SEND_IMU_TICK <= nIterCount)
            {
                nIterCount = 0;

                // Send IMU
                for( nCount = 0; nCount < 1; nCount++)
                {
                    //usb_printf("Prepare send IMU test [%d]\n", nSeikoImuRptIdx);
                    report_usb.type               = USB_HID_IMU_INPUT_REPORT;
                    report_usb.data.imuReport.len =  15;
                    memcpy( report_usb.data.imuReport.report,
                            &(seiko_imu_STREAM_0[nSeikoImuRptIdx][0]),
                            report_usb.data.imuReport.len);
                    usbSendMessage(&report_usb);

                    //usb_printf("IMU len:[%d]\n", (int)report_usb.data.imuReport.len);
                    //usb_printf("[%02X %02X %02X %02X %02X %02X %02X %02X]\n", (int)report_usb.data.imuReport.report[0],  (int)report_usb.data.imuReport.report[1], (int)report_usb.data.imuReport.report[2],  (int)report_usb.data.imuReport.report[3], (int)report_usb.data.imuReport.report[4],  (int)report_usb.data.imuReport.report[5], (int)report_usb.data.imuReport.report[6],  (int)report_usb.data.imuReport.report[7]);
                    //usb_printf("[%02X %02X %02X %02X %02X %02X %02X]\n", (int)report_usb.data.imuReport.report[8],  (int)report_usb.data.imuReport.report[9], (int)report_usb.data.imuReport.report[10], (int)report_usb.data.imuReport.report[11], (int)report_usb.data.imuReport.report[12], (int)report_usb.data.imuReport.report[13], (int)report_usb.data.imuReport.report[14]);
                    nSeikoImuRptIdx += 1;
                    nSeikoImuRptIdx %= TEST_SEIKO_REPORTS;
                }
            }

            if(TIME_2_SEND_ALS_TICK <= nALSTickCount)
            {
                nALSTickCount = 0;
                ALS_SendReport_HS( &light_sensor);
            }
        }
        else if(bScanI2c)
        {
            bScanI2c = 0;
            i2c_detect_bus();
        }
        else if(bLT7911Test)
        {
            bLT7911Test = 0;
            usb_printf("LT7911 start\n");
            lt7911d_firmware_update_init();
            usb_printf("LT7911 end\n\n");
        }

  #else
    /* Original J7EF handler */
        CE_Routine();
        if(TIME_2_SEND_ALS_TICK <= nALSTickCount)
        {
            nALSTickCount = 0;
        }
  #endif /* #if ENABLE_CDC_ENGINEERING_TEST */
        //taskYIELD();
        //osDelay(1);
    }
  /* USER CODE END CmdToFTask */
}

/* USER CODE BEGIN Header_MicRxTask */
/**
* @brief Function implementing the micRxTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_MicRxTask */
#if 0
void MicRxTask(void *argument)
{
  /* USER CODE BEGIN MicRxTask */
  /* Infinite loop */
  for(;;)
  {
      void *pinpong_ptr;
      osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);
      nTaskAudioInts += 1;
      PingPongBuffer_GetWriteBuf(&pingPong, &pinpong_ptr);
        for (uint8_t i = 0; i < sizeof(data_i2s)/4 ; i++) {
            *((int16_t *)pinpong_ptr+i) = data_i2s[i*2];
            /*TODO: Check if it is faster by using memcpy()*/
//            *((uint16_t *)pinpong_ptr+i) = *((uint16_t *)pinpong_ptr+i) << 1;
        }
        PingPongBuffer_SetWriteDone(&pingPong);

  }
  /* USER CODE END MicRxTask */
}
#endif

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

#if ENABLE_CDC_DEVCTLR_LOAD_PRINT

  #define MAX_SIZE_CPU_LOAD_INFO    512
//  static uint8_t szCpuRunInfo[MAX_SIZE_CPU_LOAD_INFO];

static void UsbCpuLoadTask(void * argument)
{
//    USB_TX_STAT_T stat;
    int           nTime = 0;

#define STATS_FROM_BOOTUP     10
    usb_waitUntilInited();

    while(1)
    {
        if(nTime > STATS_FROM_BOOTUP)
        {
//            memset( szCpuRunInfo, 0x0, MAX_SIZE_CPU_LOAD_INFO);
//
//            vTaskList( (char *)&szCpuRunInfo);
////            usbDebug("=========================================\r\n");
////            usbDebug("Task   State   Prio   StackLeft   Id\r\n");
////            usbDebug("%s", szCpuRunInfo);
////            usbDebug("=========================================\r\n");
//
//            memset( szCpuRunInfo, 0x0, MAX_SIZE_CPU_LOAD_INFO);
//            vTaskGetRunTimeStats((char *)&szCpuRunInfo);
//            usbDebug("=========================================\r\n");
//            usbDebug("Task           Oper.Count       Load\r\n");
//            usbDebug("%s", szCpuRunInfo);
//            usbDebug("=========================================\r\n\r\n");
////            usbDebug("ToF cmd/isr [%d/%d]\r\n", nExecs_CmdToF, nExecs_IsrToF);
////            usbDebug("INTs [%d/%d/%d]\r\n\r\n", nBno08xGpioInts, nTofGpioInts_1, nTofGpioInts_2);
////            usbDebug("INTs [%d/%d/%d]\r\n\r\n", nBno08xGpioInts, nIMUHIDUsbOuts, nUsbAudioInts);
//            usbDebug("nUsbAudioInts, nDMAAudioInts, nTaskAudioInts, nUsbAudioIntsNotReady \r\n");
//            usbDebug("INTs [%d/%d/%d/%d]\r\n\r\n", nUsbAudioInts,
//                        nDMAAudioInts, nTaskAudioInts, nUsbAudioIntsNotReady);
//            nBno08xGpioInts = 0;
//            nTofGpioInts_1  = 0;
//            nTofGpioInts_2  = 0;
//            nIMUHIDUsbOuts  = 0;
//            nUsbAudioInts   = 0;
//            nDMAAudioInts   = 0;
//            nTaskAudioInts  = 0;
//            nUsbAudioIntsNotReady = 0;
//
//            memset( &stat, 0x0, sizeof(stat));
//            usbGetStatistics( &stat);
            //usbDebug("nTxToF=Cplt+Fail: [%d= %d + %d]\r\n", stat.nTxToF,
            //                                                stat.nTxCompleteToF,
            //                                                stat.nTxFailToF);
            //usbDebug("nTxDev=Cplt+Fail: [%d= %d + %d]\r\n", stat.nTxDevCtlr,
            //                                                stat.nTxCompleteDevCtlr,
            //                                                stat.nTxFailDevCtlr);
            //usbDebug("nTxHid/Imu: [%d/%d]\r\n", stat.nTxHid, stat.nTxImu);
            //usbDebug("SysTick:[%d]\r\n", xTaskGetTickCount());
            //usbDebug("nTxImu:         [%d]\r\n", stat.nTxImu);
            //usbDebug("nTxAls:         [%d]\r\n", stat.nTxAls);
            //usbDebug("BUILD DATE:[%s %s]\r\n", __DATE__, __TIME__);
            usbDebug("p_threshold:[%d]\r\n", p_threshold);
        }

//        nExecs_CmdToF = 0;
//        nExecs_IsrToF = 0;

        osDelay(500);
        nTime += 1;
    }
}

#endif /* ENABLE_CDC_DEVCTLR_LOAD_PRINT */

static void ImuSensorTask(void * argument)
{
  /* USER CODE BEGIN 5 */
  HAL_GPIO_WritePin(IMU_RST_GPIO_Port, IMU_RST_Pin, GPIO_PIN_SET);

  initSensor();
  /* Infinite loop */
  sensorLoop();
  /* USER CODE END 5 */
}
#if 1
void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
//    osThreadFlagsSet(micRxTaskHandle, 0x01);
    nDMAAudioInts += 1;
    PingPongBuffer_GetWriteBuf(&pingPong, &pinpong_ptr);
    for (uint8_t i = 0; i < sizeof(data_i2s)/4 ; i++) {

        *((int16_t *)pinpong_ptr+i) = (data_i2s[i*2]+average_volume)*2;

    }
#if 0
    if(nDMAAudioInts==1000 && medianFlag ){
        memcpy(median_buf, (int16_t *)pinpong_ptr, 32);
        average_volume = median_calc(median_buf, sizeof(median_buf)/2);
        medianFlag = false;
    }
//    resample_linear(pinpong_ptr, _tmp);
#endif
    PingPongBuffer_SetWriteDone(&pingPong);

}
#endif

void ALSensorTask(void * argument)
{
    static int previous_brightness = 0;

    for(;;)
    {
    	uint32_t thread_flag = osThreadFlagsWait(0x02, osFlagsNoClear, osWaitForever);
        if (thread_flag == 0x02) {
            if(xSemaphoreTake(I2C1_Lock, portMAX_DELAY) == pdTRUE) {
                AL3010_ReadData();
                i2c1TxUnblock();
                ALS_SendReport_FS();
                if (AutoBrightness) {
                   int current_lux = light;
                   int new_brightness = map_lux_to_internal_brightness(current_lux);

                   if ((abs(new_brightness - previous_brightness) >= BRIGHTNESS_CHANGE_THRESHOLD)) {
                       while (current_brightness[PANEL_RIGHT] != new_brightness || current_brightness[PANEL_LEFT] != new_brightness)
                       {
                           smoothly_change_brightness(new_brightness, PANEL_RIGHT);
                           smoothly_change_brightness(new_brightness, PANEL_LEFT);
                       }
                       previous_brightness = new_brightness;
                       ecx343_current_data.uLCD_LUXL = current_brightness[PANEL_LEFT];
                       ecx343_current_data.uLCD_LUXR = current_brightness[PANEL_RIGHT];
                   }
                }
                osThreadFlagsClear(0x02);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

#if ENABLE_PS
void PSensorTask(void * argument)
{
    uint8_t thresholdCount = 0;
    uint16_t p_threshold = 0;

    for(;;)
    {
        uint32_t thread_flag = osThreadFlagsWait(0x01, osFlagsNoClear, osWaitForever); //osFlagsWaitAny
        if (thread_flag == 0x01) {
            if (xSemaphoreTake(I2C1_Lock, portMAX_DELAY) == pdTRUE) {
                p_threshold = RPR0521_ReadPS();
                i2c1TxUnblock();
                if ((p_threshold > PROXIMITY_THRESHOLD) != (current_state == POWER_ON)) {
                    thresholdCount++;
                    if (thresholdCount > 5) {
                        current_state = (current_state == POWER_OFF) ? POWER_ON : POWER_OFF;
                        power_control_functions[current_state]();
                        thresholdCount = 0;
                    }
                } else {
                    thresholdCount = 0;
                }
                osThreadFlagsClear(0x01);
//                usbDebug("p_threshold: %d@\r\n", p_threshold);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
#endif

void MainTask(void * argument)
{

    //Move power on/off into usbTask
//    HAL_GPIO_WritePin(ALS_RST_GPIO_Port, ALS_RST_Pin, GPIO_PIN_SET);
//    HAL_GPIO_WritePin(CAM_RST_GPIO_Port, CAM_RST_Pin, GPIO_PIN_SET);
//    HAL_GPIO_WritePin(TOF_EN_GPIO_Port, TOF_EN_Pin, GPIO_PIN_SET);
//    osDelay(1000);
    HAL_GPIO_WritePin(LT7911_RSTN_GPIO_Port, LT7911_RSTN_Pin, GPIO_PIN_SET);
    osDelay(10);
    Ecx343_data_init_default();
    osDelay(10);
    ECX343EN_Init();
    osDelay(10);

#if ENABLE_TOF
    isrToFLock       = xSemaphoreCreateBinary();
    ToFTaskHandle = osThreadNew( ToFTask, NULL, &ToFTask_attributes);
    osThreadFlagsWait(0x02, osFlagsWaitAny, osWaitForever);

#endif

#if ENABLE_PS
  PSTaskHandle = osThreadNew(PSensorTask, NULL, &PSensorTask_attributes);
#endif

#if ENABLE_ALS
  ALSensorTaskHandle = osThreadNew(ALSensorTask, NULL, &ALSensorTask_attributes);
#endif

#if ENABLE_ADC
  ADCTaskHandle = osThreadNew(ADCTask, NULL, &ADCTask_attributes);
#endif

    AL3010_Init();
    osDelay(10);

    RPR0521_Init();
    osDelay(10);
    RPR0521_SetUp();
    osDelay(10);

#if ENABLE_SCAN_I2C
  I2CScanTaskHandle = osThreadNew(I2CScanTask, NULL, &I2CScanTask_attributes);
#endif
    //Enable TOF Int
    HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);
    //Enable PS Int
    HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
    //Enable ALS Int
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
#if ENABLE_PS
#else
    ECX343EN_PowerOn();
    osDelay(10);
    current_state = 1;
#endif

    uint32_t pressTime = 0, releaseTime = 0;
    ButtonState buttonFuncCurrState = BUTTON_RELEASED;
    ButtonState buttonFuncPrevState = BUTTON_RELEASED;
    OperationMode currentMode = MODE_BRIGHTNESS;
    PowerSave displayType = MODE_RELEASE;
    ButtonClickType clickType = NO_CLICK;

#if REDUCE_BRIGHTNESS_ON_HIGH_TEMP
    uint32_t startTime = osKernelGetTickCount();
    uint32_t checkInterval = 10000;
#endif
    for(;;)
    {
		// JQueueMessage_t keyReport;
		// HID_Keypad_Report keypadReport;

		// for(uint32_t i = 0; i < 26; i++)
		// {
		//     keypadReport.reportId = 0x11;
		//     keypadReport.keys = (1 << i);

		//     keyReport.type = USB_HID_KEY_INPUT_REPORT;
		//     keyReport.data.keyReport.len = sizeof(HID_Keypad_Report);

		//     memcpy(keyReport.data.keyReport.report, (void *)&keypadReport, sizeof(keypadReport));
		//     usbSendMessage(&keyReport);
		//     osDelay(20);

		//     keypadReport.keys = 0x00000000;
		//     memcpy(keyReport.data.keyReport.report, (void *)&keypadReport, sizeof(keypadReport));
		//     usbSendMessage(&keyReport);
		//     osDelay(100);
		// }

    	osDelay(100);

    	if (!current_state) continue;

        if (command_flag)
        {
            command_flag = false;
            switchMode();
        }

        /* ButtonEvent */
        bool isReleased = false;
        bool newEventOccurred = UpdateButtonState(&buttonFuncPrevState, &buttonFuncCurrState, &pressTime, &releaseTime, &isReleased);

        if (newEventOccurred) {
            clickType = isReleased ? GetButtonClickType(pressTime, releaseTime) : LONG_PRESS;
            ProcessButtonEvent(1, &clickType, &currentMode, &displayType);
        } else if (possibleSingleClick && (osKernelGetTickCount() - lastClickTime > CLICK_DETECTION_PERIOD)) {
            possibleSingleClick = false;
            clickType = SINGLE_CLICK;
            ProcessButtonEvent(1, &clickType, &currentMode, &displayType);
        }

        uint8_t buttonEvent = 0;
        buttonEvent |= (HAL_GPIO_ReadPin(PNL_VOL_MINUS_GPIO_Port, PNL_VOL_MINUS_Pin) == GPIO_PIN_RESET) << 1;
        buttonEvent |= (HAL_GPIO_ReadPin(PNL_VOL_PLUS_GPIO_Port, PNL_VOL_PLUS_Pin) == GPIO_PIN_RESET) << 2;
        if (buttonEvent) {
            ProcessButtonEvent(buttonEvent, &clickType, &currentMode, &displayType);
        }
        /**/

#if REDUCE_BRIGHTNESS_ON_HIGH_TEMP
        /* Reduce brightness by 1000 when panel exceeds 90 degrees. */
        if ((osKernelGetTickCount() - startTime) > checkInterval)
        {
            startTime = osKernelGetTickCount();
			if (smoothed_left > 90.0f || smoothed_right > 90.0f)
			{
				for (uint8_t i = 0; i < 10; i++)
				{
					ecx343_current_data.uLCD_LUXL -= 10;
					ecx343_current_data.uLCD_LUXR -= 10;
					adjustBrightness();
					osDelay(10);
				}
			}

        }
        /**/
#endif
    }
}

void ADCTask(void *argument)
{
    for (;;)
    {
        if (current_state)
        {
        	if(DebugSwitch)
        	{
        		usbDebug("LeftOrigTemperature: %.2f\r\nRightOrigTemperature: %.2f\r\n", temperatureLeft, temperatureRight);
        	}
        	updatePanelTemperature();
        	if(DebugSwitch)
        	{
        		usbDebug("LeftTemperature: %.2f\r\nRightTemperature: %.2f\r\n", smoothed_left, smoothed_right);
        	}
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void I2CScanTask(void * argument)
{
	uint8_t result;
	uint8_t deviceAddr;

	for(;;)
	{
	    if(xSemaphoreTake(I2C1_Lock, portMAX_DELAY) == pdTRUE){
            for (uint16_t i=1; i<128; i++)
            {
                result = HAL_I2C_IsDeviceReady(I2C_BUS, (i<<1), 3, 5);
                if (result == HAL_OK)
                {
                    deviceAddr = (i<<1);
                    usbDebug("Ready deviceAddr: [%02x]\r\n", deviceAddr);
                }
            }
		i2c1TxUnblock();
	    }
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

#if ENABLE_TOF
static void ToFTask(void * argument)
{
    VL53L8CX_Configuration  Dev;
    VL53L8CX_ResultsData    Results;
    uint8_t resolution, isAlive, status;
//    uint8_t p_data_ready;
    /*ToF Initialize*/
//    HAL_GPIO_WritePin(ALS_RST_GPIO_Port, ALS_RST_Pin, GPIO_PIN_SET);
//    HAL_GPIO_WritePin(TOF_EN_GPIO_Port, TOF_EN_Pin, GPIO_PIN_SET);
//    HAL_GPIO_WritePin(CAM_RST_GPIO_Port, CAM_RST_Pin, GPIO_PIN_SET);
//    osDelay(1000);
    Dev.platform.address = VL53L8CX_DEFAULT_I2C_ADDRESS;
//    usb_waitUntilInited();
//    osDelay(500);
    Reset_Sensor(&(Dev.platform));
    while(1)
    {
        if(xSemaphoreTake(I2C1_Lock, portMAX_DELAY) == pdTRUE)
        {
            status = vl53l8cx_is_alive(&Dev, &isAlive);
            i2c1TxUnblock();
        }
        if(!isAlive)
        {
//            usbDebug("VL53L8CX not detected at requested address (0x%x) \r\n", Dev.platform.address);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        else break;

    }
//    usbDebug("Sensor initializing, please wait few seconds \r\n");
    if(xSemaphoreTake(I2C1_Lock, portMAX_DELAY) == pdTRUE)
    {
        status = vl53l8cx_init(&Dev);
        status = vl53l8cx_set_resolution(&Dev, VL53L8CX_RESOLUTION_8X8);
        status = vl53l8cx_set_ranging_frequency_hz(&Dev, 8);                // Set 2Hz ranging frequency
        status = vl53l8cx_set_ranging_mode(&Dev, VL53L8CX_RANGING_MODE_CONTINUOUS);  // Set mode continuous
//        usbDebug("Ranging starts \r\n");
        status = vl53l8cx_start_ranging(&Dev);
        i2c1TxUnblock();
    }
    nExecs_IsrToF = 0;
//    usbDebug("Ranging status:%d \r\n", status);
//    HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
//    HAL_NVIC_EnableIRQ(EXTI1_IRQn);
    osThreadFlagsSet(MainTaskHandle, 0x02);

  /* Infinite loop */
    while(1)
    {
        // Wait until isr triggered.
        xSemaphoreTake( isrToFLock, portMAX_DELAY);
//        status = vl53l8cx_check_data_ready(&Dev, &p_data_ready);
//        if(!p_data_ready)
//        {
//            osDelay(10);
//            continue;
//        }
        /* Do ToF Get data. */
        nExecs_IsrToF += 1;
        if (bRangePacketUpdated) continue;
        if(xSemaphoreTake(I2C1_Lock, portMAX_DELAY) == pdTRUE)
        {
            status = vl53l8cx_get_resolution(&Dev, &resolution);
            status = vl53l8cx_get_ranging_data(&Dev, &Results);
            i2c1TxUnblock();

#if ENABLE_TOF_DEBUG
        for(int i = 0; i < resolution;i++){
            /* Print per zone results */
            usbDebug("Zone : %2d, Nb targets : %2u, Ambient : %4lu Kcps/spads, \r\n",
                    i,
                    Results.nb_target_detected[i],
                    Results.ambient_per_spad[i]);

            /* Print per target results */
            if(Results.nb_target_detected[i] > 0){
                usbDebug("Target status : %3u, Distance : %4d mm \r\n",
                        Results.target_status[VL53L8CX_NB_TARGET_PER_ZONE * i],
                        Results.distance_mm[VL53L8CX_NB_TARGET_PER_ZONE * i]);
            }else{
                usbDebug("Target status : 255, Distance : No target\r\n");
            }
        }
        usbDebug("\r\n");
#else
        /*GOTO ToF CDC Process*/
            TickType_t timestamp = xTaskGetTickCount();
            tof_ranging_callback(&Results, timestamp);
        }
#endif
    }
}
#endif


/* USER CODE END Application */

