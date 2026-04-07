#include "button_handling.h"
#include "cmsis_os.h"
#include "usb.h"

static ButtonState ReadButtonRaw(GPIO_TypeDef *port, uint16_t pin)
{
	GPIO_PinState s;

	s = HAL_GPIO_ReadPin(port, pin);

	return (s == GPIO_PIN_RESET) ? BUTTON_PRESSED : BUTTON_RELEASED;
}

void Button_Init(ButtonContext *ctx)
{
	if (ctx == NULL) {
		return;
	}

	ctx->stableState      = BUTTON_RELEASED;
	ctx->lastRawState     = BUTTON_RELEASED;
	ctx->lastDebounceTick = 0u;

	ctx->pressTick        = 0u;
	ctx->longSent         = false;

	ctx->waitingSecond    = false;
	ctx->firstReleaseTick = 0u;
}

ButtonClickType Button_Update(ButtonContext *ctx, GPIO_TypeDef *port, uint16_t pin)
{
	uint32_t now;
	ButtonState raw;
	bool stableChanged;

	if (ctx == NULL || port == NULL) {
		return NO_CLICK;
	}

	now = osKernelGetTickCount();
	raw = ReadButtonRaw(port, pin);

	if (raw != ctx->lastRawState) {
		ctx->lastRawState     = raw;
		ctx->lastDebounceTick = now;
	}

	stableChanged = false;
	if ((now - ctx->lastDebounceTick) >= DEBOUNCE_MS) {
		if (ctx->stableState != ctx->lastRawState) {
			ctx->stableState = ctx->lastRawState;
			stableChanged    = true;
		}
	}

	if (ctx->stableState == BUTTON_PRESSED) {
		if (stableChanged) {
			ctx->pressTick = now;
			ctx->longSent  = false;
		}

		if (!ctx->longSent && (now - ctx->pressTick) >= LONG_THRESHOLD) {
			ctx->longSent      = true;
			ctx->waitingSecond = false;
			return LONG_PRESS;
		}

		return NO_CLICK;
	}

	/* stableState == RELEASED */
	if (stableChanged) {
		if (ctx->longSent) {
			ctx->longSent = false;
			return NO_CLICK;
		}

		if (ctx->waitingSecond &&
		    (now - ctx->firstReleaseTick) <= DOUBLE_THRESHOLD) {
			ctx->waitingSecond = false;
			return DOUBLE_CLICK;
		}

		ctx->waitingSecond    = true;
		ctx->firstReleaseTick = now;
	}

	if (ctx->waitingSecond &&
	    (now - ctx->firstReleaseTick) >= CLICK_DETECTION_PERIOD) {
		ctx->waitingSecond = false;
		return SINGLE_CLICK;
	}

	return NO_CLICK;
}

void ProcessButtonEvent(uint8_t buttonEvent, ButtonClickType *clickType)
{
	if (clickType == NULL) {
		return;
	}

	if (buttonEvent == 0u) {
		*clickType = NO_CLICK;
		return;
	}

	switch (*clickType) {
	case SINGLE_CLICK:
		usbDebug("BUTTON: SINGLE_CLICK\r\n");
		break;
	case DOUBLE_CLICK:
		usbDebug("BUTTON: DOUBLE_CLICK\r\n");
		break;
	case LONG_PRESS:
		usbDebug("BUTTON: LONG_PRESS\r\n");
		break;
	default:
		break;
	}

	*clickType = NO_CLICK;
}
