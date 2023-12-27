/*
 * button_handling.h
 *
 *  Created on: Dec 20, 2023
 *      Author: User
 */

#include "stm32f7xx_hal.h"
#include <stdbool.h>

#ifndef CORE_INC_BUTTON_HANDLING_H_
#define CORE_INC_BUTTON_HANDLING_H_

#define LONG_THRESHOLD 2000
#define DOUBLE_THRESHOLD 900
#define CLICK_DETECTION_PERIOD 1000

typedef enum {
    MODE_BRIGHTNESS,
    MODE_VOLUME
} OperationMode;

typedef enum {
    NO_CLICK,
    SINGLE_CLICK,
    DOUBLE_CLICK,
    LONG_PRESS
} ButtonClickType;

typedef enum {
    BUTTON_RELEASED,
    BUTTON_PRESSED
} ButtonState;

typedef enum {
    MODE_RELEASE,
    MODE_TRANSITION
} PowerSave;

bool UpdateButtonState(ButtonState *buttonPrevState, ButtonState *currentButtonState, uint32_t *pressTime, uint32_t *releaseTime, bool *isReleased);
ButtonClickType GetButtonClickType(uint32_t pressTime, uint32_t releaseTime);
void HandleButtonClick(OperationMode *currentMode, PowerSave *displayType, ButtonClickType *clickType);
void ProcessButtonEvent(uint8_t buttonEvent, ButtonClickType *clickType, OperationMode *currentMode, PowerSave *displayType);

extern uint32_t lastClickTime;
extern bool possibleSingleClick;
extern bool longPressHandled;
extern bool longPressTriggered;

#endif /* CORE_INC_BUTTON_HANDLING_H_ */
