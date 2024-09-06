/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_cdc_if.c
  * @version        : v1.0_Cube
  * @brief          : Usb device for Virtual Com Port.
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

#include "usbd_cdc_if_devctlr.h"

/* USER CODE BEGIN INCLUDE */
#include "usbd_composite.h"
#include "usb.h"
#include "main.h"
#include "debug_defs.h"
#include <stdarg.h>
#if ENABLE_DEVICECTL_CDC
/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
#define CDC_DEVCTLR_MAX_RX_MSGS     4
static JISRQueueMessage_t cdcDevCtlrRxMsg[CDC_DEVCTLR_MAX_RX_MSGS];
static int                nCdcDevCtlrRxIdx;
/* USER CODE END PV */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @brief Usb device library.
  * @{
  */

/** @addtogroup USBD_CDC_IF
  * @{
  */

/** @defgroup USBD_CDC_IF_Private_TypesDefinitions USBD_CDC_IF_Private_TypesDefinitions
  * @brief Private types.
  * @{
  */

/* USER CODE BEGIN PRIVATE_TYPES */

/* USER CODE END PRIVATE_TYPES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_Defines USBD_CDC_IF_Private_Defines
  * @brief Private defines.
  * @{
  */

/* USER CODE BEGIN PRIVATE_DEFINES */

/* USER CODE END PRIVATE_DEFINES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_Macros USBD_CDC_IF_Private_Macros
  * @brief Private macros.
  * @{
  */

/* USER CODE BEGIN PRIVATE_MACRO */

/* USER CODE END PRIVATE_MACRO */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_Variables USBD_CDC_IF_Private_Variables
  * @brief Private variables.
  * @{
  */
/* Create buffer for reception and transmission           */
/* It's up to user to redefine and/or remove those define */
/** Received data over USB are stored in this buffer      */
static uint8_t DEVCTLR_UserRxBufferHS[DEVCTLR_APP_RX_DATA_SIZE];

/** Data to send over USB CDC are stored in this buffer   */
static uint8_t DEVCTLR_UserTxBufferHS[DEVCTLR_APP_TX_DATA_SIZE];

/* USER CODE BEGIN PRIVATE_VARIABLES */

/** **/
#if ENABLE_CDC_DEFAULT_ECHOBACK
bool CDC_DEVCTLR_Echo_Ctrl_Flag = true;
#else
bool CDC_DEVCTLR_Echo_Ctrl_Flag = false;
#endif

USBD_CDC_LineCodingTypeDef DEVCTLR_LineCoding =
  {
    115200, /* baud rate*/
    0x00,   /* stop bits-1*/
    0x00,   /* parity - none*/
    0x08    /* nb. of bits 8*/
  };
/* USER CODE END PRIVATE_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Exported_Variables USBD_CDC_IF_Exported_Variables
  * @brief Public variables.
  * @{
  */

extern USBD_HandleTypeDef hUsbDeviceHS;

/* USER CODE BEGIN EXPORTED_VARIABLES */

/* USER CODE END EXPORTED_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_FunctionPrototypes USBD_CDC_IF_Private_FunctionPrototypes
  * @brief Private functions declaration.
  * @{
  */

static int8_t CDC_DEVCTLR_Init_HS(void);
static int8_t CDC_DEVCTLR_DeInit_HS(void);
static int8_t CDC_DEVCTLR_Control_HS(uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDC_DEVCTLR_Receive_HS(uint8_t* pbuf, uint32_t *Len);
static int8_t CDC_DEVCTLR_TransmitCplt_HS(uint8_t *pbuf, uint32_t *Len, uint8_t epnum);

/* USER CODE BEGIN PRIVATE_FUNCTIONS_DECLARATION */
static int8_t CDC_DEVCTLR_EnableTransmit = 0x1;
/* USER CODE END PRIVATE_FUNCTIONS_DECLARATION */

/**
  * @}
  */

USBD_CDC_ItfTypeDef USBD_Interface_fops_DEVCTLR_HS =
{
  CDC_DEVCTLR_Init_HS,
  CDC_DEVCTLR_DeInit_HS,
  CDC_DEVCTLR_Control_HS,
  CDC_DEVCTLR_Receive_HS,
  CDC_DEVCTLR_TransmitCplt_HS
};

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Initializes the CDC media low layer over the USB HS IP
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_DEVCTLR_Init_HS(void)
{
  /* USER CODE BEGIN 8 */
  nCdcDevCtlrRxIdx = 0;
  /* Set Application Buffers */
  USBD_CDC_DEVCTLR_SetTxBuffer(&hUsbDeviceHS, DEVCTLR_UserTxBufferHS, 0);
  USBD_CDC_DEVCTLR_SetRxBuffer(&hUsbDeviceHS, DEVCTLR_UserRxBufferHS);
  return (USBD_OK);
  /* USER CODE END 8 */
}

/**
  * @brief  DeInitializes the CDC media low layer
  * @param  None
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_DEVCTLR_DeInit_HS(void)
{
  /* USER CODE BEGIN 9 */
  return (USBD_OK);
  /* USER CODE END 9 */
}

/**
  * @brief  Manage the CDC class requests
  * @param  cmd: Command code
  * @param  pbuf: Buffer containing command data (request parameters)
  * @param  length: Number of data to be sent (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_DEVCTLR_Control_HS(uint8_t cmd, uint8_t* pbuf, uint16_t length)
{
  /* USER CODE BEGIN 10 */
  switch(cmd)
  {
  case CDC_DEVCTLR_SEND_ENCAPSULATED_COMMAND:

    break;

  case CDC_DEVCTLR_GET_ENCAPSULATED_RESPONSE:

    break;

  case CDC_DEVCTLR_SET_COMM_FEATURE:

    break;

  case CDC_DEVCTLR_GET_COMM_FEATURE:

    break;

  case CDC_DEVCTLR_CLEAR_COMM_FEATURE:

    break;

  /*******************************************************************************/
  /* Line Coding Structure                                                       */
  /*-----------------------------------------------------------------------------*/
  /* Offset | Field       | Size | Value  | Description                          */
  /* 0      | dwDTERate   |   4  | Number |Data terminal rate, in bits per second*/
  /* 4      | bCharFormat |   1  | Number | Stop bits                            */
  /*                                        0 - 1 Stop bit                       */
  /*                                        1 - 1.5 Stop bits                    */
  /*                                        2 - 2 Stop bits                      */
  /* 5      | bParityType |  1   | Number | Parity                               */
  /*                                        0 - None                             */
  /*                                        1 - Odd                              */
  /*                                        2 - Even                             */
  /*                                        3 - Mark                             */
  /*                                        4 - Space                            */
  /* 6      | bDataBits  |   1   | Number Data bits (5, 6, 7, 8 or 16).          */
  /*******************************************************************************/
  case CDC_DEVCTLR_SET_LINE_CODING:

    	/* set the line coding from the host */
        DEVCTLR_LineCoding.bitrate = (uint32_t)(pbuf[0] | (pbuf[1] << 8) |\
        		(pbuf[2] << 16) | (pbuf[3] << 24));
        DEVCTLR_LineCoding.format = pbuf[4];
        DEVCTLR_LineCoding.paritytype = pbuf[5];
        DEVCTLR_LineCoding.datatype = pbuf[6];
        /** TODO: check if we need to do extra config here to make
         *        sure the line coding will be applied correctly
         */
    break;

  case CDC_DEVCTLR_GET_LINE_CODING:

    	/* report the current line coding to the host */
        pbuf[0] = (uint8_t)(DEVCTLR_LineCoding.bitrate);
        pbuf[1] = (uint8_t)(DEVCTLR_LineCoding.bitrate >> 8);
        pbuf[2] = (uint8_t)(DEVCTLR_LineCoding.bitrate >> 16);
        pbuf[3] = (uint8_t)(DEVCTLR_LineCoding.bitrate >> 24);
        pbuf[4] = DEVCTLR_LineCoding.format;
        pbuf[5] = DEVCTLR_LineCoding.paritytype;
        pbuf[6] = DEVCTLR_LineCoding.datatype;
    break;

  case CDC_DEVCTLR_SET_CONTROL_LINE_STATE:

    break;

  case CDC_DEVCTLR_SEND_BREAK:

    break;

  default:
    break;
  }

  return (USBD_OK);
  /* USER CODE END 10 */
}

/**
  * @brief  Data received over USB OUT endpoint are sent over CDC interface
  *         through this function.
  *
  *         @note
  *         This function will issue a NAK packet on any OUT packet received on
  *         USB endpoint until exiting this function. If you exit this function
  *         before transfer is complete on CDC interface (ie. using DMA controller)
  *         it will result in receiving more data while previous ones are still
  *         not sent.
  *
  * @param  Buf: Buffer of data to be received
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_DEVCTLR_Receive_HS(uint8_t* Buf, uint32_t *Len)
{
  /* USER CODE BEGIN 11 */
  // always swap the buffers in ISR
  JISRQueueMessage_t *pMsg = &(cdcDevCtlrRxMsg[nCdcDevCtlrRxIdx]);

  nCdcDevCtlrRxIdx += 1;
  nCdcDevCtlrRxIdx %= CDC_DEVCTLR_MAX_RX_MSGS;

  if(*Len > 0)
  {
      pMsg->type = USB_CDC_DEVCTLR_RX_RECEIVE_MSG;
      pMsg->data.cdcRx.len = *Len;
      memcpy( pMsg->data.cdcRx.buf, Buf, *Len);

      usbSendMessageISR( pMsg);
  }

  // start the next receive process
  USBD_CDC_DEVCTLR_SetRxBuffer(&hUsbDeviceHS, &Buf[0]);
  USBD_CDC_DEVCTLR_ReceivePacket(&hUsbDeviceHS);
  return (USBD_OK);
  /* USER CODE END 11 */
}

