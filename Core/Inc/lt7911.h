#ifndef __LT7911_H__99d96366_3954_11ed_8025_536a4ffef0cb_
#define __LT7911_H__99d96366_3954_11ed_8025_536a4ffef0cb_

#include "stm32f7xx_hal.h"

#define LT7911_DEV_ADDR            (0x43)
#define LT7911_TIMEOUT             500

void LT7911_Mode_Switch(uint8_t value);
void GetBridgeVersion(uint8_t *brgFW);

#endif /* __LT7911_H__99d96366_3954_11ed_8025_536a4ffef0cb_ */
