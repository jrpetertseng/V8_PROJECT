#include "usbd_composite.h"
#include "usbd_cdc.h"
#include "usbd_customhid.h"
#include "usbd_customhid_imu.h"
#include "usbd_customhid_als.h"
#include "usbd_cdc_devctlr.h"
#include "usbd_audio_if.h"
#include "debug_defs.h"
#include "usbd_audio.h"


static USBD_CDC_HandleTypeDef            *pCDCData_Tof;
//static USBD_CUSTOM_HID_HandleTypeDef     *pHIDData;
static USBD_CUSTOM_HID_IMU_HandleTypeDef *pHIDData_IMU;
//static USBD_CUSTOM_HID_ALS_HandleTypeDef *pHIDData_ALS;
static USBD_CDC_DEVCTLR_HandleTypeDef    *pCDCData_Devctlr;

static USBD_AUDIO_HandleTypeDef          *pAUDData_MIC;
#define AUDIO_SAMPLE_FREQ(frq)         (uint8_t)(frq), (uint8_t)((frq >> 8)), (uint8_t)((frq >> 16))

#define AUDIO_PACKET_SZE(frq)          (uint8_t)(((frq * 2U * 2U)/1000U) & 0xFFU), \
                                   (uint8_t)((((frq * 2U * 2U)/1000U) >> 8) & 0xFFU)
#if UAC_USE_PCM
#define MIC_PACKET_SZE(frq)            (uint8_t)(((frq * 1U * 2U)/(1000U/AUDIO_HS_BINTERVAL)) & 0xFFU), \
                                   (uint8_t)((((frq * 1U * 2U)/(1000U/AUDIO_HS_BINTERVAL)) >> 8) & 0xFFU)
#else
#define MIC_PACKET_SZE(frq)            (uint8_t)(((frq * 1U * 1U)/(1000U/AUDIO_HS_BINTERVAL)) & 0xFFU), \
                                   (uint8_t)((((frq * 1U * 1U)/(1000U/AUDIO_HS_BINTERVAL)) >> 8) & 0xFFU)
#endif

static USBD_CUSTOM_HID_KEY_HandleTypeDef *pHIDData_KEY;