/**
  * @brief  Data to send over USB IN endpoint are sent over CDC interface
  *         through this function.
  * @param  Buf: Buffer of data to be sent
  * @param  Len: Number of data to be sent (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL or USBD_BUSY
  */
uint8_t CDC_DEVCTLR_Transmit_HS(uint8_t* Buf, uint16_t Len)
{
  uint8_t result = USBD_OK;
  /* USER CODE BEGIN 12 */
  USBD_CDC_DEVCTLR_HandleTypeDef *hcdc;

  if(!CDC_DEVCTLR_EnableTransmit)
  {
    return (USBD_OK);
  }
  /* NOTE:
   * We need manually switch the USB interface during the Tx data preparation
   * because this process is not part of operations in USBD_COMPOSITE.
   */
  USBD_Composite_Switch_Itf(&hUsbDeviceHS, USBD_CDC_DEVCTLR_INTERFACE);
  hcdc = (USBD_CDC_DEVCTLR_HandleTypeDef*)hUsbDeviceHS.pClassData;
  if (hcdc->TxState != 0){
    return USBD_BUSY;
  }
  USBD_CDC_DEVCTLR_SetTxBuffer(&hUsbDeviceHS, Buf, Len);
  result = USBD_CDC_DEVCTLR_TransmitPacket(&hUsbDeviceHS);
  /* USER CODE END 12 */
  return result;
}

