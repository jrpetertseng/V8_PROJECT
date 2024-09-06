#include "usbd_composite.h"
#include "debug_defs.h"
#include "usbd_cdc.h"

#if ENABLE_DEVICECTL_CDC
#include "usbd_cdc_devctlr.h"
static USBD_CDC_DEVCTLR_HandleTypeDef *pCDCData_Devctlr;
#endif
#include "usbd_customhid.h"
#include "usbd_customhid_sensor.h"

static USBD_CDC_HandleTypeDef *pCDCData_Tof;

static USBD_CUSTOM_HID_Sensor_HandleTypeDef *pHIDData_Sensor;
static USBD_CUSTOM_HID_Keyboard_HandleTypeDef *pHIDData_Keyboard;

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
/* All Descriptors: Configuration, Interface, Endpoint, Class, Vendor */
__ALIGN_BEGIN uint8_t USBD_Composite_CfgFSDesc[USBD_COMPOSITE_DESC_SIZE] __ALIGN_END =
{
	0x09,                                       /* bLength: Configuration Descriptor size */
	USB_DESC_TYPE_CONFIGURATION,                /* bDescriptorType: Configuration */
	WBVAL(USBD_COMPOSITE_DESC_SIZE),            /* wTotalLength */
	USBD_MAX_NUM_INTERFACES,                    /* bNumInterfaces */
	0x01,                                       /* bConfigurationValue */
	0x00,                                       /* iConfiguration */
#if (USBD_SELF_POWERED == 1U)
	0xC0,                                       /* bmAttributes: Bus Powered according to user configuration */
#else
	0x80,                                       /* bmAttributes: Bus Powered according to user configuration */
#endif
	0x96,                                       /* bMaxPower: 300 mA */
	/* 9 */

	/**************************** CDC ToF *******************************/
	USBD_IAD_DESC_SIZE,                         /* bLength: Interface Association Descriptor size */
	USBD_IAD_DESCRIPTOR_TYPE,                   /* bDescriptorType: Interface Association */
	USBD_CDC_FIRST_INTERFACE,                   /* bFirstInterface */
	USBD_CDC_INTERFACE_COUNT,                   /* bInterfaceCount */
	0x02,                                       /* bFunctionClass */
	0x02,                                       /* bFunctionSubClass */
	0x01,                                       /* bInterfaceProtocol */
	0x00,                                       /* iFunction */
	/* 8 */

	0x09,                                       /* bLength: Interface Descriptor size */
	USB_DESC_TYPE_INTERFACE,                    /* bDescriptorType: Interface descriptor type */
	USBD_CDC_CMD_INTERFACE,                     /* bInterfaceNumber: Number of Interface */
	0x00,                                       /* bAlternateSetting: Alternate setting */
	0x01,                                       /* bNumEndpoints: One endpoint used */
	0x02,                                       /* bInterfaceClass: Communication Interface Class */
	0x02,                                       /* bInterfaceSubClass: Abstract Control Model */
	0x01,                                       /* bInterfaceProtocol: Common AT commands */
	0x00,                                       /* iInterface */
	/* 9 */

	0x05,                                       /* bLength: Header Functional Descriptor size */
	0x24,                                       /* bDescriptorType: CS_INTERFACE */
	0x00,                                       /* bDescriptorSubtype: Header Func Desc */
	0x10, 0x01,                                 /* bcdCDC: spec release number */
	/* 5 */

	0x05,                                       /* bFunctionLength: Call Management Functional Descriptor size */
	0x24,                                       /* bDescriptorType: CS_INTERFACE */
	0x01,                                       /* bDescriptorSubtype: Call Management Func Desc */
	0x00,                                       /* bmCapabilities: D0+D1 */
	USBD_CDC_DATA_INTERFACE,                    /* bDataInterface */
	/* 5 */

	0x04,                                       /* bFunctionLength: ACM Functional Descriptor size */
	0x24,                                       /* bDescriptorType: CS_INTERFACE */
	0x02,                                       /* bDescriptorSubtype: Abstract Control Management desc */
	0x02,                                       /* bmCapabilities */
	/* 4 */

	0x05,                                       /* bFunctionLength: Union Functional Descriptor size */
	0x24,                                       /* bDescriptorType: CS_INTERFACE */
	0x06,                                       /* bDescriptorSubtype: Union func desc */
	USBD_CDC_CMD_INTERFACE,                     /* bMasterInterface: Communication class interface */
	USBD_CDC_DATA_INTERFACE,                    /* bSlaveInterface0: Data Class Interface */
	/* 5 */

	0x07,                                       /* bLength: Endpoint Descriptor size */
	USB_DESC_TYPE_ENDPOINT,                     /* bDescriptorType: Endpoint */
	CDC_CMD_EP,                                 /* bEndpointAddress */
	0x03,                                       /* bmAttributes: Interrupt */
	LOBYTE(CDC_CMD_PACKET_SIZE),                /* wMaxPacketSize */
	HIBYTE(CDC_CMD_PACKET_SIZE),
	CDC_HS_BINTERVAL,                           /* bInterval */
	/* 7 */

	0x09,                                       /* bLength: Data class interface descriptor size */
	USB_DESC_TYPE_INTERFACE,                    /* bDescriptorType: Interface descriptor type */
	USBD_CDC_DATA_INTERFACE,                    /* bInterfaceNumber: Number of Interface */
	0x00,                                       /* bAlternateSetting: Alternate setting */
	0x02,                                       /* bNumEndpoints: Two endpoints used */
	0x0A,                                       /* bInterfaceClass: CDC */
	0x00,                                       /* bInterfaceSubClass */
	0x00,                                       /* bInterfaceProtocol */
	0x00,                                       /* iInterface */
	/* 9 */

	0x07,                                       /* bLength: Endpoint OUT Descriptor size */
	USB_DESC_TYPE_ENDPOINT,                     /* bDescriptorType: Endpoint */
	CDC_OUT_EP,                                 /* bEndpointAddress */
	0x02,                                       /* bmAttributes: Bulk */
	LOBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),        /* wMaxPacketSize */
	HIBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),
	0x00,                                       /* bInterval: ignore for Bulk transfer */
	/* 7 */

	0x07,                                       /* bLength: Endpoint IN Descriptor size */
	USB_DESC_TYPE_ENDPOINT,                     /* bDescriptorType: Endpoint */
	CDC_IN_EP,                                  /* bEndpointAddress */
	0x02,                                       /* bmAttributes: Bulk */
	LOBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),        /* wMaxPacketSize */
	HIBYTE(CDC_DATA_HS_MAX_PACKET_SIZE),
	0x00,                                       /* bInterval: ignore for Bulk transfer */
	/* 7 */