static uint8_t  USBD_Composite_Init (USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t  USBD_Composite_DeInit (USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t  USBD_Composite_EP0_RxReady(USBD_HandleTypeDef *pdev);
static uint8_t  USBD_Composite_Setup (USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t  USBD_Composite_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  USBD_Composite_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  *USBD_Composite_GetFSCfgDesc (uint16_t *length);
static uint8_t  *USBD_Composite_GetDeviceQualifierDescriptor (uint16_t *length);

USBD_ClassTypeDef  USBD_COMPOSITE =
{
  USBD_Composite_Init,
  USBD_Composite_DeInit,
  USBD_Composite_Setup,
  NULL, /*EP0_TxSent*/
  USBD_Composite_EP0_RxReady,
  USBD_Composite_DataIn,
  USBD_Composite_DataOut,
  NULL,
  NULL,
  NULL,
  USBD_Composite_GetFSCfgDesc, /*Get HS config descriptor*/
  USBD_Composite_GetFSCfgDesc,
  USBD_Composite_GetFSCfgDesc, /*Get other speed config descriptor*/
  USBD_Composite_GetDeviceQualifierDescriptor,
};

/* USB composite device Configuration Descriptor */
/*   All Descriptors (Configuration, Interface, Endpoint, Class, Vendor */
__ALIGN_BEGIN uint8_t USBD_Composite_CfgFSDesc[USBD_COMPOSITE_DESC_SIZE]  __ALIGN_END =
{
  0x09,                                       /* bLength: Configuration Descriptor size */
  USB_DESC_TYPE_CONFIGURATION,                /* bDescriptorType: Configuration */
  WBVAL(USBD_COMPOSITE_DESC_SIZE),
  USBD_MAX_NUM_INTERFACES,                    /* bNumInterfaces: */
  0x01,                                       /* bConfigurationValue: */
  0x00,                                       /* iConfiguration: */
  0xC0,                                       /* bmAttributes: bus powered */
  0x96,                                       /* bMaxPower: 300 mA */

  /* Interface Association Descriptor */
  USBD_IAD_DESC_SIZE,                         // bLength
  USBD_IAD_DESCRIPTOR_TYPE,                   // bDescriptorType
  USBD_AUD_FIRST_INTERFACE,                   // bFirstInterface
  USBD_AUD_INTERFACE_NUM,                     // bInterfaceCount
  USB_DEVICE_CLASS_AUDIO,                     // bFunctionClass
  AUDIO_SUBCLASS_AUDIOCONTROL,                // bFunctionSubClass
  AUDIO_PROTOCOL_UNDEFINED,                   // bInterfaceProtocol
  0x00,                                       // iFunction

  /****************************UAC MIC*******************************/
  /************ Descriptor of UAC Microphone interface **************/
  /* USB Speaker Standard interface descriptor */
  AUDIO_INTERFACE_DESC_SIZE,            /* bLength */
  USB_DESC_TYPE_INTERFACE,              /* bDescriptorType */
  USBD_AUD_INTERFACE_MIC_CONTROL,       /* bInterfaceNumber */
  0x00,                                 /* bAlternateSetting */
  0x00,                                 /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOCONTROL,          /* bInterfaceSubClass */
  AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */
  0x00,                                 /* iInterface */
  /* 09 byte*/

  #if USE_FEATURE_UNIT
//////////////////////////////

  /* USB Speaker Class-specific AC Interface Descriptor */
  AUDIO_INTERFACE_DESC_SIZE,            /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_HEADER,                 /* bDescriptorSubtype */
  0x00,          /* 1.00 */             /* bcdADC */
  0x01,
  0x25 + USBD_AUDIO_CHANNEL+1,          /* [EDITED] wTotalLength = 30*/ //0x27, // Length == AC+Input+Output+Feature
  0x00,
  0x01,                                 /* bInCollection */
  USBD_AUD_INTERFACE_MIC_STREAMING,     /* baInterfaceNr */
  /* 09 byte*/

  /* MIC Input Terminal Descriptor */
  AUDIO_INPUT_TERMINAL_DESC_SIZE,       /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_INPUT_TERMINAL,         /* bDescriptorSubtype */
  0x01,                                 /* bTerminalID */
  0x01,                                 /* wTerminalType Microphone 0x0201 */ //0x01,                                 /* wTerminalType AUDIO_TERMINAL_USB_STREAMING   0x0101 */
  0x02,
  0x00,                                 /* bAssocTerminal */
  USBD_AUDIO_CHANNEL,                   /* bNrChannels */
  0x00,                                 /* wChannelConfig 0x0000  Mono */
  0x00,
  0x00,                                 /* iChannelNames */
  0x00,                                 /* iTerminal */
  /* 12 byte*/

  /* MIC Audio Feature Unit Descriptor */
  0x07 + USBD_AUDIO_CHANNEL + 1,        /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_FEATURE_UNIT,           /* bDescriptorSubtype */
  0x02,                                 /* bUnitID */
  0x01,                                 /* bSourceID */
  0x01,                                 /* bControlSize */
    #if (USBD_AUDIO_CHANNEL == 1)
  //USBD_AUDIO_CONTROL_FEATURE_UNIT_VOLUME|USBD_AUDIO_CONTROL_FEATURE_UNIT_MUTE,                                 /*bmaControls(0)       0x02 Volume Control */
  USBD_AUDIO_CONTROL_FEATURE_UNIT_VOLUME,                                 /*bmaControls(0)       0x02 Volume Control */
  0x00,                                 /*bmaControls(1)       0x00 */
    #else
      #error "NOT YET TESTED!!!"
    #endif
  0x00,                                 /* iTerminal */
  /* 09 byte*/

  /* Mic Output Terminal Descriptor */
  0x09,                                 /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_OUTPUT_TERMINAL,        /* bDescriptorSubtype */
  0x03,                                 /* bTerminalID */
  0x01,                                 /* wTerminalType = 0x0101 = USB_STREAMING */
  0x01,
  0x00,                                 /* bAssocTerminal */
  0x02,                                 /* bSourceID */
  0x00,                                 /* iTerminal */
  /* 09 byte*/

//////////////////////////////
  #else /* USE_FEATURE_UNIT */

  /* USB Speaker Class-specific AC Interface Descriptor */
  AUDIO_INTERFACE_DESC_SIZE,            /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_HEADER,                 /* bDescriptorSubtype */
  0x00,          /* 1.00 */             /* bcdADC */
  0x01,
  0x25 + USBD_AUDIO_CHANNEL + 1,            /* [EDITED] wTotalLength = 30*/ //0x27,                                 /* wTotalLength = 39*/
  0x00,
  0x01,                                 /* bInCollection */
  USBD_AUD_INTERFACE_MIC_STREAMING,     /* baInterfaceNr */
  /* 09 byte*/

  /* MIC Input Terminal Descriptor */
  AUDIO_INPUT_TERMINAL_DESC_SIZE,       /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_INPUT_TERMINAL,         /* bDescriptorSubtype */
  0x01,                                 /* bTerminalID */
  0x01,                                 /* wTerminalType Microphone 0x0201 */ //0x01,                                 /* wTerminalType AUDIO_TERMINAL_USB_STREAMING   0x0101 */
  0x02,
  0x00,                                 /* bAssocTerminal */
  USBD_AUDIO_CHANNEL,                   /* bNrChannels */
  0x00,                                 /* wChannelConfig 0x0000  Mono */
  0x00,
  0x00,                                 /* iChannelNames */
  0x00,                                 /* iTerminal */
  /* 12 byte*/

  /* Mic Output Terminal Descriptor */
  /* without volume control */
  0x09,                                 /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_OUTPUT_TERMINAL,        /* bDescriptorSubtype */
  0x02,                                 /* bTerminalID */
  0x01,                                 /* wTerminalType = 0x0101 = USB_STREAMING */
  0x01,
  0x00,                                 /* bAssocTerminal */
  0x01,                                 /* bSourceID */
  0x00,                                 /* iTerminal */
  /* 09 byte*/
  #endif


  /* USB Speaker Standard AS Interface Descriptor - Audio Streaming Zero Bandwith */
  /* Mic Standard AS Interface Descriptor - Audio Streaming Zero Bandwith */
  /* Interface 1, Alternate Setting 0                                             */
  AUDIO_INTERFACE_DESC_SIZE,            /* bLength */
  USB_DESC_TYPE_INTERFACE,              /* bDescriptorType */
  USBD_AUD_INTERFACE_MIC_STREAMING,     /* bInterfaceNumber */
  0x00,                                 /* bAlternateSetting */
  0x00,                                 /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOSTREAMING,        /* bInterfaceSubClass */
  AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */
  0x00,                                 /* iInterface */
  /* 09 byte*/

  /* USB Microphone Standard AS Interface Descriptor - Audio Streaming Operational */
  /* Mic Standard AS Interface Descriptor - Audio Streaming Operational */
  /* Interface 1, Alternate Setting 1                                           */
  AUDIO_INTERFACE_DESC_SIZE,            /* bLength */
  USB_DESC_TYPE_INTERFACE,              /* bDescriptorType */
  USBD_AUD_INTERFACE_MIC_STREAMING,     /* bInterfaceNumber */
  0x01,                                 /* bAlternateSetting */
  0x01,                                 /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOSTREAMING,        /* bInterfaceSubClass */
  AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */
  0x00,                                 /* iInterface */
  /* 09 byte*/

  /* Microphone Audio Streaming Interface Descriptor */
  AUDIO_STREAMING_INTERFACE_DESC_SIZE,  /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_STREAMING_GENERAL,              /* bDescriptorSubtype */
    #if USE_FEATURE_UNIT
  0x03,                                 /* bTerminalLink */
    #else
  0x02,                                 /* bTerminalLink */
    #endif
  0x01,                                 /* bDelay */
  #if UAC_USE_PCM
  0x01,                                 /* wFormatTag AUDIO_FORMAT_PCM  0x0001 */
  0x00,
  #else
  0x05,                                 /* wFormatTag AUDIO_FORMAT_MULAW  0x0005 */
  0x00,
  #endif
  /* 07 byte*/

  /* USB Speaker Audio Type III Format Interface Descriptor */
  /* Mic Audio Type I Format Interface Descriptor */
  0x0B,                                 /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_STREAMING_FORMAT_TYPE,          /* bDescriptorSubtype */
  AUDIO_FORMAT_TYPE_I,                  /* bFormatType */
  USBD_AUDIO_CHANNEL,                   /* bNrChannels */
  #if UAC_USE_PCM
  0x02,                                 /* bSubFrameSize :  2 Bytes per frame (16bits) */
  16,                                   /* bBitResolution (16-bits per sample) */
  #else
  0x01,                                 /* bSubFrameSize :  1 Bytes per frame (8bits) */
  16,                                   /* bBitResolution (8-bits per sample) */
  #endif
  0x01,                                 /* bSamFreqType only one frequency supported */
  AUDIO_SAMPLE_FREQ(USBD_AUDIO_FREQ),   /* Audio sampling frequency coded on 3 bytes */
  /* 11 byte*/

  /* Endpoint 1 - Standard Descriptor */
  AUDIO_STANDARD_ENDPOINT_DESC_SIZE,    /* bLength */
  USB_DESC_TYPE_ENDPOINT,               /* bDescriptorType */
  AUDIO_IN_EP,                          /* bEndpointAddress 1 out endpoint */
  USBD_EP_TYPE_ISOC,                    /* bmAttributes */
  MIC_PACKET_SZE(USBD_AUDIO_FREQ),      /* wMaxPacketSize in Bytes (Freq(Samples)*2(Stereo)*2(HalfWord)) */
  AUDIO_HS_BINTERVAL,                   /* bInterval */
  0x00,                                 /* bRefresh */
  0x00,                                 /* bSynchAddress */
  /* 09 byte*/

  /* Endpoint - Audio Streaming Descriptor*/
  AUDIO_STREAMING_ENDPOINT_DESC_SIZE,   /* bLength */
  AUDIO_ENDPOINT_DESCRIPTOR_TYPE,       /* bDescriptorType */
  AUDIO_ENDPOINT_GENERAL,               /* bDescriptor */
  0x00,                                 /* bmAttributes */
  0x00,                                 /* bLockDelayUnits */
  0x00,                                 /* wLockDelay */
  0x00,
  /* 07 byte*/

  /****************************HID IMU, only has 1 EP, to HOST (EPIN)*****/
  /************** Descriptor of CUSTOM HID interface ****************/
  /* 09 */
  0x09,                                               /* bLength: Interface Descriptor size*/
  USB_DESC_TYPE_INTERFACE,                            /* bDescriptorType: Interface descriptor type */
  USBD_HID_INTERFACE_IMU,                             /* bInterfaceNumber: Number of Interface */
  0x00,                                               /* bAlternateSetting: Alternate setting */
  0x01,                                               /* bNumEndpoints*/
  0x03,                                               /* bInterfaceClass: CUSTOM_HID */
  0x00,                                               /* bInterfaceSubClass : 1=BOOT, 0=no boot */
  0x00,                                               /* nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */
  0x00,                                               /* iInterface: Index of string descriptor */
  /******************** Descriptor of CUSTOM_HID *************************/
  /* 18 */
  0x09,                                               /* bLength: CUSTOM_HID Descriptor size */
  CUSTOM_HID_IMU_DESCRIPTOR_TYPE,                     /* bDescriptorType: CUSTOM_HID */
  0x11,                                               /* bCUSTOM_HIDUSTOM_HID: CUSTOM_HID Class Spec release number */
  0x01,
  0x00,                                               /* bCountryCode: Hardware target country */
  0x01,                                               /* bNumDescriptors: Number of CUSTOM_HID class descriptors to follow */
  0x22,                                               /* bDescriptorType */
  LOBYTE(USBD_CUSTOM_HID_IMU_REPORT_DESC_SIZE),       /* wItemLength: Total length of Report descriptor */
  HIBYTE(USBD_CUSTOM_HID_IMU_REPORT_DESC_SIZE),
  /******************** Descriptor of Custom HID endpoints ********************/
  /* 27 */
  0x07,                                               /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,                             /* bDescriptorType: */

  CUSTOM_HID_IMU_EPIN_ADDR,                           /* bEndpointAddress: Endpoint Address (IN) */
  0x03,                                               /* bmAttributes: Interrupt endpoint */
  CUSTOM_HID_IMU_EPIN_SIZE,                           /* wMaxPacketSize: 2 Byte max */
  0x00,
  CUSTOM_HID_IMU_HS_BINTERVAL,                        /* bInterval: Polling Interval */
  /* 34 */

  /****************************HID KEY*******************************/
  /************** Descriptor of CUSTOM HID interface ****************/
  /* 09 */
  0x09,                                               /* bLength: Interface Descriptor size*/
  USB_DESC_TYPE_INTERFACE,                            /* bDescriptorType: Interface descriptor type */
  USBD_HID_INTERFACE_KEY,                             /* bInterfaceNumber: Number of Interface */
  0x00,                                               /* bAlternateSetting: Alternate setting */
  0x02,                                               /* bNumEndpoints*/
  0x03,                                               /* bInterfaceClass: CUSTOM_HID */
  0x00,                                               /* bInterfaceSubClass : 1=BOOT, 0=no boot */
  0x00,                                               /* nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */
  0x00,                                               /* iInterface: Index of string descriptor */
  /******************** Descriptor of CUSTOM_HID *************************/
  /* 18 */
  0x09,                                               /* bLength: CUSTOM_HID Descriptor size */
  CUSTOM_HID_KEY_DESCRIPTOR_TYPE,                     /* bDescriptorType: CUSTOM_HID */
  0x11,                                               /* bCUSTOM_HIDUSTOM_HID: CUSTOM_HID Class Spec release number */
  0x01,
  0x00,                                               /* bCountryCode: Hardware target country */
  0x01,                                               /* bNumDescriptors: Number of CUSTOM_HID class descriptors to follow */
  0x22,                                               /* bDescriptorType */
  LOBYTE(USBD_CUSTOM_HID_KEY_REPORT_DESC_SIZE),       /* wItemLength: Total length of Report descriptor */
  HIBYTE(USBD_CUSTOM_HID_KEY_REPORT_DESC_SIZE),
  /******************** Descriptor of Custom HID endpoints ********************/
  /* 25 */
  0x07,                                               /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,                             /* bDescriptorType: */
  CUSTOM_HID_KEY_EPIN_ADDR,                           /* bEndpointAddress: Endpoint Address (IN) */
  0x03,                                               /* bmAttributes: Interrupt endpoint */
  CUSTOM_HID_KEY_EPIN_SIZE,                           /* wMaxPacketSize: 2 Byte max */
  0x00,
  CUSTOM_HID_KEY_HS_BINTERVAL,                        /* bInterval: Polling Interval */

  /* 32 */
  0x07,                                               /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,                             /* bDescriptorType: */
  CUSTOM_HID_KEY_EPOUT_ADDR,                          /* bEndpointAddress: Endpoint Address (OUT) */
  0x03,                                               /* bmAttributes: Interrupt endpoint */
  CUSTOM_HID_KEY_EPOUT_SIZE,                          /* wMaxPacketSize: 2 Bytes max  */
  0x00,
  CUSTOM_HID_KEY_HS_BINTERVAL,                        /* bInterval: Polling Interval */


  /****************************CDC ToF************************************/

  /* Interface Association Descriptor */
  USBD_IAD_DESC_SIZE,                         // bLength
  USBD_IAD_DESCRIPTOR_TYPE,                   // bDescriptorType
  USBD_CDC_FIRST_INTERFACE,                   // bFirstInterface
  USBD_CDC_INTERFACE_NUM,                     // bInterfaceCount
  0x02,                                       // bFunctionClass
  0x02,                                       // bFunctionSubClass
  0x01,                                       // bInterfaceProtocol
  0x00,                                       // iFunction

  /* **********
   *    NOTE
   * **********
   * The following contents are from the USBD_CDC_CfgFSDesc array in usbd_cdc.c
   */
  /* Interface Descriptor */
  0x09,                                       /* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,                    /* bDescriptorType: Interface */
                                              /* Interface descriptor type */
  USBD_CDC_CMD_INTERFACE,                     /* bInterfaceNumber: Number of Interface */
  0x00,                                       /* bAlternateSetting: Alternate setting */
  0x01,                                       /* bNumEndpoints: One endpoints used */
  0x02,                                       /* bInterfaceClass: Communication Interface Class */
  0x02,                                       /* bInterfaceSubClass: Abstract Control Model */
  0x01,                                       /* bInterfaceProtocol: Common AT commands */
  0x00,                                       /* iInterface: */

  /* Header Functional Descriptor */
  0x05,                                       /* bLength: Endpoint Descriptor size */
  0x24,                                       /* bDescriptorType: CS_INTERFACE */
  0x00,                                       /* bDescriptorSubtype: Header Func Desc */
  0x10,                                       /* bcdCDC: spec release number */
  0x01,

  /* Call Management Functional Descriptor */
  0x05,                                       /* bFunctionLength */
  0x24,                                       /* bDescriptorType: CS_INTERFACE */
  0x01,                                       /* bDescriptorSubtype: Call Management Func Desc */
  0x00,                                       /* bmCapabilities: D0+D1 */
  USBD_CDC_DATA_INTERFACE,                    /* bDataInterface: 1 */
  
  /* ACM Functional Descriptor */
  0x04,                                       /* bFunctionLength */
  0x24,                                       /* bDescriptorType: CS_INTERFACE */
  0x02,                                       /* bDescriptorSubtype: Abstract Control Management desc */
  0x02,                                       /* bmCapabilities */

  /* Union Functional Descriptor */
  0x05,                                       /* bFunctionLength */
  0x24,                                       /* bDescriptorType: CS_INTERFACE */
  0x06,                                       /* bDescriptorSubtype: Union func desc */
  USBD_CDC_CMD_INTERFACE,                     /* bMasterInterface: Communication class interface */
  USBD_CDC_DATA_INTERFACE,                    /* bSlaveInterface0: Data Class Interface */
  
  /* Endpoint 2 Descriptor */
  0x07,                                       /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,                     /* bDescriptorType: Endpoint */
  CDC_CMD_EP,                                 /* bEndpointAddress */
  0x03,                                       /* bmAttributes: Interrupt */
  LOBYTE(CDC_CMD_PACKET_SIZE),                /* wMaxPacketSize: */
  HIBYTE(CDC_CMD_PACKET_SIZE),
  CDC_HS_BINTERVAL,                           /* bInterval: */
  /*---------------------------------------------------------------------------*/
  
  /* Data class interface descriptor */
  0x09,                                       /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_INTERFACE,                    /* bDescriptorType: */
  USBD_CDC_DATA_INTERFACE,                    /* bInterfaceNumber: Number of Interface */
  0x00,                                       /* bAlternateSetting: Alternate setting */
  0x02,                                       /* bNumEndpoints: Two endpoints used */
  0x0A,                                       /* bInterfaceClass: CDC */
  0x00,                                       /* bInterfaceSubClass: */
  0x00,                                       /* bInterfaceProtocol: */
  0x00,                                       /* iInterface: */

  /* Endpoint OUT Descriptor */
  0x07,                                       /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,                     /* bDescriptorType: Endpoint */
  CDC_OUT_EP,                                 /* bEndpointAddress */
  0x02,                                       /* bmAttributes: Bulk */
  #if FS_OVER_HS_CTRL
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),        /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  #else
  LOBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),        /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),
  #endif
  0x00,                                       /* bInterval: ignore for Bulk transfer */
  
  /* Endpoint IN Descriptor */
  0x07,                                       /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,                     /* bDescriptorType: Endpoint */
  CDC_IN_EP,                                  /* bEndpointAddress */
  0x02,                                       /* bmAttributes: Bulk */
  #if FS_OVER_HS_CTRL
  LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),        /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),
  #else
  LOBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),        /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),
  #endif
  0x00,                                       /* bInterval: ignore for Bulk transfer */

