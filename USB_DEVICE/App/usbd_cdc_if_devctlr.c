/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_cdc_if_devctlr.c
  * @version        : v1.0_Cube
  * @brief          : Usb device for Virtual Com Port.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
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
#include "usbd_cdc_if_devctlr.h"

/* USER CODE BEGIN INCLUDE */
#include "usbd_composite.h"
#include "usb.h"

//ckhsu test
#include "main.h"
#include "debug_defs.h"
#include <stdarg.h>

  #if ENABLE_CDC_DEBUG_HANDLER
static void cdc_devctlr_cmd_handler(uint8_t* Buf, uint32_t len);
//#error
  #endif

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
/* Define size for the receive and transmit buffer over CDC */
/* It's up to user to redefine and/or remove those define */

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
static uint8_t DEVCTLR_UserRxBufferFS[DEVCTLR_APP_RX_DATA_SIZE];

/** Data to send over USB CDC are stored in this buffer   */
static uint8_t DEVCTLR_UserTxBufferFS[DEVCTLR_APP_TX_DATA_SIZE];

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
  * @brief  Initializes the CDC media low layer over the FS USB IP
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_DEVCTLR_Init_HS(void)
{
  /* USER CODE BEGIN 3 */
  nCdcDevCtlrRxIdx = 0;
  /* Set Application Buffers */
  USBD_CDC_DEVCTLR_SetTxBuffer(&hUsbDeviceHS, DEVCTLR_UserTxBufferFS, 0);
  USBD_CDC_DEVCTLR_SetRxBuffer(&hUsbDeviceHS, DEVCTLR_UserRxBufferFS);
  return (USBD_OK);
  /* USER CODE END 3 */
}

/**
  * @brief  DeInitializes the CDC media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_DEVCTLR_DeInit_HS(void)
{
  /* USER CODE BEGIN 4 */
  return (USBD_OK);
  /* USER CODE END 4 */
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
  /* USER CODE BEGIN 5 */
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
  /* USER CODE END 5 */
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
  /* USER CODE BEGIN 6 */
  // always swap the buffers in ISR
//  CDC_DEVCTLR_GetLines_ISR(&Buf, Len);
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
  //USBD_CDC_DEVCTLR_SetRxBuffer(&hUsbDeviceHS, &Buf[*Len]);
  USBD_CDC_DEVCTLR_ReceivePacket(&hUsbDeviceHS);

  #if ENABLE_CDC_DEBUG_HANDLER
  /*************************************************************/
  /* ckhsu:                                                    */
  /* User should use CDC_CmdBuff_IsUpdated below instead using */
  /* this in the interrupt,however we just debug, not a big    */
  /* deal.                                                     */
  /*************************************************************/
  cdc_devctlr_cmd_handler( Buf, *Len);
  #endif

  return (USBD_OK);
  /* USER CODE END 6 */
}

/**
  * @brief  CDC_DEVCTLR_Transmit_HS
  *         Data to send over USB IN endpoint are sent over CDC interface
  *         through this function.
  *         @note
  *
  *
  * @param  Buf: Buffer of data to be sent
  * @param  Len: Number of data to be sent (in bytes)
  * @retval USBD_OK if all operations are OK else USBD_FAIL or USBD_BUSY
  */
uint8_t CDC_DEVCTLR_Transmit_HS(uint8_t* Buf, uint16_t Len)
{
  uint8_t result = USBD_OK;
  /* USER CODE BEGIN 7 */
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

  //if (!CDC_Echo_Ctrl_Flag || !RxBuffIsSwapped) {
    USBD_CDC_DEVCTLR_SetTxBuffer(&hUsbDeviceHS, Buf, Len);
    result = USBD_CDC_DEVCTLR_TransmitPacket(&hUsbDeviceHS);
  /*} else {
    // add extra newline characters if the Rx buffer has been swapped
    uint16_t new_len = Len+2;
    uint8_t new_buf[new_len];
    uint16_t offset = 0;

    new_buf[offset++] = 0x0D;
    new_buf[offset++] = 0x0A;
    memcpy(new_buf + (offset % new_len), Buf, Len);
    USBD_CDC_SetTxBuffer(&hUsbDeviceHS, new_buf, new_len);
    result = USBD_CDC_TransmitPacket(&hUsbDeviceHS);
    RxBuffIsSwapped = false;
  }*/
  /* USER CODE END 7 */
  return result;
}

/**
  * @brief  CDC_DEVCTLR_TransmitCplt_HS
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
  /* USER CODE BEGIN 13 */
