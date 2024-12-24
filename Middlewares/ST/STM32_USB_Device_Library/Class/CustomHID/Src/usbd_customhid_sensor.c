/**
  ******************************************************************************
  * @file    usbd_customhid_sensor.c
  * @author  MCD Application Team
  * @brief   This file provides the CUSTOM_HID core functions.
  *
  * @verbatim
  *
  *          ===================================================================
  *                                CUSTOM_HID Class  Description
  *          ===================================================================
  *           This module manages the CUSTOM_HID class V1.11 following the "Device Class Definition
  *           for Human Interface Devices (CUSTOM_HID) Version 1.11 Jun 27, 2001".
  *           This driver implements the following aspects of the specification:
  *             - The Boot Interface Subclass
  *             - Usage Page : Generic Desktop
  *             - Usage : Vendor
  *             - Collection : Application
  *
  * @note     In HS mode and when the DMA is used, all variables and data structures
  *           dealing with the DMA during the transaction process should be 32-bit aligned.
  *
  *
  *  @endverbatim
  *
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

/* BSPDependencies
- "stm32xxxxx_{eval}{discovery}{nucleo_144}.c"
- "stm32xxxxx_{eval}{discovery}_io.c"
EndBSPDependencies */

/* Includes ------------------------------------------------------------------*/
#include "usbd_customhid_sensor.h"
#include "usbd_ctlreq.h"

#include "sensor_hid.h"
#include "usb.h"

/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */


/** @defgroup USBD_CUSTOM_HID
  * @brief usbd core module
  * @{
  */

/** @defgroup USBD_CUSTOM_HID_Private_TypesDefinitions
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_CUSTOM_HID_Private_Defines
  * @{
  */
#define WBVAL(x) (x & 0xFF),((x >> 8) & 0xFF)
/**
  * @}
  */


/** @defgroup USBD_CUSTOM_HID_Private_Macros
  * @{
  */
/**
  * @}
  */
/** @defgroup USBD_CUSTOM_HID_Private_FunctionPrototypes
  * @{
  */

static uint8_t USBD_CUSTOM_HID_Sensor_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_CUSTOM_HID_Sensor_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_CUSTOM_HID_Sensor_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);

static uint8_t USBD_CUSTOM_HID_Sensor_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_CUSTOM_HID_Sensor_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_CUSTOM_HID_Sensor_EP0_RxReady(USBD_HandleTypeDef  *pdev);