#if ENABLE_CDC_CMD_PORT
/****************************CDC Device Controller************************************/

  /* Interface Association Descriptor */
  USBD_IAD_DESC_SIZE,                         // bLength
  USBD_IAD_DESCRIPTOR_TYPE,                   // bDescriptorType
  USBD_CDC_DEVCTLR_FIRST_INTERFACE,           // bFirstInterface
  USBD_CDC_DEVCTLR_INTERFACE_NUM,             // bInterfaceCount
  0x02,                                       // bFunctionClass
  0x02,                                       // bFunctionSubClass
  0x01,                                       // bInterfaceProtocol
  0x00,                                       // iFunction

  /* **********
   *    NOTE
   * **********
   * The following contents are from the USBD_CDC_CfgFSDesc array in usbd_cdc.c
   */
  /* Interface Descriptor */
  0x09,                                       /* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,                    /* bDescriptorType: Interface */
                                              /* Interface descriptor type */
  USBD_CDC_DEVCTLR_CMD_INTERFACE,             /* bInterfaceNumber: Number of Interface */
  0x00,                                       /* bAlternateSetting: Alternate setting */
  0x01,                                       /* bNumEndpoints: One endpoints used */
  0x02,                                       /* bInterfaceClass: Communication Interface Class */
  0x02,                                       /* bInterfaceSubClass: Abstract Control Model */
  0x01,                                       /* bInterfaceProtocol: Common AT commands */
  0x00,                                       /* iInterface: */

  /* Header Functional Descriptor */
  0x05,                                       /* bLength: Endpoint Descriptor size */
  0x24,                                       /* bDescriptorType: CS_INTERFACE */
  0x00,                                       /* bDescriptorSubtype: Header Func Desc */
  0x10,                                       /* bcdCDC: spec release number */
  0x01,

  /* Call Management Functional Descriptor */
  0x05,                                       /* bFunctionLength */
  0x24,                                       /* bDescriptorType: CS_INTERFACE */
  0x01,                                       /* bDescriptorSubtype: Call Management Func Desc */
  0x00,                                       /* bmCapabilities: D0+D1 */
  USBD_CDC_DEVCTLR_DATA_INTERFACE,            /* bDataInterface: 6 */
  
  /* ACM Functional Descriptor */
  0x04,                                       /* bFunctionLength */
  0x24,                                       /* bDescriptorType: CS_INTERFACE */
  0x02,                                       /* bDescriptorSubtype: Abstract Control Management desc */
  0x02,                                       /* bmCapabilities */

  /* Union Functional Descriptor */
  0x05,                                       /* bFunctionLength */
  0x24,                                       /* bDescriptorType: CS_INTERFACE */
  0x06,                                       /* bDescriptorSubtype: Union func desc */
  USBD_CDC_DEVCTLR_CMD_INTERFACE,             /* bMasterInterface: Communication class interface */
  USBD_CDC_DEVCTLR_DATA_INTERFACE,            /* bSlaveInterface0: Data Class Interface */
  
  /* Endpoint 2 Descriptor */
  0x07,                                       /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,                     /* bDescriptorType: Endpoint */
  CDC_DEVCTLR_CMD_EP,                         /* bEndpointAddress */
  0x03,                                       /* bmAttributes: Interrupt */
  LOBYTE(CDC_DEVCTLR_CMD_PACKET_SIZE),        /* wMaxPacketSize: */
  HIBYTE(CDC_DEVCTLR_CMD_PACKET_SIZE),
  CDC_DEVCTLR_HS_BINTERVAL,                   /* bInterval: */
  /*---------------------------------------------------------------------------*/
  
  /* Data class interface descriptor */
  0x09,                                       /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_INTERFACE,                    /* bDescriptorType: */
  USBD_CDC_DEVCTLR_DATA_INTERFACE,            /* bInterfaceNumber: Number of Interface */
  0x00,                                       /* bAlternateSetting: Alternate setting */
  0x02,                                       /* bNumEndpoints: Two endpoints used */
  0x0A,                                       /* bInterfaceClass: CDC */
  0x00,                                       /* bInterfaceSubClass: */
  0x00,                                       /* bInterfaceProtocol: */
  0x00,                                       /* iInterface: */

  /* Endpoint OUT Descriptor */
  0x07,                                       /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,                     /* bDescriptorType: Endpoint */
  CDC_DEVCTLR_OUT_EP,                         /* bEndpointAddress */
  0x02,                                       /* bmAttributes: Bulk */
