#ifndef __LT7911_H__99d96366_3954_11ed_8025_536a4ffef0cb_
#define __LT7911_H__99d96366_3954_11ed_8025_536a4ffef0cb_

#include "stm32f7xx_hal.h"

#define ENABLE_LT7911_USB_DEBUG     0

#define LT7911_HDCP_KEY_LENGTH     (286)
#define LT7911_CHIP_ID             (0x1901)
#define LT7911_DEV_ADDR            (0x43) //7911ux
#define LT7911_TIMEOUT             1000

void LT7911_Init( void);
HAL_StatusTypeDef HDMI_WriteI2C_Byte( uint16_t addr, uint8_t buf);
HAL_StatusTypeDef HDMI_WriteI2C_ByteN( uint16_t addr, uint8_t *buf, uint16_t size);
uint8_t HDMI_ReadI2C_Byte( uint16_t addr);
HAL_StatusTypeDef HDMI_ReadI2C_ByteN( uint16_t addr, uint8_t *buf, uint16_t size);

uint16_t LT7911_ReadChipId( void);
void LT7911_Config( void);
void LT7911_BlockErase( void);

HAL_StatusTypeDef LT7911_ReadHDCPKey( uint8_t *pKey);
HAL_StatusTypeDef LT7911_WriteHDCPKey( uint8_t *pKey);
void LT7911_WriteFirmware( uint8_t *byWriteData, uint32_t DataLen);
void LT7911_ReadbackFirmware( uint8_t *byReadData, uint32_t WriteDataLen);
void LT7911_ModeSwitch();
void lt7911_firmware_update_init(void);
void lt7911uxc_version(uint8_t *run_version);

// ckhsu: below is the 7911 in J5x
// Use 100k bus
// 7911D_CSCL      ==> PB_10 (I2C2_SCL)
// 7911D_CSDA      ==> PB_11 (I2C2_SDA)
// 7911D_GPIO5     ==> PC_2  (GPIO_INPUT)
// 7911D_RSTN      ==> PC_3  (GPIO_OUTPUT, Default set low, active low)

/*
 LT7911 start␊
 Rd chip id:[1605]␊
 key[0]: [AE 24 6E 1C 2D 51 A7 7D A6 6D 31 83 E2 72 EF 70]␊
 key[1]: [36 86 2B 90 66 74 7F 0A 3B 28 46 57 D5 76 8E D0]␊
 key[2]: [8A E1 D5 53 91 EF AB 3C 53 3A 3E B3 E4 04 14 4B]␊
 key[3]: [07 D3 BC 15 3F 12 C9 9E 11 D2 FE D3 6E 59 35 8E]␊
 key[4]: [D4 EE BE C5 AC 87 15 6C 50 B8 09 D2 53 52 38 4D]␊
 key[5]: [15 65 6F F4 70 03 C2 B6 1C 76 05 A5 B4 3B BD F2]␊
 key[6]: [F5 15 65 DB 56 5E 45 9D 53 89 8A B0 AC DB FC BA]␊
 key[7]: [22 C7 11 3D 3E 7B 58 48 19 7A AB 5C 47 59 2A 09]␊
 key[8]: [C5 BA F1 6C 83 DB AF 0A 0A A4 9B 82 0E 60 4C 4D]␊
 key[9]: [2C D1 D0 09 E9 70 82 15 A3 E3 DF 47 28 C4 CA 4B]␊
 key[10]: [DB 72 C3 57 FA C5 C6 B6 5F ED 40 85 03 3D D9 FA]␊
 key[11]: [DD 79 1D BF B9 89 48 88 4A 40 0F B4 1A 82 BE B0]␊
 key[12]: [E4 6D 7A 72 28 0B 5B CB B4 FB 10 A8 39 66 48 78]␊
 key[13]: [AD 4B 82 BB BE C1 05 4C AF EE 80 69 6A 7C 19 64]␊
 key[14]: [0F 4F D5 89 11 C9 FB A3 7B 7B 03 5A E5 44 5C F6]␊
 key[15]: [CE 04 A4 41 53 ED BE 8E A3 71 B8 73 ED 57 AA 1B]␊
 key[16]: [BA C2 62 9F 98 2F 52 AC 7A 32 34 E9 0A F0 60 D1]␊
 key[17]: [31 EF D7 04 89 E0 53 E1 17 C6 50 D1 C3 78 00 00]␊
 LT7911 erase issued!␊
 FW W then R success.␊
 LT7911 update finished␊
 LT7911 end␊
 */
#endif /* __LT7911_H__99d96366_3954_11ed_8025_536a4ffef0cb_ */
