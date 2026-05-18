#include "button_handling.h"
#include "cmsis_os.h"
#include "usb.h"
#include "ecx343.h"
#include "usb_device.h"
#include "usbd_composite.h"

#include "al3010.h"

extern void SystemClock_Config(void);
extern USBD_HandleTypeDef hUsbDeviceHS;
extern uint8_t isPanelOn;
extern uint8_t Button_Log;

static uint32_t s_last_single_click_tick = 0u;
static uint32_t s_wake_lock_until_tick = 0u;
static uint8_t s_button_two_step_is_off = 0u;

#define BUTTON_LOG_PRINT(...) \
    do { \
        if (Button_Log != 0u) { \
            usbDebug(__VA_ARGS__); \
        } \
    } while (0)

static void TouchRdyInterruptEnable(uint8_t enable)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = TOUCH_RDY_Pin;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Mode = enable ? GPIO_MODE_IT_FALLING : GPIO_MODE_INPUT;
    HAL_GPIO_Init(TOUCH_RDY_GPIO_Port, &GPIO_InitStruct);
}

static void EnterStopmodebyFromButton(void)
{
    BUTTON_LOG_PRINT("BUTTON: ENTER_STOP\r\n");
    osDelay(20);

    /* Only button can wake STOP: mask Touch/ALS wake sources. */
    HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
    TouchRdyInterruptEnable(0);

    /* Make sure stale EXTI flags won't wake us immediately. */
    __HAL_GPIO_EXTI_CLEAR_IT(POWER_SW_KEY_Pin);
    __HAL_GPIO_EXTI_CLEAR_IT(TOUCH_RDY_Pin);
    __HAL_GPIO_EXTI_CLEAR_IT(ALS_INT_Pin);
    HAL_NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
    HAL_NVIC_ClearPendingIRQ(EXTI9_5_IRQn);

    HAL_SuspendTick();
    HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI);
    HAL_ResumeTick();

    SystemClock_Config();

    /* Restore sensor interrupts after wakeup. */
    TouchRdyInterruptEnable(1);
    __HAL_GPIO_EXTI_CLEAR_IT(TOUCH_RDY_Pin);
    __HAL_GPIO_EXTI_CLEAR_IT(ALS_INT_Pin);
    HAL_NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
    HAL_NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

    /* Guard against key bounce/re-trigger right after wakeup. */
    s_wake_lock_until_tick = osKernelGetTickCount() + 2000u;

    BUTTON_LOG_PRINT("BUTTON: EXIT_STOP\r\n");
}

static void ButtonTwoStep_Off(void)
{
    BUTTON_LOG_PRINT("BUTTON: STEP1 OFF (FEATURE->USB)\r\n");	

    /* Step 1: function off first. */
    if (isPanelOn) {
        executeTaskWithMutex(POWER_OFF, PANEL_BOTH);
        isPanelOn = 0u;
    }

    /* Then USB disconnect. */
    USBD_Stop(&hUsbDeviceHS);
    osDelay(20);
    USBD_DeInit(&hUsbDeviceHS);

	/* Disable ALS power */	
	HAL_GPIO_WritePin(ALS_RST_GPIO_Port, ALS_RST_Pin, GPIO_PIN_RESET);
    osDelay(100);

    /* Enter STM32 STOP mode; next button press wakes up. */
    EnterStopmodebyFromButton();
}

static void ButtonTwoStep_On(void)
{
	/* Re-enable ALS power */
	HAL_GPIO_WritePin(ALS_RST_GPIO_Port, ALS_RST_Pin, GPIO_PIN_SET);
    osDelay(100);

	AL3010_Init();
	osDelay(10);

	/* Step 2: USB connect first. */
    MX_USB_DEVICE_Init();
    osDelay(20);

    /* Then function on. */
    if (!isPanelOn) {
        executeTaskWithMutex(POWER_ON, PANEL_BOTH);
        isPanelOn = 1u;
    }

    osDelay(1000);


    BUTTON_LOG_PRINT("BUTTON: STEP2 ON (USB->FEATURE)\r\n");	
}

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
	{
		uint32_t nowTick = osKernelGetTickCount();

		/* Throttle repeated single-click events caused by mechanical bounce. */
		if ((nowTick - s_last_single_click_tick) < 250u) {
			break;
		}
		s_last_single_click_tick = nowTick;

		BUTTON_LOG_PRINT("BUTTON: SINGLE_CLICK\r\n");

		if (!s_button_two_step_is_off) {
			s_button_two_step_is_off = 1u;
			ButtonTwoStep_Off();
		} else {
			ButtonTwoStep_On();
			s_button_two_step_is_off = 0u;
		}
		break;
	}
	case DOUBLE_CLICK:
		BUTTON_LOG_PRINT("BUTTON: DOUBLE_CLICK\r\n");
		break;
	case LONG_PRESS:
		BUTTON_LOG_PRINT("BUTTON: LONG_PRESS\r\n");
		break;
	default:
		break;
	}

	*clickType = NO_CLICK;
}