#if ENABLE_DEVICECTL_CDC
	/**************************** CDC Device Controller *******************************/
	USBD_IAD_DESC_SIZE,                         /* bLength: Interface Association Descriptor size */
	USBD_IAD_DESCRIPTOR_TYPE,                   /* bDescriptorType: Interface Association */
	USBD_CDC_DEVCTLR_FIRST_INTERFACE,           /* bFirstInterface */
	USBD_CDC_DEVCTLR_INTERFACE_COUNT,           /* bInterfaceCount */
	0x02,                                       /* bFunctionClass */
	0x02,                                       /* bFunctionSubClass */
	0x01,                                       /* bInterfaceProtocol */
	0x00,                                       /* iFunction */
	/* 8 */

	0x09,                                       /* bLength: Interface Descriptor size */
	USB_DESC_TYPE_INTERFACE,                    /* bDescriptorType: Interface descriptor type */
	USBD_CDC_DEVCTLR_CMD_INTERFACE,             /* bInterfaceNumber: Number of Interface */
	0x00,                                       /* bAlternateSetting: Alternate setting */
	0x01,                                       /* bNumEndpoints: One endpoint used */
	0x02,                                       /* bInterfaceClass: Communication Interface Class */
	0x02,                                       /* bInterfaceSubClass: Abstract Control Model */
	0x01,                                       /* bInterfaceProtocol: Common AT commands */
	0x00,                                       /* iInterface */
	/* 9 */

	0x05,                                       /* bLength: Header Functional Descriptor size */
	0x24,                                       /* bDescriptorType: CS_INTERFACE */
	0x00,                                       /* bDescriptorSubtype: Header Func Desc */
	0x10, 0x01,                                 /* bcdCDC: spec release number */
	/* 5 */

	0x05,                                       /* bFunctionLength: Call Management Functional Descriptor size */
	0x24,                                       /* bDescriptorType: CS_INTERFACE */
	0x01,                                       /* bDescriptorSubtype: Call Management Func Desc */
	0x00,                                       /* bmCapabilities: D0+D1 */
	USBD_CDC_DEVCTLR_DATA_INTERFACE,            /* bDataInterface */
	/* 5 */

	0x04,                                       /* bFunctionLength: ACM Functional Descriptor size */
	0x24,                                       /* bDescriptorType: CS_INTERFACE */
	0x02,                                       /* bDescriptorSubtype: Abstract Control Management desc */
	0x02,                                       /* bmCapabilities */
	/* 4 */

	0x05,                                       /* bFunctionLength: Union Functional Descriptor size */
	0x24,                                       /* bDescriptorType: CS_INTERFACE */
	0x06,                                       /* bDescriptorSubtype: Union func desc */
	USBD_CDC_DEVCTLR_CMD_INTERFACE,             /* bMasterInterface: Communication class interface */
	USBD_CDC_DEVCTLR_DATA_INTERFACE,            /* bSlaveInterface0: Data Class Interface */
	/* 5 */

	0x07,                                       /* bLength: Endpoint Descriptor size */
	USB_DESC_TYPE_ENDPOINT,                     /* bDescriptorType: Endpoint */
	CDC_DEVCTLR_CMD_EP,                         /* bEndpointAddress */
	0x03,                                       /* bmAttributes: Interrupt */
	LOBYTE(CDC_DEVCTLR_CMD_PACKET_SIZE),        /* wMaxPacketSize */
	HIBYTE(CDC_DEVCTLR_CMD_PACKET_SIZE),
	CDC_DEVCTLR_HS_BINTERVAL,                   /* bInterval */
	/* 7 */

	0x09,                                       /* bLength: Data class interface descriptor size */
	USB_DESC_TYPE_INTERFACE,                    /* bDescriptorType: Interface descriptor type */
	USBD_CDC_DEVCTLR_DATA_INTERFACE,            /* bInterfaceNumber: Number of Interface */
	0x00,                                       /* bAlternateSetting: Alternate setting */
	0x02,                                       /* bNumEndpoints: Two endpoints used */
	0x0A,                                       /* bInterfaceClass: CDC */
	0x00,                                       /* bInterfaceSubClass */
	0x00,                                       /* bInterfaceProtocol */
	0x00,                                       /* iInterface */
	/* 9 */

	0x07,                                       /* bLength: Endpoint OUT Descriptor size */
	USB_DESC_TYPE_ENDPOINT,                     /* bDescriptorType: Endpoint */
	CDC_DEVCTLR_OUT_EP,                         /* bEndpointAddress */
	0x02,                                       /* bmAttributes: Bulk */
	LOBYTE(CDC_DEVCTLR_DATA_HS_MAX_PACKET_SIZE),/* wMaxPacketSize */
	HIBYTE(CDC_DEVCTLR_DATA_HS_MAX_PACKET_SIZE),
	0x00,                                       /* bInterval: ignore for Bulk transfer */
	/* 7 */

	0x07,                                       /* bLength: Endpoint IN Descriptor size */
	USB_DESC_TYPE_ENDPOINT,                     /* bDescriptorType: Endpoint */
	CDC_DEVCTLR_IN_EP,                          /* bEndpointAddress */
	0x02,                                       /* bmAttributes: Bulk */
	LOBYTE(CDC_DEVCTLR_DATA_HS_MAX_PACKET_SIZE),/* wMaxPacketSize */
	HIBYTE(CDC_DEVCTLR_DATA_HS_MAX_PACKET_SIZE),
	0x00,                                       /* bInterval: ignore for Bulk transfer */
	/* 7 */
