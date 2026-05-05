#include "cmd_engine.h"

#define MAX_TOF_DATA_COUNT (8 * 8 * 1)
#define TOF_4X4_DATA_PACKET_SIZE (4 + 1 + 1 + 4 + (1 + 4 + 2 + 1 + 1) * (MAX_TOF_DATA_COUNT / 4) + 4 + 4)
#define TOF_8X8_DATA_PACKET_SIZE (4 + 1 + 1 + 4 + (1 + 4 + 2 + 1 + 1) * MAX_TOF_DATA_COUNT + 4 + 4)
#define TOF_DATA_PREFIX 0x41544144 /* "DATA" */
#define TOF_DATA_SUFFIX 0x0a0d4445 /* "ED\r\n" */

#define MAX_CMD_RESP_LENGTH 256

extern uint8_t isDebugModeEnabled;
extern uint8_t isAutoBrightnessEnabled;
extern uint8_t isHighTempBrightnessEnabled;
extern uint8_t encSwitch;
extern uint8_t micSwitch;

extern uint32_t ambientLight;

uint8_t tofNumOfTargets[MAX_TOF_DATA_COUNT];
uint8_t tofRangePacket[TOF_8X8_DATA_PACKET_SIZE];
uint8_t tof_resetFlag = 0;
bool bRangePacketUpdated = false;
bool Cmd_Output = 0;

ECX343_DATA ecx343_data;
ECX343_DATA ecx343_current_data;
uint32_t ecx343RW_buf[sizeof(ECX343_DATA) / 4];

const uint32_t precenseKey = (1UL << (CE_KEY_SCROLLLOCK - CE_KEY_BASE));

static uint8_t CeCmdRespTxBuffer[MAX_CMD_RESP_LENGTH];

#if USE_USB_TX_TASK
static JQueueMessage_t keyReport;
static JQueueMessage_t cdcData;
#else
#error "USE_USB_TX_TASK or you may have race condition!!!"
#endif

typedef struct _Meta_Data {
    uint8_t* CmdData;
    uint32_t CmdLength;
    uint8_t* ResidualData;
    uint32_t ResidualLength;
} Meta_Data;

typedef struct _Command {
    CE_CmdTypeDef Cmd;
    uint8_t* Args;
    uint32_t ArgsLength;
} Command;

typedef enum {
    CE_ERR_COMMAND 		= 0,
    CE_ERR_PARAMETER 		= 1,
    CE_ERR_DEV_ACCESS 	= 2,
    CE_ERR_SYS_STATE 		= 3,
    CE_ERR_OTHERS 		= 4,
    CE_ERR_NO_ERROR 		= 255,
} CE_ErrTypeDef;

static Meta_Data get_data(uint8_t* cmd_buf, uint32_t cmd_len);
static Command string_to_command(char* str, uint32_t len);

#if USE_USB_TX_TASK
#define ENABLE_USB_SEND_MSG 1
static void usbhid_sendReport(char* pReport, int nLength) {
    keyReport.type = USB_HID_KEY_INPUT_REPORT;
    keyReport.data.keyReport.len = nLength;

    if ((0 < nLength) && (CUSTOM_HID_KEYBOARD_EPIN_SIZE  >= nLength)) {
        memcpy(keyReport.data.keyReport.report, pReport, nLength);
#if ENABLE_USB_SEND_MSG
        usbSendMessage(&keyReport);
#endif
        taskYIELD();
    }
}

void usbcdc_sendData(uint8_t* p, int nLength) {
    cdcData.type = USB_CDC_TOF_DATA;
    cdcData.data.ToFMsg.len = nLength;
    memcpy(cdcData.data.ToFMsg.p, p, nLength);
    // It is better to confirm the pass in pointer is in static address.
    // cdcData.data.ToFMsg.p   = p;

#if ENABLE_USB_SEND_MSG
    usbSendMessage(&cdcData);
#endif
    // taskYIELD(); //Not from ISR Task anymore.

}
#endif

void CE_Parse_ToF_Cmd_Data(uint8_t* cmd_buf, uint32_t cmd_buf_len) {
    Meta_Data meta_data;
    uint8_t* buf = cmd_buf;
    uint32_t len = cmd_buf_len;

    do {
        Command cmd;
        Cmd_Output = 1;

        meta_data = get_data(buf, len);
        cmd = string_to_command((char*)meta_data.CmdData, meta_data.CmdLength);

#if ENABLE_DEVICECTL_CDC
        if (cmd.Cmd <= 0x100U || cmd.Cmd >= 0x200U || cmd.Cmd == CE_GET_FW_VER) {
            CE_Execute_Command(cmd.Cmd, cmd.Args, cmd.ArgsLength);
        } else {
            usbEcho_Tof("NG %d", CE_ERR_COMMAND);
        }
#else
        CE_Execute_Command(cmd.Cmd, cmd.Args, cmd.ArgsLength);
#endif

        buf = meta_data.ResidualData;
        len = meta_data.ResidualLength;
    } while (buf && len);
}

void CE_Parse_Devctlr_Cmd_Data(uint8_t* cmd_buf, uint32_t cmd_buf_len) {
    Meta_Data meta_data;
    uint8_t* buf = cmd_buf;
    uint32_t len = cmd_buf_len;

    do {
        Command cmd;
        Cmd_Output = 0;

        meta_data = get_data(buf, len);
        cmd = string_to_command((char*)meta_data.CmdData, meta_data.CmdLength);
        if (cmd.Cmd > 0x100U && cmd.Cmd < 0x200U) {
            CE_Execute_Command(cmd.Cmd, cmd.Args, cmd.ArgsLength);
        } else {
            usbDebug("NG %d", CE_ERR_COMMAND);
        }
        buf = meta_data.ResidualData;
        len = meta_data.ResidualLength;
    } while (buf && len);
}

static Meta_Data get_data(uint8_t* cmd_buf, uint32_t cmd_len) {
    uint8_t* buf = cmd_buf;
    uint32_t checked_len = 0;
    Meta_Data meta_data;

    meta_data.CmdData = cmd_buf;
    meta_data.CmdLength = cmd_len;
    meta_data.ResidualData = NULL;
    meta_data.ResidualLength = 0;

    if (cmd_len) {
        do {
            if (*buf == '\n' || *buf++ == '\r') {
                uint32_t residual_len = cmd_buf + cmd_len - buf;
                // find the next non-new-line character
                while ((*buf == '\n' || *buf == '\r') && residual_len > 0) {
                    residual_len--;
                    buf++;
                }
                meta_data.CmdLength = checked_len;
                meta_data.ResidualData = buf;
                meta_data.ResidualLength = residual_len;
                break;
            }
        } while (cmd_len > ++checked_len);
    }

    return meta_data;
}

