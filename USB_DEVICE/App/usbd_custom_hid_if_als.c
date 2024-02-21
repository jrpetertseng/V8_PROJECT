/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_custom_hid_if_als.c
  * @version        : v1.0_Cube
  * @brief          : USB Device Custom HID interface file for ALS.
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
#include "usbd_custom_hid_if_als.h"

/* USER CODE BEGIN INCLUDE */
#include "usbd_composite.h"
/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @brief Usb device.
  * @{
  */

/** @addtogroup USBD_CUSTOM_HID
  * @{
  */

/** @defgroup USBD_CUSTOM_HID_Private_TypesDefinitions USBD_CUSTOM_HID_Private_TypesDefinitions
  * @brief Private types.
  * @{
  */

/* USER CODE BEGIN PRIVATE_TYPES */

/* USER CODE END PRIVATE_TYPES */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_Defines USBD_CUSTOM_HID_Private_Defines
  * @brief Private defines.
  * @{
  */

/* USER CODE BEGIN PRIVATE_DEFINES */

/* USER CODE END PRIVATE_DEFINES */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_Macros USBD_CUSTOM_HID_Private_Macros
  * @brief Private macros.
  * @{
  */

/* USER CODE BEGIN PRIVATE_MACRO */

/* USER CODE END PRIVATE_MACRO */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_Variables USBD_CUSTOM_HID_Private_Variables
  * @brief Private variables.
  * @{
  */

/** Usb custom HID report descriptor. */
__ALIGN_BEGIN static uint8_t CUSTOM_HID_KEY_ReportDesc_FS[USBD_CUSTOM_HID_KEY_REPORT_DESC_SIZE] __ALIGN_END =
{
	/* USER CODE BEGIN 0 */
	/* Keypad */
	0x05, 0x01,  // USAGE_PAGE (Generic Desktop)
	0x09, 0x07,  // USAGE (Keypad)
	0xA1, 0x01,  // COLLECTION (Application)
	0x85, 0x11,    // REPORT_ID (1)
	0x05, 0x07,    // USAGE_PAGE (Keyboard)
	0x75, 0x01,    // REPORT_SIZE (1)
	0x95, 0x1A,    // REPORT_COUNT (26)
	0x09, 0x04,    // USAGE (Keyboard a)
	0x09, 0x07,    // USAGE (Keyboard d)
	0x09, 0x0E,    // USAGE (Keyboard k)
	0x09, 0x0F,    // USAGE (Keyboard l)
	0x09, 0x16,    // USAGE (Keyboard s)
	0x09, 0x1A,    // USAGE (Keyboard w)
	0x19, 0x1E,    // USAGE_MINIMUM (Keyboard 1)
	0x29, 0x28,    // USAGE_MAXIMUM (Keyboard Return)
	0x09, 0x46,    // USAGE (Keyboard PrintScreen)
	0x19, 0x4F,    // USAGE_MINIMUM (Keyboard RightArrow)
	0x29, 0x52,    // USAGE_MAXIMUM (Keyboard UpArrow)
	0x09, 0x65,    // USAGE (Keyboard Application)
	0x09, 0x66,    // USAGE (Keyboard Power)
	0x09, 0x7F,    // USAGE (Keyboard Mute)
	0x09, 0xF1,    // USAGE (Keyboard Back)
	0x15, 0x00,    // LOGICAL_MINIMUM (0)
	0x25, 0x01,    // LOGICAL_MAXIMUM (1)
	0x81, 0x02,    // INPUT (Data,Var,Abs)
	0x95, 0x06,    // REPORT_COUNT (6)
	0x81, 0x03,    // INPUT (Cnst,Var,Abs)
	0xC0,          // END_COLLECTION
	/* Consumer Control */
	0x05, 0x0C,    // USAGE_PAGE (Consumer)
	0x09, 0x01,    // USAGE (Consumer Control)
	0xA1, 0x01,    // COLLECTION (Application)
	0x85, 0x12,    // REPORT_ID (2)
	0x05, 0x0C,    // USAGE_PAGE (Consumer)
	0x75, 0x01,    // REPORT_SIZE (1)
	0x95, 0x05,    // REPORT_COUNT (5)
	0x09, 0x61,    // USAGE (Consumer CC Config)
	0x19, 0xB2,    // USAGE_MINIMUM (Consumer Record)
	0x29, 0xB4,    // USAGE_MAXIMUM (Consumer Rewind)
	0x09, 0xE2,    // USAGE (Consumer Mute)
	0x15, 0x00,    // LOGICAL_MINIMUM (0)
	0x25, 0x01,    // LOGICAL_MAXIMUM (1)
	0x81, 0x06,    // INPUT (Data,Var,Rel)
	0x95, 0x0B,    // REPORT_COUNT (11)
	0x09, 0x8C,    // USAGE (Consumer Selection)
	0x19, 0xB5,    // USAGE_MINIMUM (Consumer Scan Next Track)
	0x29, 0xB7,    // USAGE_MAXIMUM (Consumer Stop)
	0x09, 0xCD,    // USAGE (Consumer Play/Pause)
	0x0A, 0x73, 0x01, // USAGE (Consumer AL Consumer Control Configuration)
	0x0A, 0xB6, 0x01, // USAGE (Consumer AL Email Reader)
	0x1A, 0x23, 0x02, // USAGE_MINIMUM (AC Home)
	0x2A, 0x26, 0x02, // USAGE_MAXIMUM (AC Stop)
	0x15, 0x00,    // LOGICAL_MINIMUM (0)
	0x25, 0x01,    // LOGICAL_MAXIMUM (1)
	0x81, 0x02,    // INPUT (Data,Var,Abs)
	0x95, 0x10,    // REPORT_COUNT (16)
	0x81, 0x03,    // INPUT (Cnst,Var,Abs)
	0xC0,          // END_COLLECTION
	/* Gamepad */
	0x05, 0x01,    // USAGE_PAGE (Generic Desktop)
	0x09, 0x05,    // USAGE (Game Pad)
	0xA1, 0x01,    // COLLECTION (Application)
	0x85, 0x13,    // REPORT_ID (3)
	0x05, 0x09,    // USAGE_PAGE (Button)
	0x75, 0x01,    // REPORT_SIZE (1)
	0x95, 0x0F,    // REPORT_COUNT (15)
	0x19, 0x01,    // USAGE_MINIMUM (Button 1)
	0x29, 0x0F,    // USAGE_MAXIMUM (Button 15)
	0x15, 0x00,    // LOGICAL_MINIMUM (0)
	0x25, 0x01,    // LOGICAL_MAXIMUM (1)
	0x81, 0x02,    // INPUT (Data,Var,Abs)
	0x95, 0x11,    // REPORT_COUNT (17)
	0x81, 0x03,    // INPUT (Cnst,Var,Abs)
	0xC0,          // END_COLLECTION
	/* USER CODE END 0 */
};

/* USER CODE BEGIN PRIVATE_VARIABLES */

/* USER CODE END PRIVATE_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Exported_Variables USBD_CUSTOM_HID_Exported_Variables
  * @brief Public variables.
  * @{
  */

extern USBD_HandleTypeDef hUsbDeviceFS;

/* USER CODE BEGIN EXPORTED_VARIABLES */
HID_Keypad_Report hid_keyboard_als_report = {0};
/* USER CODE END EXPORTED_VARIABLES */
/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_FunctionPrototypes USBD_CUSTOM_HID_Private_FunctionPrototypes
  * @brief Private functions declaration.
  * @{
  */

static int8_t CUSTOM_HID_KEY_Init_FS(void);
static int8_t CUSTOM_HID_KEY_DeInit_FS(void);
static int8_t CUSTOM_HID_KEY_OutEvent_FS(uint8_t event_idx, uint8_t state);

/**
  * @}
  */

USBD_CUSTOM_HID_ItfTypeDef USBD_CustomHID_KEY_fops_FS =
{
  CUSTOM_HID_KEY_ReportDesc_FS,
  CUSTOM_HID_KEY_Init_FS,
  CUSTOM_HID_KEY_DeInit_FS,
  CUSTOM_HID_KEY_OutEvent_FS
};

/** @defgroup USBD_CUSTOM_HID_Private_Functions USBD_CUSTOM_HID_Private_Functions
  * @brief Private functions.
  * @{
  */

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes the CUSTOM HID media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CUSTOM_HID_KEY_Init_FS(void)
{
  /* USER CODE BEGIN 8 */
  return (USBD_OK);
  /* USER CODE END 8 */
}

/**
  * @brief  DeInitializes the CUSTOM HID media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CUSTOM_HID_KEY_DeInit_FS(void)
{
  /* USER CODE BEGIN 9 */
  return (USBD_OK);
  /* USER CODE END 9 */
}

/**
  * @brief  Manage the CUSTOM HID class events
  * @param  event_idx: Event index
  * @param  state: Event state
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CUSTOM_HID_KEY_OutEvent_FS(uint8_t event_idx, uint8_t state)
{
  /* USER CODE BEGIN 10 */
  UNUSED(event_idx);
  UNUSED(state);

    /* Start next USB packet transfer once data processing is completed */
  USBD_CUSTOM_HID_KEY_ReceivePacket(&hUsbDeviceFS);

  return (USBD_OK);
  /* USER CODE END 10 */
}

/* USER CODE BEGIN 11 */
/**
  * @brief  Send the report to the Host
  * @param  report: The report to be sent
  * @param  len: The report length
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
int8_t USBD_CUSTOM_HID_KEY_SendReport_FS(uint8_t *report, uint16_t len)
{
  /* NOTE:
   * We need manually switch the USB interface during the Tx data preparation
   * because this process is not part of operations in USBD_COMPOSITE.
   */
  USBD_Composite_Switch_Itf(&hUsbDeviceFS, USBD_CUSTOMHID_KEY_INTERFACE);
  return USBD_CUSTOM_HID_KEY_SendReport(&hUsbDeviceFS, report, len);
}
/* USER CODE END 11 */

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */

/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

