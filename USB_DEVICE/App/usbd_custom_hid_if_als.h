/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_custom_hid_if_als.h
  * @version        : v1.0_Cube
  * @brief          : Header for usbd_custom_hid_if_als.c file.
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
#ifndef __USBD_CUSTOM_HID_IF_ALS_H__383afbd0_43c5_11ed_9f2d_ef24e4705944_
#define __USBD_CUSTOM_HID_IF_ALS_H__383afbd0_43c5_11ed_9f2d_ef24e4705944_

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbd_customhid.h"

/* USER CODE BEGIN INCLUDE */

/* USER CODE END INCLUDE */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @brief For Usb device.
  * @{
  */

/** @defgroup USBD_CUSTOM_HID USBD_CUSTOM_HID
  * @brief Usb custom human interface device module.
  * @{
  */

/** @defgroup USBD_CUSTOM_HID_Exported_Defines USBD_CUSTOM_HID_Exported_Defines
  * @brief Defines.
  * @{
  */

/* USER CODE BEGIN EXPORTED_DEFINES */

/* USER CODE END EXPORTED_DEFINES */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Exported_Types USBD_CUSTOM_HID_Exported_Types
  * @brief Types.
  * @{
  */

/* USER CODE BEGIN EXPORTED_TYPES */
/*
typedef __PACKED_STRUCT _HID_Keyboard_ALS_Report {
	uint8_t report_id;
	uint32_t keys;
} HID_Keyboard_ALS_Report;
*/
typedef __PACKED_STRUCT HID_Keypad_Report {
    uint8_t reportId;   // Report ID, 1 byte
    uint32_t keys;      // Key states, 4 bytes (32 bits)
} HID_Keypad_Report;
/* USER CODE END EXPORTED_TYPES */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Exported_Macros USBD_CUSTOM_HID_Exported_Macros
  * @brief Aliases.
  * @{
  */

/* USER CODE BEGIN EXPORTED_MACRO */
int8_t USBD_CUSTOM_HID_KEY_SendReport_FS(uint8_t *report, uint16_t len);
/* USER CODE END EXPORTED_MACRO */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Exported_Variables USBD_CUSTOM_HID_Exported_Variables
  * @brief Public variables.
  * @{
  */

/** CUSTOMHID Interface callback. */
extern USBD_CUSTOM_HID_ItfTypeDef USBD_CustomHID_KEY_fops_FS;

/* USER CODE BEGIN EXPORTED_VARIABLES */
// keys for KEY
extern HID_Keypad_Report hid_keyboard_als_report;
/* USER CODE END EXPORTED_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Exported_FunctionsPrototype USBD_CUSTOM_HID_Exported_FunctionsPrototype
  * @brief Public functions declaration.
  * @{
  */

/* USER CODE BEGIN EXPORTED_FUNCTIONS */
int8_t USBD_CUSTOM_HID_KEY_SendReport_FS(uint8_t *report, uint16_t len);
/* USER CODE END EXPORTED_FUNCTIONS */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __USBD_CUSTOM_HID_IF_ALS_H__383afbd0_43c5_11ed_9f2d_ef24e4705944_ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
