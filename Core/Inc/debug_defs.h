#ifndef __DEBUG_DEFS_H_06d4ee22_3e0a_11ed_8d2b_33591b182c7b__
#define __DEBUG_DEFS_H_06d4ee22_3e0a_11ed_8d2b_33591b182c7b__

/******************************************************************/
/* This debug handler is only used in debug version if we want to */
/* use CDC virtual COM interface to issue the debug command in    */
/* testing the device or command etc..                            */
/******************************************************************/
#define ENABLE_CDC_DEFAULT_ECHOBACK     0

#define ENABLE_TOF                      0
#define ENABLE_TOF_FORCE_RESET          0
#define ENABLE_TOF_DEBUG                0

#define ENABLE_IMU                      0
#define ENABLE_TOUCH                    1
#define ENABLE_ALS                      1
#define ENABLE_PS                       0
#define ENABLE_ADC                      1
#define ENABLE_SCAN_I2C                 0

#define ENABLE_PANEL                    1

#define ENABLE_KEYBOARD_BUTTON_TEST     0

#define ENABLE_COUNTING_TASK            0
//USB_HS
#define ENABLE_DEVICECTL_CDC            1


#define ENABLE_POWER_BUTTON_IRQ         0

#if defined DEBUG
  /* When this is enabled, MUST open the CDC else it will lock!!! */
  //#define ENABLE_CDC_DEVCTLR_LOAD_PRINT   1
  #define ENABLE_CDC_DEVCTLR_LOAD_PRINT   0
  #define ENABLE_STACK_CHECK              0
#else
  #define ENABLE_CDC_DEVCTLR_LOAD_PRINT   0
  #define ENABLE_STACK_CHECK              0
  //#error
#endif

#endif /* __DEBUG_DEFS_H_06d4ee22_3e0a_11ed_8d2b_33591b182c7b__ */

