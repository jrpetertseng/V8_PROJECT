/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "portmacro.h"
#include "timers.h"
#include "semphr.h"

#include "i2c.h"

#include "debug_defs.h"
#include "cmd_engine.h"
#include "usbd_composite.h"
#include "usbd_cdc.h"
#include "usbd_cdc_devctlr.h"
#include "usbd_customhid.h"
#include "usbd_customhid_sensor.h"
#include "usbd_custom_hid_if.h"
#include "usbd_custom_hid_sensor_if.h"

#include "al3010.h"
#include "rpr0521.h"
#include "lt7911.h"
#include "ecx343.h"
#include "usb.h"
#include "bno080.h"
#include "vl53l8cx_api.h"
#include "button_handling.h"
#include "iqs7211e.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define I2C_BUS             	(&hi2c1)
#define PROXIMITY_THRESHOLD 	300
#define OFF_DEBOUNCE_THRESHOLD 	pdMS_TO_TICKS(5000)
#define ON_DEBOUNCE_THRESHOLD 	pdMS_TO_TICKS(1000)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern I2C_HandleTypeDef hi2c3;
extern TIM_HandleTypeDef htim13;
extern ECX343_DATA ecx343_current_data;
VL53L8CX_Configuration Dev;
uint8_t isDebugModeEnabled = 0;
uint8_t isAutoBrightnessEnabled = 0;
uint8_t isHighTempBrightnessEnabled = 1;

uint8_t encSwitch = 0;
uint8_t micSwitch = 0;

uint8_t isPanelOn = 0;

static SemaphoreHandle_t I2C1_Lock;
static SemaphoreHandle_t I2C3_Lock;
static SemaphoreHandle_t isrToFLock;
volatile unsigned long ulHighFrequencyTimerTicks;
/* USER CODE END Variables */
/* Definitions for UsbTxTask */
osThreadId_t UsbTxTaskHandle;
uint32_t UsbTxTaskBuffer[2048];
osStaticThreadDef_t UsbTxTaskControlBlock;
const osThreadAttr_t UsbTxTask_attributes =
{ .name = "UsbTxTask", .cb_mem = &UsbTxTaskControlBlock, .cb_size =
		sizeof(UsbTxTaskControlBlock), .stack_mem = &UsbTxTaskBuffer[0],
		.stack_size = sizeof(UsbTxTaskBuffer), .priority =
				(osPriority_t) osPriorityHigh, };

/* Definitions for TofTask */
osThreadId_t TofTaskHandle;
uint32_t ToFTaskBuffer[2048];
osStaticThreadDef_t ToFTaskControlBlock;
const osThreadAttr_t TofTask_attributes =
{ .name = "TofTask", .cb_mem = &ToFTaskControlBlock, .cb_size =
		sizeof(ToFTaskControlBlock), .stack_mem = &ToFTaskBuffer[0],
		.stack_size = sizeof(ToFTaskBuffer), .priority =
				(osPriority_t) osPriorityAboveNormal, };

/* Definitions for ImuSensorTask */
osThreadId_t ImuSensorTaskHandle;
uint32_t ImuSensorTaskBuffer[2048];
osStaticThreadDef_t ImuSensorTaskControlBlock;
const osThreadAttr_t ImuSensorTask_attributes =
{ .name = "ImuSensorTask", .cb_mem = &ImuSensorTaskControlBlock, .cb_size =
		sizeof(ImuSensorTaskControlBlock), .stack_mem = &ImuSensorTaskBuffer[0],
		.stack_size = sizeof(ImuSensorTaskBuffer), .priority =
				(osPriority_t) osPriorityAboveNormal, };

/* Definitions for PSensorTask */
osThreadId_t PSensorTaskHandle;
uint32_t PSensorTaskBuffer[512];
osStaticThreadDef_t PSensorTaskControlBlock;
const osThreadAttr_t PSensorTask_attributes =
{ .name = "PSensorTask", .cb_mem = &PSensorTaskControlBlock, .cb_size =
		sizeof(PSensorTaskControlBlock), .stack_mem = &PSensorTaskBuffer[0],
		.stack_size = sizeof(PSensorTaskBuffer), .priority =
				(osPriority_t) osPriorityNormal, };

/* Definitions for TouchTask */
osThreadId_t TouchTaskHandle;
uint32_t TouchTaskBuffer[512];
osStaticThreadDef_t TouchTaskControlBlock;
const osThreadAttr_t TouchTask_attributes =
{ .name = "TouchTask", .cb_mem = &TouchTaskControlBlock, .cb_size =
		sizeof(TouchTaskControlBlock), .stack_mem = &TouchTaskBuffer[0],
		.stack_size = sizeof(TouchTaskBuffer), .priority =
				(osPriority_t) osPriorityNormal, };