static uint8_t *USBD_CUSTOM_HID_Sensor_GetFSCfgDesc(uint16_t *length);
static uint8_t *USBD_CUSTOM_HID_Sensor_GetHSCfgDesc(uint16_t *length);
static uint8_t *USBD_CUSTOM_HID_Sensor_GetOtherSpeedCfgDesc(uint16_t *length);
static uint8_t *USBD_CUSTOM_HID_Sensor_GetDeviceQualifierDesc(uint16_t *length);

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_Variables
  * @{
  */

USBD_ClassTypeDef  USBD_CUSTOM_HID_SENSOR =
{
  USBD_CUSTOM_HID_Sensor_Init,
  USBD_CUSTOM_HID_Sensor_DeInit,
  USBD_CUSTOM_HID_Sensor_Setup,
  NULL, /*EP0_TxSent*/
  USBD_CUSTOM_HID_Sensor_EP0_RxReady, /*EP0_RxReady*/ /* STATUS STAGE IN */
  USBD_CUSTOM_HID_Sensor_DataIn, /*DataIn*/
  USBD_CUSTOM_HID_Sensor_DataOut,
  NULL, /*SOF */
  NULL,
  NULL,
  USBD_CUSTOM_HID_Sensor_GetHSCfgDesc,
  USBD_CUSTOM_HID_Sensor_GetFSCfgDesc,
  USBD_CUSTOM_HID_Sensor_GetOtherSpeedCfgDesc,
  USBD_CUSTOM_HID_Sensor_GetDeviceQualifierDesc,
};

/* USB CUSTOM_HID device FS Configuration Descriptor */
__ALIGN_BEGIN static uint8_t USBD_CUSTOM_HID_Sensor_CfgFSDesc[USB_CUSTOM_HID_SENSOR_CONFIG_DESC_SIZ] __ALIGN_END =
{
  0x09,                                               /* bLength: Configuration Descriptor size */
  USB_DESC_TYPE_CONFIGURATION,                        /* bDescriptorType: Configuration */
  USB_CUSTOM_HID_SENSOR_CONFIG_DESC_SIZ,
                                                      /* wTotalLength: Bytes returned */
  0x00,
  0x01,                                               /* bNumInterfaces: 1 interface */
  0x01,                                               /* bConfigurationValue: Configuration value */
  0x00,                                               /* iConfiguration: Index of string descriptor describing the configuration */
#if (USBD_SELF_POWERED == 1U)
  0xC0,                                               /* bmAttributes: Bus Powered according to user configuration */
#else
  0x80,                                               /* bmAttributes: Bus Powered according to user configuration */
#endif
  USBD_MAX_POWER,                                     /* MaxPower 100 mA: this current is used for detecting Vbus */

  /************** Descriptor of CUSTOM HID interface ****************/
  /* 09 */
  0x09,                                               /* bLength: Interface Descriptor size*/
  USB_DESC_TYPE_INTERFACE,                            /* bDescriptorType: Interface descriptor type */
  0x00,                                               /* bInterfaceNumber: Number of Interface */
  0x00,                                               /* bAlternateSetting: Alternate setting */
  0x02,                                               /* bNumEndpoints*/
  0x03,                                               /* bInterfaceClass: CUSTOM_HID */
  0x00,                                               /* bInterfaceSubClass : 1=BOOT, 0=no boot */
  0x00,                                               /* nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */
  0x00,                                               /* iInterface: Index of string descriptor */
  /******************** Descriptor of CUSTOM_HID *************************/
  /* 18 */
  0x09,                                               /* bLength: CUSTOM_HID Descriptor size */
  CUSTOM_HID_SENSOR_DESCRIPTOR_TYPE,                  /* bDescriptorType: CUSTOM_HID */
  0x11,                                               /* bCUSTOM_HIDUSTOM_HID: CUSTOM_HID Class Spec release number */
  0x01,
  0x00,                                               /* bCountryCode: Hardware target country */
  0x01,                                               /* bNumDescriptors: Number of CUSTOM_HID class descriptors to follow */
  0x22,                                               /* bDescriptorType */
  LOBYTE(USBD_CUSTOM_HID_SENSOR_REPORT_DESC_SIZE),    /* wItemLength: Total length of Report descriptor */
  HIBYTE(USBD_CUSTOM_HID_SENSOR_REPORT_DESC_SIZE),
  /******************** Descriptor of Custom HID endpoints ********************/
  /* 27 */
  0x07,                                               /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,                             /* bDescriptorType: */

  CUSTOM_HID_SENSOR_EPIN_ADDR,                        /* bEndpointAddress: Endpoint Address (IN) */
  0x03,                                               /* bmAttributes: Interrupt endpoint */
  CUSTOM_HID_SENSOR_EPIN_SIZE,                        /* wMaxPacketSize: 2 Byte max */
  0x00,
  CUSTOM_HID_SENSOR_FS_BINTERVAL,                     /* bInterval: Polling Interval */
  /* 34 */

  0x07,                                               /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,                             /* bDescriptorType: */
  CUSTOM_HID_SENSOR_EPOUT_ADDR,                       /* bEndpointAddress: Endpoint Address (OUT) */
  0x03,                                               /* bmAttributes: Interrupt endpoint */
  CUSTOM_HID_SENSOR_EPOUT_SIZE,                       /* wMaxPacketSize: 2 Bytes max  */
  0x00,
  CUSTOM_HID_SENSOR_FS_BINTERVAL,                     /* bInterval: Polling Interval */
  /* 41 */
};

/* USB CUSTOM_HID device HS Configuration Descriptor */
__ALIGN_BEGIN static uint8_t USBD_CUSTOM_HID_Sensor_CfgHSDesc[USB_CUSTOM_HID_SENSOR_CONFIG_DESC_SIZ] __ALIGN_END =
{
  0x09,                                               /* bLength: Configuration Descriptor size */
  USB_DESC_TYPE_CONFIGURATION,                        /* bDescriptorType: Configuration */
  USB_CUSTOM_HID_SENSOR_CONFIG_DESC_SIZ,
                                                      /* wTotalLength: Bytes returned */
  0x00,
  0x01,                                               /* bNumInterfaces: 1 interface */
  0x01,                                               /* bConfigurationValue: Configuration value */
  0x00,                                               /* iConfiguration: Index of string descriptor describing the configuration */
#if (USBD_SELF_POWERED == 1U)
  0xC0,                                               /* bmAttributes: Bus Powered according to user configuration */
#else
  0x80,                                               /* bmAttributes: Bus Powered according to user configuration */
#endif
  USBD_MAX_POWER,                                     /* MaxPower 100 mA: this current is used for detecting Vbus */

  /************** Descriptor of CUSTOM HID interface ****************/
  /* 09 */
  0x09,                                               /* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,                            /* bDescriptorType: Interface descriptor type */
  0x00,                                               /* bInterfaceNumber: Number of Interface */
  0x00,                                               /* bAlternateSetting: Alternate setting */
  0x02,                                               /* bNumEndpoints */
  0x03,                                               /* bInterfaceClass: CUSTOM_HID */
  0x00,                                               /* bInterfaceSubClass : 1=BOOT, 0=no boot */
  0x00,                                               /* nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */
  0,                                                  /* iInterface: Index of string descriptor */
  /******************** Descriptor of CUSTOM_HID *************************/
  /* 18 */
  0x09,                                               /* bLength: CUSTOM_HID Descriptor size */
  CUSTOM_HID_SENSOR_DESCRIPTOR_TYPE,                  /* bDescriptorType: CUSTOM_HID */
  0x11,                                               /* bCUSTOM_HIDUSTOM_HID: CUSTOM_HID Class Spec release number */
  0x01,
  0x00,                                               /* bCountryCode: Hardware target country */
  0x01,                                               /* bNumDescriptors: Number of CUSTOM_HID class descriptors to follow */
  0x22,                                               /* bDescriptorType */
  LOBYTE(USBD_CUSTOM_HID_SENSOR_REPORT_DESC_SIZE),    /* wItemLength: Total length of Report descriptor */
  HIBYTE(USBD_CUSTOM_HID_SENSOR_REPORT_DESC_SIZE),
  /******************** Descriptor of Custom HID endpoints ********************/
  /* 27 */
  0x07,                                               /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,                             /* bDescriptorType: */

  CUSTOM_HID_SENSOR_EPIN_ADDR,                        /* bEndpointAddress: Endpoint Address (IN) */
  0x03,                                               /* bmAttributes: Interrupt endpoint */
  CUSTOM_HID_SENSOR_EPIN_SIZE,                        /* wMaxPacketSize: 2 Byte max */
  0x00,
  CUSTOM_HID_SENSOR_HS_BINTERVAL,                     /* bInterval: Polling Interval */
  /* 34 */

  0x07,                                               /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,                             /* bDescriptorType: */
  CUSTOM_HID_SENSOR_EPOUT_ADDR,                       /* bEndpointAddress: Endpoint Address (OUT) */
  0x03,                                               /* bmAttributes: Interrupt endpoint */
  CUSTOM_HID_SENSOR_EPOUT_SIZE,                       /* wMaxPacketSize: 2 Bytes max  */
  0x00,
  CUSTOM_HID_SENSOR_HS_BINTERVAL,                     /* bInterval: Polling Interval */
  /* 41 */
};

/* USB CUSTOM_HID device Other Speed Configuration Descriptor */
__ALIGN_BEGIN static uint8_t USBD_CUSTOM_HID_Sensor_OtherSpeedCfgDesc[USB_CUSTOM_HID_SENSOR_CONFIG_DESC_SIZ] __ALIGN_END =
{
  0x09,                                               /* bLength: Configuration Descriptor size */
  USB_DESC_TYPE_CONFIGURATION,                        /* bDescriptorType: Configuration */
  USB_CUSTOM_HID_SENSOR_CONFIG_DESC_SIZ,
                                                      /* wTotalLength: Bytes returned */
  0x00,
  0x01,                                               /* bNumInterfaces: 1 interface */
  0x01,                                               /* bConfigurationValue: Configuration value */
  0x00,                                               /* iConfiguration: Index of string descriptor describing the configuration */
#if (USBD_SELF_POWERED == 1U)
  0xC0,                                               /* bmAttributes: Bus Powered according to user configuration */
#else
  0x80,                                               /* bmAttributes: Bus Powered according to user configuration */
#endif
  USBD_MAX_POWER,                                     /* MaxPower 100 mA: this current is used for detecting Vbus */

  /************** Descriptor of CUSTOM HID interface ****************/
  /* 09 */
  0x09,                                               /* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,                            /* bDescriptorType: Interface descriptor type */
  0x00,                                               /* bInterfaceNumber: Number of Interface */
  0x00,                                               /* bAlternateSetting: Alternate setting */
  0x02,                                               /* bNumEndpoints */
  0x03,                                               /* bInterfaceClass: CUSTOM_HID */
  0x00,                                               /* bInterfaceSubClass : 1=BOOT, 0=no boot */
  0x00,                                               /* nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */
  0,                                                  /* iInterface: Index of string descriptor */
  /******************** Descriptor of CUSTOM_HID *************************/
  /* 18 */
  0x09,                                               /* bLength: CUSTOM_HID Descriptor size */
  CUSTOM_HID_SENSOR_DESCRIPTOR_TYPE,                         /* bDescriptorType: CUSTOM_HID */
  0x11,                                               /* bCUSTOM_HIDUSTOM_HID: CUSTOM_HID Class Spec release number */
  0x01,
  0x00,                                               /* bCountryCode: Hardware target country */
  0x01,                                               /* bNumDescriptors: Number of CUSTOM_HID class descriptors to follow */
  0x22,                                               /* bDescriptorType */
  LOBYTE(USBD_CUSTOM_HID_SENSOR_REPORT_DESC_SIZE),    /* wItemLength: Total length of Report descriptor */
  HIBYTE(USBD_CUSTOM_HID_SENSOR_REPORT_DESC_SIZE),
  /******************** Descriptor of Custom HID endpoints ********************/
  /* 27 */
  0x07,                                               /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,                             /* bDescriptorType: */

  CUSTOM_HID_SENSOR_EPIN_ADDR,                        /* bEndpointAddress: Endpoint Address (IN) */
  0x03,                                               /* bmAttributes: Interrupt endpoint */
  CUSTOM_HID_SENSOR_EPIN_SIZE,                        /* wMaxPacketSize: 2 Bytes max */
  0x00,
  CUSTOM_HID_SENSOR_FS_BINTERVAL,                     /* bInterval: Polling Interval */
  /* 34 */

  0x07,                                               /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,                             /* bDescriptorType: */
  CUSTOM_HID_SENSOR_EPOUT_ADDR,                       /* bEndpointAddress: Endpoint Address (OUT) */
  0x03,                                               /* bmAttributes: Interrupt endpoint */
  CUSTOM_HID_SENSOR_EPOUT_SIZE,                       /* wMaxPacketSize: 2 Bytes max */
  0x00,
  CUSTOM_HID_SENSOR_FS_BINTERVAL,                     /* bInterval: Polling Interval */
  /* 41 */
};

/* USB CUSTOM_HID device Configuration Descriptor */
__ALIGN_BEGIN static uint8_t USBD_CUSTOM_HID_Sensor_Desc[USB_CUSTOM_HID_SENSOR_DESC_SIZ] __ALIGN_END =
{
  /* 18 */
  0x09,                                               /* bLength: CUSTOM_HID Descriptor size */
  CUSTOM_HID_SENSOR_DESCRIPTOR_TYPE,                  /* bDescriptorType: CUSTOM_HID */
  0x11,                                               /* bCUSTOM_HIDUSTOM_HID: CUSTOM_HID Class Spec release number */
  0x01,
  0x00,                                               /* bCountryCode: Hardware target country */
  0x01,                                               /* bNumDescriptors: Number of CUSTOM_HID class descriptors to follow */
  0x22,                                               /* bDescriptorType */
  LOBYTE(USBD_CUSTOM_HID_SENSOR_REPORT_DESC_SIZE),    /* wItemLength: Total length of Report descriptor */
  HIBYTE(USBD_CUSTOM_HID_SENSOR_REPORT_DESC_SIZE),
};

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_CUSTOM_HID_Sensor_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
{
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0x00,
  0x00,
  0x00,
  0x40,
  0x01,
  0x00,
};

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_Functions
  * @{
  */
static USBD_CUSTOM_HID_Sensor_HandleTypeDef hhidSensor;

#define SEIKO_FEATURE_REPORT_LENGTH    15

static char seiko_feature_report_response[SEIKO_FEATURE_REPORT_LENGTH] =
{
    0x01, 0x02, 0x00, 0x06, 0x02, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static char seiko_feature_report_response_01[SEIKO_FEATURE_REPORT_LENGTH] =
{
    0x01, 0x02, 0x00, 0x06, 0x02, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static char seiko_feature_report_response_09[SEIKO_FEATURE_REPORT_LENGTH] =
{
    0x09, 0x02, 0x00, 0x06, 0x02, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define SEIKO_INPUT_REPORT_LENGTH_MAX  28
#define SEIKO_INPUT_REPORT_LENGTH_12   28
#define SEIKO_INPUT_REPORT_LENGTH_14   28
#define SEIKO_INPUT_REPORT_LENGTH_16   28
#define SEIKO_INPUT_REPORT_LENGTH_18   5

static char seiko_input_report_response[SEIKO_INPUT_REPORT_LENGTH_MAX] =
{
    0x00, 0xc2, 0x01, 0x00, 0x00, 0x00, 0x08, 0xc8,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

/**
  * @brief  USBD_CUSTOM_HID_Init
  *         Initialize the CUSTOM_HID interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t USBD_CUSTOM_HID_Sensor_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  UNUSED(cfgidx);
  USBD_CUSTOM_HID_Sensor_HandleTypeDef *hhid;

  hhid = &hhidSensor;

  pdev->pClassData = (void *)hhid;

  if (pdev->dev_speed == USBD_SPEED_HIGH)
  {
    pdev->ep_in[CUSTOM_HID_SENSOR_EPIN_ADDR & 0xFU].bInterval = CUSTOM_HID_SENSOR_HS_BINTERVAL;
    pdev->ep_out[CUSTOM_HID_SENSOR_EPOUT_ADDR & 0xFU].bInterval = CUSTOM_HID_SENSOR_HS_BINTERVAL;
  }
  else   /* LOW and FULL-speed endpoints */
  {
    pdev->ep_in[CUSTOM_HID_SENSOR_EPIN_ADDR & 0xFU].bInterval = CUSTOM_HID_SENSOR_FS_BINTERVAL;
    pdev->ep_out[CUSTOM_HID_SENSOR_EPOUT_ADDR & 0xFU].bInterval = CUSTOM_HID_SENSOR_FS_BINTERVAL;
  }

  /* Open EP IN */
  (void)USBD_LL_OpenEP(pdev, CUSTOM_HID_SENSOR_EPIN_ADDR, USBD_EP_TYPE_INTR,
                       CUSTOM_HID_SENSOR_EPIN_SIZE);

  pdev->ep_in[CUSTOM_HID_SENSOR_EPIN_ADDR & 0xFU].is_used = 1U;

  /* Open EP OUT */
  (void)USBD_LL_OpenEP(pdev, CUSTOM_HID_SENSOR_EPOUT_ADDR, USBD_EP_TYPE_INTR,
                       CUSTOM_HID_SENSOR_EPOUT_SIZE);

  pdev->ep_out[CUSTOM_HID_SENSOR_EPOUT_ADDR & 0xFU].is_used = 1U;

  hhid->state = CUSTOM_HID_IDLE;

  ((USBD_CUSTOM_HID_ItfTypeDef *)pdev->pUserData)->Init();

  /* Prepare Out endpoint to receive 1st packet */
  (void)USBD_LL_PrepareReceive(pdev, CUSTOM_HID_SENSOR_EPOUT_ADDR, hhid->Report_buf,
                               USBD_CUSTOMHID_SENSOR_OUTREPORT_BUF_SIZE);

  return (uint8_t)USBD_OK;
}

/**
  * @brief  USBD_CUSTOM_HID_DeInit
  *         DeInitialize the CUSTOM_HID layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t USBD_CUSTOM_HID_Sensor_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  UNUSED(cfgidx);

  /* Close CUSTOM_HID EP IN */
  (void)USBD_LL_CloseEP(pdev, CUSTOM_HID_SENSOR_EPIN_ADDR);
  pdev->ep_in[CUSTOM_HID_SENSOR_EPIN_ADDR & 0xFU].is_used = 0U;
  pdev->ep_in[CUSTOM_HID_SENSOR_EPIN_ADDR & 0xFU].bInterval = 0U;

  /* Close CUSTOM_HID EP OUT */
  (void)USBD_LL_CloseEP(pdev, CUSTOM_HID_SENSOR_EPOUT_ADDR);
  pdev->ep_out[CUSTOM_HID_SENSOR_EPOUT_ADDR & 0xFU].is_used = 0U;
  pdev->ep_out[CUSTOM_HID_SENSOR_EPOUT_ADDR & 0xFU].bInterval = 0U;

  /* Free allocated memory */
  if (pdev->pClassData != NULL)
  {
    ((USBD_CUSTOM_HID_ItfTypeDef *)pdev->pUserData)->DeInit();
    pdev->pClassData = NULL;
  }

  return (uint8_t)USBD_OK;
}

/**
  * @brief  USBD_CUSTOM_HID_Setup
  *         Handle the CUSTOM_HID specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t USBD_CUSTOM_HID_Sensor_Setup(USBD_HandleTypeDef *pdev,
                                           USBD_SetupReqTypedef *req)
{
    USBD_CUSTOM_HID_Sensor_HandleTypeDef *hhid = (USBD_CUSTOM_HID_Sensor_HandleTypeDef *)pdev->pClassData;
    uint16_t len = 0U;
    uint8_t *pbuf = NULL;
    uint16_t status_info = 0U;
    uint8_t report_type, report_id;
    char report_content[SEIKO_INPUT_REPORT_LENGTH_MAX];
    uint32_t report_length = 0U;
    USBD_StatusTypeDef ret = USBD_OK;

    if (hhid == NULL)
    {
        return (uint8_t)USBD_FAIL;
    }

    switch (req->bmRequest & USB_REQ_TYPE_MASK)
    {
        case USB_REQ_TYPE_CLASS:
            switch (req->bRequest)
            {
                case CUSTOM_HID_SENSOR_REQ_SET_PROTOCOL:
                    hhid->Protocol = (uint8_t)(req->wValue);
                    break;

                case CUSTOM_HID_SENSOR_REQ_GET_PROTOCOL:
                    USBD_CtlSendData(pdev, (uint8_t *)&hhid->Protocol, 1U);
                    break;

                case CUSTOM_HID_SENSOR_REQ_SET_IDLE:
                    hhid->IdleState = (uint8_t)(req->wValue >> 8);
                    break;

                case CUSTOM_HID_SENSOR_REQ_GET_IDLE:
                    USBD_CtlSendData(pdev, (uint8_t *)&hhid->IdleState, 1U);
                    break;

                case CUSTOM_HID_SENSOR_REQ_SET_REPORT:
                    hhid->IsReportAvailable = 1U;
                    USBD_CtlPrepareRx(pdev, hhid->Report_buf, req->wLength);
                    break;

                case CUSTOM_HID_SENSOR_REQ_GET_REPORT:
                    report_type = req->wValue >> 8;
                    report_id = req->wValue & 0xFF;

                    if (HID_REPORT_TYPE_INPUT == report_type)
                    {
                        switch (report_id)
                        {
                            case SEIKO_INPUT_REPORT_ID_12:
                                memcpy(report_content, seiko_input_report_response, SEIKO_INPUT_REPORT_LENGTH_12);
                                report_length = SEIKO_INPUT_REPORT_LENGTH_12;
                                break;

                            case SEIKO_INPUT_REPORT_ID_14:
                                memcpy(report_content, seiko_input_report_response, SEIKO_INPUT_REPORT_LENGTH_14);
                                report_length = SEIKO_INPUT_REPORT_LENGTH_14;
                                break;

                            case SEIKO_INPUT_REPORT_ID_16:
                                memcpy(report_content, seiko_input_report_response, SEIKO_INPUT_REPORT_LENGTH_16);
                                report_length = SEIKO_INPUT_REPORT_LENGTH_16;
                                break;

                            case SEIKO_INPUT_REPORT_ID_18:
                                memcpy(report_content, seiko_input_report_response, SEIKO_INPUT_REPORT_LENGTH_18);
                                report_length = SEIKO_INPUT_REPORT_LENGTH_18;
                                break;

                            default:
                                report_length = 0;
                                break;
                        }
                    }
                    else if (HID_REPORT_TYPE_FEATURE == report_type)
                    {
                        memcpy(report_content, seiko_feature_report_response, SEIKO_FEATURE_REPORT_LENGTH);
                        report_length = SEIKO_FEATURE_REPORT_LENGTH;

                        switch (report_id)
                        {
                            case SEIKO_FEATURE_REPORT_ID_01:
                                memcpy(report_content, seiko_feature_report_response_01, SEIKO_FEATURE_REPORT_LENGTH);
                                break;

                            case SEIKO_FEATURE_REPORT_ID_03:
                                report_content[0] = SEIKO_FEATURE_REPORT_ID_03;
                                break;

                            case SEIKO_FEATURE_REPORT_ID_05:
                                report_content[0] = SEIKO_FEATURE_REPORT_ID_05;
                                break;

                            case SEIKO_FEATURE_REPORT_ID_07:
                                report_content[0] = SEIKO_FEATURE_REPORT_ID_07;
                                break;

                            case SEIKO_FEATURE_REPORT_ID_09:
                                memcpy(report_content, seiko_feature_report_response_09, SEIKO_FEATURE_REPORT_LENGTH);
                                break;

                            case SEIKO_FEATURE_REPORT_ID_0B:
                                report_content[0] = SEIKO_FEATURE_REPORT_ID_0B;
                                break;

                            case SEIKO_FEATURE_REPORT_ID_0D:
                                report_content[0] = SEIKO_FEATURE_REPORT_ID_0D;
                                break;

                            case SEIKO_FEATURE_REPORT_ID_0F:
                                report_content[0] = SEIKO_FEATURE_REPORT_ID_0F;
                                break;

                            case SEIKO_FEATURE_REPORT_ID_11:
                                report_content[0] = SEIKO_FEATURE_REPORT_ID_11;
                                break;

                            case SEIKO_FEATURE_REPORT_ID_13:
                                report_content[0] = SEIKO_FEATURE_REPORT_ID_13;
                                break;

                            case SEIKO_FEATURE_REPORT_ID_15:
                                report_content[0] = SEIKO_FEATURE_REPORT_ID_15;
                                break;

                            case SEIKO_FEATURE_REPORT_ID_17:
                                report_content[0] = SEIKO_FEATURE_REPORT_ID_17;
                                break;

                            default:
                                report_length = 0;
                                break;
                        }
                    }

                    if (report_length > 0)
                    {
                        USBD_CtlSendData(pdev, (uint8_t *)report_content, report_length);
                    }
                    else
                    {
                        USBD_CtlError(pdev, req);
                        ret = USBD_FAIL;
                    }
                    break;

                default:
                    USBD_CtlError(pdev, req);
                    ret = USBD_FAIL;
                    break;
            }
            break;

        case USB_REQ_TYPE_STANDARD:
            switch (req->bRequest)
            {
                case USB_REQ_GET_STATUS:
                    if (pdev->dev_state == USBD_STATE_CONFIGURED)
                    {
                        USBD_CtlSendData(pdev, (uint8_t *)&status_info, 2U);
                    }
                    else
                    {
                        USBD_CtlError(pdev, req);
                        ret = USBD_FAIL;
                    }
                    break;

                case USB_REQ_GET_DESCRIPTOR:
                    if ((req->wValue >> 8) == CUSTOM_HID_SENSOR_REPORT_DESC)
                    {
                        len = MIN(USBD_CUSTOM_HID_SENSOR_REPORT_DESC_SIZE, req->wLength);
                        pbuf = ((USBD_CUSTOM_HID_ItfTypeDef *)pdev->pUserData)->pReport;
                    }
                    else if ((req->wValue >> 8) == CUSTOM_HID_SENSOR_DESCRIPTOR_TYPE)
                    {
                        pbuf = USBD_CUSTOM_HID_Sensor_Desc;
                        len = MIN(USB_CUSTOM_HID_SENSOR_DESC_SIZ, req->wLength);
                    }

                    USBD_CtlSendData(pdev, pbuf, len);
                    break;

                case USB_REQ_GET_INTERFACE:
                    if (pdev->dev_state == USBD_STATE_CONFIGURED)
                    {
                        USBD_CtlSendData(pdev, (uint8_t *)&hhid->AltSetting, 1U);
                    }
                    else
                    {
                        USBD_CtlError(pdev, req);
                        ret = USBD_FAIL;
                    }
                    break;

                case USB_REQ_SET_INTERFACE:
                    if (pdev->dev_state == USBD_STATE_CONFIGURED)
                    {
                        hhid->AltSetting = (uint8_t)(req->wValue);
                    }
                    else
                    {
                        USBD_CtlError(pdev, req);
                        ret = USBD_FAIL;
                    }
                    break;

                case USB_REQ_CLEAR_FEATURE:
                    break;

                default:
                    USBD_CtlError(pdev, req);
                    ret = USBD_FAIL;
                    break;
            }
            break;

        default:
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
            break;
    }

    return (uint8_t)ret;
}

/**
  * @brief  USBD_CUSTOM_HID_SendReport
  *         Send CUSTOM_HID Report
  * @param  pdev: device instance
  * @param  buff: pointer to report
  * @retval status
  */
uint8_t USBD_CUSTOM_HID_Sensor_SendReport(USBD_HandleTypeDef *pdev,
                                   uint8_t *report, uint16_t len)
{
  USBD_CUSTOM_HID_Sensor_HandleTypeDef *hhid;

  if (pdev->pClassData == NULL)
  {
    return (uint8_t)USBD_FAIL;
  }

  hhid = (USBD_CUSTOM_HID_Sensor_HandleTypeDef*)pdev->pClassData;

  if (pdev->dev_state == USBD_STATE_CONFIGURED)
  {
    if (hhid->state == CUSTOM_HID_IDLE)
    {
      hhid->state = CUSTOM_HID_BUSY;
      (void)USBD_LL_Transmit(pdev, CUSTOM_HID_SENSOR_EPIN_ADDR, report, len);
    }
    else
    {
      return (uint8_t)USBD_BUSY;
    }
  }
  return (uint8_t)USBD_OK;
}

/**
  * @brief  USBD_CUSTOM_HID_GetFSCfgDesc
  *         return FS configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t *USBD_CUSTOM_HID_Sensor_GetFSCfgDesc(uint16_t *length)
{
  *length = (uint16_t)sizeof(USBD_CUSTOM_HID_Sensor_CfgFSDesc);

  return USBD_CUSTOM_HID_Sensor_CfgFSDesc;
}

/**
  * @brief  USBD_CUSTOM_HID_GetHSCfgDesc
  *         return HS configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t *USBD_CUSTOM_HID_Sensor_GetHSCfgDesc(uint16_t *length)
{
  *length = (uint16_t)sizeof(USBD_CUSTOM_HID_Sensor_CfgHSDesc);

  return USBD_CUSTOM_HID_Sensor_CfgHSDesc;
}

/**
  * @brief  USBD_CUSTOM_HID_GetOtherSpeedCfgDesc
  *         return other speed configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t *USBD_CUSTOM_HID_Sensor_GetOtherSpeedCfgDesc(uint16_t *length)
{
  *length = (uint16_t)sizeof(USBD_CUSTOM_HID_Sensor_OtherSpeedCfgDesc);

  return USBD_CUSTOM_HID_Sensor_OtherSpeedCfgDesc;
}

/**
  * @brief  USBD_CUSTOM_HID_DataIn
  *         handle data IN Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t USBD_CUSTOM_HID_Sensor_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  UNUSED(epnum);

  /* Ensure that the FIFO is empty before a new transfer, this condition could
  be caused by  a new transfer before the end of the previous transfer */
  ((USBD_CUSTOM_HID_Sensor_HandleTypeDef *)pdev->pClassData)->state = CUSTOM_HID_IDLE;

  return (uint8_t)USBD_OK;
}

/**
  * @brief  USBD_CUSTOM_HID_DataOut
  *         handle data OUT Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t USBD_CUSTOM_HID_Sensor_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  UNUSED(epnum);
  USBD_CUSTOM_HID_Sensor_HandleTypeDef *hhid;

  if (pdev->pClassData == NULL)
  {
    return (uint8_t)USBD_FAIL;
  }

  hhid = (USBD_CUSTOM_HID_Sensor_HandleTypeDef *)pdev->pClassData;

  /* USB data will be immediately processed, this allow next USB traffic being
  NAKed till the end of the application processing */
  ((USBD_CUSTOM_HID_ItfTypeDef *)pdev->pUserData)->OutEvent(hhid->Report_buf[0],
                                                            hhid->Report_buf[1]);

  return (uint8_t)USBD_OK;
}


/**
  * @brief  USBD_CUSTOM_HID_ReceivePacket
  *         prepare OUT Endpoint for reception
  * @param  pdev: device instance
  * @retval status
  */
uint8_t USBD_CUSTOM_HID_Sensor_ReceivePacket(USBD_HandleTypeDef *pdev)
{
  USBD_CUSTOM_HID_Sensor_HandleTypeDef *hhid;

  if (pdev->pClassData == NULL)
  {
    return (uint8_t)USBD_FAIL;
  }

  hhid = (USBD_CUSTOM_HID_Sensor_HandleTypeDef *)pdev->pClassData;

  /* Resume USB Out process */
  (void)USBD_LL_PrepareReceive(pdev, CUSTOM_HID_SENSOR_EPOUT_ADDR, hhid->Report_buf,
                               USBD_CUSTOMHID_SENSOR_OUTREPORT_BUF_SIZE);

  return (uint8_t)USBD_OK;
}


/**
  * @brief  USBD_CUSTOM_HID_EP0_RxReady
  *         Handles control request data.
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t USBD_CUSTOM_HID_Sensor_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
    // Validate class data
    USBD_CUSTOM_HID_Sensor_HandleTypeDef *hhid = (USBD_CUSTOM_HID_Sensor_HandleTypeDef *)pdev->pClassData;
    if (hhid == NULL)
    {
        return (uint8_t)USBD_FAIL;
    }

    // Extract report type and ID from the request
    uint8_t report_type = pdev->request.wValue >> 8;
    uint8_t report_id = pdev->request.wValue & 0xFF;

    // Check if a report is available
    if (hhid->IsReportAvailable == 1U)
    {
        // Handle feature report types
        if (HID_REPORT_TYPE_FEATURE == report_type)
        {
            switch (report_id)
            {
                case SEIKO_FEATURE_REPORT_ID_01:
                    memcpy(seiko_feature_report_response_01, hhid->Report_buf, pdev->request.wLength);
                    break;

                case SEIKO_FEATURE_REPORT_ID_09:
                    memcpy(seiko_feature_report_response_09, hhid->Report_buf, pdev->request.wLength);
                    break;

                default:
                    // Unhandled report ID
                    break;
            }
        }

        // Notify the application of the Out event
        ((USBD_CUSTOM_HID_ItfTypeDef *)pdev->pUserData)->OutEvent(hhid->Report_buf[0],
                                                                  hhid->Report_buf[1]);

        // Mark the report as processed
        hhid->IsReportAvailable = 0U;
    }

    return (uint8_t)USBD_OK;
}

/**
* @brief  DeviceQualifierDescriptor
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
static uint8_t *USBD_CUSTOM_HID_Sensor_GetDeviceQualifierDesc(uint16_t *length)
{
  *length = (uint16_t)sizeof(USBD_CUSTOM_HID_Sensor_DeviceQualifierDesc);

  return USBD_CUSTOM_HID_Sensor_DeviceQualifierDesc;
}

/**
* @brief  USBD_CUSTOM_HID_RegisterInterface
  * @param  pdev: device instance
  * @param  fops: CUSTOMHID Interface callback
  * @retval status
  */
uint8_t USBD_CUSTOM_HID_Sensor_RegisterInterface(USBD_HandleTypeDef *pdev,
                                          USBD_CUSTOM_HID_ItfTypeDef *fops)
{
  if (fops == NULL)
  {
    return (uint8_t)USBD_FAIL;
  }

  pdev->pUserData = fops;

  return (uint8_t)USBD_OK;
}
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