#if FS_OVER_HS_CTRL
  LOBYTE(CDC_DEVCTLR_DATA_FS_MAX_PACKET_SIZE),        /* wMaxPacketSize: */
  HIBYTE(CDC_DEVCTLR_DATA_FS_MAX_PACKET_SIZE),
#else
  LOBYTE(CDC_DEVCTLR_DATA_HS_MAX_PACKET_SIZE),        /* wMaxPacketSize: */
  HIBYTE(CDC_DEVCTLR_DATA_HS_MAX_PACKET_SIZE),
#endif
  0x00,                                       /* bInterval: ignore for Bulk transfer */
  
  /* Endpoint IN Descriptor */
  0x07,                                       /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,                     /* bDescriptorType: Endpoint */
  CDC_DEVCTLR_IN_EP,                          /* bEndpointAddress */
  0x02,                                       /* bmAttributes: Bulk */
#if FS_OVER_HS_CTRL
  LOBYTE(CDC_DEVCTLR_DATA_FS_MAX_PACKET_SIZE),        /* wMaxPacketSize: */
  HIBYTE(CDC_DEVCTLR_DATA_FS_MAX_PACKET_SIZE),
#else
  LOBYTE(CDC_DEVCTLR_DATA_HS_MAX_PACKET_SIZE),        /* wMaxPacketSize: */
  HIBYTE(CDC_DEVCTLR_DATA_HS_MAX_PACKET_SIZE),
