#include <stdio.h>
#include <string.h>

#include "main.h"
#include "config.h"
#include "usbd_desc.h"
#include "usb.h"
#include "cmd_engine.h"
#include "usbd_cdc_if_devctlr.h"

struct Argc {
    char *ptr;
    int len;
} argc[COMMAND_ARG_MAX_SIZE];

typedef int (*CmdFunc) (struct Argc *argc, int argv);

static int cmdEnterBootloader(struct Argc *argc, int argv);
static int cmdGetFirmwareVersion(struct Argc *argc, int argv);
static int cmdGetHardwareSerialNumber(struct Argc *argc, int argv);
static int cmdSetDebugEnable(struct Argc *argc, int argv);
static int cmdSetDebugDisable(struct Argc *argc, int argv);

struct Command {
    char category[USB_COMMAND_CATEGORY_MAX_SIZE];
    int categorySize;
    char name[USB_COMMAND_NAME_MAX_SIZE];
    int nameSize;
    int argSize;
    CmdFunc func;
} supportCommands[] = {
        {
            .category = "enter",
            .categorySize = 5,
            .name = "bootloader",
            .nameSize = 10,
            .argSize = 0,
            .func = cmdEnterBootloader
        },
        {
            .category = "get",
            .categorySize = 3,
            .name = "firmver",
            .nameSize = 7,
            .argSize = 0,
            .func = cmdGetFirmwareVersion
        },
        {
            .category = "get",
            .categorySize = 3,
            .name = "hwsn",
            .nameSize = 4,
            .argSize = 0,
            .func = cmdGetHardwareSerialNumber
        },
        {
            .category = "set",
            .categorySize = 3,
            .name = "dbgE",
            .nameSize = 4,
            .argSize = 0,
            .func = cmdSetDebugEnable
        },
        {
            .category = "set",
            .categorySize = 3,
            .name = "dbgD",
            .nameSize = 4,
            .argSize = 0,
            .func = cmdSetDebugDisable
        },
};

//static int supportCommandSize =
//        sizeof(supportCommands)/sizeof(struct Command);

typedef struct _CommandBuf
{
    char buf[USB_COMMAND_BUF_MAX_SIZE];
    int pos;
    int len;
} CommandBuf_T;

static CommandBuf_T cmdBuf_devCtlr;
static CommandBuf_T cmdBuf_ToF;

static int cmdEnterBootloader(struct Argc *argc, int argv)
{
    boot_UserDFU();

    return 0;
}

static int cmdGetFirmwareVersion(struct Argc *argc, int argv)
{
    usbDebug("%d.%d.%d %s\r\n\r\n", V_MAJOR, V_MINOR, V_PATCH, MODEL_SUFFIX);
    return 0;
}

static int cmdGetHardwareSerialNumber(struct Argc *argc, int argv)
{
    usbDebug("%08X%04X\r\n\r\n",
            *(unsigned int*) DEVICE_ID1 + *(unsigned int*) DEVICE_ID3,
            (*(unsigned int*) DEVICE_ID2 >> 16));
    return 0;
}

static int cmdSetDebugEnable(struct Argc *argc, int argv)
{
    CDC_DEVCTLR_SetEnableTransmit( 1);
    return 0;
}

static int cmdSetDebugDisable(struct Argc *argc, int argv)
{
    CDC_DEVCTLR_SetEnableTransmit( 0);
    return 0;
}

int copyToBuffer_devCtlr(char *buf, int len)
{
    if ((cmdBuf_devCtlr.len + len)
        >= USB_COMMAND_BUF_MAX_SIZE) return -1;

    memcpy(cmdBuf_devCtlr.buf+cmdBuf_devCtlr.len, buf, len);
//    cmdBuf_devCtlr.len += len;
//
//    cmdBuf_devCtlr.buf[cmdBuf_devCtlr.len] = '\0';
    cmdBuf_devCtlr.len += (len+2);
    cmdBuf_devCtlr.buf[cmdBuf_devCtlr.len-2] = '\r';
    cmdBuf_devCtlr.buf[cmdBuf_devCtlr.len-1] = '\n';
    cmdBuf_devCtlr.buf[cmdBuf_devCtlr.len] = '\0';

    return 0;
}

