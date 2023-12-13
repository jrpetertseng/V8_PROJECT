///* USER CODE BEGIN Header */
///**
//  ******************************************************************************
//  * @file           : usbd_custom_hid_if_als.c
//  * @version        : v1.0_Cube
//  * @brief          : USB Device Custom HID interface file for ALS.
//  ******************************************************************************
//  * @attention
//  *
//  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
//  * All rights reserved.</center></h2>
//  *
//  * This software component is licensed by ST under Ultimate Liberty license
//  * SLA0044, the "License"; You may not use this file except in compliance with
//  * the License. You may obtain a copy of the License at:
//  *                             www.st.com/SLA0044
//  *
//  ******************************************************************************
//  */
///* USER CODE END Header */
//
///* Includes ------------------------------------------------------------------*/
//#include "usbd_custom_hid_if_als.h"
//
///* USER CODE BEGIN INCLUDE */
//#include "usbd_composite.h"
///* USER CODE END INCLUDE */
//
///* Private typedef -----------------------------------------------------------*/
///* Private define ------------------------------------------------------------*/
///* Private macro -------------------------------------------------------------*/
//
///* USER CODE BEGIN PV */
///* Private variables ---------------------------------------------------------*/
//
///* USER CODE END PV */
//
///** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
//  * @brief Usb device.
//  * @{
//  */
//
///** @addtogroup USBD_CUSTOM_HID
//  * @{
//  */
//
///** @defgroup USBD_CUSTOM_HID_Private_TypesDefinitions USBD_CUSTOM_HID_Private_TypesDefinitions
//  * @brief Private types.
//  * @{
//  */
//
///* USER CODE BEGIN PRIVATE_TYPES */
//
///* USER CODE END PRIVATE_TYPES */
//
///**
//  * @}
//  */
//
///** @defgroup USBD_CUSTOM_HID_Private_Defines USBD_CUSTOM_HID_Private_Defines
//  * @brief Private defines.
//  * @{
//  */
//
///* USER CODE BEGIN PRIVATE_DEFINES */
//
///* USER CODE END PRIVATE_DEFINES */
//
///**
//  * @}
//  */
//
///** @defgroup USBD_CUSTOM_HID_Private_Macros USBD_CUSTOM_HID_Private_Macros
//  * @brief Private macros.
//  * @{
//  */
//
///* USER CODE BEGIN PRIVATE_MACRO */
//
///* USER CODE END PRIVATE_MACRO */
//
///**
//  * @}
//  */
//
///** @defgroup USBD_CUSTOM_HID_Private_Variables USBD_CUSTOM_HID_Private_Variables
//  * @brief Private variables.
//  * @{
//  */
//
///** Usb custom HID report descriptor. */
//__ALIGN_BEGIN static uint8_t CUSTOM_HID_ALS_ReportDesc_FS[USBD_CUSTOM_HID_ALS_REPORT_DESC_SIZE] __ALIGN_END =
//{
//0x05, 0x20,        // Usage Page (0x20)
//0x09, 0x8E,        // Usage (0x8E)
//0xA1, 0x00,        // Collection (Physical)
//0x85, 0x0F,        //   Report ID (15)
//0x05, 0x20,        //   Usage Page (0x20)
//0x0A, 0x09, 0x03,  //   Usage (0x0309)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x02,        //   Logical Maximum (2)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x30, 0x08,  //     Usage (0x0830)
//0x0A, 0x31, 0x08,  //     Usage (0x0831)
//0x0A, 0x32, 0x08,  //     Usage (0x0832)
//0xB1, 0x01,        //     Feature (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0xC0,              //   End Collection
//0x0A, 0x16, 0x03,  //   Usage (0x0316)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x05,        //   Logical Maximum (5)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x40, 0x08,  //     Usage (0x0840)
//0x0A, 0x41, 0x08,  //     Usage (0x0841)
//0x0A, 0x42, 0x08,  //     Usage (0x0842)
//0x0A, 0x43, 0x08,  //     Usage (0x0843)
//0x0A, 0x44, 0x08,  //     Usage (0x0844)
//0x0A, 0x45, 0x08,  //     Usage (0x0845)
//0xB1, 0x00,        //     Feature (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0xC0,              //   End Collection
//0x0A, 0x19, 0x03,  //   Usage (0x0319)
//0x15, 0x01,        //   Logical Minimum (1)
//0x25, 0x06,        //   Logical Maximum (6)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x50, 0x08,  //     Usage (0x0850)
//0x0A, 0x51, 0x08,  //     Usage (0x0851)
//0x0A, 0x52, 0x08,  //     Usage (0x0852)
//0x0A, 0x53, 0x08,  //     Usage (0x0853)
//0x0A, 0x54, 0x08,  //     Usage (0x0854)
//0x0A, 0x55, 0x08,  //     Usage (0x0855)
//0xB1, 0x00,        //     Feature (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0xC0,              //   End Collection
//0x0A, 0x01, 0x02,  //   Usage (0x0201)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x06,        //   Logical Maximum (6)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x00, 0x08,  //     Usage (0x0800)
//0x0A, 0x01, 0x08,  //     Usage (0x0801)
//0x0A, 0x02, 0x08,  //     Usage (0x0802)
//0x0A, 0x03, 0x08,  //     Usage (0x0803)
//0x0A, 0x04, 0x08,  //     Usage (0x0804)
//0x0A, 0x05, 0x08,  //     Usage (0x0805)
//0x0A, 0x06, 0x08,  //     Usage (0x0806)
//0xB1, 0x00,        //     Feature (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0xC0,              //   End Collection
//0x0A, 0x0E, 0x03,  //   Usage (0x030E)
//0x15, 0x00,        //   Logical Minimum (0)
//0x27, 0xFF, 0xFF, 0xFF, 0x00,  //   Logical Maximum (16777214)
//0x75, 0x20,        //   Report Size (32)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x00,        //   Unit Exponent (0)
//0xB1, 0x03,        //   Feature (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0x0A, 0x83, 0x14,  //   Usage (0x1483)
//0x15, 0x00,        //   Logical Minimum (0)
//0x26, 0xFF, 0x00,  //   Logical Maximum (255)
//0x75, 0x10,        //   Report Size (16)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x0E,        //   Unit Exponent (-2)
//0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0x0A, 0x83, 0x24,  //   Usage (0x2483)
//0x16, 0x01, 0x80,  //   Logical Minimum (-32767)
//0x26, 0xFF, 0x7F,  //   Logical Maximum (32767)
//0x75, 0x10,        //   Report Size (16)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x01,        //   Unit Exponent (1)
//0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0x0A, 0x83, 0x34,  //   Usage (0x3483)
//0x16, 0x01, 0x80,  //   Logical Minimum (-32767)
//0x26, 0xFF, 0x7F,  //   Logical Maximum (32767)
//0x75, 0x10,        //   Report Size (16)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x01,        //   Unit Exponent (1)
//0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0x85, 0x10,        //   Report ID (16)
//0x05, 0x20,        //   Usage Page (0x20)
//0x0A, 0x01, 0x02,  //   Usage (0x0201)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x06,        //   Logical Maximum (6)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x00, 0x08,  //     Usage (0x0800)
//0x0A, 0x01, 0x08,  //     Usage (0x0801)
//0x0A, 0x02, 0x08,  //     Usage (0x0802)
//0x0A, 0x03, 0x08,  //     Usage (0x0803)
//0x0A, 0x04, 0x08,  //     Usage (0x0804)
//0x0A, 0x05, 0x08,  //     Usage (0x0805)
//0x0A, 0x06, 0x08,  //     Usage (0x0806)
//0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0xC0,              //   End Collection
//0x0A, 0x02, 0x02,  //   Usage (0x0202)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x05,        //   Logical Maximum (5)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x10, 0x08,  //     Usage (0x0810)
//0x0A, 0x11, 0x08,  //     Usage (0x0811)
//0x0A, 0x12, 0x08,  //     Usage (0x0812)
//0x0A, 0x13, 0x08,  //     Usage (0x0813)
//0x0A, 0x14, 0x08,  //     Usage (0x0814)
//0x0A, 0x15, 0x08,  //     Usage (0x0815)
//0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0xC0,              //   End Collection
//0x0A, 0x83, 0x04,  //   Usage (0x0483)
//0x17, 0x01, 0x00, 0x00, 0x80,  //   Logical Minimum (-2147483648)
//0x27, 0xFF, 0xFF, 0xFF, 0x7F,  //   Logical Maximum (2147483646)
//0x75, 0x20,        //   Report Size (32)
//0x95, 0x04,        //   Report Count (4)
//0x55, 0x08,        //   Unit Exponent (-8)
//0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0xC0,              // End Collection
//0x05, 0x20,        // Usage Page (0x20)
//0x09, 0xE1,        // Usage (0xE1)
//0xA1, 0x00,        // Collection (Physical)
//0x85, 0x11,        //   Report ID (17)
//0x05, 0x20,        //   Usage Page (0x20)
//0x0A, 0x09, 0x03,  //   Usage (0x0309)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x02,        //   Logical Maximum (2)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x30, 0x08,  //     Usage (0x0830)
//0x0A, 0x31, 0x08,  //     Usage (0x0831)
//0x0A, 0x32, 0x08,  //     Usage (0x0832)
//0xB1, 0x00,        //     Feature (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0xC0,              //   End Collection
//0x0A, 0x16, 0x03,  //   Usage (0x0316)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x05,        //   Logical Maximum (5)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x40, 0x08,  //     Usage (0x0840)
//0x0A, 0x41, 0x08,  //     Usage (0x0841)
//0x0A, 0x42, 0x08,  //     Usage (0x0842)
//0x0A, 0x43, 0x08,  //     Usage (0x0843)
//0x0A, 0x44, 0x08,  //     Usage (0x0844)
//0x0A, 0x45, 0x08,  //     Usage (0x0845)
//0xB1, 0x00,        //     Feature (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0xC0,              //   End Collection
//0x0A, 0x19, 0x03,  //   Usage (0x0319)
//0x15, 0x01,        //   Logical Minimum (1)
//0x25, 0x06,        //   Logical Maximum (6)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x50, 0x08,  //     Usage (0x0850)
//0x0A, 0x51, 0x08,  //     Usage (0x0851)
//0x0A, 0x52, 0x08,  //     Usage (0x0852)
//0x0A, 0x53, 0x08,  //     Usage (0x0853)
//0x0A, 0x54, 0x08,  //     Usage (0x0854)
//0x0A, 0x55, 0x08,  //     Usage (0x0855)
//0xB1, 0x00,        //     Feature (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0xC0,              //   End Collection
//0x0A, 0x01, 0x02,  //   Usage (0x0201)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x06,        //   Logical Maximum (6)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x00, 0x08,  //     Usage (0x0800)
//0x0A, 0x01, 0x08,  //     Usage (0x0801)
//0x0A, 0x02, 0x08,  //     Usage (0x0802)
//0x0A, 0x03, 0x08,  //     Usage (0x0803)
//0x0A, 0x04, 0x08,  //     Usage (0x0804)
//0x0A, 0x05, 0x08,  //     Usage (0x0805)
//0x0A, 0x06, 0x08,  //     Usage (0x0806)
//0xB1, 0x00,        //     Feature (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0xC0,              //   End Collection
//0x0A, 0x0E, 0x03,  //   Usage (0x030E)
//0x15, 0x00,        //   Logical Minimum (0)
//0x27, 0xFF, 0xFF, 0xFF, 0x00,  //   Logical Maximum (16777214)
//0x75, 0x20,        //   Report Size (32)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x00,        //   Unit Exponent (0)
//0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0x0A, 0x40, 0x15,  //   Usage (0x1540)
//0x15, 0x00,        //   Logical Minimum (0)
//0x26, 0xFF, 0x00,  //   Logical Maximum (255)
//0x75, 0x10,        //   Report Size (16)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x0E,        //   Unit Exponent (-2)
//0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0x0A, 0x40, 0x25,  //   Usage (0x2540)
//0x16, 0x01, 0x80,  //   Logical Minimum (-32767)
//0x26, 0xFF, 0x7F,  //   Logical Maximum (32767)
//0x75, 0x10,        //   Report Size (16)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x0E,        //   Unit Exponent (-2)
//0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0x0A, 0x40, 0x35,  //   Usage (0x3540)
//0x16, 0x01, 0x80,  //   Logical Minimum (-32767)
//0x26, 0xFF, 0x7F,  //   Logical Maximum (32767)
//0x75, 0x10,        //   Report Size (16)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x0E,        //   Unit Exponent (-2)
//0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0x85, 0x12,        //   Report ID (18)
//0x05, 0x20,        //   Usage Page (0x20)
//0x0A, 0x01, 0x02,  //   Usage (0x0201)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x06,        //   Logical Maximum (6)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x00, 0x08,  //     Usage (0x0800)
//0x0A, 0x01, 0x08,  //     Usage (0x0801)
//0x0A, 0x02, 0x08,  //     Usage (0x0802)
//0x0A, 0x03, 0x08,  //     Usage (0x0803)
//0x0A, 0x04, 0x08,  //     Usage (0x0804)
//0x0A, 0x05, 0x08,  //     Usage (0x0805)
//0x0A, 0x06, 0x08,  //     Usage (0x0806)
//0x81, 0x00,        //     Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0xC0,              //   End Collection
//0x0A, 0x02, 0x02,  //   Usage (0x0202)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x05,        //   Logical Maximum (5)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x10, 0x08,  //     Usage (0x0810)
//0x0A, 0x11, 0x08,  //     Usage (0x0811)
//0x0A, 0x12, 0x08,  //     Usage (0x0812)
//0x0A, 0x13, 0x08,  //     Usage (0x0813)
//0x0A, 0x14, 0x08,  //     Usage (0x0814)
//0x0A, 0x15, 0x08,  //     Usage (0x0815)
//0x81, 0x00,        //     Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0xC0,              //   End Collection
//0x0A, 0x41, 0x05,  //   Usage (0x0541)
//0x15, 0x00,        //   Logical Minimum (0)
//0x15, 0xFF,        //   Logical Minimum (-1)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x00,        //   Unit Exponent (0)
//0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0x0A, 0x44, 0x05,  //   Usage (0x0544)
//0x17, 0x01, 0x00, 0x00, 0x80,  //   Logical Minimum (-2147483648)
//0x27, 0xFF, 0xFF, 0xFF, 0x7F,  //   Logical Maximum (2147483646)
//0x75, 0x20,        //   Report Size (32)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x08,        //   Unit Exponent (-8)
//0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0x0A, 0x45, 0x05,  //   Usage (0x0545)
//0x17, 0x01, 0x00, 0x00, 0x80,  //   Logical Minimum (-2147483648)
//0x27, 0xFF, 0xFF, 0xFF, 0x7F,  //   Logical Maximum (2147483646)
//0x75, 0x20,        //   Report Size (32)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x08,        //   Unit Exponent (-8)
//0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0x0A, 0x46, 0x05,  //   Usage (0x0546)
//0x17, 0x01, 0x00, 0x00, 0x80,  //   Logical Minimum (-2147483648)
//0x27, 0xFF, 0xFF, 0xFF, 0x7F,  //   Logical Maximum (2147483646)
//0x75, 0x20,        //   Report Size (32)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x08,        //   Unit Exponent (-8)
//0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0x0A, 0x47, 0x05,  //   Usage (0x0547)
//0x17, 0x01, 0x00, 0x00, 0x80,  //   Logical Minimum (-2147483648)
//0x27, 0xFF, 0xFF, 0xFF, 0x7F,  //   Logical Maximum (2147483646)
//0x75, 0x20,        //   Report Size (32)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x08,        //   Unit Exponent (-8)
//0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0x0A, 0x48, 0x05,  //   Usage (0x0548)
//0x17, 0x01, 0x00, 0x00, 0x80,  //   Logical Minimum (-2147483648)
//0x27, 0xFF, 0xFF, 0xFF, 0x7F,  //   Logical Maximum (2147483646)
//0x75, 0x20,        //   Report Size (32)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x08,        //   Unit Exponent (-8)
//0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0x0A, 0x49, 0x05,  //   Usage (0x0549)
//0x17, 0x01, 0x00, 0x00, 0x80,  //   Logical Minimum (-2147483648)
//0x27, 0xFF, 0xFF, 0xFF, 0x7F,  //   Logical Maximum (2147483646)
//0x75, 0x20,        //   Report Size (32)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x08,        //   Unit Exponent (-8)
//0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0xC0,              // End Collection
//0x05, 0x20,        // Usage Page (0x20)
//0x09, 0xE1,        // Usage (0xE1)
//0xA1, 0x00,        // Collection (Physical)
//0x85, 0x13,        //   Report ID (19)
//0x05, 0x20,        //   Usage Page (0x20)
//0x0A, 0x09, 0x03,  //   Usage (0x0309)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x02,        //   Logical Maximum (2)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x30, 0x08,  //     Usage (0x0830)
//0x0A, 0x31, 0x08,  //     Usage (0x0831)
//0x0A, 0x32, 0x08,  //     Usage (0x0832)
//0xB1, 0x00,        //     Feature (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0xC0,              //   End Collection
//0x0A, 0x16, 0x03,  //   Usage (0x0316)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x05,        //   Logical Maximum (5)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x40, 0x08,  //     Usage (0x0840)
//0x0A, 0x41, 0x08,  //     Usage (0x0841)
//0x0A, 0x42, 0x08,  //     Usage (0x0842)
//0x0A, 0x43, 0x08,  //     Usage (0x0843)
//0x0A, 0x44, 0x08,  //     Usage (0x0844)
//0x0A, 0x45, 0x08,  //     Usage (0x0845)
//0xB1, 0x00,        //     Feature (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0xC0,              //   End Collection
//0x0A, 0x19, 0x03,  //   Usage (0x0319)
//0x15, 0x01,        //   Logical Minimum (1)
//0x25, 0x06,        //   Logical Maximum (6)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x50, 0x08,  //     Usage (0x0850)
//0x0A, 0x51, 0x08,  //     Usage (0x0851)
//0x0A, 0x52, 0x08,  //     Usage (0x0852)
//0x0A, 0x53, 0x08,  //     Usage (0x0853)
//0x0A, 0x54, 0x08,  //     Usage (0x0854)
//0x0A, 0x55, 0x08,  //     Usage (0x0855)
//0xB1, 0x00,        //     Feature (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0xC0,              //   End Collection
//0x0A, 0x01, 0x02,  //   Usage (0x0201)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x06,        //   Logical Maximum (6)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x00, 0x08,  //     Usage (0x0800)
//0x0A, 0x01, 0x08,  //     Usage (0x0801)
//0x0A, 0x02, 0x08,  //     Usage (0x0802)
//0x0A, 0x03, 0x08,  //     Usage (0x0803)
//0x0A, 0x04, 0x08,  //     Usage (0x0804)
//0x0A, 0x05, 0x08,  //     Usage (0x0805)
//0x0A, 0x06, 0x08,  //     Usage (0x0806)
//0xB1, 0x00,        //     Feature (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0xC0,              //   End Collection
//0x0A, 0x0E, 0x03,  //   Usage (0x030E)
//0x15, 0x00,        //   Logical Minimum (0)
//0x27, 0xFF, 0xFF, 0xFF, 0x00,  //   Logical Maximum (16777214)
//0x75, 0x20,        //   Report Size (32)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x00,        //   Unit Exponent (0)
//0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0x0A, 0x40, 0x15,  //   Usage (0x1540)
//0x15, 0x00,        //   Logical Minimum (0)
//0x26, 0xFF, 0x00,  //   Logical Maximum (255)
//0x75, 0x10,        //   Report Size (16)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x0E,        //   Unit Exponent (-2)
//0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0x0A, 0x40, 0x25,  //   Usage (0x2540)
//0x16, 0x01, 0x80,  //   Logical Minimum (-32767)
//0x26, 0xFF, 0x7F,  //   Logical Maximum (32767)
//0x75, 0x10,        //   Report Size (16)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x0E,        //   Unit Exponent (-2)
//0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0x0A, 0x40, 0x35,  //   Usage (0x3540)
//0x16, 0x01, 0x80,  //   Logical Minimum (-32767)
//0x26, 0xFF, 0x7F,  //   Logical Maximum (32767)
//0x75, 0x10,        //   Report Size (16)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x0E,        //   Unit Exponent (-2)
//0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0x85, 0x14,        //   Report ID (20)
//0x05, 0x20,        //   Usage Page (0x20)
//0x0A, 0x01, 0x02,  //   Usage (0x0201)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x06,        //   Logical Maximum (6)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x00, 0x08,  //     Usage (0x0800)
//0x0A, 0x01, 0x08,  //     Usage (0x0801)
//0x0A, 0x02, 0x08,  //     Usage (0x0802)
//0x0A, 0x03, 0x08,  //     Usage (0x0803)
//0x0A, 0x04, 0x08,  //     Usage (0x0804)
//0x0A, 0x05, 0x08,  //     Usage (0x0805)
//0x0A, 0x06, 0x08,  //     Usage (0x0806)
//0x81, 0x00,        //     Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0xC0,              //   End Collection
//0x0A, 0x02, 0x02,  //   Usage (0x0202)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x05,        //   Logical Maximum (5)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x10, 0x08,  //     Usage (0x0810)
//0x0A, 0x11, 0x08,  //     Usage (0x0811)
//0x0A, 0x12, 0x08,  //     Usage (0x0812)
//0x0A, 0x13, 0x08,  //     Usage (0x0813)
//0x0A, 0x14, 0x08,  //     Usage (0x0814)
//0x0A, 0x15, 0x08,  //     Usage (0x0815)
//0x81, 0x00,        //     Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0xC0,              //   End Collection
//0x0A, 0x41, 0x05,  //   Usage (0x0541)
//0x15, 0x00,        //   Logical Minimum (0)
//0x15, 0xFF,        //   Logical Minimum (-1)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x00,        //   Unit Exponent (0)
//0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0x0A, 0x44, 0x05,  //   Usage (0x0544)
//0x17, 0x01, 0x00, 0x00, 0x80,  //   Logical Minimum (-2147483648)
//0x27, 0xFF, 0xFF, 0xFF, 0x7F,  //   Logical Maximum (2147483646)
//0x75, 0x20,        //   Report Size (32)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x0A,        //   Unit Exponent (-6)
//0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0x0A, 0x45, 0x05,  //   Usage (0x0545)
//0x17, 0x01, 0x00, 0x00, 0x80,  //   Logical Minimum (-2147483648)
//0x27, 0xFF, 0xFF, 0xFF, 0x7F,  //   Logical Maximum (2147483646)
//0x75, 0x20,        //   Report Size (32)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x0A,        //   Unit Exponent (-6)
//0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0x0A, 0x46, 0x05,  //   Usage (0x0546)
//0x17, 0x01, 0x00, 0x00, 0x80,  //   Logical Minimum (-2147483648)
//0x27, 0xFF, 0xFF, 0xFF, 0x7F,  //   Logical Maximum (2147483646)
//0x75, 0x20,        //   Report Size (32)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x0A,        //   Unit Exponent (-6)
//0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0x0A, 0x47, 0x05,  //   Usage (0x0547)
//0x17, 0x01, 0x00, 0x00, 0x80,  //   Logical Minimum (-2147483648)
//0x27, 0xFF, 0xFF, 0xFF, 0x7F,  //   Logical Maximum (2147483646)
//0x75, 0x20,        //   Report Size (32)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x0A,        //   Unit Exponent (-6)
//0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0x0A, 0x48, 0x05,  //   Usage (0x0548)
//0x17, 0x01, 0x00, 0x00, 0x80,  //   Logical Minimum (-2147483648)
//0x27, 0xFF, 0xFF, 0xFF, 0x7F,  //   Logical Maximum (2147483646)
//0x75, 0x20,        //   Report Size (32)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x0A,        //   Unit Exponent (-6)
//0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0x0A, 0x49, 0x05,  //   Usage (0x0549)
//0x17, 0x01, 0x00, 0x00, 0x80,  //   Logical Minimum (-2147483648)
//0x27, 0xFF, 0xFF, 0xFF, 0x7F,  //   Logical Maximum (2147483646)
//0x75, 0x20,        //   Report Size (32)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x0A,        //   Unit Exponent (-6)
//0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0xC0,              // End Collection
//0x05, 0x20,        // Usage Page (0x20)
//0x09, 0xE1,        // Usage (0xE1)
//0xA1, 0x00,        // Collection (Physical)
//0x85, 0x15,        //   Report ID (21)
//0x05, 0x20,        //   Usage Page (0x20)
//0x0A, 0x09, 0x03,  //   Usage (0x0309)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x02,        //   Logical Maximum (2)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x30, 0x08,  //     Usage (0x0830)
//0x0A, 0x31, 0x08,  //     Usage (0x0831)
//0x0A, 0x32, 0x08,  //     Usage (0x0832)
//0xB1, 0x00,        //     Feature (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0xC0,              //   End Collection
//0x0A, 0x16, 0x03,  //   Usage (0x0316)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x05,        //   Logical Maximum (5)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x40, 0x08,  //     Usage (0x0840)
//0x0A, 0x41, 0x08,  //     Usage (0x0841)
//0x0A, 0x42, 0x08,  //     Usage (0x0842)
//0x0A, 0x43, 0x08,  //     Usage (0x0843)
//0x0A, 0x44, 0x08,  //     Usage (0x0844)
//0x0A, 0x45, 0x08,  //     Usage (0x0845)
//0xB1, 0x00,        //     Feature (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0xC0,              //   End Collection
//0x0A, 0x19, 0x03,  //   Usage (0x0319)
//0x15, 0x01,        //   Logical Minimum (1)
//0x25, 0x06,        //   Logical Maximum (6)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x50, 0x08,  //     Usage (0x0850)
//0x0A, 0x51, 0x08,  //     Usage (0x0851)
//0x0A, 0x52, 0x08,  //     Usage (0x0852)
//0x0A, 0x53, 0x08,  //     Usage (0x0853)
//0x0A, 0x54, 0x08,  //     Usage (0x0854)
//0x0A, 0x55, 0x08,  //     Usage (0x0855)
//0xB1, 0x00,        //     Feature (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0xC0,              //   End Collection
//0x0A, 0x01, 0x02,  //   Usage (0x0201)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x06,        //   Logical Maximum (6)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x00, 0x08,  //     Usage (0x0800)
//0x0A, 0x01, 0x08,  //     Usage (0x0801)
//0x0A, 0x02, 0x08,  //     Usage (0x0802)
//0x0A, 0x03, 0x08,  //     Usage (0x0803)
//0x0A, 0x04, 0x08,  //     Usage (0x0804)
//0x0A, 0x05, 0x08,  //     Usage (0x0805)
//0x0A, 0x06, 0x08,  //     Usage (0x0806)
//0xB1, 0x00,        //     Feature (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0xC0,              //   End Collection
//0x0A, 0x0E, 0x03,  //   Usage (0x030E)
//0x15, 0x00,        //   Logical Minimum (0)
//0x27, 0xFF, 0xFF, 0xFF, 0x00,  //   Logical Maximum (16777214)
//0x75, 0x20,        //   Report Size (32)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x00,        //   Unit Exponent (0)
//0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0x0A, 0x40, 0x15,  //   Usage (0x1540)
//0x15, 0x00,        //   Logical Minimum (0)
//0x26, 0xFF, 0x00,  //   Logical Maximum (255)
//0x75, 0x10,        //   Report Size (16)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x0E,        //   Unit Exponent (-2)
//0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0x0A, 0x40, 0x25,  //   Usage (0x2540)
//0x16, 0x01, 0x80,  //   Logical Minimum (-32767)
//0x26, 0xFF, 0x7F,  //   Logical Maximum (32767)
//0x75, 0x10,        //   Report Size (16)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x0E,        //   Unit Exponent (-2)
//0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0x0A, 0x40, 0x35,  //   Usage (0x3540)
//0x16, 0x01, 0x80,  //   Logical Minimum (-32767)
//0x26, 0xFF, 0x7F,  //   Logical Maximum (32767)
//0x75, 0x10,        //   Report Size (16)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x0E,        //   Unit Exponent (-2)
//0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0x85, 0x16,        //   Report ID (22)
//0x05, 0x20,        //   Usage Page (0x20)
//0x0A, 0x01, 0x02,  //   Usage (0x0201)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x06,        //   Logical Maximum (6)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x00, 0x08,  //     Usage (0x0800)
//0x0A, 0x01, 0x08,  //     Usage (0x0801)
//0x0A, 0x02, 0x08,  //     Usage (0x0802)
//0x0A, 0x03, 0x08,  //     Usage (0x0803)
//0x0A, 0x04, 0x08,  //     Usage (0x0804)
//0x0A, 0x05, 0x08,  //     Usage (0x0805)
//0x0A, 0x06, 0x08,  //     Usage (0x0806)
//0x81, 0x00,        //     Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0xC0,              //   End Collection
//0x0A, 0x02, 0x02,  //   Usage (0x0202)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x05,        //   Logical Maximum (5)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x10, 0x08,  //     Usage (0x0810)
//0x0A, 0x11, 0x08,  //     Usage (0x0811)
//0x0A, 0x12, 0x08,  //     Usage (0x0812)
//0x0A, 0x13, 0x08,  //     Usage (0x0813)
//0x0A, 0x14, 0x08,  //     Usage (0x0814)
//0x0A, 0x15, 0x08,  //     Usage (0x0815)
//0x81, 0x00,        //     Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0xC0,              //   End Collection
//0x0A, 0x41, 0x05,  //   Usage (0x0541)
//0x15, 0x00,        //   Logical Minimum (0)
//0x15, 0xFF,        //   Logical Minimum (-1)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x00,        //   Unit Exponent (0)
//0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0x0A, 0x44, 0x05,  //   Usage (0x0544)
//0x17, 0x01, 0x00, 0x00, 0x80,  //   Logical Minimum (-2147483648)
//0x27, 0xFF, 0xFF, 0xFF, 0x7F,  //   Logical Maximum (2147483646)
//0x75, 0x20,        //   Report Size (32)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x0C,        //   Unit Exponent (-4)
//0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0x0A, 0x45, 0x05,  //   Usage (0x0545)
//0x17, 0x01, 0x00, 0x00, 0x80,  //   Logical Minimum (-2147483648)
//0x27, 0xFF, 0xFF, 0xFF, 0x7F,  //   Logical Maximum (2147483646)
//0x75, 0x20,        //   Report Size (32)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x0C,        //   Unit Exponent (-4)
//0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0x0A, 0x46, 0x05,  //   Usage (0x0546)
//0x17, 0x01, 0x00, 0x00, 0x80,  //   Logical Minimum (-2147483648)
//0x27, 0xFF, 0xFF, 0xFF, 0x7F,  //   Logical Maximum (2147483646)
//0x75, 0x20,        //   Report Size (32)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x0C,        //   Unit Exponent (-4)
//0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0x0A, 0x47, 0x05,  //   Usage (0x0547)
//0x17, 0x01, 0x00, 0x00, 0x80,  //   Logical Minimum (-2147483648)
//0x27, 0xFF, 0xFF, 0xFF, 0x7F,  //   Logical Maximum (2147483646)
//0x75, 0x20,        //   Report Size (32)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x0C,        //   Unit Exponent (-4)
//0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0x0A, 0x48, 0x05,  //   Usage (0x0548)
//0x17, 0x01, 0x00, 0x00, 0x80,  //   Logical Minimum (-2147483648)
//0x27, 0xFF, 0xFF, 0xFF, 0x7F,  //   Logical Maximum (2147483646)
//0x75, 0x20,        //   Report Size (32)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x0C,        //   Unit Exponent (-4)
//0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0x0A, 0x49, 0x05,  //   Usage (0x0549)
//0x17, 0x01, 0x00, 0x00, 0x80,  //   Logical Minimum (-2147483648)
//0x27, 0xFF, 0xFF, 0xFF, 0x7F,  //   Logical Maximum (2147483646)
//0x75, 0x20,        //   Report Size (32)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x0C,        //   Unit Exponent (-4)
//0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0xC0,              // End Collection
//0x05, 0x20,        // Usage Page (0x20)
//0x09, 0xE1,        // Usage (0xE1)
//0xA1, 0x00,        // Collection (Physical)
//0x85, 0x17,        //   Report ID (23)
//0x05, 0x20,        //   Usage Page (0x20)
//0x0A, 0x09, 0x03,  //   Usage (0x0309)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x02,        //   Logical Maximum (2)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x30, 0x08,  //     Usage (0x0830)
//0x0A, 0x31, 0x08,  //     Usage (0x0831)
//0x0A, 0x32, 0x08,  //     Usage (0x0832)
//0xB1, 0x00,        //     Feature (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0xC0,              //   End Collection
//0x0A, 0x16, 0x03,  //   Usage (0x0316)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x05,        //   Logical Maximum (5)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x40, 0x08,  //     Usage (0x0840)
//0x0A, 0x41, 0x08,  //     Usage (0x0841)
//0x0A, 0x42, 0x08,  //     Usage (0x0842)
//0x0A, 0x43, 0x08,  //     Usage (0x0843)
//0x0A, 0x44, 0x08,  //     Usage (0x0844)
//0x0A, 0x45, 0x08,  //     Usage (0x0845)
//0xB1, 0x00,        //     Feature (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0xC0,              //   End Collection
//0x0A, 0x19, 0x03,  //   Usage (0x0319)
//0x15, 0x01,        //   Logical Minimum (1)
//0x25, 0x06,        //   Logical Maximum (6)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x50, 0x08,  //     Usage (0x0850)
//0x0A, 0x51, 0x08,  //     Usage (0x0851)
//0x0A, 0x52, 0x08,  //     Usage (0x0852)
//0x0A, 0x53, 0x08,  //     Usage (0x0853)
//0x0A, 0x54, 0x08,  //     Usage (0x0854)
//0x0A, 0x55, 0x08,  //     Usage (0x0855)
//0xB1, 0x00,        //     Feature (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0xC0,              //   End Collection
//0x0A, 0x01, 0x02,  //   Usage (0x0201)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x06,        //   Logical Maximum (6)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x00, 0x08,  //     Usage (0x0800)
//0x0A, 0x01, 0x08,  //     Usage (0x0801)
//0x0A, 0x02, 0x08,  //     Usage (0x0802)
//0x0A, 0x03, 0x08,  //     Usage (0x0803)
//0x0A, 0x04, 0x08,  //     Usage (0x0804)
//0x0A, 0x05, 0x08,  //     Usage (0x0805)
//0x0A, 0x06, 0x08,  //     Usage (0x0806)
//0xB1, 0x00,        //     Feature (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0xC0,              //   End Collection
//0x0A, 0x0E, 0x03,  //   Usage (0x030E)
//0x15, 0x00,        //   Logical Minimum (0)
//0x27, 0xFF, 0xFF, 0xFF, 0x00,  //   Logical Maximum (16777214)
//0x75, 0x20,        //   Report Size (32)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x00,        //   Unit Exponent (0)
//0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0x0A, 0x40, 0x15,  //   Usage (0x1540)
//0x15, 0x00,        //   Logical Minimum (0)
//0x26, 0xFF, 0x00,  //   Logical Maximum (255)
//0x75, 0x10,        //   Report Size (16)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x0E,        //   Unit Exponent (-2)
//0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0x0A, 0x40, 0x25,  //   Usage (0x2540)
//0x15, 0x00,        //   Logical Minimum (0)
//0x26, 0xFF, 0x00,  //   Logical Maximum (255)
//0x75, 0x10,        //   Report Size (16)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x00,        //   Unit Exponent (0)
//0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0x0A, 0x40, 0x35,  //   Usage (0x3540)
//0x15, 0x00,        //   Logical Minimum (0)
//0x26, 0xFF, 0x00,  //   Logical Maximum (255)
//0x75, 0x10,        //   Report Size (16)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x00,        //   Unit Exponent (0)
//0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
//0x85, 0x18,        //   Report ID (24)
//0x05, 0x20,        //   Usage Page (0x20)
//0x0A, 0x01, 0x02,  //   Usage (0x0201)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x06,        //   Logical Maximum (6)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x00, 0x08,  //     Usage (0x0800)
//0x0A, 0x01, 0x08,  //     Usage (0x0801)
//0x0A, 0x02, 0x08,  //     Usage (0x0802)
//0x0A, 0x03, 0x08,  //     Usage (0x0803)
//0x0A, 0x04, 0x08,  //     Usage (0x0804)
//0x0A, 0x05, 0x08,  //     Usage (0x0805)
//0x0A, 0x06, 0x08,  //     Usage (0x0806)
//0x81, 0x00,        //     Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0xC0,              //   End Collection
//0x0A, 0x02, 0x02,  //   Usage (0x0202)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x05,        //   Logical Maximum (5)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0xA1, 0x02,        //   Collection (Logical)
//0x0A, 0x10, 0x08,  //     Usage (0x0810)
//0x0A, 0x11, 0x08,  //     Usage (0x0811)
//0x0A, 0x12, 0x08,  //     Usage (0x0812)
//0x0A, 0x13, 0x08,  //     Usage (0x0813)
//0x0A, 0x14, 0x08,  //     Usage (0x0814)
//0x0A, 0x15, 0x08,  //     Usage (0x0815)
//0x81, 0x00,        //     Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0xC0,              //   End Collection
//0x0A, 0x41, 0x05,  //   Usage (0x0541)
//0x15, 0x00,        //   Logical Minimum (0)
//0x15, 0xFF,        //   Logical Minimum (-1)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x00,        //   Unit Exponent (0)
//0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0x0A, 0x44, 0x05,  //   Usage (0x0544)
//0x15, 0x00,        //   Logical Minimum (0)
//0x25, 0x50,        //   Logical Maximum (80)
//0x75, 0x08,        //   Report Size (8)
//0x95, 0x01,        //   Report Count (1)
//0x55, 0x00,        //   Unit Exponent (0)
//0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
//0xC0,              // End Collection
//// 1892 bytes
//};
//
///* USER CODE BEGIN PRIVATE_VARIABLES */
//
///* USER CODE END PRIVATE_VARIABLES */
//
///**
//  * @}
//  */
//
///** @defgroup USBD_CUSTOM_HID_Exported_Variables USBD_CUSTOM_HID_Exported_Variables
//  * @brief Public variables.
//  * @{
//  */
//
//extern USBD_HandleTypeDef hUsbDeviceFS;
//
///* USER CODE BEGIN EXPORTED_VARIABLES */
//HID_Keyboard_ALS_Report hid_keyboard_als_report = {0};
///* USER CODE END EXPORTED_VARIABLES */
///**
//  * @}
//  */
//
///** @defgroup USBD_CUSTOM_HID_Private_FunctionPrototypes USBD_CUSTOM_HID_Private_FunctionPrototypes
//  * @brief Private functions declaration.
//  * @{
//  */
//
//static int8_t CUSTOM_HID_ALS_Init_FS(void);
//static int8_t CUSTOM_HID_ALS_DeInit_FS(void);
//static int8_t CUSTOM_HID_ALS_OutEvent_FS(uint8_t event_idx, uint8_t state);
//
///**
//  * @}
//  */
//
//USBD_CUSTOM_HID_ItfTypeDef USBD_CustomHID_ALS_fops_FS =
//{
//  CUSTOM_HID_ALS_ReportDesc_FS,
//  CUSTOM_HID_ALS_Init_FS,
//  CUSTOM_HID_ALS_DeInit_FS,
//  CUSTOM_HID_ALS_OutEvent_FS
//};
//
///** @defgroup USBD_CUSTOM_HID_Private_Functions USBD_CUSTOM_HID_Private_Functions
//  * @brief Private functions.
//  * @{
//  */
//
///* Private functions ---------------------------------------------------------*/
//
///**
//  * @brief  Initializes the CUSTOM HID media low layer
//  * @retval USBD_OK if all operations are OK else USBD_FAIL
//  */
//static int8_t CUSTOM_HID_ALS_Init_FS(void)
//{
//  /* USER CODE BEGIN 8 */
//  return (USBD_OK);
//  /* USER CODE END 8 */
//}
//
///**
//  * @brief  DeInitializes the CUSTOM HID media low layer
//  * @retval USBD_OK if all operations are OK else USBD_FAIL
//  */
//static int8_t CUSTOM_HID_ALS_DeInit_FS(void)
//{
//  /* USER CODE BEGIN 9 */
//  return (USBD_OK);
//  /* USER CODE END 9 */
//}
//
///**
//  * @brief  Manage the CUSTOM HID class events
//  * @param  event_idx: Event index
//  * @param  state: Event state
//  * @retval USBD_OK if all operations are OK else USBD_FAIL
//  */
//static int8_t CUSTOM_HID_ALS_OutEvent_FS(uint8_t event_idx, uint8_t state)
//{
//  /* USER CODE BEGIN 10 */
//  UNUSED(event_idx);
//  UNUSED(state);
//
//    /* Start next USB packet transfer once data processing is completed */
//  USBD_CUSTOM_HID_ALS_ReceivePacket(&hUsbDeviceFS);
//
//  return (USBD_OK);
//  /* USER CODE END 10 */
//}
//
///* USER CODE BEGIN 11 */
///**
//  * @brief  Send the report to the Host
//  * @param  report: The report to be sent
//  * @param  len: The report length
//  * @retval USBD_OK if all operations are OK else USBD_FAIL
//  */
//int8_t USBD_CUSTOM_HID_ALS_SendReport_FS(uint8_t *report, uint16_t len)
//{
//  /* NOTE:
//   * We need manually switch the USB interface during the Tx data preparation
//   * because this process is not part of operations in USBD_COMPOSITE.
//   */
//  USBD_Composite_Switch_Itf(&hUsbDeviceFS, USBD_CUSTOMHID_ALS_INTERFACE);
//  return USBD_CUSTOM_HID_ALS_SendReport(&hUsbDeviceFS, report, len);
//}
///* USER CODE END 11 */
//
///* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */
//
///* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */
///**
//  * @}
//  */
//
///**
//  * @}
//  */
//
///**
//  * @}
//  */
//
///************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
//