#endif
  0x00,                                       /* bInterval: ignore for Bulk transfer */
#endif

};


/* USB Standard Device Descriptor */
__ALIGN_BEGIN  uint8_t USBD_Composite_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC]  __ALIGN_END =
{
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0xEF, // https://learn.microsoft.com/zh-tw/windows-hardware/drivers/usbcon/usb-interface-association-descriptor
  0x02,
  0x01,
  0x40,
  0x01,
  0x00,
};





/**
  * @brief  USBD_Composite_Init
  *         Initialize the Composite interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_Composite_Init (USBD_HandleTypeDef *pdev,
                            uint8_t cfgidx)
{
  uint8_t res = 0;

  pdev->pUserData = &USBD_AUDIO_fops_HS;
  res +=  USBD_AUDIO.Init(pdev,cfgidx);
  pAUDData_MIC = pdev->pClassData;

  pdev->pUserData = &USBD_Interface_fops_HS;
  res +=  USBD_CDC.Init(pdev,cfgidx);
  pCDCData_Tof = pdev->pClassData;
  
//  pdev->pUserData = &USBD_CustomHID_fops_HS;
//  res +=  USBD_CUSTOM_HID.Init(pdev,cfgidx);
//  pHIDData = pdev->pClassData;
  
  pdev->pUserData = &USBD_CustomHID_IMU_fops_HS;
  res +=  USBD_CUSTOM_HID_IMU.Init(pdev,cfgidx);
  pHIDData_IMU = pdev->pClassData;

  pdev->pUserData = &USBD_CustomHID_KEY_fops_HS;
  res +=  USBD_CUSTOM_HID_KEY.Init(pdev,cfgidx);
  pHIDData_KEY = pdev->pClassData;
  
  pdev->pUserData = &USBD_Interface_fops_DEVCTLR_HS;
  res +=  USBD_CDC_DEVCTLR.Init(pdev,cfgidx);
  pCDCData_Devctlr = pdev->pClassData;
  
  
  return res;
}

/**
  * @brief  USBD_Composite_DeInit
  *         DeInitilaize  the Composite configuration
  * @param  pdev: device instance
  * @param  cfgidx: configuration index
  * @retval status
  */
