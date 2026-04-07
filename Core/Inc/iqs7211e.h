#ifndef INC_IQS7211E_H_
#define INC_IQS7211E_H_

#include "stm32f7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
	IQS_STATE_FAIL = 0,
	IQS_STATE_INIT = 1,
	IQS_STATE_RUN  = 2
} iqs_state_e;

typedef enum {
	IQS_G_NONE = 0,
	IQS_G_SINGLE_TAP,
	IQS_G_DOUBLE_TAP,
	IQS_G_TRIPLE_TAP,
	IQS_G_PRESS_HOLD,
	IQS_G_PALM,
	IQS_G_SWIPE_X_POS,
	IQS_G_SWIPE_X_NEG,
	IQS_G_SWIPE_Y_POS,
	IQS_G_SWIPE_Y_NEG,
	IQS_G_SWIPE_HOLD_X_POS,
	IQS_G_SWIPE_HOLD_X_NEG,
	IQS_G_SWIPE_HOLD_Y_POS,
	IQS_G_SWIPE_HOLD_Y_NEG
} iqs_gesture_e;

HAL_StatusTypeDef IQS7211E_Init(void);
void              IQS7211E_Run(void);

iqs_state_e       IQS7211E_GetState(void);
bool              IQS7211E_IsPresent(void);

bool              IQS7211E_HasNewData(void);
void              IQS7211E_ClearNewData(void);

iqs_gesture_e     IQS7211E_GetGesture(void);
HAL_StatusTypeDef IQS7211E_GetAbsXY(uint8_t finger, uint16_t *x, uint16_t *y);

HAL_StatusTypeDef IQS7211E_ReadDeltaAll(int16_t out_delta[42]);
HAL_StatusTypeDef IQS7211E_Suspend(bool enable);

#endif /* INC_IQS7211E_H_ */
