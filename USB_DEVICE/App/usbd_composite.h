/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USBD_COMPOSITE_H
#define __USBD_COMPOSITE_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbd_customhid.h"
#include "usbd_customhid_imu.h"
#include "usbd_customhid_als.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
#include "usbd_custom_hid_if.h"
#include "usbd_custom_hid_if_imu.h"
#include "usbd_custom_hid_if_als.h"
#include "usbd_cdc_devctlr.h"
#include "usbd_cdc_if_devctlr.h"
#include "debug_defs.h"
#include "usbd_audio.h"
#include "usbd_audio_if.h"

#define WBVAL(x) (x & 0xFF),((x >> 8) & 0xFF)
#define DBVAL(x) (x & 0xFF),((x >> 8) & 0xFF),((x >> 16) & 0xFF),((x >> 24) & 0xFF)


#define USE_USB_UAC_MIC         1

#define USBD_IAD_DESC_SIZE           0x08
#define USBD_IAD_DESCRIPTOR_TYPE     0x0B


#define USBD_AUD_INTERFACE_MIC_CONTROL       0
#define USBD_AUD_INTERFACE_MIC_STREAMING     (USBD_AUD_INTERFACE_MIC_CONTROL+1)

#define USBD_AUD_FIRST_INTERFACE             USBD_AUD_INTERFACE_MIC_CONTROL
#define USBD_AUD_INTERFACE_NUM               2          /* CDC Interface NUM */


#define USBD_CDC_FIRST_INTERFACE     3          /* CDC FirstInterface */
#define USBD_CDC_INTERFACE_NUM       2          /* CDC Interface NUM */
#define USBD_CDC_CMD_INTERFACE       USBD_CDC_FIRST_INTERFACE
#define USBD_CDC_DATA_INTERFACE      (USBD_CDC_CMD_INTERFACE+1)
//#if ENABLE_CDC_CMD_PORT
    #define USBD_CDC_DEVCTLR_FIRST_INTERFACE     5          /* CDC FirstInterface */
    #define USBD_CDC_DEVCTLR_INTERFACE_NUM       2          /* CDC Interface NUM */
    #define USBD_CDC_DEVCTLR_CMD_INTERFACE       5
    #define USBD_CDC_DEVCTLR_DATA_INTERFACE      6
//#endif
// ckhsu
#define USBD_HID_INTERFACE_IMU       2
#define USBD_HID_INTERFACE_KEY       5

#define CDC_INDATA_NUM               (CDC_IN_EP & 0x0F)
#define CDC_OUTDATA_NUM              (CDC_OUT_EP & 0x0F)
#define CDC_OUTCMD_NUM               (CDC_CMD_EP & 0x0F)
#if ENABLE_CDC_CMD_PORT
    #define CDC_DEVCTLR_INDATA_NUM               (CDC_DEVCTLR_IN_EP & 0x0F)
    #define CDC_DEVCTLR_OUTDATA_NUM              (CDC_DEVCTLR_OUT_EP & 0x0F)
    #define CDC_DEVCTLR_OUTCMD_NUM               (CDC_DEVCTLR_CMD_EP & 0x0F)
#endif
#define HID_IMU_INDATA_NUM               (CUSTOM_HID_IMU_EPIN_ADDR & 0x0F)
#define HID_IMU_OUTDATA_NUM              (CUSTOM_HID_IMU_EPOUT_ADDR & 0x0F)

#define HID_KEY_INDATA_NUM               (CUSTOM_HID_KEY_EPIN_ADDR & 0x0F)
#define HID_KEY_OUTDATA_NUM              (CUSTOM_HID_KEY_EPOUT_ADDR & 0x0F)

#define AUD_MIC_INDATA_NUM               (AUDIO_IN_EP & 0x0F)
#define AUD_MIC_OUTDATA_NUM              (AUDIO_OUT_EP & 0x0F)

//#define USBD_COMPOSITE_DESC_SIZE     ((USB_CUSTOM_HID_CONFIG_DESC_SIZ -9 + USB_CDC_CONFIG_DESC_SIZ +8) + (USB_CUSTOM_HID_CONFIG_DESC_SIZ-8-7) + (USB_AUDIO_CONFIG_DESC_SIZ-1) + (USB_CDC_DEVCTLR_CONFIG_DESC_SIZ-2))
#if ENABLE_CDC_CMD_PORT
    #define USBD_COMPOSITE_DESC_SIZE     306 //208+66+32  274
#else
    #define USBD_COMPOSITE_DESC_SIZE     240
#endif
extern USBD_ClassTypeDef    USBD_COMPOSITE;

typedef enum {
	USBD_AUD_MIC_INTERFACE,
	USBD_CDC_INTERFACE,

	USBD_CUSTOMHID_IMU_INTERFACE,

	USBD_CDC_DEVCTLR_INTERFACE,
    USBD_CUSTOMHID_KEY_INTERFACE,
} USBD_COMPOSITE_ItfTypeDef;

void USBD_Composite_Switch_Itf(USBD_HandleTypeDef *pdev, USBD_COMPOSITE_ItfTypeDef interface);
USBD_COMPOSITE_ItfTypeDef USBD_Composite_Get_Current_Itf(USBD_HandleTypeDef *pdev);

#ifdef __cplusplus
}
#endif

#endif  /* __USBD_COMPOSITE_H */