static uint8_t  USBD_Composite_DeInit (USBD_HandleTypeDef *pdev,
                              uint8_t cfgidx)
{
    uint8_t res = 0;

    pdev->pClassData = pAUDData_MIC;
    pdev->pUserData = &USBD_AUDIO_fops_HS;
    res +=  USBD_AUDIO.DeInit(pdev,cfgidx);

    pdev->pClassData = pCDCData_Tof;
    pdev->pUserData = &USBD_Interface_fops_HS;
    res +=  USBD_CDC.DeInit(pdev,cfgidx);

//    pdev->pClassData = pHIDData;
//    pdev->pUserData = &USBD_CustomHID_fops_HS;
//    res +=  USBD_CUSTOM_HID.DeInit(pdev,cfgidx);

    pdev->pClassData = pHIDData_IMU;
    pdev->pUserData = &USBD_CustomHID_IMU_fops_HS;
    res +=  USBD_CUSTOM_HID_IMU.DeInit(pdev,cfgidx);

    pdev->pClassData = pHIDData_KEY;
    pdev->pUserData = &USBD_CustomHID_KEY_fops_HS;
    res +=  USBD_CUSTOM_HID_KEY.DeInit(pdev,cfgidx);

    pdev->pClassData = pCDCData_Devctlr;
    pdev->pUserData = &USBD_Interface_fops_DEVCTLR_HS;
    res +=  USBD_CDC_DEVCTLR.DeInit(pdev,cfgidx);

    return res;
}


static uint8_t  USBD_Composite_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
	switch(pdev->request.wIndex) {
	    case USBD_AUD_INTERFACE_MIC_CONTROL:
	    case USBD_AUD_INTERFACE_MIC_STREAMING:
	        pdev->pClassData = pAUDData_MIC;
	        pdev->pUserData =  &USBD_AUDIO_fops_HS;
	        return(USBD_AUDIO.EP0_RxReady (pdev));
	    case USBD_CDC_DATA_INTERFACE:
	    case USBD_CDC_CMD_INTERFACE:
	        pdev->pClassData = pCDCData_Tof;
	        pdev->pUserData =  &USBD_Interface_fops_HS;
	        return(USBD_CDC.EP0_RxReady(pdev));

//	    case USBD_HID_INTERFACE:
//	        pdev->pClassData = pHIDData;
//	        pdev->pUserData =  &USBD_CustomHID_fops_HS;
//	        return(USBD_CUSTOM_HID.EP0_RxReady (pdev));

	    case USBD_HID_INTERFACE_IMU:
	        pdev->pClassData = pHIDData_IMU;
	        pdev->pUserData =  &USBD_CustomHID_IMU_fops_HS;
	        return(USBD_CUSTOM_HID_IMU.EP0_RxReady (pdev));

	    case USBD_CDC_DEVCTLR_CMD_INTERFACE:
	    case USBD_CDC_DEVCTLR_DATA_INTERFACE:
	        pdev->pClassData = pCDCData_Devctlr;
	        pdev->pUserData =  &USBD_Interface_fops_DEVCTLR_HS;
	        return(USBD_CDC_DEVCTLR.EP0_RxReady(pdev));

	    case USBD_HID_INTERFACE_KEY:
	        pdev->pClassData = pHIDData_KEY;
	        pdev->pUserData =  &USBD_CustomHID_KEY_fops_HS;
	        return(USBD_CUSTOM_HID_KEY.EP0_RxReady (pdev));          

	    default:
	        break;
	}

    return USBD_OK;
}