#endif
	/**************************** HID Sensor (1 EP to HOST, EPIN) *************************/
	0x09,                                               /* bLength: Interface Descriptor size */
	USB_DESC_TYPE_INTERFACE,                            /* bDescriptorType: Interface descriptor type */
	USBD_HID_INTERFACE_SENSOR,                          /* bInterfaceNumber: Number of Interface */
	0x00,                                               /* bAlternateSetting: Alternate setting */
	0x01,                                               /* bNumEndpoints */
	0x03,                                               /* bInterfaceClass: CUSTOM_HID */
	0x00,                                               /* bInterfaceSubClass: 1=BOOT, 0=no boot */
	0x00,                                               /* nInterfaceProtocol: 0=none, 1=keyboard, 2=mouse */
	0x00,                                               /* iInterface: Index of string descriptor */
	/* 9 */

	0x09,                                               /* bLength: CUSTOM_HID Descriptor size */
	CUSTOM_HID_SENSOR_DESCRIPTOR_TYPE,                  /* bDescriptorType: CUSTOM_HID */
	0x11, 0x01,                                         /* bCUSTOM_HID: CUSTOM_HID Class Spec release number */
	0x00,                                               /* bCountryCode: Hardware target country */
	0x01,                                               /* bNumDescriptors: Number of CUSTOM_HID class descriptors to follow */
	0x22,                                               /* bDescriptorType */
	LOBYTE(USBD_CUSTOM_HID_SENSOR_REPORT_DESC_SIZE),    /* wItemLength: Total length of Report descriptor */
	HIBYTE(USBD_CUSTOM_HID_SENSOR_REPORT_DESC_SIZE),
	/* 9 */

	0x07,                                               /* bLength: Endpoint Descriptor size */
	USB_DESC_TYPE_ENDPOINT,                             /* bDescriptorType: Endpoint */
	CUSTOM_HID_SENSOR_EPIN_ADDR,                        /* bEndpointAddress: Endpoint Address (IN) */
	0x03,                                               /* bmAttributes: Interrupt endpoint */
	CUSTOM_HID_SENSOR_EPIN_SIZE,                        /* wMaxPacketSize: 2 Byte max */
	0x00,                                               /* wMaxPacketSize: High Byte (if needed) */
	CUSTOM_HID_SENSOR_HS_BINTERVAL,                     /* bInterval: Polling Interval */
	/* 7 */

	/**************************** HID Keyboard *******************************/
	0x09,                                               /* bLength: Interface Descriptor size */
	USB_DESC_TYPE_INTERFACE,                            /* bDescriptorType: Interface descriptor type */
	USBD_HID_INTERFACE_KEYBOARD,                        /* bInterfaceNumber: Number of Interface */
	0x00,                                               /* bAlternateSetting: Alternate setting */
	0x02,                                               /* bNumEndpoints */
	0x03,                                               /* bInterfaceClass: CUSTOM_HID */
	0x00,                                               /* bInterfaceSubClass: 1=BOOT, 0=no boot */
	0x00,                                               /* nInterfaceProtocol: 0=none, 1=keyboard, 2=mouse */
	0x00,                                               /* iInterface: Index of string descriptor */
	/* 9 */

	0x09,                                               /* bLength: CUSTOM_HID Descriptor size */
	CUSTOM_HID_KEYBOARD_DESCRIPTOR_TYPE,                /* bDescriptorType: CUSTOM_HID */
	0x11, 0x01,                                         /* bCUSTOM_HID: CUSTOM_HID Class Spec release number */
	0x00,                                               /* bCountryCode: Hardware target country */
	0x01,                                               /* bNumDescriptors: Number of CUSTOM_HID class descriptors to follow */
	0x22,                                               /* bDescriptorType */
	LOBYTE(USBD_CUSTOM_HID_KEYBOARD_REPORT_DESC_SIZE),  /* wItemLength: Total length of Report descriptor */
	HIBYTE(USBD_CUSTOM_HID_KEYBOARD_REPORT_DESC_SIZE),
	/* 9 */

	0x07,                                               /* bLength: Endpoint Descriptor size */
	USB_DESC_TYPE_ENDPOINT,                             /* bDescriptorType: Endpoint */
	CUSTOM_HID_KEYBOARD_EPIN_ADDR,                      /* bEndpointAddress: Endpoint Address (IN) */
	0x03,                                               /* bmAttributes: Interrupt endpoint */
	CUSTOM_HID_KEYBOARD_EPIN_SIZE,                      /* wMaxPacketSize: 2 Byte max */
	0x00,                                               /* wMaxPacketSize: High Byte (if needed) */
	CUSTOM_HID_KEYBOARD_HS_BINTERVAL,                   /* bInterval: Polling Interval */
	/* 7 */

	0x07,                                               /* bLength: Endpoint Descriptor size */
	USB_DESC_TYPE_ENDPOINT,                             /* bDescriptorType: Endpoint */
	CUSTOM_HID_KEYBOARD_EPOUT_ADDR,                     /* bEndpointAddress: Endpoint Address (OUT) */
	0x03,                                               /* bmAttributes: Interrupt endpoint */
	CUSTOM_HID_KEYBOARD_EPOUT_SIZE,                     /* wMaxPacketSize: 2 Byte max */
	0x00,                                               /* wMaxPacketSize: High Byte (if needed) */
	CUSTOM_HID_KEYBOARD_HS_BINTERVAL,                   /* bInterval: Polling Interval */
	/* 7 */
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
static uint8_t  USBD_Composite_Init (USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
	uint8_t res = 0;

	pdev->pUserData = &USBD_Interface_fops_HS;
	res +=  USBD_CDC.Init(pdev,cfgidx);
	pCDCData_Tof = pdev->pClassData;
#if ENABLE_DEVICECTL_CDC
	pdev->pUserData = &USBD_Interface_fops_DEVCTLR_HS;
	res +=  USBD_CDC_DEVCTLR.Init(pdev,cfgidx);
	pCDCData_Devctlr = pdev->pClassData;
#endif
	pdev->pUserData = &USBD_CustomHID_Sensor_fops_HS;
	res +=  USBD_CUSTOM_HID_SENSOR.Init(pdev,cfgidx);
	pHIDData_Sensor = pdev->pClassData;

	pdev->pUserData = &USBD_CustomHID_Keyboard_fops_HS;
	res +=  USBD_CUSTOM_HID_KEYBOARD.Init(pdev,cfgidx);
	pHIDData_Keyboard = pdev->pClassData;

	return res;
}

