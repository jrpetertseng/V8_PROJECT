#ifndef CORE_INC_BUTTON_HANDLING_H_
#define CORE_INC_BUTTON_HANDLING_H_

#include "stm32f7xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#define DEBOUNCE_MS              30u
#define LONG_THRESHOLD         1500u
#define DOUBLE_THRESHOLD        350u
#define CLICK_DETECTION_PERIOD  450u

typedef enum {
	NO_CLICK = 0,
	SINGLE_CLICK,
	DOUBLE_CLICK,
	LONG_PRESS
} ButtonClickType;

typedef enum {
	BUTTON_RELEASED = 0,
	BUTTON_PRESSED
} ButtonState;

typedef enum
{
  BUTTON_WAKEUP = 0
}Button_TypeDef;

typedef enum
{
  BUTTON_MODE_GPIO = 0,
  BUTTON_MODE_EXTI = 1
}ButtonMode_TypeDef;

typedef struct {
	ButtonState stableState;
	ButtonState lastRawState;
	uint32_t lastDebounceTick;

	uint32_t pressTick;
	bool longSent;

	bool waitingSecond;
	uint32_t firstReleaseTick;
} ButtonContext;

void Button_Init(ButtonContext *ctx);
ButtonClickType Button_Update(ButtonContext *ctx, GPIO_TypeDef *port, uint16_t pin);

void ProcessButtonEvent(uint8_t buttonEvent, ButtonClickType *clickType);

#endif /* CORE_INC_BUTTON_HANDLING_H_ */
