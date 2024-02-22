/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_custom_hid_if.c
  * @version        : v1.0_Cube
  * @brief          : USB Device Custom HID interface file.
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
#include "usbd_custom_hid_if.h"

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
__ALIGN_BEGIN static uint8_t CUSTOM_HID_ReportDesc_HS[USBD_CUSTOM_HID_REPORT_DESC_SIZE] __ALIGN_END =
{
  /* USER CODE BEGIN 1 */
  // ref: see https://github.com/torvalds/linux/blob/master/drivers/hid/hid-input.c for the kernel code
  // ref: see https://source.android.com/devices/input/keyboard-devices for details
  /* Keypad */
  0x05, 0x01,	// USAGE_PAGE (Generic Desktop)
  0x09, 0x07,	// USAGE (Keypad)
  0xa1, 0x01,	// COLLECTION (Application)
  0x85, 0x01,		// REPORT ID (1)
  0x05, 0x07,		// USAGE_PAGE (Keyboard)
  0x75, 0x01,		// REPORT_SIZE (1)
  0x95, 0x1D,		// REPORT_COUNT (29)
  0x09, 0x04,		// USAGE keyboard a
  0x09, 0x07,		// USAGE keyboard d
  0x09, 0x0E,		// USAGE keyboard k
  0x09, 0x0F,		// USAGE keyboard l
  0x09, 0x16,		// USAGE keyboard s
  0x09, 0x1A,		// USAGE keyboard w
  0x19, 0x1E,		// USAGE_MINUMUM (1)
  0x29, 0x28,		// USAGE_MAXIMUM (return)
  //	0x1E:			// USAGE keyboard 1
  //	0x1F:			// USAGE keyboard 2
  //	0x20:			// USAGE keyboard 3
  //	0x21:			// USAGE keyboard 4
  //	0x22:			// USAGE keyboard 5
  //	0x23:			// USAGE keyboard 6
  //	0x24:			// USAGE keyboard 7
  //	0x25:			// USAGE keyboard 8
  //	0x26:			// USAGE keyboard 9
  //	0x27:			// USAGE keyboard 0
  //    0x28:			// USAGE keyboard return
  //0x09, 0x2B,		// USAGE keyboard tab
  0x09, 0x46,		// USAGE keyboard print screen
  0x09, 0x47,		// USAGE keyboard scroll lock
  0x09, 0x4B,		// USAGE keyboard page up
  0x19, 0x4E,		// USAGE_MINUMUM (page down)
  0x29, 0x52,		// USAGE_MAXIMUM (up arrow)
  //	0x4F:			// USAGE keyboard right arrow
  //	0x50:			// USAGE keyboard left arrow
  //	0x51:			// USAGE keyboard down arrow
  //	0x52:			// USAGE keyboard up arrow
  0x09, 0x65,		// USAGE keyboard application
  0x09, 0x66,		// USAGE keyboard power
  0x09, 0x7F,		// USAGE keyboard mute
  //0x09, 0xE8,		// USAGE keyboard play/pause (not defined in HID Usage Tables)
  //0x09, 0xEA,		// USAGE keyboard previous song (not defined in HID Usage Tables)
  //0x09, 0xEB,		// USAGE keyboard next song (not defined in HID Usage Tables)
  0x09, 0xF1,		// USAGE keyboard back (not defined in HID Usage Tables)
  0x15, 0x00,		// LOGICAL_MINIMUM (0)
  0x25, 0x01,		// LOGICAL_MAXIMUM (1)
  0x81, 0x02,		// Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x95, 0x04,		// REPORT_COUNT (4)
  0x81, 0x03,		// Input (Cnst,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0xC0,			// END_COLLECTION
  /* consumer keys */
  0x05, 0x0C,	// USAGE_PAGE (Consumer)
  0x09, 0x01,	// USAGE (Consumer Control)
  0xA1, 0x01,	// COLLECTION (Application)
  0x85, 0x02,		// Report ID (2)
  0x05, 0x0C,		// USAGE_PAGE (Consumer)
  0x75, 0x01,		// REPORT_SIZE (1)
  0x95, 0x13,		// REPORT_COUNT (19)
  0x09, 0x61,		// USAGE closed caption (OOC)
  0x19, 0x69,		// USAGE_MINUMUM (red)
  0x29, 0x6C,		// USAGE_MAXIMUM (yellow)
  //	0x69:			// USAGE red (MC)
  //	0x6A:			// USAGE green (MC)
  //	0x6B:			// USAGE blue (MC)
  //	0x6C:			// USAGE yellow (MC)
  0x09, 0x6F,		// USAGE brightness increment (RTC)
  0x09, 0x70,		// USAGE brightness decrement (RTC)
  //0x09, 0x8C,		// USAGE telephone (Sel)
  0x19, 0xB2,		// USAGE_MINUMUM (record)
  0x29, 0xB4,		// USAGE_MAXIMUM (rewind)
  //	0xB2:			// USAGE record (OOC)
  //	0xB3:			// USAGE fast forward (OOC)
  //	0xB4:			// USAGE rewind (OOC)
  0x09, 0xE2,		// USAGE mute (OOC)
  0x09, 0xE9,		// USAGE volume increment (RTC)
  0x09, 0xEA,		// USAGE volume decrement (RTC)
  //0x0A, 0xB6, 0x01,	// USAGE AL image browser (HEADSETHOOK in Android) (Sel)
  0x0A, 0xCB, 0x01,	// USAGE AL assistant (Sel)
  0x0A, 0x21, 0x02,	// USAGE search (Sel)
  0x1A, 0x23, 0x02,	// USAGE_MINUMUM (AC home)
  0x2A, 0x26, 0x02,	// USAGE_MAXIMUM (AC stop)
  //	0x0223:			// USAGE AC home (Sel)
  //	0x0224:			// USAGE AC back (Sel)
  //	0x0225:			// USAGE AC forward (Sel)
  //	0x0226:			// USAGE AC stop (Sel)
  0x15, 0x00,		// LOGICAL_MINIMUM (0)
  0x25, 0x01,		// LOGICAL_MAXIMUM (1)
  0x81, 0x06,		// Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
  0x95, 0x06,		// REPORT_COUNT (6)
  0x19, 0xB5,		// USAGE_MINUMUM (scan next track)
  0x29, 0xB7,		// USAGE_MAXIMUM (stop)
  //	0xB5:			// USAGE scan next track (OSC)
  //	0xB6:			// USAGE scan previous track (OSC)
  //	0xB7:			// USAGE stop (OSC)
  0x09, 0xCD,		// USAGE play/pause (OSC)
  0x09, 0xCF,		// USAGE voice command (OSC)
  0x0A, 0x73, 0x01,	// USAGE alternate audio increment (OSC)
  0x15, 0x00,		// LOGICAL_MINIMUM (0)
  0x25, 0x01,		// LOGICAL_MAXIMUM (1)
  0x81, 0x02,		// Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x95, 0x07,		// REPORT_COUNT (7)
  0x81, 0x03,		// Input (Cnst,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0xC0,			// END_COLLECTION
  /* gamepad */
  0x05, 0x01,	// USAGE_PAGE (Generic Desktop)
  0x09, 0x05,	// USAGE (Gamepad)
  0xA1, 0x01,	// COLLECTION (Application)
  0x85, 0x03,		// Report ID (2)
  0x05, 0x09,		// USAGE_PAGE (Button)
  0x75, 0x01,		// REPORT_SIZE (1)
  0x95, 0x0F,		// REPORT_COUNT (15)
  /* 15 buttons (BtnA, BtnB, BtnC, BtnX, BtnY, BtnZ, BtnTL, BtnTR,
   *             BtnTL2, BtnTR2, BtnSelect, BtnStart, BtnMode, BtnThumbL, BtnThumbR) */
  0x19, 0x01,		// USAGE_MINUMUM (Button 1)
  0x29, 0x0F,		// USAGE_MAXIMUM (Button 15)
  0x15, 0x00,		// LOGICAL_MINIMUM (0)
  0x25, 0x01,		// LOGICAL_MAXIMUM (1)
  0x81, 0x02,		// Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x95, 0x11,		// REPORT_COUNT (17)
  0x81, 0x03,		// Input (Cnst,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  /* USER CODE END 1 */
   0xC0    /*     END_COLLECTION             */
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

extern USBD_HandleTypeDef hUsbDeviceHS;

/* USER CODE BEGIN EXPORTED_VARIABLES */
HID_Keyboard_Ipnput_Report HID_Keyboard_Report = {0};
/* USER CODE END EXPORTED_VARIABLES */
/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_FunctionPrototypes USBD_CUSTOM_HID_Private_FunctionPrototypes
  * @brief Private functions declaration.
  * @{
  */

static int8_t CUSTOM_HID_Init_HS(void);
static int8_t CUSTOM_HID_DeInit_HS(void);
static int8_t CUSTOM_HID_OutEvent_HS(uint8_t event_idx, uint8_t state);

/**
  * @}
  */

USBD_CUSTOM_HID_ItfTypeDef USBD_CustomHID_fops_HS =
{
  CUSTOM_HID_ReportDesc_HS,
  CUSTOM_HID_Init_HS,
  CUSTOM_HID_DeInit_HS,
  CUSTOM_HID_OutEvent_HS
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
static int8_t CUSTOM_HID_Init_HS(void)
{
  /* USER CODE BEGIN 8 */
  return (USBD_OK);
  /* USER CODE END 8 */
}

/**
  * @brief  DeInitializes the CUSTOM HID media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CUSTOM_HID_DeInit_HS(void)
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
static int8_t CUSTOM_HID_OutEvent_HS(uint8_t event_idx, uint8_t state)
{
  /* USER CODE BEGIN 10 */
  UNUSED(event_idx);
  UNUSED(state);

    /* Start next USB packet transfer once data processing is completed */
  USBD_CUSTOM_HID_ReceivePacket(&hUsbDeviceHS);

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
int8_t USBD_CUSTOM_HID_SendReport_HS(uint8_t *report, uint16_t len)
{
  /* NOTE:
   * We need manually switch the USB interface during the Tx data preparation
   * because this process is not part of operations in USBD_COMPOSITE.
   */
//  USBD_Composite_Switch_Itf(&hUsbDeviceHS, USBD_CUSTOMHID_INTERFACE);
  return USBD_CUSTOM_HID_SendReport(&hUsbDeviceHS, report, len);
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