static Command string_to_command(char* str, uint32_t len) {
    Command cmd = { CE_INVALID_CMD, NULL };

    // all supported commands are longer than 4 characters
    if (len > 4) {
        uint32_t buf_offset = 0;

        if (!strncmp(str, "set", 3)) {
            if (!strncmp(str + 3, "echo", 4)) {
                cmd.Cmd = CE_SET_ECHO;
                buf_offset = 7;
            } else if (!strncmp(str + 3, "tofpwr", 6)) {
                cmd.Cmd = CE_SET_TOF_PWR;
                buf_offset = 9;
            } else if (!strncmp(str + 3, "tofmode", 7)) {
                cmd.Cmd = CE_SET_TOF_MODE;
                buf_offset = 10;
            } else if (!strncmp(str + 3, "tofconf", 7)) {
                cmd.Cmd = CE_SET_TOF_CONF;
                buf_offset = 10;
            } else if (!strncmp(str + 3, "tofcaldata", 10)) {
                cmd.Cmd = CE_SET_TOF_CALDATA;
                buf_offset = 13;
            } else if (!strncmp(str + 3, "tofkey", 6)) {
                cmd.Cmd = CE_SET_TOF_KEY;
                buf_offset = 9;
            } else if (!strncmp(str + 3, "lcdinvl", 7)) {
                cmd.Cmd = CE_SET_LCD_INVL;
                buf_offset = 10;
            } else if (!strncmp(str + 3, "lcdinvr", 7)) {
                cmd.Cmd = CE_SET_LCD_INVR;
                buf_offset = 10;
            } else if (!strncmp(str + 3, "lcdluxl", 7)) {
                cmd.Cmd = CE_SET_LCD_LUXL;
                buf_offset = 10;
            } else if (!strncmp(str + 3, "lcdluxr", 7)) {
                cmd.Cmd = CE_SET_LCD_LUXR;
                buf_offset = 10;
            } else if (!strncmp(str + 3, "lcdhorl", 7)) {
                cmd.Cmd = CE_SET_LCD_HORL;
                buf_offset = 10;
            } else if (!strncmp(str + 3, "lcdhorr", 7)) {
                cmd.Cmd = CE_SET_LCD_HORR;
                buf_offset = 10;
            } else if (!strncmp(str + 3, "lcdvorl", 7)) {
                cmd.Cmd = CE_SET_LCD_VORL;
                buf_offset = 10;
            } else if (!strncmp(str + 3, "lcdvorr", 7)) {
                cmd.Cmd = CE_SET_LCD_VORR;
                buf_offset = 10;
            } else if (!strncmp(str + 3, "lcdclear", 8)) {
                cmd.Cmd = CE_SET_LCD_CLEAR;
                buf_offset = 11;
            } else if (!strncmp(str + 3, "lcddefault", 10)) {
                cmd.Cmd = CE_SET_LCD_DEFAULT;
                buf_offset = 13;
            } else if (!strncmp(str + 3, "lcddevicel", 10)) {
                cmd.Cmd = CE_SET_LCD_DEVICEL;
                buf_offset = 13;
            } else if (!strncmp(str + 3, "lcddevicer", 10)) {
                cmd.Cmd = CE_SET_LCD_DEVICER;
                buf_offset = 13;
            } else if (!strncmp(str + 3, "lcdmode", 7)) {
                cmd.Cmd = CE_SET_LCD_MODE;
                buf_offset = 10;
            } else if (!strncmp(str + 3, "tempbn", 6)) {
                cmd.Cmd = CE_SET_HIGH_TEMP_BRIGHTNESS;
                buf_offset = 11;
            } else if (!strncmp(str + 3, "autobn", 6)) {
                cmd.Cmd = CE_SET_AUTO_BRIGHTNESS;
                buf_offset = 11;
            } else if (!strncmp(str + 3, "debug", 5)) {
                cmd.Cmd = CE_SET_DEBUG_MODE;
                buf_offset = 8;
            } else if (!strncmp(str + 3, "enc", 3)) {
                cmd.Cmd = CE_SET_ENC_EN;
                buf_offset = 6;
            } else if (!strncmp(str + 3, "mic", 3)) {
                cmd.Cmd = CE_SET_MIC_MUTE;
                buf_offset = 6;
            }
        } else if (!strncmp(str, "get", 3)) {
            if (!strncmp(str + 3, "echo", 4)) {
                cmd.Cmd = CE_GET_ECHO;
                buf_offset = 7;
            } else if (!strncmp(str + 3, "tofpwr", 6)) {
                cmd.Cmd = CE_GET_TOF_PWR;
                buf_offset = 9;
            } else if (!strncmp(str + 3, "tofmode", 7)) {
                cmd.Cmd = CE_GET_TOF_MODE;
                buf_offset = 10;
            } else if (!strncmp(str + 3, "tofkeycode", 7)) {
                cmd.Cmd = CE_GET_TOF_KEYCODE;
                buf_offset = 13;
            } else if (!strncmp(str + 3, "tofconf", 7)) {
                cmd.Cmd = CE_GET_TOF_CONF;
                buf_offset = 10;
            } else if (!strncmp(str + 3, "tofcaldata", 10)) {
                cmd.Cmd = CE_GET_TOF_CALDATA;
                buf_offset = 13;
            } else if (!strncmp(str + 3, "firmver", 7)) {
                cmd.Cmd = CE_GET_FW_VER;
                buf_offset = 10;
            } else if (!strncmp(str + 3, "hwsn", 4)) {
                cmd.Cmd = CE_GET_MCU_SN;
                buf_offset = 7;
            } else if (!strncmp(str + 3, "lcdvalue", 8)) {
                cmd.Cmd = CE_GET_LCD_VALUE;
                buf_offset = 11;
            } else if (!strncmp(str + 3, "brgfw", 5)) {
                cmd.Cmd = CE_GET_BRG_FW;
                buf_offset = 8;
            } else if (!strncmp(str + 3, "alsreg", 6)) {
                cmd.Cmd = CE_GET_ALS_REG;
                buf_offset = 9;
            }
        } else if (!strncmp(str, "key", 3)) {
            if (!strncmp(str + 3, "ltr", 3)) {
                if (str[6] == 'a') {
                    cmd.Cmd = CE_KEY_A;
                    buf_offset = 7;
                } else if (str[6] == 'd') {
                    cmd.Cmd = CE_KEY_D;
                    buf_offset = 7;
                } else if (str[6] == 'k') {
                    cmd.Cmd = CE_KEY_K;
                    buf_offset = 7;
                } else if (str[6] == 'l') {
                    cmd.Cmd = CE_KEY_L;
                    buf_offset = 7;
                } else if (str[6] == 's') {
                    cmd.Cmd = CE_KEY_S;
                    buf_offset = 7;
                } else if (str[6] == 'w') {
                    cmd.Cmd = CE_KEY_W;
                    buf_offset = 7;
                }
            } else if (!strncmp(str + 3, "num", 3)) {
                if (str[6] == '1') {
                    cmd.Cmd = CE_KEY_1;
                    buf_offset = 7;
                } else if (str[6] == '2') {
                    cmd.Cmd = CE_KEY_2;
                    buf_offset = 7;
                } else if (str[6] == '3') {
                    cmd.Cmd = CE_KEY_3;
                    buf_offset = 7;
                } else if (str[6] == '4') {
                    cmd.Cmd = CE_KEY_4;
                    buf_offset = 7;
                } else if (str[6] == '5') {
                    cmd.Cmd = CE_KEY_5;
                    buf_offset = 7;
                } else if (str[6] == '6') {
                    cmd.Cmd = CE_KEY_6;
                    buf_offset = 7;
                } else if (str[6] == '7') {
                    cmd.Cmd = CE_KEY_7;
                    buf_offset = 7;
                } else if (str[6] == '8') {
                    cmd.Cmd = CE_KEY_8;
                    buf_offset = 7;
                } else if (str[6] == '9') {
                    cmd.Cmd = CE_KEY_9;
                    buf_offset = 7;
                } else if (str[6] == '0') {
                    cmd.Cmd = CE_KEY_0;
                    buf_offset = 7;
                }
            } else if (!strncmp(str + 3, "enter", 5)) {
                cmd.Cmd = CE_KEY_ENTER;
                buf_offset = 8;
                /*
                } else if (!strncmp(str+3, "tab", 3)) {
                    cmd.Cmd =  CE_KEY_TAB;
                    buf_offset = 6;
                */
            } else if (!strncmp(str + 3, "prtsc", 5)) {
                cmd.Cmd = CE_KEY_PRTSC;
                buf_offset = 8;
            } else if (!strncmp(str + 3, "scrolllock", 10)) {
                cmd.Cmd = CE_KEY_PRTSC;
                buf_offset = 13;
            } else if (!strncmp(str + 3, "pageup", 6)) {
                cmd.Cmd = CE_KEY_PAGEUP;
                buf_offset = 9;
            } else if (!strncmp(str + 3, "pagedown", 8)) {
                cmd.Cmd = CE_KEY_PAGEDOWN;
                buf_offset = 11;
            } else if (!strncmp(str + 3, "right", 5)) {
                cmd.Cmd = CE_KEY_RIGHT;
                buf_offset = 8;
            } else if (!strncmp(str + 3, "left", 4)) {
                cmd.Cmd = CE_KEY_LEFT;
                buf_offset = 7;
            } else if (!strncmp(str + 3, "down", 4)) {
                cmd.Cmd = CE_KEY_DOWN;
                buf_offset = 7;
            } else if (!strncmp(str + 3, "up", 2)) {
                cmd.Cmd = CE_KEY_UP;
                buf_offset = 5;
            } else if (!strncmp(str + 3, "compose", 7)) {
                cmd.Cmd = CE_KEY_COMPOSE;
                buf_offset = 10;
            } else if (!strncmp(str + 3, "power", 5)) {
                cmd.Cmd = CE_KEY_POWER;
                buf_offset = 8;
            } else if (!strncmp(str + 3, "mute", 4)) {
                cmd.Cmd = CE_KEY_MUTE;
                buf_offset = 7;
                /*} else if (!strncmp(str + 3, "play", 4)) {
                    cmd.Cmd = CE_KEY_PLAY;
                    buf_offset = 7;
                } else if (!strncmp(str + 3, "previous", 8)) {
                    cmd.Cmd = CE_KEY_PREVIOUS;
                    buf_offset = 11;
                } else if (!strncmp(str + 3, "next", 4)) {
                    cmd.Cmd = CE_KEY_NEXT;
                    buf_offset = 7;*/
            } else if (!strncmp(str + 3, "back", 4)) {
                cmd.Cmd = CE_KEY_BACK;
                buf_offset = 7;
            } else if (!strncmp(str + 3, "none", 4)) {
                cmd.Cmd = CE_KEY_NONE;
                buf_offset = 7;
            }
        } else if (!strncmp(str, "cr", 2)) {
            if (!strncmp(str + 2, "subtitle", 8)) {
                cmd.Cmd = CE_CR_SUBTITLE;
                buf_offset = 10;
            } else if (!strncmp(str + 2, "red", 3)) {
                cmd.Cmd = CE_CR_RED;
                buf_offset = 5;
            } else if (!strncmp(str + 2, "green", 5)) {
                cmd.Cmd = CE_CR_GREEN;
                buf_offset = 7;
            } else if (!strncmp(str + 2, "blue", 4)) {
                cmd.Cmd = CE_CR_BLUE;
                buf_offset = 6;
            } else if (!strncmp(str + 2, "yellow", 6)) {
                cmd.Cmd = CE_CR_YELLOW;
                buf_offset = 8;
            } else if (!strncmp(str + 2, "brtup", 5)) {
                cmd.Cmd = CE_CR_BRTUP;
                buf_offset = 7;
            } else if (!strncmp(str + 2, "brtdown", 7)) {
                cmd.Cmd = CE_CR_BRTDOWN;
                buf_offset = 9;
                /*} else if (!strncmp(str + 2, "phone", 5)) {
                    cmd.Cmd = CE_CR_PHONE;
                    buf_offset = 7;*/
            } else if (!strncmp(str + 2, "record", 6)) {
                cmd.Cmd = CE_CR_RECORD;
                buf_offset = 8;
            } else if (!strncmp(str + 2, "forward", 7)) {
                cmd.Cmd = CE_CR_FORWARD;
                buf_offset = 9;
            } else if (!strncmp(str + 2, "rewind", 6)) {
                cmd.Cmd = CE_CR_REWIND;
                buf_offset = 8;
            } else if (!strncmp(str + 2, "mute", 4)) {
                cmd.Cmd = CE_CR_MUTE;
                buf_offset = 6;
            } else if (!strncmp(str + 2, "volup", 5)) {
                cmd.Cmd = CE_CR_VOLUP;
                buf_offset = 7;
            } else if (!strncmp(str + 2, "voldown", 7)) {
                cmd.Cmd = CE_CR_VOLDOWN;
                buf_offset = 9;
                /*} else if (!strncmp(str + 2, "media", 5)) {
                    cmd.Cmd = CE_CR_MEDIA;
                    buf_offset = 7;*/
            } else if (!strncmp(str + 2, "assistant", 9)) {
                cmd.Cmd = CE_CR_ASSISTANT;
                buf_offset = 11;
            } else if (!strncmp(str + 2, "ac", 2)) {
                if (!strncmp(str + 4, "search", 6)) {
                    cmd.Cmd = CE_CR_AC_SEARCH;
                    buf_offset = 10;
                } else if (!strncmp(str + 4, "home", 4)) {
                    cmd.Cmd = CE_CR_AC_HOME;
                    buf_offset = 8;
                } else if (!strncmp(str + 4, "back", 4)) {
                    cmd.Cmd = CE_CR_AC_BACK;
                    buf_offset = 8;
                } else if (!strncmp(str + 4, "forward", 7)) {
                    cmd.Cmd = CE_CR_AC_FORWARD;
                    buf_offset = 11;
                } else if (!strncmp(str + 4, "stop", 4)) {
                    cmd.Cmd = CE_CR_AC_STOP;
                    buf_offset = 8;
                }
            } else if (!strncmp(str + 2, "next", 4)) {
                cmd.Cmd = CE_CR_NEXT;
                buf_offset = 6;
            } else if (!strncmp(str + 2, "previous", 8)) {
                cmd.Cmd = CE_CR_PREVIOUS;
                buf_offset = 10;
            } else if (!strncmp(str + 2, "stop", 4)) {
                cmd.Cmd = CE_CR_STOP;
                buf_offset = 6;
            } else if (!strncmp(str + 2, "play", 4)) {
                cmd.Cmd = CE_CR_PLAY;
                buf_offset = 6;
            } else if (!strncmp(str + 2, "voice", 5)) {
                cmd.Cmd = CE_CR_VOICE;
                buf_offset = 7;
            } else if (!strncmp(str + 2, "altaudio", 8)) {
                cmd.Cmd = CE_CR_ALTAUDIO;
                buf_offset = 10;
            }
        } else if (!strncmp(str, "gpad", 4)) {
            if (!strncmp(str + 4, "btn", 3)) {
                if (str[7] == 'a') {
                    cmd.Cmd = CE_GPAD_BTN1;
                    buf_offset = 8;
                } else if (str[7] == 'b') {
                    cmd.Cmd = CE_GPAD_BTN2;
                    buf_offset = 8;
                } else if (str[7] == 'c') {
                    cmd.Cmd = CE_GPAD_BTN3;
                    buf_offset = 8;
                } else if (str[7] == 'x') {
                    cmd.Cmd = CE_GPAD_BTN4;
                    buf_offset = 8;
                } else if (str[7] == 'y') {
                    cmd.Cmd = CE_GPAD_BTN5;
                    buf_offset = 8;
                } else if (str[7] == 'z') {
                    cmd.Cmd = CE_GPAD_BTN6;
                    buf_offset = 8;
                } else if (!strncmp(str + 7, "lb", 2)) {
                    cmd.Cmd = CE_GPAD_BTN7;
                    buf_offset = 9;
                } else if (!strncmp(str + 7, "rb", 2)) {
                    cmd.Cmd = CE_GPAD_BTN8;
                    buf_offset = 9;
                } else if (!strncmp(str + 7, "lt", 2)) {
                    cmd.Cmd = CE_GPAD_BTN9;
                    buf_offset = 9;
                } else if (!strncmp(str + 7, "rt", 2)) {
                    cmd.Cmd = CE_GPAD_BTN10;
                    buf_offset = 9;
                } else if (!strncmp(str + 7, "select", 6)) {
                    cmd.Cmd = CE_GPAD_BTN11;
                    buf_offset = 13;
                } else if (!strncmp(str + 7, "start", 5)) {
                    cmd.Cmd = CE_GPAD_BTN12;
                    buf_offset = 12;
                } else if (!strncmp(str + 7, "mode", 4)) {
                    cmd.Cmd = CE_GPAD_BTN13;
                    buf_offset = 11;
                } else if (!strncmp(str + 7, "tl", 2)) {
                    cmd.Cmd = CE_GPAD_BTN14;
                    buf_offset = 9;
                } else if (!strncmp(str + 7, "tr", 2)) {
                    cmd.Cmd = CE_GPAD_BTN15;
                    buf_offset = 9;
                }
            }
        } else if (!strncmp(str, "enter", 5)) {
            if (!strncmp(str + 5, "bootloader", 10)) {
                cmd.Cmd = CE_ENTER_BOOTLOADER;
                buf_offset = 15;
            }
        } else if (!strncmp(str, "flash", 5)) {
            if (!strncmp(str + 5, "w", 1)) { // CE_ENTER_RFLASH
                cmd.Cmd = CE_FLASH_WRITE;
                buf_offset = 6;
            } else if (!strncmp(str + 5, "r", 1)) { // CE_ENTER_RFLASH
                cmd.Cmd = CE_FLASH_READ;
                buf_offset = 6;
            }
        }

        if (len > buf_offset) {
            cmd.ArgsLength = len - buf_offset;
            cmd.Args = (uint8_t*)str + buf_offset;
        }
    }

    return cmd;
}