/**
* @brief  USBD_Composite_Setup
*         Handle the Composite requests
* @param  pdev: device instance
* @param  req: USB request
* @retval status
*/
static uint8_t  USBD_Composite_Setup (USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  switch (req->bmRequest & USB_REQ_RECIPIENT_MASK)
  {
   case USB_REQ_RECIPIENT_INTERFACE:
     //switch(req->wIndex)
     switch(LOBYTE(req->wIndex))
      {
         case USBD_AUD_INTERFACE_MIC_CONTROL:
	     case USBD_AUD_INTERFACE_MIC_STREAMING:
             pdev->pClassData = pAUDData_MIC;
             pdev->pUserData =  &USBD_AUDIO_fops_HS;
           return(USBD_AUDIO.Setup (pdev, req));
         case USBD_CDC_DATA_INTERFACE:
         case USBD_CDC_CMD_INTERFACE:
             pdev->pClassData = pCDCData_Tof;
             pdev->pUserData =  &USBD_Interface_fops_HS;
           return(USBD_CDC.Setup(pdev, req));

//         case USBD_HID_INTERFACE:
//             pdev->pClassData = pHIDData;
//             pdev->pUserData =  &USBD_CustomHID_fops_HS;
//           return(USBD_CUSTOM_HID.Setup (pdev, req));
		 case USBD_HID_INTERFACE_IMU:
             pdev->pClassData = pHIDData_IMU;
             pdev->pUserData =  &USBD_CustomHID_IMU_fops_HS;
           return(USBD_CUSTOM_HID_IMU.Setup (pdev, req));
         case USBD_CDC_DEVCTLR_DATA_INTERFACE:
         case USBD_CDC_DEVCTLR_CMD_INTERFACE:
             pdev->pClassData = pCDCData_Devctlr;
             pdev->pUserData =  &USBD_Interface_fops_DEVCTLR_HS;
           return(USBD_CDC_DEVCTLR.Setup (pdev, req));
		 case USBD_HID_INTERFACE_KEY:
             pdev->pClassData = pHIDData_KEY;
             pdev->pUserData =  &USBD_CustomHID_KEY_fops_HS;
           return(USBD_CUSTOM_HID_KEY.Setup (pdev, req));           
         default:
            break;
     }
     break;

   case USB_REQ_RECIPIENT_ENDPOINT:
     //switch(req->wIndex)
     switch(LOBYTE(req->wIndex))
     {
         case AUDIO_IN_EP:
         case AUDIO_OUT_EP:
             pdev->pClassData = pAUDData_MIC;
             pdev->pUserData =  &USBD_AUDIO_fops_HS;
           return(USBD_AUDIO.Setup (pdev, req));
         case CDC_IN_EP:
         case CDC_OUT_EP:
         case CDC_CMD_EP:
             pdev->pClassData = pCDCData_Tof;
             pdev->pUserData =  &USBD_Interface_fops_HS;
           return(USBD_CDC.Setup(pdev, req));

//         case CUSTOM_HID_EPIN_ADDR:
//         case CUSTOM_HID_EPOUT_ADDR:
//             pdev->pClassData = pHIDData;
//             pdev->pUserData =  &USBD_CustomHID_fops_HS;
//           return(USBD_CUSTOM_HID.Setup (pdev, req));

         case CUSTOM_HID_IMU_EPIN_ADDR:
         case CUSTOM_HID_IMU_EPOUT_ADDR:
             pdev->pClassData = pHIDData_IMU;
             pdev->pUserData =  &USBD_CustomHID_IMU_fops_HS;
           return(USBD_CUSTOM_HID_IMU.Setup (pdev, req));

         case CDC_DEVCTLR_IN_EP:
         case CDC_DEVCTLR_OUT_EP:
         case CDC_DEVCTLR_CMD_EP:
             pdev->pClassData = pCDCData_Devctlr;
             pdev->pUserData =  &USBD_Interface_fops_DEVCTLR_HS;
           return(USBD_CDC_DEVCTLR.Setup(pdev, req));
         case CUSTOM_HID_KEY_EPIN_ADDR:
         case CUSTOM_HID_KEY_EPOUT_ADDR:
             pdev->pClassData = pHIDData_KEY;
             pdev->pUserData =  &USBD_CustomHID_KEY_fops_HS;
           return(USBD_CUSTOM_HID_KEY.Setup (pdev, req));

         default:
            break;
     }
     break;
  }
  return USBD_OK;
}




/**
* @brief  USBD_Composite_DataIn
*         handle data IN Stage
* @param  pdev: device instance
* @param  epnum: endpoint index
* @retval status
*/
uint8_t  USBD_Composite_DataIn (USBD_HandleTypeDef *pdev,
                              uint8_t epnum)
{
  switch(epnum)
  {
      case AUD_MIC_INDATA_NUM:
             pdev->pClassData = pAUDData_MIC;
             pdev->pUserData =  &USBD_AUDIO_fops_HS;
         return(USBD_AUDIO.DataIn(pdev,epnum));
      case CDC_INDATA_NUM:
        pdev->pClassData = pCDCData_Tof;
        pdev->pUserData =  &USBD_Interface_fops_HS;
         return(USBD_CDC.DataIn(pdev,epnum));

//      case HID_INDATA_NUM:
//             pdev->pClassData = pHIDData;
//             pdev->pUserData =  &USBD_CustomHID_fops_HS;
//         return(USBD_CUSTOM_HID.DataIn(pdev,epnum));

      case HID_IMU_INDATA_NUM:
             pdev->pClassData = pHIDData_IMU;
             pdev->pUserData =  &USBD_CustomHID_IMU_fops_HS;
         return(USBD_CUSTOM_HID_IMU.DataIn(pdev,epnum));

      case CDC_DEVCTLR_INDATA_NUM:
        pdev->pClassData = pCDCData_Devctlr;
        pdev->pUserData =  &USBD_Interface_fops_DEVCTLR_HS;
         return(USBD_CDC_DEVCTLR.DataIn(pdev,epnum));
      case HID_KEY_INDATA_NUM:
             pdev->pClassData = pHIDData_KEY;
             pdev->pUserData =  &USBD_CustomHID_KEY_fops_HS;
         return(USBD_CUSTOM_HID_KEY.DataIn(pdev,epnum));

      default:
         break;

  }
  return USBD_FAIL;
}