//  static JISRQueueMessage_t cdcDevCtlrTxCpltMsg;
  UNUSED(Buf);
  UNUSED(Len);
  UNUSED(epnum);
  /* USER CODE END 13 */
  return result;
}

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */
/*uint8_t CDC_DEVCTLR_CmdBuff_IsUpdated(void) { return DEVCTLR_CmdBuffIsUpdated; }

void CDC_DEVCTLR_Get_CmdBuff(uint8_t** pbuf, uint32_t *len)
{
	*pbuf = (uint8_t*)DEVCTLR_CmdBuffer;
	*len = DEVCTLR_CmdBuffLength;
}

void CDC_DEVCTLR_Update_CmdBuff(void) { DEVCTLR_RxBuffIsSwapped = DEVCTLR_CmdBuffIsUpdated = true; }

void CDC_DEVCTLR_Clear_CmdBuff(void) {
	DEVCTLR_CmdBuffIsUpdated = (DEVCTLR_PendingBuffLength > 0 &&
			CDC_DEVCTLR_GetLines((uint8_t**)&DEVCTLR_PendingBuffer, (uint32_t*)&DEVCTLR_PendingBuffLength, false));
}

static void CDC_DEVCTLR_GetLines_ISR(uint8_t** pbuf, uint32_t *buf_len)
{
	if (*buf_len) {
		if (CDC_DEVCTLR_CmdBuff_IsUpdated()) {
			DEVCTLR_RxBuffLength += *buf_len;
			// NOTE: PendingBuffer will be overwritten in CDC_Clear_CmdBuff()
			DEVCTLR_PendingBuffLength += *buf_len;
		} else
			CDC_DEVCTLR_GetLines(pbuf, buf_len, true);
	}
}

static bool CDC_DEVCTLR_GetLines(uint8_t** pbuf, uint32_t *buf_len, bool do_swap)
{
	uint32_t stashed_rx_len = DEVCTLR_RxBuffLength; // keep the entering rx buffer length
	uint32_t checked_len = 0;
	uint8_t *checked_buf = *pbuf;
	static uint32_t DEVCTLR_buf_offset = 0;

	while (*buf_len > checked_len) {
		//
		 // Windows sends 0x0D 0x0A while Linux sends 0x0A for new line
		 // Besides, Putty by default sends only \r
		 //   \r   - return
		 //   \n   - newline
		 //
		if (*checked_buf == '\n' || *checked_buf++ == '\r') {
			uint32_t residual_len = *pbuf + *buf_len - checked_buf;
			uint8_t *next_rx_buf = (uint8_t*)DEVCTLR_CmdBuffer - DEVCTLR_buf_offset;
			// find the next non-new-line character
			while ((*checked_buf == '\n' || *checked_buf == '\r') && residual_len > 0) {
				residual_len--;
				checked_buf++;
			}

			if (do_swap) {
				// calculate the buffer offset for the next non-swapped call
				DEVCTLR_buf_offset = DEVCTLR_PendingBuffer - DEVCTLR_RxBuffer;
				// point CmdBuffer to the unprocessed data (e.g., PendingBuffer)
				DEVCTLR_CmdBuffer = DEVCTLR_PendingBuffer;
				// calculate the total length
				DEVCTLR_CmdBuffLength = *pbuf - DEVCTLR_CmdBuffer + checked_len;
				// prepare the next Rx buffer
				if (residual_len) memcpy(next_rx_buf, checked_buf, residual_len);
				// update the Rx buffer and pointers
				DEVCTLR_PendingBuffer = DEVCTLR_RxBuffer = *pbuf = next_rx_buf;
				// update the buffer length
				DEVCTLR_PendingBuffLength = DEVCTLR_RxBuffLength = *buf_len = residual_len;
				// reset the echoed buffer length
				DEVCTLR_EchoedRxBuffLength = 0;
			} else {
				// reset the buffer offset
				DEVCTLR_buf_offset = 0;
				// calculate the total length
				DEVCTLR_CmdBuffLength = checked_len;
				// prepare CmdBuffer
				if (DEVCTLR_CmdBuffLength) memcpy((uint8_t*)DEVCTLR_CmdBuffer, *pbuf, DEVCTLR_CmdBuffLength);
				// prepare the next DEVCTLR_PendingBuffer (e.g., pbuf)
				*pbuf = checked_buf;
				// update the DEVCTLR_PendingBuffer length (e.g., buf_len)
				*buf_len = residual_len
						// adjust the length with the current rx buffer length
						+ (DEVCTLR_RxBuffLength - stashed_rx_len);
			}
			// add an ending character in CmdBuffer
			DEVCTLR_CmdBuffer[DEVCTLR_CmdBuffLength] = 0;
			// set the updated flag
			CDC_DEVCTLR_Update_CmdBuff();
			return true;
		}
		checked_len++;
	}
	if (do_swap) {
		DEVCTLR_RxBuffLength += *buf_len;
		DEVCTLR_PendingBuffLength += *buf_len;
	}
	return false;
}

void CDC_DEVCTLR_EchoBack(void)
{
	// use local variables to keep it thread safe
	uint32_t target_len = DEVCTLR_RxBuffLength;
	uint8_t* buf = (uint8_t*)DEVCTLR_RxBuffer;
	uint32_t start_len = DEVCTLR_EchoedRxBuffLength;

	bool line_mode;
	uint32_t skip_len = 0;

	// sanity check
	if (start_len >= target_len) return;

	// search for new line characters if the command buffer has been updated
	if ((line_mode = CDC_DEVCTLR_CmdBuff_IsUpdated())) {
		uint32_t idx;
		for (idx=start_len; idx < target_len; idx++) {
			if (*(buf+idx) == '\n' || *(buf+idx) == '\r') {
				// try skipping the new line characters
				for (skip_len=idx; skip_len < target_len &&
						(*(buf+skip_len) == '\n' || *(buf+skip_len) == '\r'); skip_len++);
				break;
			}
		}
		// sanity check
		if (idx == target_len) return;

		target_len = idx;
		skip_len -= target_len;
	}

	// send out the echo back characters
	while (start_len == DEVCTLR_EchoedRxBuffLength && buf == DEVCTLR_RxBuffer &&
			CDC_DEVCTLR_Transmit_HS(buf+start_len, target_len-start_len) != HAL_OK);

	// update the echoed buffer length only when it hasn't been changed
	if (start_len == DEVCTLR_EchoedRxBuffLength)
		DEVCTLR_EchoedRxBuffLength = target_len + skip_len;
}*/

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

  #if ENABLE_CDC_DEBUG_HANDLER

void cdc_devctlr_cmd_handler(uint8_t* Buf, uint32_t len)
{
  #if ENABLE_CDC_ENGINEERING_TEST
    usb_DEVCTLR_printf("STM32 CDC_DEVCTLR recv: [%s]\n", Buf);
  #else
	usb_DEVCTLR_printf("STM32 CDC_DEVCTLR recv: [%s]\n", Buf);
  #endif
}
  #endif

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
/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
