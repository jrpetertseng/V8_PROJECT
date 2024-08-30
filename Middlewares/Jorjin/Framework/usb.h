#ifndef _JORJIN_FRAMEWORK_USB_H_
#define _JORJIN_FRAMEWORK_USB_H_

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "usbd_customhid.h"
#include "usbd_customhid_sensor.h"
#include "config.h"
#include "sensor_hid.h"
#include "usbd_cdc.h"

typedef uint8_t sh2_SensorId_t;

enum MassageType {
    USB_HID_FEATURE_REPORT = 0,
    USB_HID_INPUT_REPORT = 1,
    USB_DEBUG_MSG = 2,
    USB_CDC_TX_COMPLETE_MSG = 3,
    USB_CDC_RX_RECEIVE_MSG = 4,
    USB_CDC_TOF_DATA,

    USB_HID_IMU_FEATURE_REPORT,
    USB_HID_IMU_INPUT_REPORT,
    USB_HID_KEY_FEATURE_REPORT,
    USB_HID_KEY_INPUT_REPORT,

    USB_CDC_DEVCTLR_TX_COMPLETE_MSG,
    USB_CDC_DEVCTLR_RX_RECEIVE_MSG,

};

struct UsbHidFeatureReport {
    sh2_SensorId_t id;
    int interval;
};

struct UsbDebugMsg {
    int len;
    char str[USB_CDC_MAX_STRING_SIZE];
};

struct UsbToFMsg {
    int            len;
    unsigned char  p[USB_TOF_MAX_DATA_SIZE];
  //unsigned char *p;
};

struct UsbHidInputReport {
    int len;
    uint8_t report[8];
};

struct UsbHidImuReport {
    int len;
    uint8_t report[CUSTOM_HID_SENSOR_EPIN_SIZE];
};

struct UsbHidKeyReport {
    int len;
    uint8_t report[CUSTOM_HID_KEYBOARD_EPIN_SIZE];
};

struct UsbCDCRx {
    int len;
    char buf[CDC_DATA_FS_IN_PACKET_SIZE];
};

typedef struct JQueueMessage {
    enum MassageType type;
    union {
        struct UsbHidFeatureReport featureReport;
        struct UsbDebugMsg         debugMsg;
        struct UsbToFMsg           ToFMsg;
//        struct UsbHidInputReport   inputReport;
        struct UsbHidImuReport     imuReport;
        struct UsbHidKeyReport     keyReport;
    } data;
} JQueueMessage_t;

typedef struct JISRQueueMessage {
    enum MassageType type;
    union {
        struct UsbHidFeatureReport featureReport;
        struct UsbCDCRx cdcRx;
        struct UsbHidInputReport   inputReport;
    } data;

} JISRQueueMessage_t;

typedef struct _USB_TX_STAT
{
    int nTxToF;          /* should be nTxCompleteToF+nTxFailToF */
    int nTxCompleteToF;
    int nTxFailToF;    
    
    int nTxDevCtlr;          /* should be nTxCompleteDevCtlr+nTxFailDevCtlr */
    int nTxCompleteDevCtlr;
    int nTxFailDevCtlr;    
    
    int nTxHid;
    int nTxImu;
    int nTxKey;
} USB_TX_STAT_T;

typedef struct JUsb
{
    SemaphoreHandle_t qLock;
    SemaphoreHandle_t txLock;
    QueueHandle_t     queue;
    QueueHandle_t     isrQueue;
    
    int               bInited;
    
    USB_TX_STAT_T     stat;
} JUsb_t;

int  usbInit( void);
void usbDataInit( void);
void usbDebug(char *fmt, ...);
void usbEcho_Tof(char *fmt, ...);
void usbDebugChars(char *buf, int len);
void usbSendMessage(JQueueMessage_t *msg);
void usbSendMessageISR(JISRQueueMessage_t *msg);
void usbLoop( void);

void usbGetStatistics( USB_TX_STAT_T *pStatistics);
int  usbIsInited( void);
void usb_waitUntilInited( void);


uint8_t CDC_CmdBuff_IsUpdated(void);

#endif // _JORJIN_FRAMEWORK_USB_H_

