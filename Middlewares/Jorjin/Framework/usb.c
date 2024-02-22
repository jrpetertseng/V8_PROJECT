#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "usb.h"
#include "cmd.h"
#include "config.h"
#include "usbd_cdc_if.h"
#include "usbd_cdc_if_devctlr.h"
#include "usbd_custom_hid_if_imu.h"
#include "usbd_custom_hid_if_als.h"
#include "debug_defs.h"

#include "cmsis_os.h"

#include "portmacro.h"
//#include "bno080/bno080.h"

//#include "properties.h"

static JUsb_t gCtx;

extern int8_t USBD_CUSTOM_HID_SendReport_FS(uint8_t *report, uint16_t len);

static void UsbEscapeISRTask(void * argument);

extern uint32_t                 nExecs_IsrToF;

#if (0x20001U == osCMSIS)

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

typedef StaticTask_t osStaticThreadDef_t;

static uint32_t usbEscapeISRTaskBuffer[ 512 ]; //512
osStaticThreadDef_t usbEscapeISRTaskControlBlock;
const osThreadAttr_t usbEscapeISRTask_attributes = {
  .name = "usbEscapeISRTask",
  .cb_mem = &usbEscapeISRTaskControlBlock,
  .cb_size = sizeof(usbEscapeISRTaskControlBlock),
  .stack_mem = &usbEscapeISRTaskBuffer[0],
  .stack_size = sizeof(usbEscapeISRTaskBuffer),
  .priority = (osPriority_t) osPriorityHigh,
};
static osThreadId_t usbEscapeISRTaskHandle;

#elif (0x10002 == osCMSIS)
static osThreadId usbEscapeISRTaskHandle;
#endif

int usbInit() {
	gCtx.bInited = 0x0;
	
    gCtx.qLock = xSemaphoreCreateBinary();
    if (gCtx.qLock == NULL) return -1;

    // The state of Semaphore was empty, so it
    // needed to call xSemaphoreGive first, before
    // call xSemaphoreTake.
    xSemaphoreGive(gCtx.qLock);

    gCtx.txLock = xSemaphoreCreateBinary();
    if (gCtx.txLock == NULL) return -1;
    xSemaphoreGive(gCtx.txLock);

    gCtx.queue = xQueueCreate(USB_QUEUE_SIZE, sizeof(JQueueMessage_t));
    if (gCtx.queue == NULL) return -1;

    gCtx.isrQueue = xQueueCreate(USB_RX_QUEUE_SIZE, sizeof(JISRQueueMessage_t));
    if (gCtx.isrQueue == NULL) return -1;

  #if (0x20001U == osCMSIS)
    //#error "freeRTOS v2 used"
    /* This is a v2 FreeRTOS we supported in STM32F723VE/ */
    usbEscapeISRTaskHandle = osThreadNew( UsbEscapeISRTask, NULL, &usbEscapeISRTask_attributes);
  #elif (0x10002 == osCMSIS)
    #error "freeRTOS v1 used"
    // This is a CMSIS v1 API, now we use v2
    osThreadDef(usbEscapeISRTask, UsbEscapeISRTask, osPriorityNormal, 0, 1024);
    usbEscapeISRTaskHandle = osThreadCreate(osThread(usbEscapeISRTask), NULL);
  #else
    #error "Unknown CMSIS version!!!"
  #endif


    return 0;
}

static inline void usbTxBlock( void)
{
  #if ENABLE_CDC_DEVCTLR_LOAD_PRINT
    xSemaphoreTake(gCtx.txLock, 100 * portTICK_PERIOD_MS);
  #else
    xSemaphoreTake(gCtx.txLock, portMAX_DELAY);
  #endif
}

static inline void usbTxUnblock( void)
{
    xSemaphoreGive(gCtx.txLock);
}

static inline void usbToF_TxBlock( void)
{
    usbTxBlock();
}

static inline void usbImu_TxBlock( void)
{
    usbTxBlock();
}

static inline void usbAls_TxBlock( void)
{
    usbTxBlock();
}

static inline void usbTx_inc_hid_report( void)
{
    gCtx.stat.nTxHid += 1;
}

static inline void usbTx_inc_imu_report( void)
{
    gCtx.stat.nTxImu += 1;
}

static inline void usbTx_inc_key_report( void)
{
    gCtx.stat.nTxKey += 1;
}

static inline void usbTx_inc_tof( void)
{
    gCtx.stat.nTxToF += 1;
}

static inline void usbTx_inc_devctlr( void)
{
    gCtx.stat.nTxDevCtlr += 1;
}

static inline void usbTx_inc_tof_error( void)
{
    gCtx.stat.nTxFailToF += 1;
}

static inline void usbTx_inc_devctlr_error( void)
{
    gCtx.stat.nTxFailDevCtlr += 1;
}