int copyToBuffer_ToF(char *buf, int len)
{
    if ((cmdBuf_ToF.len + len)
        >= USB_COMMAND_BUF_MAX_SIZE) return -1;

    memcpy(cmdBuf_ToF.buf+cmdBuf_ToF.len, buf, len);
//    cmdBuf_ToF.len += len;
    cmdBuf_ToF.len += (len+2);
    cmdBuf_ToF.buf[cmdBuf_ToF.len-2] = '\r';
    cmdBuf_ToF.buf[cmdBuf_ToF.len-1] = '\n';
    cmdBuf_ToF.buf[cmdBuf_ToF.len] = '\0';

    return 0;
}

void clearBuffer_devCtlr( void)
{
    cmdBuf_devCtlr.pos = 0;
    cmdBuf_devCtlr.len = 0;
}

void clearBuffer_ToF( void)
{
    cmdBuf_ToF.pos = 0;
    cmdBuf_ToF.len = 0;
}

static void updateBuffer_devCtlr(int used)
{
    int residual = 0;
    if (!used) {
        cmdBuf_devCtlr.pos = cmdBuf_devCtlr.len;
        return;
    }

    residual = cmdBuf_devCtlr.len - used;
    if (!residual) {
        clearBuffer_devCtlr();
        return;
    }

    memcpy( cmdBuf_devCtlr.buf, cmdBuf_devCtlr.buf+used, residual);
    cmdBuf_devCtlr.pos = residual;
    cmdBuf_devCtlr.len = residual;

    cmdBuf_devCtlr.buf[cmdBuf_devCtlr.len] = '\0';
}

static void updateBuffer_ToF(int used)
{
    int residual = 0;
    if (!used) {
        cmdBuf_ToF.pos = cmdBuf_ToF.len;
        return;
    }

    residual = cmdBuf_ToF.len - used;
    if (!residual) {
        clearBuffer_ToF();
        return;
    }

    memcpy( cmdBuf_ToF.buf, cmdBuf_ToF.buf+used, residual);
    cmdBuf_ToF.pos = residual;
    cmdBuf_ToF.len = residual;

    cmdBuf_ToF.buf[cmdBuf_ToF.len] = '\0';
}

static int isEmptyBuffer_devCtlr( void)
{
    return cmdBuf_devCtlr.len == 0;
}

static int isEmptyBuffer_ToF( void)
{
    return cmdBuf_ToF.len == 0;
}

static int isUpdatedBuffer_devCtlr( void)
{
    return (!isEmptyBuffer_devCtlr()) &&
           (cmdBuf_devCtlr.len != cmdBuf_devCtlr.pos);
}

static int isUpdatedBuffer_ToF( void)
{
    return (!isEmptyBuffer_ToF()) &&
           (cmdBuf_ToF.len != cmdBuf_ToF.pos);
}

static int findLineEnd(char *buf, int pos, int len)
{
    while ((pos < len) &&
           (buf[pos] != '\r') &&
           (buf[pos] != '\n')) {
        pos++;
    }
    if (pos < len) return pos;
    else return -1;
}

static int skipLineEnd(char *buf, int pos, int len)
{
    while ((pos < len) &&
           ((buf[pos] == '\r') ||
           (buf[pos] == '\n'))) {
        pos++;
    }

    return pos;
}

