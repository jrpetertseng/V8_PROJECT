#ifndef _JORJIN_FRAMEWORK_UTILS_H_
#define _JORJIN_FRAMEWORK_UTILS_H_

int  copyToBuffer_devCtlr(char *buf, int len);
int  copyToBuffer_ToF    (char *buf, int len);

void clearBuffer_devCtlr ( void);
void clearBuffer_ToF     ( void);

void processCommands_devCtlr( void);
void processCommands_ToF    ( void);

uint8_t CDC_CmdBuff_IsUpdated(void);
void CDC_Get_CmdBuff(uint8_t** pbuf, uint32_t *len);
void CDC_Clear_CmdBuff( void);

#endif // _JORJIN_FRAMEWORK_UTILS_H_