static inline void usbTx_inc_tof_complete( void)
{
    gCtx.stat.nTxCompleteToF += 1;
}

static inline void usbTx_inc_devctlr_complete( void)
{
    gCtx.stat.nTxCompleteDevCtlr += 1;
}

void usbDebug(char *fmt, ...) {
    JQueueMessage_t msg;
    int ret = 0;

    if (gCtx.qLock == NULL ||
        gCtx.queue == NULL) return;

    va_list argptr;
    va_start(argptr,fmt);
    ret = vsnprintf(msg.data.debugMsg.str, USB_CDC_MAX_STRING_SIZE, fmt, argptr);
    va_end(argptr);

    if (ret < 0) return;
    if (ret >= USB_CDC_MAX_STRING_SIZE) return;

    msg.type = USB_DEBUG_MSG;
    msg.data.debugMsg.len = ret;

    usbSendMessage(&msg);
}

void usbDebugChars(char *buf, int len) {
    JQueueMessage_t msg;

    if (gCtx.qLock == NULL ||
        gCtx.queue == NULL) return;

    msg.type = USB_DEBUG_MSG;
    memcpy(msg.data.debugMsg.str, buf, len);
    msg.data.debugMsg.len = len;

    usbSendMessage(&msg);
}

static JQueueMessage_t msgEcho_ToF;
static JQueueMessage_t msgEchoChars_ToF;

void usbEcho_Tof(char *fmt, ...)
{
    int ret = 0;

    if (gCtx.qLock == NULL ||
        gCtx.queue == NULL) return;

    va_list argptr;
    va_start(argptr,fmt);
    ret = vsnprintf( (char *)msgEcho_ToF.data.ToFMsg.p, USB_CDC_MAX_STRING_SIZE, fmt, argptr);
    va_end(argptr);

    if (ret < 0) return;
    if (ret >= USB_CDC_MAX_STRING_SIZE) return;

    msgEcho_ToF.type = USB_CDC_TOF_DATA;
    msgEcho_ToF.data.ToFMsg.len = ret;

    usbSendMessage( &msgEcho_ToF);
}

static void usbEchoChars_ToF( char *buf, int len)
{
    if (gCtx.qLock == NULL ||
        gCtx.queue == NULL) return;

    msgEchoChars_ToF.type = USB_CDC_TOF_DATA;
    memcpy( msgEchoChars_ToF.data.ToFMsg.p, buf, len);
    msgEchoChars_ToF.data.ToFMsg.len = len;

    usbSendMessage( &msgEchoChars_ToF);
}

void usbSendMessage(JQueueMessage_t *msg) {
    BaseType_t inISR;

    if (gCtx.queue == NULL) return;

    inISR = xPortIsInsideInterrupt();
    if (inISR) return;
    xSemaphoreTake(gCtx.qLock, portMAX_DELAY);
    xQueueSend(gCtx.queue, msg, 0);
    xSemaphoreGive(gCtx.qLock);
}

