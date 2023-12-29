/*
 * button_handling.c
 *
 *  Created on: Dec 20, 2023
 *      Author: User
 */

#include "button_handling.h"
#include "ecx343.h"
#include "usb.h"

extern ECX343_DATA ecx343_current_data;

uint32_t lastClickTime = 0;
bool possibleSingleClick = false;
bool longPressHandled = false;
bool longPressTriggered = false;

bool UpdateButtonState(ButtonState *buttonPrevState, ButtonState *currentButtonState, uint32_t *pressTime, uint32_t *releaseTime, bool *isReleased) {
    bool newEventOccurred = false;
    ButtonState readState = HAL_GPIO_ReadPin(SW_KEY_2D3D_GPIO_Port, SW_KEY_2D3D_Pin) == GPIO_PIN_RESET ? BUTTON_PRESSED : BUTTON_RELEASED;

    if (readState == BUTTON_PRESSED && *buttonPrevState == BUTTON_RELEASED) {
        *pressTime = osKernelGetTickCount();
        *isReleased = false;
        longPressHandled = false;
        longPressTriggered = false;
    } else if (readState == BUTTON_RELEASED && *buttonPrevState == BUTTON_PRESSED) {
        *releaseTime = osKernelGetTickCount();
        *isReleased = true;
        newEventOccurred = !longPressHandled && !longPressTriggered;
        longPressHandled = false;
        longPressTriggered = false;
    }

    if (readState == BUTTON_PRESSED && !longPressHandled && !longPressTriggered && (osKernelGetTickCount() - *pressTime > LONG_THRESHOLD)) {
        longPressHandled = true;
        longPressTriggered = true;
        return true;
    }

    *buttonPrevState = readState;
    return newEventOccurred;
}

ButtonClickType GetButtonClickType(uint32_t pressTime, uint32_t releaseTime) {
    uint32_t currentTime = osKernelGetTickCount();

    if (longPressTriggered) {
        longPressTriggered = false;
        possibleSingleClick = false;
        return NO_CLICK;
    }

    if (currentTime - lastClickTime < DOUBLE_THRESHOLD) {
        if (possibleSingleClick) {
            possibleSingleClick = false;
            lastClickTime = currentTime;
            return DOUBLE_CLICK;
        }
    } else {
        if (possibleSingleClick) {
            possibleSingleClick = false;
            lastClickTime = currentTime;
            return SINGLE_CLICK;
        }
    }

    possibleSingleClick = true;
    lastClickTime = currentTime;
    return NO_CLICK;
}

void HandleButtonClick(OperationMode *currentMode, PowerSave *displayType, ButtonClickType *clickType) {
    switch (*clickType) {
        case NO_CLICK:
            break;
        case SINGLE_CLICK:
        	uint8_t mode_state;
//			uint8_t result;
//			uint8_t panel = 0;

            // *displayType = !(*displayType);
            // ECX343EN_PowerSaving(*displayType);
            // usbDebug("#power %d@\r\n", *displayType);

            mode_state = (ecx343_current_data.uLCD_MODE + 1) % 4;
            flag_Freq = mode_state & 0x01;
            flag_2D3D = (mode_state & 0x02) >> 1;
            switchMode();
//            usbDebug("#lcdmode %d@\r\n", ecx343_current_data.uLCD_MODE);

//			usbDebug("panel: %d\r\n", panel);
//			for (uint16_t i=0x00; i<0xFF; i++)
//			{
//				panel_reg_read(0, i, &result, panel);
//				usbDebug("Addr [0x%02X]: [0x%02X]\r\n", i, result);
//				osDelay(20);
//			}
//			usbDebug("----------\r\n");
//			panel = !panel;
//			usbDebug("panel: %d\r\n", panel);
//			for (uint16_t i=0x00; i<0xFF; i++)
//			{
//				panel_reg_read(0, i, &result, panel);
//				usbDebug("Addr [0x%02X]: [0x%02X]\r\n", i, result);
//				osDelay(20);
//			}
            break;
        case DOUBLE_CLICK:
            *currentMode = (*currentMode == MODE_BRIGHTNESS) ? MODE_VOLUME : MODE_BRIGHTNESS;
//            usbDebug("#btmode %d@\r\n", *currentMode);
            break;
        case LONG_PRESS:
//			usbDebug("#take a picture@\r\n");
            break;
    }
    *clickType = NO_CLICK;
}

void ProcessButtonEvent(uint8_t buttonEvent, ButtonClickType *clickType, OperationMode *currentMode, PowerSave *displayType) {
    switch (buttonEvent) {
        case 1:
            HandleButtonClick(currentMode, displayType, clickType);
            break;
        case 2:
            if (*currentMode == MODE_BRIGHTNESS) {
                ecx343_current_data.uLCD_LUXL = (ecx343_current_data.uLCD_LUXL > 100) ?
                    ecx343_current_data.uLCD_LUXL - 10 : ecx343_current_data.uLCD_LUXL;
                ecx343_current_data.uLCD_LUXR = (ecx343_current_data.uLCD_LUXR > 100) ?
                    ecx343_current_data.uLCD_LUXR - 10 : ecx343_current_data.uLCD_LUXR;
                adjustBrightness();
//                usbDebug("#lcdlux %d,%d@\r\n", ecx343_current_data.uLCD_LUXL*10, ecx343_current_data.uLCD_LUXR*10);
                break;
            } else {
//            	usbDebug("#volume -@\r\n");
            }
            break;
        case 4:
            if (*currentMode == MODE_BRIGHTNESS) {
                ecx343_current_data.uLCD_LUXL = (ecx343_current_data.uLCD_LUXL < 500) ?
                    ecx343_current_data.uLCD_LUXL + 10 : ecx343_current_data.uLCD_LUXL;
                ecx343_current_data.uLCD_LUXR = (ecx343_current_data.uLCD_LUXR < 500) ?
                    ecx343_current_data.uLCD_LUXR + 10 : ecx343_current_data.uLCD_LUXR;
                adjustBrightness();
//                usbDebug("#lcdlux %d,%d@\r\n", ecx343_current_data.uLCD_LUXL*10, ecx343_current_data.uLCD_LUXR*10);
            } else {
//            	usbDebug("#volume +@\r\n");
            }
            break;
    }
}
