/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_custom_hid_sensor_if.c
  * @version        : v1.0_Cube
  * @brief          : USB Device Custom HID interface file.
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
#include "usbd_custom_hid_sensor_if.h"

/* USER CODE BEGIN INCLUDE */
#include "usbd_composite.h"
#include "HidSensorSpec.h"
#include "sensor_hid.h"
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
__ALIGN_BEGIN static uint8_t CUSTOM_HID_Sensor_ReportDesc_HS[USBD_CUSTOM_HID_SENSOR_REPORT_DESC_SIZE] __ALIGN_END =
{
0x05, 0x20,                     /*  Usage Page (20h),                   */
0x09, 0x41,                     /*  Usage (41h),                        */
0xA1, 0x00,                     /*  Collection (Physical),              */
0x85, 0x09,                     /*      Report ID (9),                  */
0x05, 0x20,                     /*      Usage Page (20h),               */
0x0A, 0x09, 0x03,               /*      Usage (0309h),                  */
0x15, 0x00,                     /*      Logical Minimum (0),            */
0x25, 0x02,                     /*      Logical Maximum (2),            */
0x75, 0x08,                     /*      Report Size (8),                */
0x95, 0x01,                     /*      Report Count (1),               */
0xA1, 0x02,                     /*      Collection (Logical),           */
0x0A, 0x30, 0x08,               /*          Usage (0830h),              */
0x0A, 0x31, 0x08,               /*          Usage (0831h),              */
0x0A, 0x32, 0x08,               /*          Usage (0832h),              */
0xB1, 0x01,                     /*          Feature (Constant),         */
0xC0,                           /*      End Collection,                 */
0x0A, 0x16, 0x03,               /*      Usage (0316h),                  */
0x15, 0x00,                     /*      Logical Minimum (0),            */
0x25, 0x05,                     /*      Logical Maximum (5),            */
0x75, 0x08,                     /*      Report Size (8),                */
0x95, 0x01,                     /*      Report Count (1),               */
0xA1, 0x02,                     /*      Collection (Logical),           */
0x0A, 0x40, 0x08,               /*          Usage (0840h),              */
0x0A, 0x41, 0x08,               /*          Usage (0841h),              */
0x0A, 0x42, 0x08,               /*          Usage (0842h),              */
0x0A, 0x43, 0x08,               /*          Usage (0843h),              */
0x0A, 0x44, 0x08,               /*          Usage (0844h),              */
0x0A, 0x45, 0x08,               /*          Usage (0845h),              */
0xB1, 0x00,                     /*          Feature,                    */
0xC0,                           /*      End Collection,                 */
0x0A, 0x19, 0x03,               /*      Usage (0319h),                  */
0x15, 0x01,                     /*      Logical Minimum (1),            */
0x25, 0x06,                     /*      Logical Maximum (6),            */
0x75, 0x08,                     /*      Report Size (8),                */
0x95, 0x01,                     /*      Report Count (1),               */
0xA1, 0x02,                     /*      Collection (Logical),           */
0x0A, 0x50, 0x08,               /*          Usage (0850h),              */
0x0A, 0x51, 0x08,               /*          Usage (0851h),              */
0x0A, 0x52, 0x08,               /*          Usage (0852h),              */
0x0A, 0x53, 0x08,               /*          Usage (0853h),              */
0x0A, 0x54, 0x08,               /*          Usage (0854h),              */
0x0A, 0x55, 0x08,               /*          Usage (0855h),              */
0xB1, 0x00,                     /*          Feature,                    */
0xC0,                           /*      End Collection,                 */
0x0A, 0x01, 0x02,               /*      Usage (0201h),                  */
0x15, 0x00,                     /*      Logical Minimum (0),            */
0x25, 0x06,                     /*      Logical Maximum (6),            */
0x75, 0x08,                     /*      Report Size (8),                */
0x95, 0x01,                     /*      Report Count (1),               */
0xA1, 0x02,                     /*      Collection (Logical),           */
0x0A, 0x00, 0x08,               /*          Usage (0800h),              */
0x0A, 0x01, 0x08,               /*          Usage (0801h),              */
0x0A, 0x02, 0x08,               /*          Usage (0802h),              */
0x0A, 0x03, 0x08,               /*          Usage (0803h),              */
0x0A, 0x04, 0x08,               /*          Usage (0804h),              */
0x0A, 0x05, 0x08,               /*          Usage (0805h),              */
0x0A, 0x06, 0x08,               /*          Usage (0806h),              */
0xB1, 0x00,                     /*          Feature,                    */
0xC0,                           /*      End Collection,                 */
0x0A, 0x0E, 0x03,               /*      Usage (030Eh),                  */
0x15, 0x00,                     /*      Logical Minimum (0),            */
0x27, 0xFF, 0xFF, 0xFF, 0x00,   /*      Logical Maximum (16777215),     */
0x75, 0x20,                     /*      Report Size (32),               */
0x95, 0x01,                     /*      Report Count (1),               */
0x55, 0x00,                     /*      Unit Exponent (0),              */
0xB1, 0x03,                     /*      Feature (Constant, Variable),   */
0x0A, 0xD1, 0x14,               /*      Usage (14D1h),                  */
0x15, 0x00,                     /*      Logical Minimum (0),            */
0x26, 0xFF, 0x00,               /*      Logical Maximum (255),          */
0x75, 0x10,                     /*      Report Size (16),               */
0x95, 0x01,                     /*      Report Count (1),               */
0x55, 0x00,                     /*      Unit Exponent (0),              */
0xB1, 0x02,                     /*      Feature (Variable),             */
0x0A, 0xD1, 0x24,               /*      Usage (24D1h),                  */
0x15, 0x00,                     /*      Logical Minimum (0),            */
0x26, 0xFF, 0x00,               /*      Logical Maximum (255),          */
0x75, 0x10,                     /*      Report Size (16),               */
0x95, 0x01,                     /*      Report Count (1),               */
0x67, 0xE1, 0x00, 0x00, 0x01,   /*      Unit (Centimeter^-2 * Candela), */
0x55, 0x00,                     /*      Unit Exponent (0),              */
0xB1, 0x02,                     /*      Feature (Variable),             */
0x0A, 0xD1, 0x34,               /*      Usage (34D1h),                  */
0x15, 0x00,                     /*      Logical Minimum (0),            */
0x26, 0xFF, 0x00,               /*      Logical Maximum (255),          */
0x75, 0x10,                     /*      Report Size (16),               */
0x95, 0x01,                     /*      Report Count (1),               */
0x67, 0xE1, 0x00, 0x00, 0x01,   /*      Unit (Centimeter^-2 * Candela), */
0x55, 0x00,                     /*      Unit Exponent (0),              */
0xB1, 0x02,                     /*      Feature (Variable),             */
0x85, 0x0A,                     /*      Report ID (10),                 */
0x05, 0x20,                     /*      Usage Page (20h),               */
0x0A, 0x01, 0x02,               /*      Usage (0201h),                  */
0x15, 0x00,                     /*      Logical Minimum (0),            */
0x25, 0x06,                     /*      Logical Maximum (6),            */
0x75, 0x08,                     /*      Report Size (8),                */
0x95, 0x01,                     /*      Report Count (1),               */
0xA1, 0x02,                     /*      Collection (Logical),           */
0x0A, 0x00, 0x08,               /*          Usage (0800h),              */
0x0A, 0x01, 0x08,               /*          Usage (0801h),              */
0x0A, 0x02, 0x08,               /*          Usage (0802h),              */
0x0A, 0x03, 0x08,               /*          Usage (0803h),              */
0x0A, 0x04, 0x08,               /*          Usage (0804h),              */
0x0A, 0x05, 0x08,               /*          Usage (0805h),              */
0x0A, 0x06, 0x08,               /*          Usage (0806h),              */
0x81, 0x02,                     /*          Input (Variable),           */
0xC0,                           /*      End Collection,                 */
0x0A, 0x02, 0x02,               /*      Usage (0202h),                  */
0x15, 0x00,                     /*      Logical Minimum (0),            */
0x25, 0x05,                     /*      Logical Maximum (5),            */
0x75, 0x08,                     /*      Report Size (8),                */
0x95, 0x01,                     /*      Report Count (1),               */
0xA1, 0x02,                     /*      Collection (Logical),           */
0x0A, 0x10, 0x08,               /*          Usage (0810h),              */
0x0A, 0x11, 0x08,               /*          Usage (0811h),              */
0x0A, 0x12, 0x08,               /*          Usage (0812h),              */
0x0A, 0x13, 0x08,               /*          Usage (0813h),              */
0x0A, 0x14, 0x08,               /*          Usage (0814h),              */
0x0A, 0x15, 0x08,               /*          Usage (0815h),              */
0x81, 0x02,                     /*          Input (Variable),           */
0xC0,                           /*      End Collection,                 */
0x0A, 0xD1, 0x04,               /*      Usage (04D1h),                  */
0x17, 0x01, 0x00, 0x00, 0x80,   /*      Logical Minimum (-2147483647),  */
0x27, 0xFF, 0xFF, 0xFF, 0x7F,   /*      Logical Maximum (2147483647),   */
0x67, 0xE1, 0x00, 0x00, 0x01,   /*      Unit (Centimeter^-2 * Candela), */
0x75, 0x20,                     /*      Report Size (32),               */
0x95, 0x01,                     /*      Report Count (1),               */
0x55, 0x00,                     /*      Unit Exponent (0),              */
0x81, 0x02,                     /*      Input (Variable),               */
0xC0                            /*  End Collection                      */
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

/* USER CODE END EXPORTED_VARIABLES */
/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_FunctionPrototypes USBD_CUSTOM_HID_Private_FunctionPrototypes
  * @brief Private functions declaration.
  * @{
  */

static int8_t CUSTOM_HID_Sensor_Init_HS(void);
static int8_t CUSTOM_HID_Sensor_DeInit_HS(void);
static int8_t CUSTOM_HID_Sensor_OutEvent_HS(uint8_t event_idx, uint8_t state);

/**
  * @}
  */

USBD_CUSTOM_HID_ItfTypeDef USBD_CustomHID_Sensor_fops_HS =
{
  CUSTOM_HID_Sensor_ReportDesc_HS,
  CUSTOM_HID_Sensor_Init_HS,
  CUSTOM_HID_Sensor_DeInit_HS,
  CUSTOM_HID_Sensor_OutEvent_HS
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
static int8_t CUSTOM_HID_Sensor_Init_HS(void)
{
  /* USER CODE BEGIN 8 */
  return (USBD_OK);
  /* USER CODE END 8 */
}

/**
  * @brief  DeInitializes the CUSTOM HID media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CUSTOM_HID_Sensor_DeInit_HS(void)
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
static int8_t CUSTOM_HID_Sensor_OutEvent_HS(uint8_t event_idx, uint8_t state)
{
  /* USER CODE BEGIN 10 */
  UNUSED(event_idx);
  UNUSED(state);

    /* Start next USB packet transfer once data processing is completed */
  USBD_CUSTOM_HID_Sensor_ReceivePacket(&hUsbDeviceHS);

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

int8_t USBD_CUSTOM_HID_Sensor_SendReport_HS(uint8_t *report, uint16_t len)
{
  /* NOTE:
   * We need manually switch the USB interface during the Tx data preparation
   * because this process is not part of operations in USBD_COMPOSITE.
   */
  USBD_Composite_Switch_Itf(&hUsbDeviceHS, USBD_CUSTOMHID_SENSOR_INTERFACE);
  return USBD_CUSTOM_HID_Sensor_SendReport(&hUsbDeviceHS, report, len);
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

