#ifndef __DEBUG_DEFS_H_06d4ee22_3e0a_11ed_8d2b_33591b182c7b__
#define __DEBUG_DEFS_H_06d4ee22_3e0a_11ed_8d2b_33591b182c7b__

/******************************************************************/
/* This debug handler is only used in debug version if we want to */
/* use CDC virtual COM interface to issue the debug command in    */
/* testing the device or command etc..                            */
/******************************************************************/
#define ENABLE_CDC_DEBUG_HANDLER        0
#define ENABLE_CDC_ENGINEERING_TEST     0
#define ENABLE_CDC_DEFAULT_ECHOBACK     0
#define ENABLE_CDC_PRINT_CMD_ENGINE     0
#define ENABLE_CMD                      0
#define ENABLE_TOF                      1
#define ENABLE_TOF_FORCE_RESET          0
#define ENABLE_TOF_15HZ                 1
#define ENABLE_TOF_DEBUG                0
#define ENABLE_OLD_TOF                  0
#define ENABLE_IMU                      1
#define ENABLE_MIS                      0
#define MIC_DOWNSAMPLING                0
#define MIC_UPSAMPLING                  0

#define ENABLE_ALS                      1
#define ENABLE_PS                       1
#define ENABLE_ADC                      1
#define ENABLE_SCAN_I2C                 0

#define REDUCE_BRIGHTNESS_ON_HIGH_TEMP  0
#define ENABLE_PANEL                    0

#define ADD_HID_KEYBOARD                0
#define ENABLE_HID_KEYBOARD_TEST        0

#define ENABLE_CDC_CMD_PORT             1

#define ENABLE_AUDIO_PROCESS            0
#define ENABLE_FAKE_DATA                0



#define _SAMEPLE_SIZE   378     //324
#define _PACK_SIZE      375    //1
#define _RESAMPLE_SIZE  126     //108
#define _DMA_SIZE       1
#define _DMA_LOOP       (_RESAMPLE_SIZE/_DMA_SIZE)

#define RESAMPLE_BUFFER_SIZE (AUDIO_IN_PACKET*_SAMEPLE_SIZE/2)
#define RING_BUFFER_SIZE (AUDIO_IN_PACKET*_PACK_SIZE/2)


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