void CE_Execute_Command(CE_CmdTypeDef cmd, uint8_t* args, uint32_t args_len) {
    char* data = (char*)args;
    char* reply = (char*)CeCmdRespTxBuffer;
    int value = 0;

    // Add prefix new line characters if the echo back feature is enabled
    if (CDC_Echo_Ctrl_Flag)
        reply += sprintf(reply, "\r\n");

    switch (cmd) {
        /* set command set */
    case CE_SET_ECHO:
        if (args_len == 0) {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
            break;
        }
        value = strtol(data, &data, 10);
        if (value == 0 && *(data - 1) != '0') {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        } else if (value <= 1) {
            char* padding = ((value == 0) != CDC_Echo_Ctrl_Flag) ? "" : "\r\n";
            CDC_Echo_Ctrl_Flag = (value == 1) ? 1 : 0;
            reply += sprintf(reply, "%sOK", padding);
        } else {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        }
        break;

    case CE_SET_TOF_PWR:
        if (args_len == 0) {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
            break;
        }
        value = strtol(data, &data, 10);
        if (value == 0 && *(data - 1) == '0') {
            HAL_NVIC_DisableIRQ(EXTI1_IRQn);
            interruptTofEnable = 0;
            reply += sprintf(reply, "OK");
        } else if (value == 2) {
            tof_resetFlag = 1;
            reply += sprintf(reply, "OK");
        } else if (value == 3) {
            usbDebug("tofResetCount: %d \r\n", tofResetCount);
            //            reply += sprintf(reply, "OK");
        } else {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        }
        break;

    case CE_SET_TOF_MODE:
        reply += sprintf(reply, "OK");
        break;

    case CE_SET_TOF_CONF:
        if (!args_len) {
			HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
			HAL_NVIC_EnableIRQ(EXTI1_IRQn);
			interruptTofEnable = 1;
			reply += sprintf(reply, "OK");
		} else {
			reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
		}
        break;

    case CE_SET_TOF_KEY:
        reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        break;

    case CE_SET_LCD_INVL:
    {
        char* endPtrInvl = NULL;
        if (args_len == 0) {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
            break;
        }
        value = strtol(data, &endPtrInvl, 10);
        if (endPtrInvl == data || value < 0 || value > 3) {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        } else {
            ecx343_current_data.uLCD_INVL = (uint8_t)value;
            executeTaskWithMutex(INVERSION, PANEL_LEFT, (uint8_t)value);
            reply += sprintf(reply, "OK");
        }
    }
    break;

    case CE_SET_LCD_INVR:
    {
        char* endPtrInvr = NULL;
        if (args_len == 0) {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
            break;
        }
        value = strtol(data, &endPtrInvr, 10);
        if (endPtrInvr == data || value < 0 || value > 3) {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        } else {
            ecx343_current_data.uLCD_INVR = (uint8_t)value;
            executeTaskWithMutex(INVERSION, PANEL_RIGHT, (uint8_t)value);
            reply += sprintf(reply, "OK");
        }
    }
    break;

    case CE_SET_LCD_LUXL:
    {
        char* endPtrLuxl = NULL;
        if (args_len == 0) {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
            break;
        }
        value = strtol(data, &endPtrLuxl, 10);
        if (endPtrLuxl == data || value < 1000 || value > 5000 || value % 100 != 0) {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        } else {
            if (isAutoBrightnessEnabled) break;
            value /= 10;
            ecx343_current_data.uLCD_LUXL = (uint16_t)value;
            executeTaskWithMutex(ADJUST_BRIGHTNESS);
            reply += sprintf(reply, "OK");
        }
    }
    break;

    case CE_SET_LCD_LUXR:
    {
        char* endPtrLuxr = NULL;
        if (args_len == 0) {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
            break;
        }
        value = strtol(data, &endPtrLuxr, 10);
        if (endPtrLuxr == data || value < 1000 || value > 5000 || value % 100 != 0) {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        } else {
            if (isAutoBrightnessEnabled) break;
            value /= 10;
            ecx343_current_data.uLCD_LUXR = (uint16_t)value;
            executeTaskWithMutex(ADJUST_BRIGHTNESS);
            reply += sprintf(reply, "OK");
        }
    }
    break;

    case CE_SET_LCD_HORL:
    {
        char* endPtrHorl = NULL;
        if (args_len == 0) {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
            break;
        }
        value = strtol(data, &endPtrHorl, 10);
        if (endPtrHorl == data || value < 0 || (value > 16 && (value < 100 || value > 116))) {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        } else {
            ecx343_current_data.uLCD_HORBL = (uint32_t)value;
            uint8_t tmp = LcdHorbit(value);
            executeTaskWithMutex(ORBIT_H, PANEL_LEFT, tmp);
            reply += sprintf(reply, "OK");
        }
    }
    break;

    case CE_SET_LCD_HORR:
    {
        char* endPtrHorr = NULL;
        if (args_len == 0) {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
            break;
        }
        value = strtol(data, &endPtrHorr, 10);
        if (endPtrHorr == data || value < 0 || (value > 16 && (value < 100 || value > 116))) {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        } else {
            ecx343_current_data.uLCD_HORBR = (uint32_t)value;
            uint8_t tmp = LcdHorbit(value);
            executeTaskWithMutex(ORBIT_H, PANEL_RIGHT, tmp);
            reply += sprintf(reply, "OK");
        }
    }
    break;

    case CE_SET_LCD_VORL:
    {
        char* endPtrVorl = NULL;
        if (args_len == 0) {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
            break;
        }
        value = strtol(data, &endPtrVorl, 10);
        if (endPtrVorl == data || value < 0 || (value > 16 && (value < 100 || value > 116))) {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        } else {
            ecx343_current_data.uLCD_VORBL = (uint32_t)value;
            uint8_t tmp = LcdVorbit(value);
            executeTaskWithMutex(ORBIT_V, PANEL_LEFT, tmp);
            reply += sprintf(reply, "OK");
        }
    }
    break;

    case CE_SET_LCD_VORR:
    {
        char* endPtrVorr = NULL;
        if (args_len == 0) {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
            break;
        }
        value = strtol(data, &endPtrVorr, 10);
        if (endPtrVorr == data || value < 0 || (value > 16 && (value < 100 || value > 116))) {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        } else {
            ecx343_current_data.uLCD_VORBR = (uint32_t)value;
            uint8_t tmp = LcdVorbit(value);
            executeTaskWithMutex(ORBIT_V, PANEL_RIGHT, tmp);
            reply += sprintf(reply, "OK");
        }
    }
    break;

    case CE_SET_LCD_CLEAR:
        if (!args_len) {
            memset(ecx343RW_buf, 0xFF, sizeof(ecx343RW_buf));
            if (Flash_Write_Data(0x08010000, ecx343RW_buf, sizeof(ecx343RW_buf) / 4))
                reply += sprintf(reply, "LCD setting in Flash Clear Error");
            else
                reply += sprintf(reply, "LCD setting in Flash Clear OK");
        } else {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        }
        break;

    case CE_SET_LCD_DEFAULT:
        if (!args_len) {
            isDebugModeEnabled = 0;
            isAutoBrightnessEnabled = 0;
            isHighTempBrightnessEnabled = 0;
            memset(ecx343RW_buf, 0, sizeof(ecx343RW_buf));
            memcpy(ecx343RW_buf, (void*)&ecx343_data, sizeof(ECX343_DATA));
            if (Flash_Write_Data(0x08010000, ecx343RW_buf, sizeof(ecx343RW_buf) / 4))
                reply += sprintf(reply, "Error");
            else
                reply += sprintf(reply, "OK");

            ecx343_current_data = ecx343_data;
            currentPanelMode = ecx343_current_data.uLCD_MODE;
            switchMode();
            executeTaskWithMutex(WRITE_REGISTERS, &ecx343_current_data);
        } else {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        }
        break;

    case CE_SET_LCD_DEVICEL:
        if (args_len == 0 || args_len > 20) {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
            break;
        }
        memset(ecx343_current_data.uLCD_DEVICEL, 0, sizeof(ecx343_current_data.uLCD_DEVICEL));
        memcpy(ecx343_current_data.uLCD_DEVICEL, data, args_len);
        reply += sprintf(reply, "OK");
        break;

    case CE_SET_LCD_DEVICER:
        if (args_len == 0 || args_len > 20) {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
            break;
        }
        memset(ecx343_current_data.uLCD_DEVICER, 0, sizeof(ecx343_current_data.uLCD_DEVICER));
        memcpy(ecx343_current_data.uLCD_DEVICER, data, args_len);
        reply += sprintf(reply, "OK");
        break;

    case CE_SET_LCD_MODE:
    {
        char* endPtrMode = NULL;
        if (args_len == 0) {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
            break;
        }
        value = strtol(data, &endPtrMode, 10);
        if (endPtrMode == data || value < 0 || value > 3) {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        } else {
            currentPanelMode = value;
            reply += sprintf(reply, "OK");
            switchMode();
        }
    }
    break;

    case CE_SET_HIGH_TEMP_BRIGHTNESS:
        if (!args_len) {
            isHighTempBrightnessEnabled = !isHighTempBrightnessEnabled;
            reply += sprintf(reply, "Set High Temp Brightness: %d", isHighTempBrightnessEnabled);
        } else {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        }
        break;

    case CE_SET_AUTO_BRIGHTNESS:
        if (!args_len) {
            isAutoBrightnessEnabled = !isAutoBrightnessEnabled;
            reply += sprintf(reply, "Set Auto Brightness: %d", isAutoBrightnessEnabled);
        } else {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        }
        break;

    case CE_SET_DEBUG_MODE:
        if (!args_len) {
            isDebugModeEnabled = !isDebugModeEnabled;
            reply += sprintf(reply, "Set Debug Mode: %d", isDebugModeEnabled);
        } else {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        }
        break;

    case CE_SET_ENC_EN:
        if (!args_len) {
            encSwitch = 1;
            reply += sprintf(reply, "OK");
        } else {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        }
        break;

    case CE_SET_MIC_MUTE:
        if (!args_len) {
            micSwitch = 1;
            reply += sprintf(reply, "OK");
        } else {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        }
        break;

        /* get command set */
    case CE_GET_ECHO:
        if (!args_len)
            reply += sprintf(reply, "%d", CDC_Echo_Ctrl_Flag);
        else
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        break;

    case CE_GET_TOF_PWR:
        reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        break;

    case CE_GET_TOF_MODE:
        reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        break;

    case CE_GET_TOF_KEYCODE:
        reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        break;

    case CE_GET_TOF_CONF:
        reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        break;

    case CE_GET_TOF_CALDATA:
        reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        break;
    case CE_GET_FW_VER:
        if (!args_len)
            reply += sprintf(reply, "%d.%d.%d.%s", V_MAJOR, V_MINOR, V_PATCH, MODEL_SUFFIX);
        else
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        break;
    case CE_GET_MCU_SN:
        if (!args_len)
            reply += sprintf(reply, "%08X%04X", *(unsigned int*)DEVICE_ID1 + *(unsigned int*)DEVICE_ID3, (*(unsigned int*)DEVICE_ID2 >> 16));
        else
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        break;

    case CE_GET_LCD_VALUE:
        if (!args_len) {
            char devicer[20] = { 0 }, devicel[20] = { 0 };
            memcpy(devicel, ecx343_current_data.uLCD_DEVICEL, sizeof(ecx343_current_data.uLCD_DEVICEL));
            memcpy(devicer, ecx343_current_data.uLCD_DEVICER, sizeof(ecx343_current_data.uLCD_DEVICER));
            reply += sprintf(reply, "INVL:%ld, INVR:%ld, LUXL:%ld, LUXR:%ld, HORBL:%ld, HORBR:%ld, VORBL:%ld, VORBR:%ld, MODE:%ld, SERNL:%s, SERNR:%s, checksum:%ld",
                ecx343_current_data.uLCD_INVL, ecx343_current_data.uLCD_INVR, ecx343_current_data.uLCD_LUXL * 10,
                ecx343_current_data.uLCD_LUXR * 10, ecx343_current_data.uLCD_HORBL, ecx343_current_data.uLCD_HORBR,
                ecx343_current_data.uLCD_VORBL, ecx343_current_data.uLCD_VORBR, ecx343_current_data.uLCD_MODE, devicel, devicer,
                ecx343_current_data.checksum);
        } else {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        }
        break;

    case CE_GET_BRG_FW:
        if (!args_len) {
            uint8_t brgFW[4];
            GetBridgeVersion(brgFW);
            reply += sprintf(reply, "%02X%02X%02X%02X", brgFW[0], brgFW[1], brgFW[2], brgFW[3]);
        } else {
            reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
        }
        break;
    case CE_GET_ALS_REG:
	    usbDebug("ambientLight= %d \r\n", ambientLight);

        break;        

    /* key command set */
    case CE_KEY_A:
    case CE_KEY_D:
    case CE_KEY_K:
    case CE_KEY_L:
    case CE_KEY_S:
    case CE_KEY_W:
    case CE_KEY_1:
    case CE_KEY_2:
    case CE_KEY_3:
    case CE_KEY_4:
    case CE_KEY_5:
    case CE_KEY_6:
    case CE_KEY_7:
    case CE_KEY_8:
    case CE_KEY_9:
    case CE_KEY_0:
    case CE_KEY_ENTER:
        // case CE_KEY_TAB:
    case CE_KEY_PRTSC:
    case CE_KEY_SCROLLLOCK:
    case CE_KEY_PAGEUP:
    case CE_KEY_PAGEDOWN:
    case CE_KEY_RIGHT:
    case CE_KEY_LEFT:
    case CE_KEY_DOWN:
    case CE_KEY_UP:
    case CE_KEY_COMPOSE:
    case CE_KEY_POWER:
    case CE_KEY_MUTE:
        // case CE_KEY_PLAY:
        // case CE_KEY_PREVIOUS:
        // case CE_KEY_NEXT:
    case CE_KEY_BACK:
        /* consumer command set */
    case CE_CR_SUBTITLE:
    case CE_CR_RED:
    case CE_CR_GREEN:
    case CE_CR_BLUE:
    case CE_CR_YELLOW:
    case CE_CR_BRTUP:
    case CE_CR_BRTDOWN:
        // case CE_CR_PHONE:
    case CE_CR_RECORD:
    case CE_CR_FORWARD:
    case CE_CR_REWIND:
    case CE_CR_MUTE:
    case CE_CR_VOLUP:
    case CE_CR_VOLDOWN:
        // case CE_CR_MEDIA:
    case CE_CR_ASSISTANT:
    case CE_CR_AC_SEARCH:
    case CE_CR_AC_HOME:
    case CE_CR_AC_BACK:
    case CE_CR_AC_FORWARD:
    case CE_CR_AC_STOP:
    case CE_CR_NEXT:
    case CE_CR_PREVIOUS:
    case CE_CR_STOP:
    case CE_CR_PLAY:
    case CE_CR_VOICE:
    case CE_CR_ALTAUDIO:
        /* gamepad command set */
    case CE_GPAD_BTN1:
    case CE_GPAD_BTN2:
    case CE_GPAD_BTN3:
    case CE_GPAD_BTN4:
    case CE_GPAD_BTN5:
    case CE_GPAD_BTN6:
    case CE_GPAD_BTN7:
    case CE_GPAD_BTN8:
    case CE_GPAD_BTN9:
    case CE_GPAD_BTN10:
    case CE_GPAD_BTN11:
    case CE_GPAD_BTN12:
    case CE_GPAD_BTN13:
    case CE_GPAD_BTN14:
    case CE_GPAD_BTN15:
        if (args_len) {
            value = strtol(data, &data, 10);
            if (value < 0 || value > 9 || (value == 0 && *(data - 1) != '0')) {
                reply += sprintf(reply, "NG %d", CE_ERR_PARAMETER);
                value = 0;
                break;
            } else {
                value = (1UL << (CE_KEY_1 - CE_KEY_BASE + (value + 9) % 10));
            }
        }

        HID_keyboard_report.report_id = (cmd < CE_CR_BASE) ?
            0x01 : (cmd < CE_GPAD_BASE) ?
            0x02 : 0x03;

        HID_keyboard_report.keys = (1UL << (cmd - ((cmd < CE_CR_BASE) ?
            CE_KEY_BASE : (cmd < CE_GPAD_BASE) ?
            CE_CR_BASE : CE_GPAD_BASE)));

        usbhid_sendReport((char*)&HID_keyboard_report, sizeof(HID_KeyboardReport));
        HAL_Delay(20);
        reply += sprintf(reply, "OK");
        break;

    case CE_KEY_NONE:
        reply += sprintf(reply, "OK");
        break;

    case CE_ENTER_BOOTLOADER:
        // ckhsu, move to F7xx reboot function.
        //*((uint32_t *)0x20017FF0) = 0xDEADBEEF;
        // NVIC_SystemReset();
        // while (true);
        boot_UserDFU();
        break;

    case CE_FLASH_WRITE:
        ecx343_current_data.checksum = Cal_Ecx343_data_checksum(ecx343_current_data);
        memset(ecx343RW_buf, 0, sizeof(ecx343RW_buf));
        memcpy(ecx343RW_buf, (void*)&ecx343_current_data, sizeof(ECX343_DATA));
        if (Flash_Write_Data(0x08010000, ecx343RW_buf, sizeof(ecx343RW_buf) / 4))
            reply += sprintf(reply, "Error");
        else
            reply += sprintf(reply, "OK");
        break;

    case CE_FLASH_READ:
        memset(ecx343RW_buf, 0, sizeof(ecx343RW_buf));
        memset((void*)&ecx343_current_data, 0, sizeof(ECX343_DATA));
        Flash_Read_Data(0x08010000, ecx343RW_buf, (sizeof(ecx343RW_buf) / 4) - 1);
        memcpy((void*)&ecx343_current_data, ecx343RW_buf, sizeof(ECX343_DATA));
        if (Check_Ecx343_data_checksum(ecx343_current_data))
            reply += sprintf(reply, "OK");
        else
            reply += sprintf(reply, "Error");
        break;

    default:
        reply += sprintf(reply, "NG %d", CE_ERR_COMMAND);
        break;
    }

    // Adding trailing newline characters
    reply += sprintf(reply, "%s", (CDC_Echo_Ctrl_Flag) ? "\r\n\r\n" : "\r\n");

    if (cmd == CE_GET_FW_VER) {
        if (Cmd_Output) {
            usbEcho_Tof((char*)CeCmdRespTxBuffer);
        } else {
            usbDebug((char*)CeCmdRespTxBuffer);
        }
    } else {
        // Tof output
        if (cmd <= 0x100U || cmd >= 0x200U) {
            usbEcho_Tof((char*)CeCmdRespTxBuffer);
        }
        // Devctlr output
        else {
            usbDebug((char*)CeCmdRespTxBuffer);
        }
    }

    // Send sync key if it's an HID event
    if (cmd >= CE_HID_MIN && cmd <= CE_HID_MAX) {
        HID_keyboard_report.keys = 0;  // Send a key release event

        // Send key release report
        usbhid_sendReport((char*)&HID_keyboard_report, sizeof(HID_KeyboardReport));
    }
}