/* Definitions for ALSensorTask */
osThreadId_t ALSensorTaskHandle;
uint32_t ALSensorTaskBuffer[512];
osStaticThreadDef_t ALSensorTaskControlBlock;
const osThreadAttr_t ALSensorTask_attributes =
{ .name = "ALSensorTask", .cb_mem = &ALSensorTaskControlBlock, .cb_size =
		sizeof(ALSensorTaskControlBlock), .stack_mem = &ALSensorTaskBuffer[0],
		.stack_size = sizeof(ALSensorTaskBuffer), .priority =
				(osPriority_t) osPriorityNormal, };

/* Definitions for ADCTask */
osThreadId_t ADCTaskHandle;
uint32_t ADCTaskBuffer[512];
osStaticThreadDef_t ADCTaskControlBlock;
const osThreadAttr_t ADCTask_attributes =
{ .name = "ADCTask", .cb_mem = &ADCTaskControlBlock, .cb_size =
		sizeof(ADCTaskControlBlock), .stack_mem = &ADCTaskBuffer[0],
		.stack_size = sizeof(ADCTaskBuffer), .priority =
				(osPriority_t) osPriorityNormal, };

/* Definitions for I2CScanTask */
osThreadId_t I2CScanTaskHandle;
uint32_t I2CScanTaskBuffer[512];
osStaticThreadDef_t I2CScanTaskControlBlock;
const osThreadAttr_t I2CScanTask_attributes =
{ .name = "I2CScanTask", .cb_mem = &I2CScanTaskControlBlock, .cb_size =
		sizeof(I2CScanTaskControlBlock), .stack_mem = &I2CScanTaskBuffer[0],
		.stack_size = sizeof(I2CScanTaskBuffer), .priority =
				(osPriority_t) osPriorityNormal, };

/* Definitions for MainTask */
osThreadId_t MainTaskHandle;
uint32_t MainTaskBuffer[512];
osStaticThreadDef_t MainTaskControlBlock;
const osThreadAttr_t MainTask_attributes =
{ .name = "MainTask", .cb_mem = &MainTaskControlBlock, .cb_size =
		sizeof(MainTaskControlBlock), .stack_mem = &MainTaskBuffer[0],
		.stack_size = sizeof(MainTaskBuffer), .priority =
				(osPriority_t) osPriorityNormal, };

#if ENABLE_COUNTING_TASK
osThreadId_t CountingTaskHandle;
uint32_t CountingTaskBuffer[512];
osStaticThreadDef_t CountingTaskControlBlock;
const osThreadAttr_t CountingTask_attributes =
{ .name = "CountingTask", .cb_mem = &CountingTaskControlBlock, .cb_size =
        sizeof(CountingTaskControlBlock), .stack_mem = &CountingTaskBuffer[0],
        .stack_size = sizeof(CountingTaskBuffer), .priority =
                (osPriority_t) osPriorityNormal, };
#endif

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);
static inline void i2c1TxBlock(void);
static inline void i2c1TxUnblock(void);
static inline void i2c3TxBlock(void);
static inline void i2c3TxUnblock(void);

void checkAndReduceBrightness(uint32_t *startTime, uint32_t *lastHighTempTime);
void ResetTof(void);
void Tof_Hard_reset(void);
/* USER CODE END FunctionPrototypes */