/**
* @brief  USBD_Composite_DataOut
*         handle data OUT Stage
* @param  pdev: device instance
* @param  epnum: endpoint index
* @retval status
*/
uint8_t  USBD_Composite_DataOut (USBD_HandleTypeDef *pdev,
                               uint8_t epnum)
{
  switch(epnum)
  {
      case AUD_MIC_OUTDATA_NUM:
             pdev->pClassData = pAUDData_MIC;
             pdev->pUserData =  &USBD_AUDIO_fops_HS;
         return(USBD_AUDIO.DataOut(pdev,epnum));
      case CDC_OUTDATA_NUM:
      case CDC_OUTCMD_NUM:
        pdev->pClassData = pCDCData_Tof;
        pdev->pUserData =  &USBD_Interface_fops_HS;
         return(USBD_CDC.DataOut(pdev,epnum));

//      case HID_OUTDATA_NUM:
//             pdev->pClassData = pHIDData;
//             pdev->pUserData =  &USBD_CustomHID_fops_HS;
//         return(USBD_CUSTOM_HID.DataOut(pdev,epnum));

      case HID_IMU_OUTDATA_NUM:
             pdev->pClassData = pHIDData_IMU;
             pdev->pUserData =  &USBD_CustomHID_IMU_fops_HS;
         return(USBD_CUSTOM_HID_IMU.DataOut(pdev,epnum));

      case CDC_DEVCTLR_OUTDATA_NUM:
      case CDC_DEVCTLR_OUTCMD_NUM:
        pdev->pClassData = pCDCData_Devctlr;
        pdev->pUserData =  &USBD_Interface_fops_DEVCTLR_HS;
         return(USBD_CDC_DEVCTLR.DataOut(pdev,epnum));
      case HID_KEY_OUTDATA_NUM:
             pdev->pClassData = pHIDData_KEY;
             pdev->pUserData =  &USBD_CustomHID_KEY_fops_HS;
         return(USBD_CUSTOM_HID_KEY.DataOut(pdev,epnum));

      default:
         break;

  }
  return USBD_FAIL;
}



/**
* @brief  USBD_Composite_GetHSCfgDesc
*         return configuration descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
uint8_t  *USBD_Composite_GetFSCfgDesc (uint16_t *length)
{
   *length = sizeof (USBD_Composite_CfgFSDesc);
   return USBD_Composite_CfgFSDesc;
}

/**
* @brief  DeviceQualifierDescriptor
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
uint8_t  *USBD_Composite_GetDeviceQualifierDescriptor (uint16_t *length)
{
  *length = sizeof (USBD_Composite_DeviceQualifierDesc);
  return USBD_Composite_DeviceQualifierDesc;
}


/**
  * @}
  */

void USBD_Composite_Switch_Itf(USBD_HandleTypeDef *pdev, USBD_COMPOSITE_ItfTypeDef interface)
{
  //if (!pdev || !pCDCData_Tof || !pHIDData || !pHIDData_IMU || !pHIDData_ALS) return;
if (!pdev || !pCDCData_Tof || !pAUDData_MIC || !pHIDData_IMU || !pCDCData_Devctlr|| !pHIDData_KEY) return;
  switch (interface)
  {
  case USBD_AUD_MIC_INTERFACE:
      pdev->pClassData = pAUDData_MIC;
      pdev->pUserData = &USBD_AUDIO_fops_HS;
      break;
  case USBD_CDC_INTERFACE:
      pdev->pClassData = pCDCData_Tof;
      pdev->pUserData =  &USBD_Interface_fops_HS;
      break;
//  case USBD_CUSTOMHID_INTERFACE:
//      pdev->pClassData = pHIDData;
//      pdev->pUserData = &USBD_CustomHID_fops_HS;
//      break;
  case USBD_CUSTOMHID_IMU_INTERFACE:
      pdev->pClassData = pHIDData_IMU;
      pdev->pUserData = &USBD_CustomHID_IMU_fops_HS;
      break;
  case USBD_CDC_DEVCTLR_INTERFACE:
      pdev->pClassData = pCDCData_Devctlr;
      pdev->pUserData =  &USBD_Interface_fops_DEVCTLR_HS;
      break;
  case USBD_CUSTOMHID_KEY_INTERFACE:
      pdev->pClassData = pHIDData_KEY;
      pdev->pUserData = &USBD_CustomHID_KEY_fops_HS;
      break;
  default:
	  break;
  }
}

USBD_COMPOSITE_ItfTypeDef USBD_Composite_Get_Current_Itf(USBD_HandleTypeDef *pdev)
{
  //return (pdev->pClassData == pHIDData)? USBD_CUSTOMHID_INTERFACE : USBD_CDC_INTERFACE;
  if(pdev->pClassData == pAUDData_MIC)
  {
	  return USBD_AUD_MIC_INTERFACE;
  }
  else if(pdev->pClassData == pCDCData_Tof)
  {
	  return USBD_CDC_INTERFACE;
  }
  else if(pdev->pClassData == pHIDData_IMU)
  {
	  return USBD_CUSTOMHID_IMU_INTERFACE;
  }
  else if(pdev->pClassData == pCDCData_Devctlr)
  {
	  return USBD_CDC_DEVCTLR_INTERFACE;
  }
  
  else if(pdev->pClassData == pHIDData_KEY)
  {
	  return USBD_CUSTOMHID_KEY_INTERFACE;
  }

  return USBD_AUD_MIC_INTERFACE;
}
/**
  * @}
  */


/**
  * @}
  */
