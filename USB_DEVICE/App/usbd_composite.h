/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USBD_COMPOSITE_H
#define __USBD_COMPOSITE_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "debug_defs.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"

#include "usbd_customhid.h"
#include "usbd_custom_hid_if.h"
#include "usbd_customhid_sensor.h"
#include "usbd_custom_hid_sensor_if.h"

#define WBVAL(x) (x & 0xFF),((x >> 8) & 0xFF)
#define DBVAL(x) (x & 0xFF),((x >> 8) & 0xFF),((x >> 16) & 0xFF),((x >> 24) & 0xFF)

#define USBD_IAD_DESC_SIZE           0x08
#define USBD_IAD_DESCRIPTOR_TYPE     0x0B

/* CDC Interface Definitions */
#define USBD_CDC_FIRST_INTERFACE            1  /* CDC First Interface */
#define USBD_CDC_INTERFACE_COUNT            2  /* Number of CDC Interfaces */
#define USBD_CDC_CMD_INTERFACE              (USBD_CDC_FIRST_INTERFACE)
#define USBD_CDC_DATA_INTERFACE             (USBD_CDC_CMD_INTERFACE + 1)

#if ENABLE_DEVICECTL_CDC
#include "usbd_cdc_devctlr.h"
#include "usbd_cdc_if_devctlr.h"
/* CDC Device Controller Interface Definitions */
#define USBD_CDC_DEVCTLR_FIRST_INTERFACE    5  /* CDC DevCtlr First Interface */
#define USBD_CDC_DEVCTLR_INTERFACE_COUNT    2  /* Number of CDC DevCtlr Interfaces */
#define USBD_CDC_DEVCTLR_CMD_INTERFACE      (USBD_CDC_DEVCTLR_FIRST_INTERFACE)
#define USBD_CDC_DEVCTLR_DATA_INTERFACE     (USBD_CDC_DEVCTLR_CMD_INTERFACE + 1)
#define CDC_DEVCTLR_INDATA_NUM              (CDC_DEVCTLR_IN_EP & 0x0F)
#define CDC_DEVCTLR_OUTDATA_NUM             (CDC_DEVCTLR_OUT_EP & 0x0F)
#define CDC_DEVCTLR_OUTCMD_NUM              (CDC_DEVCTLR_CMD_EP & 0x0F)
#endif

/* HID Interface Definitions */
#define USBD_HID_INTERFACE_SENSOR           3  /* HID Sensor Interface */
#define USBD_HID_INTERFACE_KEYBOARD         4  /* HID Keyboard Interface */

/* Endpoint Number Definitions */
#define CDC_INDATA_NUM                      (CDC_IN_EP & 0x0F)
#define CDC_OUTDATA_NUM                     (CDC_OUT_EP & 0x0F)
#define CDC_OUTCMD_NUM                      (CDC_CMD_EP & 0x0F)


#define HID_SENSOR_INDATA_NUM               (CUSTOM_HID_SENSOR_EPIN_ADDR & 0x0F)
#define HID_SENSOR_OUTDATA_NUM              (CUSTOM_HID_SENSOR_EPOUT_ADDR & 0x0F)

#define HID_KEYBOARD_INDATA_NUM             (CUSTOM_HID_KEYBOARD_EPIN_ADDR & 0x0F)
#define HID_KEYBOARD_OUTDATA_NUM            (CUSTOM_HID_KEYBOARD_EPOUT_ADDR & 0x0F)

#if ENABLE_DEVICECTL_CDC
#define USBD_COMPOSITE_DESC_SIZE 		198
#else
#define USBD_COMPOSITE_DESC_SIZE       132
#endif

extern USBD_ClassTypeDef USBD_COMPOSITE;

typedef enum {
	USBD_CDC_INTERFACE,
	USBD_CUSTOMHID_SENSOR_INTERFACE,
    USBD_CUSTOMHID_KEYBOARD_INTERFACE,
#if ENABLE_DEVICECTL_CDC
    USBD_CDC_DEVCTLR_INTERFACE,
#endif
} USBD_COMPOSITE_ItfTypeDef;

void USBD_Composite_Switch_Itf(USBD_HandleTypeDef *pdev, USBD_COMPOSITE_ItfTypeDef interface);
USBD_COMPOSITE_ItfTypeDef USBD_Composite_Get_Current_Itf(USBD_HandleTypeDef *pdev);

#ifdef __cplusplus
}
#endif

#endif  /* __USBD_COMPOSITE_H */