void Ecx343_data_init_default(void) {
    const char* strL = "JORJIN V8";
    const char* strR = "JORJIN V8";
    memset(ecx343_data.uLCD_DEVICEL, 0, sizeof(ecx343_data.uLCD_DEVICEL));
    memcpy(ecx343_data.uLCD_DEVICEL, strL, strlen(strL));

    memset(ecx343_data.uLCD_DEVICER, 0, sizeof(ecx343_data.uLCD_DEVICER));
    memcpy(ecx343_data.uLCD_DEVICER, strR, strlen(strR));
    ecx343_data.uLCD_INVL 	= 0;
    ecx343_data.uLCD_INVR 	= 0;
    ecx343_data.uLCD_LUXL 	= 350;
    ecx343_data.uLCD_LUXR 	= 350;
    ecx343_data.uLCD_HORBL 	= 0;
    ecx343_data.uLCD_HORBR 	= 0;
    ecx343_data.uLCD_VORBL 	= 0;
    ecx343_data.uLCD_VORBR 	= 0;
    ecx343_data.uLCD_MODE 	= 0;
    ecx343_data.checksum = Cal_Ecx343_data_checksum(ecx343_data);
}

uint32_t Cal_Ecx343_data_checksum(ECX343_DATA data) {
    uint32_t checksum = data.uLCD_INVL + data.uLCD_INVR +
        data.uLCD_LUXL + data.uLCD_LUXR + data.uLCD_HORBL +
        data.uLCD_HORBR + data.uLCD_VORBL + data.uLCD_VORBR + data.uLCD_MODE;
    return checksum;
}