/**
  * @brief  CDC_TransmitCplt_HS
  *         Data transmitted callback
  *
  *         @note
  *         This function is IN transfer complete callback used to inform user that
  *         the submitted Data is successfully sent over USB.
  *
  * @param  Buf: Buffer of data to be received
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_DEVCTLR_TransmitCplt_HS(uint8_t *Buf, uint32_t *Len, uint8_t epnum)
{
  uint8_t result = USBD_OK;
  /* USER CODE BEGIN 14 */
  UNUSED(Buf);
  UNUSED(Len);
  UNUSED(epnum);
  /* USER CODE END 14 */
  return result;
}

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */
void usb_DEVCTLR_printf(const char *format, ...)
{
  #if USE_USB_TX_TASK
    /* Prevent use this, please use usbDebug */

  #else
    va_list args;
    uint32_t length;
#define MAX_RETRY       100
    uint32_t nRetry;

    va_start(args, format);
    length = vsnprintf((char *)DEVCTLR_UserTxBufferFS, DEVCTLR_APP_TX_DATA_SIZE, (char *)format, args);
    va_end(args);

    nRetry = 0;
    while( (USBD_OK !=  CDC_DEVCTLR_Transmit_HS(DEVCTLR_UserTxBufferFS, length)) &&
           (nRetry < MAX_RETRY) )
    {
        HAL_Delay(1);
        nRetry += 1;
    }
  #endif
}

void CDC_DEVCTLR_SetEnableTransmit( uint8_t bEnable)
{
    if(bEnable)
    {
        CDC_DEVCTLR_EnableTransmit = 0x1;
    }
    else
    {
        CDC_DEVCTLR_EnableTransmit = 0x0;
    }
}
#endif
/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */

/**
  * @}
  */

/**
  * @}
  */