/**
  * @brief  USBD_Composite_DeInit
  *         DeInitilaize  the Composite configuration
  * @param  pdev: device instance
  * @param  cfgidx: configuration index
  * @retval status
  */
static uint8_t  USBD_Composite_DeInit (USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    uint8_t res = 0;

    pdev->pClassData = pCDCData_Tof;
    pdev->pUserData = &USBD_Interface_fops_HS;
    res +=  USBD_CDC.DeInit(pdev,cfgidx);
#if ENABLE_DEVICECTL_CDC
    pdev->pClassData = pCDCData_Devctlr;
    pdev->pUserData = &USBD_Interface_fops_DEVCTLR_HS;
    res +=  USBD_CDC_DEVCTLR.DeInit(pdev,cfgidx);
#endif
    pdev->pClassData = pHIDData_Sensor;
    pdev->pUserData = &USBD_CustomHID_Sensor_fops_HS;
    res +=  USBD_CUSTOM_HID_SENSOR.DeInit(pdev,cfgidx);

    pdev->pClassData = pHIDData_Keyboard;
    pdev->pUserData = &USBD_CustomHID_Keyboard_fops_HS;
    res +=  USBD_CUSTOM_HID_KEYBOARD.DeInit(pdev,cfgidx);

    return res;
}

static uint8_t USBD_Composite_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
	switch(pdev->request.wIndex) {
    	case USBD_CDC_CMD_INTERFACE:
	    case USBD_CDC_DATA_INTERFACE:
	        pdev->pClassData = pCDCData_Tof;
	        pdev->pUserData =  &USBD_Interface_fops_HS;
	        return(USBD_CDC.EP0_RxReady(pdev));
#if ENABLE_DEVICECTL_CDC
	    case USBD_CDC_DEVCTLR_CMD_INTERFACE:
	    case USBD_CDC_DEVCTLR_DATA_INTERFACE:
	        pdev->pClassData = pCDCData_Devctlr;
	        pdev->pUserData =  &USBD_Interface_fops_DEVCTLR_HS;
	        return(USBD_CDC_DEVCTLR.EP0_RxReady(pdev));
#endif
	    case USBD_HID_INTERFACE_SENSOR:
	        pdev->pClassData = pHIDData_Sensor;
	        pdev->pUserData =  &USBD_CustomHID_Sensor_fops_HS;
	        return(USBD_CUSTOM_HID_SENSOR.EP0_RxReady (pdev));

	    case USBD_HID_INTERFACE_KEYBOARD:
	        pdev->pClassData = pHIDData_Keyboard;
	        pdev->pUserData =  &USBD_CustomHID_Keyboard_fops_HS;
	        return(USBD_CUSTOM_HID_KEYBOARD.EP0_RxReady (pdev));

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
			switch(LOBYTE(req->wIndex))
			{
				case USBD_CDC_CMD_INTERFACE:
				case USBD_CDC_DATA_INTERFACE:
					pdev->pClassData = pCDCData_Tof;
					pdev->pUserData =  &USBD_Interface_fops_HS;
					return(USBD_CDC.Setup(pdev, req));
#if ENABLE_DEVICECTL_CDC
				case USBD_CDC_DEVCTLR_CMD_INTERFACE:
				case USBD_CDC_DEVCTLR_DATA_INTERFACE:
					pdev->pClassData = pCDCData_Devctlr;
					pdev->pUserData =  &USBD_Interface_fops_DEVCTLR_HS;
					return(USBD_CDC_DEVCTLR.Setup (pdev, req));
#endif
				case USBD_HID_INTERFACE_SENSOR:
					pdev->pClassData = pHIDData_Sensor;
					pdev->pUserData =  &USBD_CustomHID_Sensor_fops_HS;
					return(USBD_CUSTOM_HID_SENSOR.Setup (pdev, req));

				case USBD_HID_INTERFACE_KEYBOARD:
					pdev->pClassData = pHIDData_Keyboard;
					pdev->pUserData =  &USBD_CustomHID_Keyboard_fops_HS;
					return(USBD_CUSTOM_HID_KEYBOARD.Setup (pdev, req));

				default:
					break;
			}
			break;

	case USB_REQ_RECIPIENT_ENDPOINT:
		switch(LOBYTE(req->wIndex))
		{
			case CDC_IN_EP:
			case CDC_OUT_EP:
			case CDC_CMD_EP:
				pdev->pClassData = pCDCData_Tof;
				pdev->pUserData =  &USBD_Interface_fops_HS;
				return(USBD_CDC.Setup(pdev, req));
#if ENABLE_DEVICECTL_CDC
			case CDC_DEVCTLR_IN_EP:
			case CDC_DEVCTLR_OUT_EP:
			case CDC_DEVCTLR_CMD_EP:
				pdev->pClassData = pCDCData_Devctlr;
				pdev->pUserData =  &USBD_Interface_fops_DEVCTLR_HS;
				return(USBD_CDC_DEVCTLR.Setup(pdev, req));
#endif
			case CUSTOM_HID_SENSOR_EPIN_ADDR:
			case CUSTOM_HID_SENSOR_EPOUT_ADDR:
				pdev->pClassData = pHIDData_Sensor;
				pdev->pUserData =  &USBD_CustomHID_Sensor_fops_HS;
				return(USBD_CUSTOM_HID_SENSOR.Setup (pdev, req));

			case CUSTOM_HID_KEYBOARD_EPIN_ADDR:
			case CUSTOM_HID_KEYBOARD_EPOUT_ADDR:
				pdev->pClassData = pHIDData_Keyboard;
				pdev->pUserData =  &USBD_CustomHID_Keyboard_fops_HS;
				return(USBD_CUSTOM_HID_KEYBOARD.Setup (pdev, req));

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
uint8_t  USBD_Composite_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
	switch(epnum)
	{
		case CDC_INDATA_NUM:
			pdev->pClassData = pCDCData_Tof;
			pdev->pUserData =  &USBD_Interface_fops_HS;
			return(USBD_CDC.DataIn(pdev,epnum));
#if ENABLE_DEVICECTL_CDC
		case CDC_DEVCTLR_INDATA_NUM:
			pdev->pClassData = pCDCData_Devctlr;
			pdev->pUserData =  &USBD_Interface_fops_DEVCTLR_HS;
			return(USBD_CDC_DEVCTLR.DataIn(pdev,epnum));
#endif
		case HID_SENSOR_INDATA_NUM:
			pdev->pClassData = pHIDData_Sensor;
			pdev->pUserData =  &USBD_CustomHID_Sensor_fops_HS;
			return(USBD_CUSTOM_HID_SENSOR.DataIn(pdev,epnum));

		case HID_KEYBOARD_INDATA_NUM:
			pdev->pClassData = pHIDData_Keyboard;
			pdev->pUserData =  &USBD_CustomHID_Keyboard_fops_HS;
			return(USBD_CUSTOM_HID_KEYBOARD.DataIn(pdev,epnum));

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
uint8_t  USBD_Composite_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
	switch(epnum)
	{
		case CDC_OUTDATA_NUM:
		case CDC_OUTCMD_NUM:
			pdev->pClassData = pCDCData_Tof;
			pdev->pUserData =  &USBD_Interface_fops_HS;
			return(USBD_CDC.DataOut(pdev,epnum));
#if ENABLE_DEVICECTL_CDC
		case CDC_DEVCTLR_OUTDATA_NUM:
		case CDC_DEVCTLR_OUTCMD_NUM:
			pdev->pClassData = pCDCData_Devctlr;
			pdev->pUserData =  &USBD_Interface_fops_DEVCTLR_HS;
			return(USBD_CDC_DEVCTLR.DataOut(pdev,epnum));
#endif
		case HID_SENSOR_OUTDATA_NUM:
			pdev->pClassData = pHIDData_Sensor;
			pdev->pUserData =  &USBD_CustomHID_Sensor_fops_HS;
			return(USBD_CUSTOM_HID_SENSOR.DataOut(pdev,epnum));

		case HID_KEYBOARD_OUTDATA_NUM:
			pdev->pClassData = pHIDData_Keyboard;
			pdev->pUserData =  &USBD_CustomHID_Keyboard_fops_HS;
			return(USBD_CUSTOM_HID_KEYBOARD.DataOut(pdev,epnum));

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
#if ENABLE_DEVICECTL_CDC
	if (!pdev || !pCDCData_Tof || !pCDCData_Devctlr || !pHIDData_Sensor || !pHIDData_Keyboard) return;
#else
	if (!pdev || !pCDCData_Tof || !pHIDData_Sensor || !pHIDData_Keyboard) return;
#endif
	switch (interface) {
		case USBD_CDC_INTERFACE:
			pdev->pClassData = pCDCData_Tof;
			pdev->pUserData =  &USBD_Interface_fops_HS;
			break;
#if ENABLE_DEVICECTL_CDC
		case USBD_CDC_DEVCTLR_INTERFACE:
			pdev->pClassData = pCDCData_Devctlr;
			pdev->pUserData =  &USBD_Interface_fops_DEVCTLR_HS;
			break;
#endif
		case USBD_CUSTOMHID_SENSOR_INTERFACE:
			pdev->pClassData = pHIDData_Sensor;
			pdev->pUserData = &USBD_CustomHID_Sensor_fops_HS;
			break;
		case USBD_CUSTOMHID_KEYBOARD_INTERFACE:
			pdev->pClassData = pHIDData_Keyboard;
			pdev->pUserData = &USBD_CustomHID_Keyboard_fops_HS;
			break;
		default:
			break;
	}
}

USBD_COMPOSITE_ItfTypeDef USBD_Composite_Get_Current_Itf(USBD_HandleTypeDef *pdev)
{
	if(pdev->pClassData == pCDCData_Tof) {
		return USBD_CDC_INTERFACE;
	}
#if ENABLE_DEVICECTL_CDC
	else if(pdev->pClassData == pCDCData_Devctlr) {
		return USBD_CDC_DEVCTLR_INTERFACE;
	}
#endif
	else if(pdev->pClassData == pHIDData_Sensor) {
		return USBD_CUSTOMHID_SENSOR_INTERFACE;
	}
	return USBD_CUSTOMHID_KEYBOARD_INTERFACE;
}
/**
  * @}
  */


/**
  * @}
  */