//static int parseArgs(char *buf, int len)
//{
//    int argv = 0;
//    int start = 0;
//    int pos = 0;
//
//    if (!len) return 0;
//    if (buf[0] != ' ') return -1;
//
//    while ((pos < len) &&
//           (buf[pos] == ' ')) {
//        pos++;
//    }
//    start = pos;
//
//    while (pos < len) {
//        if ((buf[pos] == ' ')  ||
//            (buf[pos] == '\r') ||
//            (buf[pos] == '\n')) {
//            argc[argv].ptr = &buf[start];
//            argc[argv].len = pos - start;
//            argv++;
//
//            while ((pos < len) &&
//                   (buf[pos] == ' ')) {
//                pos++;
//            }
//            start = pos;
//        }
//        else pos++;
//    }
//
//    if (start != pos) {
//        argc[argv].ptr = &buf[start];
//        argc[argv].len = pos - start;
//        argv++;
//    }
//
//    return argv;
//}

//static void executeCommands(char *cmd, int len)
//{
//    int i;
//    if (!len) return;
//
//    //usbDebug("Execute: ");
//    //usbDebugChars(cmd, len);
//    //usbDebug("\r\n");
//
//    for (i=0;i<supportCommandSize;i++) {
//        struct Command *support = &supportCommands[i];
//
//        if (len <
//            (support->categorySize +
//             support->nameSize)) {
//            continue;
//        }
//
//        if (!strncmp(cmd, support->category, support->categorySize)) {
//            if (!strncmp(cmd+support->categorySize, support->name, support->nameSize)) {
//                int argv = -1;
//                if (support->argSize) {
//                    argv = parseArgs(cmd+support->categorySize+support->nameSize,
//                              len-support->categorySize-support->nameSize);
//                    if (argv != support->argSize) {
//                        continue;
//                    }
//                }
//                support->func(argc, argv);
//                break;
//            }
//        }
//    }
//
//    // output an error message if no command has been executed
//    if (i == supportCommandSize) usbDebug("NG 0\r\n\r\n");
//}

void processCommands_devCtlr( void)
{
    int pos = cmdBuf_devCtlr.pos;
    int used = 0;
    int lineEnd = 0;

    if (!isUpdatedBuffer_devCtlr()) return;
    while (1) {
        lineEnd = findLineEnd(cmdBuf_devCtlr.buf, pos, cmdBuf_devCtlr.len);
        if (lineEnd < 0) break;
//        executeCommands(cmdBuf_devCtlr.buf+pos-cmdBuf_devCtlr.pos, lineEnd - (pos-cmdBuf_devCtlr.pos));
        CE_Parse_Devctlr_Cmd_Data( (uint8_t *)(cmdBuf_devCtlr.buf+pos-cmdBuf_devCtlr.pos),
                                       (lineEnd - (pos-cmdBuf_devCtlr.pos)));
        pos = skipLineEnd(cmdBuf_devCtlr.buf, lineEnd+1, cmdBuf_devCtlr.len);
        used = pos;
    }

    updateBuffer_devCtlr(used);
}

void processCommands_ToF( void)
{
    int pos = cmdBuf_ToF.pos;
    int used = 0;
    int lineEnd = 0;

    if(!isUpdatedBuffer_ToF()) return;

    while (1)
    {
        lineEnd = findLineEnd( cmdBuf_ToF.buf, pos, cmdBuf_ToF.len);
        if (lineEnd < 0) break;
        CE_Parse_ToF_Cmd_Data( (uint8_t *)(cmdBuf_ToF.buf+pos-cmdBuf_ToF.pos),
                               (lineEnd - (pos-cmdBuf_ToF.pos)));
        pos = skipLineEnd( cmdBuf_ToF.buf, lineEnd+1, cmdBuf_ToF.len);
        used = pos;
    }

    updateBuffer_ToF( used);
}

/* Try to complied to single thread lib. */
uint8_t CDC_CmdBuff_IsUpdated(void)
{
    return 0;
}

void CDC_Get_CmdBuff(uint8_t** pbuf, uint32_t *len)
{
    //*pbuf = (uint8_t*)CmdBuffer;
    //*len = CmdBuffLength;
}

void CDC_Clear_CmdBuff( void)
{
    clearBuffer_ToF();
}

