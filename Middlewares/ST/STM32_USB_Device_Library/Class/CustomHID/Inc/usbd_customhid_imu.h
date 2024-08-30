/**
  ******************************************************************************
  * @file    usbd_customhid.h
  * @author  MCD Application Team
  * @brief   header file for the usbd_customhid.c file.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2015 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                      www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_CUSTOMHID_SENSOR_H
#define __USB_CUSTOMHID_SENSORIMU_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include  "usbd_ioreq.h"
#include  "usbd_customhid.h"

/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */

/** @defgroup USBD_CUSTOM_HID
  * @brief This file is the Header file for USBD_customhid.c
  * @{
  */


/** @defgroup USBD_CUSTOM_HID_Exported_Defines
  * @{
  */
#define CUSTOM_HID_SENSOR_EPIN_ADDR                         0x85U
#define CUSTOM_HID_SENSOR_EPIN_SIZE                         0x80U

#define CUSTOM_HID_SENSOR_EPOUT_ADDR                        0x05U
#define CUSTOM_HID_SENSOR_EPOUT_SIZE                        0x40U

#define USB_CUSTOM_HID_SENSOR_CONFIG_DESC_SIZ               41U
#define USB_CUSTOM_HID_SENSOR_DESC_SIZ                      9U

#ifndef CUSTOM_HID_SENSOR_HS_BINTERVAL
#define CUSTOM_HID_SENSOR_HS_BINTERVAL                      0x01U
#endif /* CUSTOM_HID_SENSOR_HS_BINTERVAL */

#ifndef CUSTOM_HID_SENSOR_FS_BINTERVAL
#define CUSTOM_HID_SENSOR_FS_BINTERVAL                      0x01U
#endif /* CUSTOM_HID_SENSOR_FS_BINTERVAL */

#ifndef USBD_CUSTOMHID_SENSOR_OUTREPORT_BUF_SIZE
#define USBD_CUSTOMHID_SENSOR_OUTREPORT_BUF_SIZE            64U
#endif /* USBD_CUSTOMHID_SENSOR_OUTREPORT_BUF_SIZE */

#ifndef USBD_CUSTOM_HID_SENSOR_REPORT_DESC_SIZE
#define USBD_CUSTOM_HID_SENSOR_REPORT_DESC_SIZE             2061U
#endif /* USBD_CUSTOM_HID_SENSOR_REPORT_DESC_SIZE */

#define CUSTOM_HID_SENSOR_DESCRIPTOR_TYPE                   0x21U
#define CUSTOM_HID_SENSOR_REPORT_DESC                       0x22U

#define CUSTOM_HID_SENSOR_REQ_SET_PROTOCOL                  0x0BU
#define CUSTOM_HID_SENSOR_REQ_GET_PROTOCOL                  0x03U

#define CUSTOM_HID_SENSOR_REQ_SET_IDLE                      0x0AU
#define CUSTOM_HID_SENSOR_REQ_GET_IDLE                      0x02U

#define CUSTOM_HID_SENSOR_REQ_SET_REPORT                    0x09U
#define CUSTOM_HID_SENSOR_REQ_GET_REPORT                    0x01U
/**
  * @}
  */
/* ckhsu, from Raito's capture of Seiko Epson response */
#define SEIKO_FEATURE_REPORT_ID_01      0x01
#define SEIKO_FEATURE_REPORT_ID_03      0x03
#define SEIKO_FEATURE_REPORT_ID_05      0x05
#define SEIKO_FEATURE_REPORT_ID_07      0x07
#define SEIKO_FEATURE_REPORT_ID_09      0x09
#define SEIKO_FEATURE_REPORT_ID_0B      0x0B
#define SEIKO_FEATURE_REPORT_ID_0D      0x0D
#define SEIKO_FEATURE_REPORT_ID_0F      0x0F
#define SEIKO_FEATURE_REPORT_ID_11      0x11
#define SEIKO_FEATURE_REPORT_ID_13      0x13
#define SEIKO_FEATURE_REPORT_ID_15      0x15
#define SEIKO_FEATURE_REPORT_ID_17      0x17

#define SEIKO_INPUT_REPORT_ID_12        0x12
#define SEIKO_INPUT_REPORT_ID_14        0x14
#define SEIKO_INPUT_REPORT_ID_16        0x16
#define SEIKO_INPUT_REPORT_ID_18        0x18


/** @defgroup USBD_CORE_Exported_TypesDefinitions
  * @{
  */
typedef struct
{
  uint8_t  Report_buf[USBD_CUSTOMHID_SENSOR_OUTREPORT_BUF_SIZE];
  uint32_t Protocol;
  uint32_t IdleState;
  uint32_t AltSetting;
  uint32_t IsReportAvailable;
  CUSTOM_HID_StateTypeDef state;
} USBD_CUSTOM_HID_Sensor_HandleTypeDef;
/**
  * @}
  */



/** @defgroup USBD_CORE_Exported_Macros
  * @{
  */

/**
  * @}
  */

/** @defgroup USBD_CORE_Exported_Variables
  * @{
  */

extern USBD_ClassTypeDef USBD_CUSTOM_HID_SENSOR;
#define USBD_CUSTOM_HID_SENSOR_CLASS &USBD_CUSTOM_HID_SENSOR
/**
  * @}
  */

/** @defgroup USB_CORE_Exported_Functions
  * @{
  */
uint8_t USBD_CUSTOM_HID_Sensor_SendReport(USBD_HandleTypeDef *pdev,
		uint8_t *report, uint16_t len);
uint8_t USBD_CUSTOM_HID_Sensor_ReceivePacket(USBD_HandleTypeDef *pdev);
uint8_t USBD_CUSTOM_HID_Sensor_RegisterInterface(USBD_HandleTypeDef *pdev,
		USBD_CUSTOM_HID_ItfTypeDef *fops);

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif  /* __USB_CUSTOMHID_H */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