void usbSendMessageISR(JISRQueueMessage_t *msg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t inISR;

    if (gCtx.isrQueue == NULL) return;

    inISR = xPortIsInsideInterrupt();
    if (!inISR) return;

    xQueueSendFromISR(gCtx.isrQueue, msg, &xHigherPriorityTaskWoken);
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

void usbLoop() {
    static JQueueMessage_t msg;
    uint8_t         ret;
#if ENABLE_STACK_CHECK
    UBaseType_t uxHighWaterMark;
#endif
    if (gCtx.queue == NULL) return;
    
    gCtx.bInited = 0x1;
    
    while (1) {
        while (xQueueReceive(gCtx.queue, &msg, portMAX_DELAY) == pdPASS) {
            switch (msg.type) {
            case USB_HID_INPUT_REPORT:
                break;
            case USB_HID_IMU_INPUT_REPORT:
//                nIMUHIDUsbOuts += 1;
                nIMUHIDUsbOuts = 0;
                usbImu_TxBlock();
                usbTx_inc_imu_report();
                ret = USBD_CUSTOM_HID_IMU_SendReport_HS(msg.data.imuReport.report, msg.data.imuReport.len);
                if(USBD_OK != ret)
                {
                    /* Fail, release the lock. */
                    usbTxUnblock();
                }
                /* Success, release the lock. */
                usbTxUnblock();
                break;
           case USB_HID_KEY_INPUT_REPORT:
               usbAls_TxBlock();
               usbTx_inc_key_report();
               //ret = USBD_OK;
               ret = USBD_CUSTOM_HID_KEY_SendReport_FS(msg.data.keyReport.report, msg.data.keyReport.len);
               if(USBD_OK != ret)
               {
                   /* Fail, release the lock. */
                   //usbTxUnblock();
               }
               /* Success, release the lock. */
				usbTxUnblock();
               break;
            case USB_CDC_TOF_DATA:
                usbToF_TxBlock();
//                nTofGpioInts_1 += 1;
//                usbTx_inc_tof();
                ret = CDC_Transmit_FS((uint8_t *)msg.data.ToFMsg.p, msg.data.ToFMsg.len);
                if(USBD_OK != ret)
                {
                    /* Fail, release the lock. */
                    usbTx_inc_tof_error();
//                    usbTxUnblock();
                }
                usbTxUnblock();
                break;
            case USB_DEBUG_MSG:
                usbToF_TxBlock();
                usbTx_inc_devctlr();
//                ret = CDC_Transmit_FS((uint8_t *)msg.data.debugMsg.str, msg.data.debugMsg.len);
                ret = CDC_DEVCTLR_Transmit_FS((uint8_t *)msg.data.debugMsg.str, msg.data.debugMsg.len);
                if(USBD_OK != ret)
                {
                    /* Fail, release the lock. */
                    usbTx_inc_devctlr_error();
//                    usbTxUnblock();
                }
#if ENABLE_STACK_CHECK
//                osDelay(1000);
#endif
                usbTxUnblock();
                break;
            default:
                break;
            }
#if ENABLE_STACK_CHECK
            //test: check high water
            if(nExecs_IsrToF%100==0 && nExecs_IsrToF!=0)
            {
                uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
                usbDebug("USB_task free stack：%lu\n", uxHighWaterMark);
            }
#endif

        }
    }
}

static void UsbEscapeISRTask(void * argument)
{
    JISRQueueMessage_t msg;
#if SENSOR_DYNAMIC_INTERVAL
    sh2_SensorId_t id = 0;
#endif
    int ret = -1;

	usb_waitUntilInited();

    while (1) {
        while (xQueueReceive(gCtx.isrQueue, &msg, portMAX_DELAY) == pdPASS) {
            switch (msg.type) {
            case USB_HID_FEATURE_REPORT:
#if SENSOR_DYNAMIC_INTERVAL
                switch (msg.data.featureReport.id) {
                case REPORT_ID_ACCEL3_FEATURE:
                    id = SH2_ACCELEROMETER;
                    break;
                }
                if (id != 0) {
                    configSensor(msg.data.featureReport.id,
                                         msg.data.featureReport.interval);
                }
#endif
                break;
            case USB_CDC_TX_COMPLETE_MSG:
                usbTx_inc_tof_complete();
                usbTxUnblock();
                break;
            case USB_CDC_RX_RECEIVE_MSG:
                ret = copyToBuffer_ToF( msg.data.cdcRx.buf, msg.data.cdcRx.len);
                if(CDC_Echo_Ctrl_Flag)
                {
                    if(msg.data.cdcRx.buf[0] == '\r')
                    {
                        // press Enter, it only send '\r'
                        usbEcho_Tof("\r\n");
                    }
                    else
                    {
                        usbEchoChars_ToF( msg.data.cdcRx.buf, msg.data.cdcRx.len);
                    }
                }

                if(ret < 0)
                {
                    usbDebug("Drop ToF command\r\n");
                    clearBuffer_ToF();
                    continue;
                }

                /* Send to ToF command task. and reset it. */
                processCommands_ToF();
                break;
            case USB_CDC_DEVCTLR_TX_COMPLETE_MSG:
                usbTx_inc_devctlr_complete();
                usbTxUnblock();
                break;
            case USB_CDC_DEVCTLR_RX_RECEIVE_MSG:
                ret = copyToBuffer_devCtlr( msg.data.cdcRx.buf, msg.data.cdcRx.len);

                if (CDC_DEVCTLR_Echo_Ctrl_Flag) {
                    if (msg.data.cdcRx.buf[0] == '\r') {
                        // press Enter, it only send '\r'
                        usbDebug("\r\n");
                    }
                    else {
                        usbDebugChars(msg.data.cdcRx.buf, msg.data.cdcRx.len);
                    }
                }

                if (ret < 0) {
                    usbDebug("Exceed maximum buffer size: %d\r\n", USB_COMMAND_BUF_MAX_SIZE);
                    //Drop all residual data
                    clearBuffer_devCtlr();
                    continue;
                }
                processCommands_devCtlr();
                break;
            default:
                break;
            }
        }
    }
}

void usbGetStatistics( USB_TX_STAT_T *pStatistics)
{
    if(pStatistics)
    {
        memcpy( pStatistics, &gCtx.stat, sizeof(USB_TX_STAT_T));
    }
}

int  usbIsInited( void)
{
    return gCtx.bInited;
}

void usb_waitUntilInited( void)
{
    do
    {
        if(usbIsInited())
        {
            break;
        }

        osDelay(100);
    } while(1);
}
