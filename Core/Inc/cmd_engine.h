#ifndef __CMD_ENGINE_H
#define __CMD_ENGINE_H
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "string.h"

#include "stm32f7xx_hal.h"
#include "main.h"
#include "debug_defs.h"
#include "vl53l8cx_api.h"
#include "ecx343.h"
#include "lt7911.h"
#include "flash_rw_process.h"
#include "usb.h"
#include "usbd_desc.h"

// Override USB configurations

typedef enum {
    CE_INVALID_CMD      = 0x0U,
    /* ToF Commands */
    //Set
    CE_SET_ECHO             = 0x1U,
    CE_SET_TOF_PWR          = 0x2U,
    CE_SET_TOF_MODE         = 0x3U,
    CE_SET_TOF_CONF         = 0x4U,
    CE_SET_TOF_CALDATA      = 0x5U,
    CE_SET_TOF_KEY          = 0x6U,
    //Get
    CE_GET_TOF_PWR          = 0x7U,
    CE_GET_TOF_MODE         = 0x8U,
    CE_GET_TOF_KEYCODE      = 0x9U,
    CE_GET_TOF_CONF         = 0x10U,
    CE_GET_TOF_CALDATA      = 0x11U,

    /* Devctlr Commands */
    //Set
    CE_SET_LCD_INVL         = 0x101U,
    CE_SET_LCD_INVR         = 0x102U,
    CE_SET_LCD_LUXL         = 0x103U,
    CE_SET_LCD_LUXR         = 0x104U,
    CE_SET_LCD_HORL         = 0x105U,
    CE_SET_LCD_HORR         = 0x106U,
    CE_SET_LCD_VORL         = 0x107U,
    CE_SET_LCD_VORR         = 0x108U,
    CE_SET_LCD_CLEAR        = 0x109U,
    CE_SET_LCD_DEFAULT      = 0x10AU,
    CE_SET_LCD_DEVICEL      = 0x10BU,
    CE_SET_LCD_DEVICER      = 0x10CU,
    CE_SET_LCD_MODE         = 0x10DU,
    CE_SET_HIGH_TEMP_BRIGHTNESS = 0x10EU,
    CE_SET_AUTO_BRIGHTNESS  = 0x10FU,
    CE_SET_DEBUG_MODE       = 0x110U,
    CE_SET_ENC_EN           = 0x111U,
    CE_SET_MIC_MUTE         = 0x112U,
   
    CE_SET_ALS_START        = 0x115U,    
    CE_SET_ALS_STOP         = 0x116U,    

    //Get
    CE_GET_ECHO             = 0x150U,
    CE_GET_FW_VER           = 0x151U,
    CE_GET_MCU_SN           = 0x152U,
    CE_GET_LCD_VALUE        = 0x153U,
    CE_GET_BRG_FW           = 0x154U,
    CE_GET_ALS_REG          = 0x155U,    
    CE_GET_PANEL_REG        = 0x156U,        

    // Enter ST Bootloader
    CE_ENTER_BOOTLOADER     = 0x170U,
    // Flash RW
    CE_FLASH_WRITE          = 0x171U,   //CE_ENTER_WFLASH
    CE_FLASH_READ           = 0x172U,   //CE_ENTER_RFLASH

    //i2c
    CE_I2C_SCAN_START       = 0x180U,
    CE_I2C_SCAN_STOP        = 0x181U,

    /* Trigger HID Keyboard Event */
    CE_HID_MIN              = 0x200U,
    CE_KEY_BASE             = 0x200U,
    CE_KEY_A                = CE_KEY_BASE + 0U,   // KEY_A; KEYCODE_A
    CE_KEY_D                = CE_KEY_BASE + 1U,   // KEY_D; KEYCODE_D
    CE_KEY_K                = CE_KEY_BASE + 2U,   // KEY_K; KEYCODE_K
    CE_KEY_L                = CE_KEY_BASE + 3U,   // KEY_L; KEYCODE_L
    CE_KEY_S                = CE_KEY_BASE + 4U,   // KEY_S; KEYCODE_S
    CE_KEY_W                = CE_KEY_BASE + 5U,   // KEY_W; KEYCODE_W
    CE_KEY_1                = CE_KEY_BASE + 6U,   // KEY_1; KEYCODE_1
    CE_KEY_2                = CE_KEY_BASE + 7U,   // KEY_2; KEYCODE_2
    CE_KEY_3                = CE_KEY_BASE + 8U,   // KEY_3; KEYCODE_3
    CE_KEY_4                = CE_KEY_BASE + 9U,   // KEY_4; KEYCODE_4
    CE_KEY_5                = CE_KEY_BASE + 10U,  // KEY_5; KEYCODE_5
    CE_KEY_6                = CE_KEY_BASE + 11U,  // KEY_6; KEYCODE_6
    CE_KEY_7                = CE_KEY_BASE + 12U,  // KEY_7; KEYCODE_7
    CE_KEY_8                = CE_KEY_BASE + 13U,  // KEY_8; KEYCODE_8
    CE_KEY_9                = CE_KEY_BASE + 14U,  // KEY_9; KEYCODE_9
    CE_KEY_0                = CE_KEY_BASE + 15U,  // KEY_0; KEYCODE_0
    CE_KEY_ENTER            = CE_KEY_BASE + 16U,  // KEY_ENTER; KEYCODE_ENTER
    //CE_KEY_TAB            = 0x201U,   // KEY_TAB; KEYCODE_TAB
    CE_KEY_PRTSC            = CE_KEY_BASE + 17U,  // KEY_SYSRQ; KEYCODE_SYSRQ
    CE_KEY_SCROLLLOCK       = CE_KEY_BASE + 18U,  // KEY_SCROLLLOCK; KEYCODE_SCROLL_LOCK
    CE_KEY_PAGEUP           = CE_KEY_BASE + 19U,  // KEY_PAGEUP; KEYCODE_PAGE_UP
    CE_KEY_PAGEDOWN         = CE_KEY_BASE + 20U,  // KEY_PAGEDOWN; KEYCODE_PAGE_DOWN
    CE_KEY_RIGHT            = CE_KEY_BASE + 21U,  // KEY_RIGHT; KEYCODE_DPAD_RIGHT
    CE_KEY_LEFT             = CE_KEY_BASE + 22U,  // KEY_LEFT; KEYCODE_DPAD_LEFT
    CE_KEY_DOWN             = CE_KEY_BASE + 23U,  // KEY_DOWN; KEYCODE_DPAD_DOWN
    CE_KEY_UP               = CE_KEY_BASE + 24U,  // KEY_UP; KEYCODE_DPAD_UP
    CE_KEY_COMPOSE          = CE_KEY_BASE + 25U,  // KEY_COMPOSE; KEYCODE_MENU
    CE_KEY_POWER            = CE_KEY_BASE + 26U,  // KEY_POWER; KEYCODE_POWER
    CE_KEY_MUTE             = CE_KEY_BASE + 27U,  // KEY_MUTE; KEYCODE_VOLUME_MUTE
    //CE_KEY_PLAY           = 0x20CU,   // KEY_PLAYPAUSE; KEYCODE_MEDIA_PLAY_PAUSE (NOT in HID Usage Tables)
    //CE_KEY_PREVIOUS       = 0x20DU,   // KEY_PREVIOUSSONG; KEYCODE_MEDIA_PREVIOUS (NOT in HID Usage Tables)
    //CE_KEY_NEXT           = 0x20EU,   // KEY_NEXTSONG; KEYCODE_MEDIA_NEXT (NOT in HID Usage Tables)
    CE_KEY_BACK             = CE_KEY_BASE + 28U,  // KEY_BACK; KEYCODE_BACK (NOT in HID Usage Tables)
    /* Trigger HID Consumer Event */
    CE_CR_BASE              = 0x21DU,
    CE_CR_SUBTITLE          = CE_CR_BASE + 0U,    // KEY_SUBTITLE; KEYCODE_CAPTIONS
    CE_CR_RED               = CE_CR_BASE + 1U,    // KEY_RED; KEYCODE_PROG_RED
    CE_CR_GREEN             = CE_CR_BASE + 2U,    // KEY_GREEN; KEYCODE_PROG_GREEN
    CE_CR_BLUE              = CE_CR_BASE + 3U,    // KEY_BLUE; KEYCODE_PROG_BLUE
    CE_CR_YELLOW            = CE_CR_BASE + 4U,    // KEY_YELLOW; KEYCODE_PROG_YELLOW
    CE_CR_BRTUP             = CE_CR_BASE + 5U,    // KEY_BRIGHTNESSUP; N/A
    CE_CR_BRTDOWN           = CE_CR_BASE + 6U,    // KEY_BRIGHTNESSDOWN; N/A
    //CE_CR_PHONE           = 0x21FU,   // KEY_PHONE; KEYCODE_CALL
    CE_CR_RECORD            = CE_CR_BASE + 7U,    // KEY_RECORD; KEYCODE_PROG_RED
    CE_CR_FORWARD           = CE_CR_BASE + 8U,    // KEY_FASTFORWARD; KEYCODE_MEDIA_FAST_FORWARD
    CE_CR_REWIND            = CE_CR_BASE + 9U,    // KEY_REWIND; KEYCODE_MEDIA_REWIND
    CE_CR_MUTE              = CE_CR_BASE + 10U,   // KEY_MUTE; KEYCODE_VOLUME_MUTE
    CE_CR_VOLUP             = CE_CR_BASE + 11U,   // KEY_VOLUMEUP; KEYCODE_VOLUME_UP
    CE_CR_VOLDOWN           = CE_CR_BASE + 12U,   // KEY_VOLUMEDOWN; KEYCODE_VOLUME_DOWN
    //CE_CR_MEDIA           = 0x225U,   // KEY_MEDIA; KEYCODE_HEADSETHOOK
    CE_CR_ASSISTANT         = CE_CR_BASE + 13U,   // KEY_ASSISTANT; N/A
    CE_CR_AC_SEARCH         = CE_CR_BASE + 14U,   // KEY_SEARCH; KEYCODE_SEARCH
    CE_CR_AC_HOME           = CE_CR_BASE + 15U,   // KEY_HOMEPAGE; KEYCODE_HOME
    CE_CR_AC_BACK           = CE_CR_BASE + 16U,   // KEY_BACK; KEYCODE_BACK
    CE_CR_AC_FORWARD        = CE_CR_BASE + 17U,   // KEY_FORWARD; KEYCODE_FORWARD
    CE_CR_AC_STOP           = CE_CR_BASE + 18U,   // KEY_STOP; KEYCODE_MEDIA_STOP
    CE_CR_NEXT              = CE_CR_BASE + 19U,   // KEY_NEXTSONG; KEYCODE_MEDIA_NEXT
    CE_CR_PREVIOUS          = CE_CR_BASE + 20U,   // KEY_PREVIOUSSONG; KEYCODE_MEDIA_PREVIOUS
    CE_CR_STOP              = CE_CR_BASE + 21U,   // KEY_STOPCD; KEYCODE_MEDIA_STOP
    CE_CR_PLAY              = CE_CR_BASE + 22U,   // KEY_PLAYPAUSE; KEYCODE_MEDIA_PLAY_PAUSE
    CE_CR_VOICE             = CE_CR_BASE + 23U,   // KEY_VOICECOMMAND; N/A
    CE_CR_ALTAUDIO          = CE_CR_BASE + 24U,   // N/A; KEYCODE_MEDIA_AUDIO_TRACK
    /* Trigger HID Gamepad Event */
    CE_GPAD_BASE            = 0x236U,
    CE_GPAD_BTN1            = CE_GPAD_BASE + 0U,  // A; BTN_A / BTN_SOUTH
    CE_GPAD_BTN2            = CE_GPAD_BASE + 1U,  // B; BTN_B / BTN_EAST
    CE_GPAD_BTN3            = CE_GPAD_BASE + 2U,  // C; BTN_C
    CE_GPAD_BTN4            = CE_GPAD_BASE + 3U,  // X; BTN_X / BTN_NORTH
    CE_GPAD_BTN5            = CE_GPAD_BASE + 4U,  // Y; BTN_Y / BTN_WEST
    CE_GPAD_BTN6            = CE_GPAD_BASE + 5U,  // Z; BTN_Z
    CE_GPAD_BTN7            = CE_GPAD_BASE + 6U,  // LB: BTN_TL
    CE_GPAD_BTN8            = CE_GPAD_BASE + 7U,  // RB; BTN_TR
    CE_GPAD_BTN9            = CE_GPAD_BASE + 8U,  // LT; BTN_TL2
    CE_GPAD_BTN10           = CE_GPAD_BASE + 9U,  // RT; BTN_TR2
    CE_GPAD_BTN11           = CE_GPAD_BASE + 10U, // Select; BTN_SELECT
    CE_GPAD_BTN12           = CE_GPAD_BASE + 11U, // Start; BTN_START
    CE_GPAD_BTN13           = CE_GPAD_BASE + 12U, // Mode; BTN_MODE
    CE_GPAD_BTN14           = CE_GPAD_BASE + 13U, // TL; BTN_THUMBL
    CE_GPAD_BTN15           = CE_GPAD_BASE + 14U, // TR; BTN_THUMBR
    CE_KEY_NONE             = 0x2FFU,
    CE_HID_MAX              = 0x2FFU,

} CE_CmdTypeDef;


void CE_Execute_Command(CE_CmdTypeDef cmd, uint8_t* args, uint32_t args_len);
void CE_Parse_ToF_Cmd_Data(uint8_t* cmd_buf, uint32_t cmd_buf_len);
void CE_Parse_Devctlr_Cmd_Data(uint8_t* cmd_buf, uint32_t cmd_buf_len);
void Ecx343_data_init_default(void);
void Ecx343_data_update(void);
uint32_t Cal_Ecx343_data_checksum(ECX343_DATA data);
int Check_Ecx343_data_checksum(ECX343_DATA data);
uint8_t LcdHorbit(int value);
uint8_t LcdVorbit(int value);

#if ENABLE_TOF
extern bool bRangePacketUpdated;
extern uint8_t tof_resetFlag;
void tof_ranging_callback(VL53L8CX_ResultsData* range_data, uint32_t time_stamp);
#endif

#endif /* __CMD_ENGINE_H */