int Check_Ecx343_data_checksum(ECX343_DATA data) {
    if (data.checksum != (data.uLCD_INVL + data.uLCD_INVR +
        data.uLCD_LUXL + data.uLCD_LUXR + data.uLCD_HORBL +
        data.uLCD_HORBR + data.uLCD_VORBL + data.uLCD_VORBR + data.uLCD_MODE))
        return CheckSum_FAIL;
    else
        return CheckSum_OK;
}

uint8_t LcdHorbit(int value) {
    uint8_t tmp = 0;
    if (value / 100 == 0) {
        tmp = (~(value % 100) + 1) & 0x3F;
    } else if (value / 100 == 1) {
        tmp = (value % 100);
    }
    return tmp;
}

uint8_t LcdVorbit(int value) {
    uint8_t tmp = 0;
    if (value / 100 == 0) {
        tmp = value % 100;
    } else if (value / 100 == 1) {
        tmp = (~(value % 100) + 1) & 0x3F;
    }
    return tmp;
}

#if ENABLE_TOF
void tof_ranging_callback(VL53L8CX_ResultsData* range_data, uint32_t time_stamp) {

    uint8_t* ptr = tofRangePacket, zone_idx;
    uint32_t checksum = 0;
    uint16_t target_idx;
    uint8_t data_count = MAX_TOF_DATA_COUNT;

    // drop the new data if the packet has been updated since the last CDC transmission
    if (bRangePacketUpdated) return;

    // add the "DATA" prefix
    *((uint32_t*)ptr) = TOF_DATA_PREFIX;
    ptr += 4;
    // add the number of zone
    *(ptr++) = data_count;
    // add the silicon temperature (degree Celsius)
    *(ptr++) = range_data->silicon_temp_degc;
    // add the time_stamp
    *((uint32_t*)ptr) = time_stamp;
    ptr += 4;
    // add per zone/target data
    for (target_idx = 0, zone_idx = 0;
        target_idx < data_count * VL53L8CX_NB_TARGET_PER_ZONE;
        target_idx += VL53L8CX_NB_TARGET_PER_ZONE, zone_idx++) {
        uint8_t offset = 0;
        // add the number of targets
        *ptr = range_data->nb_target_detected[zone_idx];
        tofNumOfTargets[zone_idx] = *ptr;
        // pick the strongest signal for output if there're multiple targets
        if (*ptr > 1) {
            uint8_t max_offset = 0;
            uint32_t max_signal_rate = 0;
            for (; offset < *ptr; offset++) {
                uint32_t tmp = range_data->signal_per_spad[target_idx + offset];
                if (tmp > max_signal_rate) {
                    max_signal_rate = tmp;
                    max_offset = offset;
                }
            }
            offset = max_offset;
        }
        checksum += *(ptr++);
        // add the peak signal rate (kcps/SPAD)
        *((int32_t*)ptr) = range_data->signal_per_spad[target_idx + offset];
        //        tofPeakRate[zone_idx] = *((int32_t*)ptr);
        checksum += *((uint32_t*)ptr);
        ptr += 4;
        // add the median range (mm)
        *((int16_t*)ptr) = range_data->distance_mm[target_idx + offset];
        //        tofMedianRange[zone_idx] =
        //                ((tofPeakRate[zone_idx]<MIN_SIGNAL_RATE) || tofNumOfTargets[zone_idx] == 0)?
        //                        -1 : *((int16_t*)ptr);
        checksum += *((uint16_t*)ptr);
        ptr += 2;
        // add the reflectance (%)
        *ptr = range_data->reflectance[target_idx + offset];
        checksum += *(ptr++);
        // add the target status
        *ptr = range_data->target_status[target_idx + offset];
        checksum += *(ptr++);
    }
    // add checksum
    *((uint32_t*)ptr) = checksum;
    // add the "ED\r\n" suffix
    *((uint32_t*)(ptr + 4)) = TOF_DATA_SUFFIX;
    // set the updated flag
    bRangePacketUpdated = true;
    usbcdc_sendData(tofRangePacket, TOF_8X8_DATA_PACKET_SIZE);

    return;
}
#endif
