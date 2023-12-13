#ifndef _JORJIN_FRAMEWORK_CONFIG_H_
#define _JORJIN_FRAMEWORK_CONFIG_H_

#include "main.h"
#include "usbd_cdc.h"
#include "debug_defs.h"

// The below USB_QUEUE_SIZE/USB_RX_QUEUE_SIZE is related to the HEAP size allocated in freeRTOSConfig.h
#define USB_QUEUE_SIZE                  32
#define USB_RX_QUEUE_SIZE               USB_QUEUE_SIZE

#define USB_CDC_MAX_STRING_SIZE         64
#define USB_CDC_COMMAND_MAX_SIZE        32
#define USB_COMMAND_BUF_MAX_SIZE        2048
#define USB_COMMAND_CATEGORY_MAX_SIZE   16
#define USB_COMMAND_NAME_MAX_SIZE       16

#define USB_TOF_MAX_DATA_SIZE           640

#define COMMAND_ARG_MAX_SIZE            16

#define SENSOR_DEFAULT_ON               1
#define SENSOR_DYNAMIC_INTERVAL         0

#define USB_CDC_ONLY                    0

  #if ENABLE_CDC_DEVCTLR_LOAD_PRINT
    //#error "ENABLE_CDC_DEVCTLR_LOAD_PRINT defined"
    #undef  USB_CDC_MAX_STRING_SIZE
    #define USB_CDC_MAX_STRING_SIZE         512
  #endif

#endif // _JORJIN_FRAMEWORK_CONFIG_H_