void StartUsbTxTask(void *argument);
void StartTofTask(void *argument);
void StartImuSensorTask(void *argument);
void StartPSensorTask(void *argument);
void StartTouchTask(void *argument);
void StartALSensorTask(void *argument);
void StartADCTask(void *argument);
void StartI2CScanTask(void *argument);
void StartMainTask(void *argument);
void CountingTask(void *argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void)
{
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
	/* creation of UsbTxTask */
	UsbTxTaskHandle = osThreadNew(StartUsbTxTask, NULL, &UsbTxTask_attributes);

	/* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */
	/* USER CODE END RTOS_THREADS */

	/* USER CODE BEGIN RTOS_EVENTS */
	/* add events, ... */
	/* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartUsbTxTask */
/**
 * @brief Function implementing the usbTxTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartUsbTxTask */
void StartUsbTxTask(void *argument)
{
	/* init code for USB_DEVICE */
	HAL_GPIO_WritePin(ALS_RST_GPIO_Port, ALS_RST_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(CAM_RST_GPIO_Port, CAM_RST_Pin, GPIO_PIN_SET);
//	HAL_GPIO_WritePin(TOF_EN_GPIO_Port, TOF_EN_Pin, GPIO_PIN_SET);
	osDelay(500);

	HAL_GPIO_WritePin(LT7911_RSTN_GPIO_Port, LT7911_RSTN_Pin, GPIO_PIN_SET);

	usbInit();

	MX_USB_DEVICE_Init();
	osDelay(1000);

	/* creation of ImuSensorTask */
#if ENABLE_IMU
	ImuSensorTaskHandle = osThreadNew(StartImuSensorTask, NULL, &ImuSensorTask_attributes);
#endif
	I2C1_Lock = xSemaphoreCreateBinary();
	if (I2C1_Lock != NULL)
	{
		xSemaphoreGive(I2C1_Lock);
	}

	I2C3_Lock = xSemaphoreCreateBinary();
	if (I2C3_Lock != NULL)
	{
		xSemaphoreGive(I2C3_Lock);
	}

	/* creation of MainTask */
	MainTaskHandle = osThreadNew(StartMainTask, NULL, &MainTask_attributes);

	/* USER CODE BEGIN StartUsbTxTask */
	/* Infinite loop */
	usbLoop();

	/* USER CODE END StartUsbTxTask */
}

#if ENABLE_TOF
/* USER CODE BEGIN Header_StartTofTask */
/**
 * @brief Function implementing the TofTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartTofTask */
void StartTofTask(void *argument)
{
	/* USER CODE BEGIN StartTofTask */
	static VL53L8CX_ResultsData Results;
	uint8_t resolution;
	uint8_t status;
	TickType_t timestamp;
	ResetTof();

	/* Infinite loop */
	for (;;)
	{
		// Wait until isr triggered.
		xSemaphoreTake(isrToFLock, portMAX_DELAY);

		/* Do ToF Get data. */
		nExecs_IsrToF += 1;
		if (bRangePacketUpdated)
			continue;

		status = vl53l8cx_get_resolution(&Dev, &resolution);
		status = vl53l8cx_get_ranging_data(&Dev, &Results);
#if ENABLE_TOF_DEBUG
		for (int i = 0; i < resolution; i++)
		{
			/* Print per zone results */
			usbDebug(
					"Zone : %2d, Nb targets : %2u, Ambient : %4lu Kcps/spads, \r\n",
					i, Results.nb_target_detected[i],
					Results.ambient_per_spad[i]);

			/* Print per target results */
			if (Results.nb_target_detected[i] > 0)
			{
				usbDebug("Target status : %3u, Distance : %4d mm \r\n",
						Results.target_status[VL53L8CX_NB_TARGET_PER_ZONE * i],
						Results.distance_mm[VL53L8CX_NB_TARGET_PER_ZONE * i]);
			}
			else
			{
				usbDebug("Target status : 255, Distance : No target\r\n");
			}
		}
		usbDebug("\r\n");
#else
		/*GOTO ToF CDC Process*/
		timestamp = xTaskGetTickCount();
		tof_ranging_callback(&Results, timestamp);
#if ENABLE_STACK_CHECK
		//test: check high water
		UBaseType_t uxHighWaterMark;
		if (nExecs_IsrToF % 100 == 0)
		{
			uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
			usbDebug("ToF_task free stack：%lu\n", uxHighWaterMark);
		}
#endif
	}
#endif
	/* USER CODE END StartTofTask */
}
#endif

#if ENABLE_IMU
/* USER CODE BEGIN Header_StartImuSensorTask */
/**
 * @brief Function implementing the ImuSensorTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartImuSensorTask */
void StartImuSensorTask(void *argument)
{
	/* USER CODE BEGIN StartImuSensorTask */
	HAL_GPIO_WritePin(IMU_RST_GPIO_Port, IMU_RST_Pin, GPIO_PIN_SET);
	osDelay(1000);

	initSensor();
	/* Infinite loop */
	sensorLoop();
	/* USER CODE END StartImuSensorTask */
}
#endif

#if ENABLE_PS
/* USER CODE BEGIN Header_StartPSensorTask */
/**
 * @brief Function implementing the PSensorTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartPSensorTask */
void StartPSensorTask(void *argument)
{
	/* USER CODE BEGIN StartPSensorTask */
	uint8_t task_count = 0;
	uint16_t proximity_value = 0;
	static TickType_t lastTransitionTick = 0;

	/* Infinite loop */
	for (;;)
	{
		uint32_t thread_event_flag = osThreadFlagsWait(PS_EVT_FLAG, osFlagsNoClear, osWaitForever);
		if (thread_event_flag == PS_EVT_FLAG)
		{
			if (xSemaphoreTake(I2C1_Lock, portMAX_DELAY) == pdTRUE)
			{
				proximity_value = RPR0521_ReadPS();
				i2c1TxUnblock();

				bool isObjectDetected = (proximity_value > PROXIMITY_THRESHOLD) ? true : false;

				if (isObjectDetected != isPanelOn)
				{
					TickType_t currentTick = xTaskGetTickCount();
					TickType_t debounceThreshold = isPanelOn ? OFF_DEBOUNCE_THRESHOLD : ON_DEBOUNCE_THRESHOLD;

					if ((currentTick - lastTransitionTick) >= debounceThreshold)
					{
						isPanelOn = !isPanelOn;
						isPanelOn ? executeTaskWithMutex(POWER_ON, PANEL_BOTH) : executeTaskWithMutex(POWER_OFF, PANEL_BOTH);
						lastTransitionTick = currentTick;
					}
				}
				else
				{
					lastTransitionTick = xTaskGetTickCount(); // Reset the timer if the condition is no longer met
				}
				osThreadFlagsClear(PS_EVT_FLAG);

				if (isDebugModeEnabled && task_count >= 10)
				{
					usbDebug("Proximity value: %d\r\n", proximity_value);
					task_count = 0;
				}
			}
		}
		task_count += 1;
        osDelay(100);
	}
	/* USER CODE END StartPSensorTask */
}
#endif

#if ENABLE_TOUCH
#define I2C_LOCK_TIMEOUT_MS       200u
#define INIT_TOTAL_TIMEOUT_MS     2000u
#define POLL_INT_MS               10u

/* Keyboard report (Report ID 0x01) bit masks per original descriptor order */
#define HID_KBD_REPORT_ID         0x01u

/* Digits 1/2/3 are usage 0x1E/0x1F/0x20, mapped to bit6/bit7/bit8 */
#define HID_KEY_1_MASK            (1UL << 6)
#define HID_KEY_2_MASK            (1UL << 7)
#define HID_KEY_3_MASK            (1UL << 8)

/* Digit 0 is usage 0x27, mapped to bit15 */
#define HID_KEY_0_MASK            (1UL << 15)

/* Arrow keys are usage 0x4F..0x52, mapped to bit21..bit24 (range starts at 0x4E) */
#define HID_KEY_RIGHT_MASK        (1UL << 21)
#define HID_KEY_LEFT_MASK         (1UL << 22)
#define HID_KEY_DOWN_MASK         (1UL << 23)
#define HID_KEY_UP_MASK           (1UL << 24)

/* Edge-trigger state: avoid repeated PRESS_HOLD firing */
static iqs_gesture_e s_last_gesture = IQS_G_NONE;

static BaseType_t TakeI2C3LockWithLog(const char *tag, TickType_t to)
{
	BaseType_t ok = xSemaphoreTake(I2C3_Lock, to);

	if (ok != pdTRUE) {
		usbDebug("%s: I2C3_Lock TIMEOUT (%lu ms)\r\n",
		         tag,
		         (unsigned long)(to * portTICK_PERIOD_MS));
	}

	return ok;
}

static BaseType_t iqs_run_with_lock(const char *tag, TickType_t lock_to)
{
	if (TakeI2C3LockWithLog(tag, lock_to) != pdTRUE) {
		return pdFALSE;
	}

	IQS7211E_Run();
	i2c3TxUnblock();
	return pdTRUE;
}

static inline int rdy_level(void)
{
	return (HAL_GPIO_ReadPin(TOUCH_RDY_GPIO_Port, TOUCH_RDY_Pin) == GPIO_PIN_SET);
}

static const char *iqs_gesture_str(iqs_gesture_e g)
{
	switch (g) {
	case IQS_G_NONE:             return "NONE";
	case IQS_G_SINGLE_TAP:       return "SINGLE_TAP";
	case IQS_G_DOUBLE_TAP:       return "DOUBLE_TAP";
	case IQS_G_TRIPLE_TAP:       return "TRIPLE_TAP";
	case IQS_G_PRESS_HOLD:       return "PRESS_HOLD";
	case IQS_G_SWIPE_X_POS:      return "SWIPE_X_POS";
	case IQS_G_SWIPE_X_NEG:      return "SWIPE_X_NEG";
	case IQS_G_SWIPE_Y_POS:      return "SWIPE_Y_POS";
	case IQS_G_SWIPE_Y_NEG:      return "SWIPE_Y_NEG";
	default:                     return "UNKNOWN";
	}
}

static void usbhid_send_key_mask(uint32_t mask)
{
	JQueueMessage_t msg;

	msg.type               = USB_HID_KEY_INPUT_REPORT;
	msg.data.keyReport.len = sizeof(HID_KeyboardReport);

	HID_keyboard_report.report_id = HID_KBD_REPORT_ID;
	HID_keyboard_report.keys      = mask;
	memcpy(msg.data.keyReport.report, (void *)&HID_keyboard_report, sizeof(HID_keyboard_report));
	usbSendMessage(&msg);

	vTaskDelay(pdMS_TO_TICKS(20));

	HID_keyboard_report.keys = 0x00000000u;
	memcpy(msg.data.keyReport.report, (void *)&HID_keyboard_report, sizeof(HID_keyboard_report));
	usbSendMessage(&msg);

	vTaskDelay(pdMS_TO_TICKS(20));
}

static void handle_iqs_gesture(iqs_gesture_e g)
{
	if (g == IQS_G_PRESS_HOLD) {
		if (s_last_gesture != IQS_G_PRESS_HOLD) {
			usbhid_send_key_mask(HID_KEY_0_MASK);
		}
		s_last_gesture = g;
		return;
	}

	switch (g) {
	case IQS_G_SINGLE_TAP:
		usbhid_send_key_mask(HID_KEY_1_MASK);
		break;

	case IQS_G_DOUBLE_TAP:
		usbhid_send_key_mask(HID_KEY_2_MASK);
		break;

	case IQS_G_TRIPLE_TAP:
		usbhid_send_key_mask(HID_KEY_3_MASK);
		break;

	case IQS_G_SWIPE_X_POS:
		usbhid_send_key_mask(HID_KEY_RIGHT_MASK);
		break;

	case IQS_G_SWIPE_X_NEG:
		usbhid_send_key_mask(HID_KEY_LEFT_MASK);
		break;

	case IQS_G_SWIPE_Y_POS:
		usbhid_send_key_mask(HID_KEY_UP_MASK);
		break;

	case IQS_G_SWIPE_Y_NEG:
		usbhid_send_key_mask(HID_KEY_DOWN_MASK);
		break;

	default:
		break;
	}

	s_last_gesture = g;
}

static void TouchExtiEnable(void)
{
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void StartTouchTask(void *argument)
{
	(void)argument;

	usbDebug("touch task start\r\n");

	TouchExtiEnable();
	usbDebug("EXTI15_10 enabled, RDY=%d\r\n", rdy_level());

	if (rdy_level() == 0) {
		xTaskNotifyGive(TouchTaskHandle);
		usbDebug("RDY already LOW, notify injected\r\n");
	}

	/* One-shot init */
	if (TakeI2C3LockWithLog("init.take", pdMS_TO_TICKS(I2C_LOCK_TIMEOUT_MS)) == pdTRUE) {
		HAL_StatusTypeDef st = IQS7211E_Init();

		usbDebug("IQS7211E_Init st=%d\r\n", (int)st);
		i2c3TxUnblock();
	}

	/* Bounded polling until RUN */
	if (IQS7211E_IsPresent()) {
		const TickType_t lock_to  = pdMS_TO_TICKS(I2C_LOCK_TIMEOUT_MS);
		const TickType_t poll_int = pdMS_TO_TICKS(POLL_INT_MS);
		const TickType_t init_to  = pdMS_TO_TICKS(INIT_TOTAL_TIMEOUT_MS);
		TickType_t start          = xTaskGetTickCount();
		TickType_t next_wake      = start;

		for (;;) {
			(void)iqs_run_with_lock("init.poll", lock_to);

			if (IQS7211E_GetState() == IQS_STATE_RUN) {
				usbDebug("IQS7211E init done (RUN)\r\n");
				break;
			}

			if ((TickType_t)(xTaskGetTickCount() - start) >= init_to) {
				usbDebug("IQS7211E init TIMEOUT, state=%d\r\n",
				         (int)IQS7211E_GetState());
				break;
			}

			vTaskDelayUntil(&next_wake, poll_int);
		}
	} else {
		usbDebug("IQS7211E not present\r\n");
	}

	/* Main loop */
	for (;;) {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		if (!iqs_run_with_lock("evt", pdMS_TO_TICKS(I2C_LOCK_TIMEOUT_MS))) {
			usbDebug("evt: drop due to lock timeout\r\n");
			continue;
		}

		if (IQS7211E_HasNewData()) {
			iqs_gesture_e g = IQS7211E_GetGesture();

			if (g != IQS_G_NONE) {
				usbDebug("gesture=%s (%d)\r\n", iqs_gesture_str(g), (int)g);
				handle_iqs_gesture(g);
			} else {
				s_last_gesture = IQS_G_NONE;
			}

			IQS7211E_ClearNewData();
		}
	}
}
#endif

#if ENABLE_ALS
/* USER CODE BEGIN Header_StartALSensorTask */
/**
 * @brief Function implementing the ALSensorTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartALSensorTask */
void StartALSensorTask(void *argument)
{
	/* USER CODE BEGIN StartALSensorTask */
	static int current_lux_value = 0;
	static int target_panel_brightness = 0;
	static int current_lux_index = -3;
	static int previous_lux_index = -3;

	/* Infinite loop */
	for (;;)
	{
		uint32_t thread_event_flag = osThreadFlagsWait(ALS_EVT_FLAG, osFlagsNoClear, osWaitForever);
		if (thread_event_flag == ALS_EVT_FLAG)
		{
			if (xSemaphoreTake(I2C1_Lock, portMAX_DELAY) == pdTRUE)
			{
				AL3010_ReadData();
				i2c1TxUnblock();
				ALS_SendReport_HS();

				if (isAutoBrightnessEnabled)
				{
					if (ambientLight != current_lux_value)
					{
						current_lux_value = ambientLight;
						target_panel_brightness = mapLuxToPanelBrightness(current_lux_value, &current_lux_index);

						if (previous_lux_index != current_lux_index)
						{
							previous_lux_index = current_lux_index;
							if (isDebugModeEnabled)
							{
								usbDebug("Interval: %d\n", current_lux_index);
							}
						}
						smoothlyChangeBrightness(target_panel_brightness);
					}
				}
				osThreadFlagsClear(ALS_EVT_FLAG);
			}
		}
        osDelay(100);
	}
	/* USER CODE END StartALSensorTask */
}
#endif

#if ENABLE_ADC
/* USER CODE BEGIN Header_StartADCTask */
/**
 * @brief Function implementing the ADCTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartADCTask */
void StartADCTask(void *argument)
{
	/* USER CODE BEGIN StartADCTask */
	TickType_t lastWakeUpTime = xTaskGetTickCount();

	/* Infinite loop */
	for (;;)
	{
		if (isPanelOn)
		{
			if (isDebugModeEnabled)
			{
				usbDebug("LeftOrigTemperature: %.2f\r\nRightOrigTemperature: %.2f\r\n",
						g_temperatureLeftRaw, g_temperatureRightRaw);
			}
			updatePanelTemperature();
			if (isDebugModeEnabled)
			{
				usbDebug("LeftTemperature: %.2f\r\nRightTemperature: %.2f\r\n",
						g_temperatureLeftSmoothed, g_temperatureRightSmoothed);
			}
		}

		vTaskDelayUntil(&lastWakeUpTime, pdMS_TO_TICKS(5000));
	}
	/* USER CODE END StartADCTask */
}
#endif

#if ENABLE_SCAN_I2C
/* USER CODE BEGIN Header_StartI2CScanTask */
/**
 * @brief Function implementing the I2CScanTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartI2CScanTask */
void StartI2CScanTask(void *argument)
{
    /* USER CODE BEGIN StartI2CScanTask */
    uint8_t result;
    uint8_t deviceAddr;

    /* Infinite loop */
    for (;;)
    {
        for (uint16_t i = 1; i < 128; i++)
        {
            result = HAL_I2C_IsDeviceReady(I2C_BUS, (i << 1), 3, 5);
            if (result == HAL_OK)
            {
                deviceAddr = (i << 1);
                usbDebug("Ready deviceAddr: 0x[%02X]\r\n", deviceAddr);
                osDelay(20);
            }
        }
        osDelay(10000);
    }
    /* USER CODE END StartI2CScanTask */
}
#endif

/* USER CODE BEGIN Header_StartMainTask */
/**
 * @brief Function implementing the MainTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartMainTask */
void StartMainTask(void *argument)
{
	/* USER CODE BEGIN StartMainTask */
	Ecx343_data_init_default();
	osDelay(10);
	ECX343EN_Init();
	osDelay(1000);

#if ENABLE_TOF
	isrToFLock = xSemaphoreCreateBinary();
	/* creation of TofTask */
	TofTaskHandle = osThreadNew(StartTofTask, NULL, &TofTask_attributes);

	osThreadFlagsWait(0x02, osFlagsWaitAny, osWaitForever);
#endif

#if ENABLE_ALS
	AL3010_Init();
	osDelay(10);

	//Enable ALS Int
	HAL_NVIC_SetPriority(EXTI9_5_IRQn, 6, 0);
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

	/* creation of ALSensorTask */
	ALSensorTaskHandle = osThreadNew(StartALSensorTask, NULL, &ALSensorTask_attributes);
#endif

#if ENABLE_TOUCH
	TouchTaskHandle = osThreadNew(StartTouchTask, NULL, &TouchTask_attributes);
#endif

#if ENABLE_PANEL
	executeTaskWithMutex(POWER_ON, PANEL_BOTH);
	osDelay(10);
	isPanelOn = 1;
#else
	#if ENABLE_PS
		RPR0521_Init();
		osDelay(10);
		RPR0521_SetUp();
		osDelay(10);

		//Enable PS Int
		HAL_NVIC_SetPriority(EXTI0_IRQn, 6, 0);
		HAL_NVIC_EnableIRQ(EXTI0_IRQn);

		/* creation of PSensorTask */
		PSensorTaskHandle = osThreadNew(StartPSensorTask, NULL,	&PSensorTask_attributes);
	#endif
#endif

#if ENABLE_ADC
	/* creation of ADCTask */
	ADCTaskHandle = osThreadNew(StartADCTask, NULL, &ADCTask_attributes);
#endif

#if ENABLE_COUNTING_TASK
	CountingTaskHandle = osThreadNew(CountingTask, NULL, &CountingTask_attributes);
#endif

#if ENABLE_SCAN_I2C
	/* creation of I2CScanTask */
	I2CScanTaskHandle = osThreadNew(StartI2CScanTask, NULL, &I2CScanTask_attributes);
#endif

	ButtonClickType clickType = NO_CLICK;

	uint32_t startTime = osKernelGetTickCount();
	uint32_t lastHighTempTime = 0;

	uint8_t isEncEnable = 0;
	uint8_t isMicMute   = 0;
	/* Infinite loop */
	for (;;)
	{
		osDelay(20);

		if (encSwitch)
		{
        	isEncEnable = !isEncEnable;
			HAL_GPIO_WritePin(CM7001N_ENC_ENB_M_GPIO_Port, CM7001N_ENC_ENB_M_Pin, isEncEnable);
			usbDebug("Enc Enable: %d\r\n", isEncEnable);
			encSwitch = 0;
		}
		if (micSwitch)
		{
        	isMicMute = !isMicMute;
			HAL_GPIO_WritePin(CM6542_MIC_MUTE_M_GPIO_Port, CM6542_MIC_MUTE_M_Pin, isMicMute);
			usbDebug("Mic Mute: %d\r\n", isMicMute);
			micSwitch = 0;
		}

#if ENABLE_TOF_FORCE_RESET
		nTofGpioInts_1 += 1;
		if (nTofGpioInts_1 >= 10 && interruptTofEnable)
		{
			tofResetCount += 1;
			Tof_Hard_reset();
//    	    McuReset();
		}
		if (tof_resetFlag)
		{
			Tof_Hard_reset();
		}
#endif

		if (!isPanelOn) continue;

		#define BUTTON_FUNC_MASK  (1u)

		static ButtonContext buttonFuncCtx;
		static bool buttonFuncCtxInit = false;

		if (!buttonFuncCtxInit) {
			Button_Init(&buttonFuncCtx);
			buttonFuncCtxInit = true;
		}

		clickType = Button_Update(&buttonFuncCtx,
															POWER_SW_KEY_GPIO_Port,
															POWER_SW_KEY_Pin);

		if (clickType != NO_CLICK) {
			ProcessButtonEvent(BUTTON_FUNC_MASK, &clickType);
		}

		if(isHighTempBrightnessEnabled)
		{
			/* Reduce brightness to 3500 when the panel exceeds 70 degrees. */
			checkAndReduceBrightness(&startTime, &lastHighTempTime);
		}
	}
	/* USER CODE END StartMainTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
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

void isrToFTaskTrigger(void)
{
	static BaseType_t xHigherPriorityTaskWoken;

	xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(isrToFLock, &xHigherPriorityTaskWoken);

	/* If xHigherPriorityTaskWoken was set to true we should yield. */
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static inline void i2c1TxBlock(void)
{
	xSemaphoreTake(I2C1_Lock, portMAX_DELAY);
}

static inline void i2c1TxUnblock(void)
{
	xSemaphoreGive(I2C1_Lock);
}

static inline void i2c3TxBlock(void)
{
	xSemaphoreTake(I2C3_Lock, portMAX_DELAY);
}

static inline void i2c3TxUnblock(void)
{
	xSemaphoreGive(I2C3_Lock);
}

void checkAndReduceBrightness(uint32_t *startTime, uint32_t *lastHighTempTime) {
	const uint8_t luxStep = 10;
	const uint16_t defaultLux = 250;
    const uint32_t checkInterval = 1000;
    const uint32_t highTempThresholdTime = 60000;
    static bool reduceBrightness = false;

    uint32_t currentTick = osKernelGetTickCount();

    if (!reduceBrightness && (currentTick - *startTime) <= checkInterval) {
        return;
    }

    if (!reduceBrightness) {
        *startTime = currentTick;

        bool tempTooHigh = g_temperatureLeftSmoothed > 70.0f || g_temperatureRightSmoothed > 70.0f;

        if (tempTooHigh) {
            if (*lastHighTempTime == 0) {
                *lastHighTempTime = currentTick;
            } else if ((currentTick - *lastHighTempTime) > highTempThresholdTime) {
                reduceBrightness = true;
            }
        } else {
            *lastHighTempTime = 0;
            return;
        }
    }

    bool brightnessReduced = false;

    if (ecx343_current_data.uLCD_LUXL > defaultLux) {
        ecx343_current_data.uLCD_LUXL -= luxStep;
        brightnessReduced = true;
    }

    if (ecx343_current_data.uLCD_LUXR > defaultLux) {
        ecx343_current_data.uLCD_LUXR -= luxStep;
        brightnessReduced = true;
    }

    if (brightnessReduced) {
        isAutoBrightnessEnabled = 0;
        executeTaskWithMutex(ADJUST_BRIGHTNESS);
    }

    if (ecx343_current_data.uLCD_LUXL <= defaultLux && ecx343_current_data.uLCD_LUXR <= defaultLux) {
        reduceBrightness = false;
    }
}

void ResetTof(void)
{
	uint8_t isAlive, status;
	
	Dev.platform.address = VL53L8CX_DEFAULT_I2C_ADDRESS;
	
	VL53L8CX_Reset_Sensor(&(Dev.platform));
	
	while (1)
	{
		status = vl53l8cx_is_alive(&Dev, &isAlive);
		if (!isAlive)
		{
//        	usbDebug("VL53L8CX not detected at requested address (0x%x) \r\n", Dev.platform.address);
			VL53L8CX_Reset_Sensor(&(Dev.platform));
			vTaskDelay(pdMS_TO_TICKS(500));
		}
		else
			break;
	}
//	usbDebug("Sensor initializing, please wait few seconds \r\n");
	status = vl53l8cx_init(&Dev);
	status = vl53l8cx_set_resolution(&Dev, VL53L8CX_RESOLUTION_8X8);
	status = vl53l8cx_set_ranging_frequency_hz(&Dev, 15);
	status = vl53l8cx_set_target_order(&Dev, VL53L8CX_TARGET_ORDER_CLOSEST);
	status = vl53l8cx_set_ranging_mode(&Dev, VL53L8CX_RANGING_MODE_CONTINUOUS); // Set mode continuous

//	usbDebug("Ranging starts \r\n");
	status = vl53l8cx_start_ranging(&Dev);

	nExecs_IsrToF = 0;
//	usbDebug("Ranging status:%d \r\n", status);
//	HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
//	HAL_NVIC_EnableIRQ(EXTI1_IRQn);
	osThreadFlagsSet(MainTaskHandle, 0x02);
}

#if ENABLE_TOF
void Tof_Hard_reset(void)
{
	HAL_NVIC_DisableIRQ(EXTI1_IRQn);
	xSemaphoreGive(isrToFLock);
	xSemaphoreTake(isrToFLock, portMAX_DELAY);
//    osDelay(2000);

	I2C2_SoftwareReset();
	ResetTof();
	tof_resetFlag = 0;
	nTofGpioInts_1 = 0;
	osDelay(20);
	HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(EXTI1_IRQn);
}
#endif

void CountingTask(void *argument)
{
    bool bHIDTest = true;
	TickType_t countingTaskLastWakeUpTime = xTaskGetTickCount();
    uint8_t bCDC_Delay = 10; //ISSUED: GRAV and MAGN

    for (;;)
    {
        if(bHIDTest)
        {
            usbDebug("ACCEL: %d\r\n", accel3_count);
            osDelay(bCDC_Delay);
            usbDebug("GYRO: %d\r\n", gyro3_count);
            osDelay(bCDC_Delay);
            usbDebug("GRAV: %d\r\n", grav3_count);
            osDelay(bCDC_Delay);
            usbDebug("MAGN: %d\r\n", tstMag3_count);
            osDelay(bCDC_Delay);
            usbDebug("QUAT: %d\r\n", tstQuat_count);
            osDelay(bCDC_Delay);
            usbDebug("TOTAL: %d\r\n", nIMUHIDUsbOuts);
            osDelay(bCDC_Delay);
            usbDebug("FAIL: %d\r\n", nUsbfailed);
            osDelay(bCDC_Delay);
            usbDebug("BUSY: %d\r\n", nUsbBusy);
            osDelay(bCDC_Delay);
            usbDebug("-------------------\r\n");
            accel3_count = gyro3_count = grav3_count = tstMag3_count
                    = tstQuat_count = nIMUHIDUsbOuts = nUsbfailed = nUsbBusy = 0;
        }
		vTaskDelayUntil(&countingTaskLastWakeUpTime, pdMS_TO_TICKS(5000));
    }
}
/* USER CODE END Application */

