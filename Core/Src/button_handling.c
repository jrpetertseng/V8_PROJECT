#include "button_handling.h"
#include "cmsis_os.h"
#include "usb.h"

extern void SystemClock_Config(void);

static uint32_t s_last_single_click_tick = 0u;
static uint32_t s_wake_lock_until_tick = 0u;

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
    usbDebug("BUTTON: ENTER_STOP\r\n");
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

    usbDebug("BUTTON: EXIT_STOP\r\n");
}

#include "debug_defs.h"

#if ENABLE_SUSPEND_RESUME
extern void SystemClock_Config(void);

extern void MX_PB_Init(ButtonMode_TypeDef ButtonMode);

/* RTC handler declaration */
//RTC_HandleTypeDef RTCHandle;

#if SLEEP_MODE
static void EnterSleepMode(void)
{
	   GPIO_InitTypeDef GPIO_InitStruct;

	   usbDebug("s+ \r\n", __func__);
#if 0
	   /* Configure all GPIO as analog to reduce current consumption on non used IOs */
	  /* Enable GPIOs clock */
	   __HAL_RCC_GPIOA_CLK_ENABLE();
	   __HAL_RCC_GPIOB_CLK_ENABLE();
	   __HAL_RCC_GPIOC_CLK_ENABLE();
	   __HAL_RCC_GPIOD_CLK_ENABLE();
	   __HAL_RCC_GPIOE_CLK_ENABLE();
	   __HAL_RCC_GPIOF_CLK_ENABLE();
	   __HAL_RCC_GPIOG_CLK_ENABLE();
	   __HAL_RCC_GPIOH_CLK_ENABLE();
	   __HAL_RCC_GPIOI_CLK_ENABLE();

	  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
	  GPIO_InitStruct.Pull = GPIO_NOPULL;
	  GPIO_InitStruct.Pin = GPIO_PIN_All;
	  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
	  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
	  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
	  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
	  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);
	  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	  /* Disable GPIOs clock */
	   __HAL_RCC_GPIOA_CLK_DISABLE();
	   __HAL_RCC_GPIOB_CLK_DISABLE();
	   __HAL_RCC_GPIOC_CLK_DISABLE();
	   __HAL_RCC_GPIOD_CLK_DISABLE();
	   __HAL_RCC_GPIOE_CLK_DISABLE();
	   __HAL_RCC_GPIOF_CLK_DISABLE();
	   __HAL_RCC_GPIOG_CLK_DISABLE();
	   __HAL_RCC_GPIOH_CLK_DISABLE();
	   __HAL_RCC_GPIOI_CLK_DISABLE();
#endif

	  /* Configure USER Button */
	   MX_PB_Init(BUTTON_MODE_EXTI);

	  /* Suspend Tick increment to prevent wakeup by Systick interrupt.
	     Otherwise the Systick interrupt will wake up the device within 1ms (HAL time base) */
	  HAL_SuspendTick();

	  /* Request to enter SLEEP mode */
	  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

	  /* Resume Tick interrupt if disabled prior to sleep mode entry */
	  HAL_ResumeTick();

	  usbDebug("s- \r\n", __func__);
}
#endif

#if STOP_MODE
static void EnterStopMode(void)
{
	  GPIO_InitTypeDef GPIO_InitStruct;

	  usbDebug("s+ \r\n", __func__);

	  /* Configure all GPIO as analog to reduce current consumption on non used IOs */
	  /* Enable GPIOs clock */
	   __HAL_RCC_GPIOA_CLK_ENABLE();
	   __HAL_RCC_GPIOB_CLK_ENABLE();
	   __HAL_RCC_GPIOC_CLK_ENABLE();
	   __HAL_RCC_GPIOD_CLK_ENABLE();
	   __HAL_RCC_GPIOE_CLK_ENABLE();
	   __HAL_RCC_GPIOF_CLK_ENABLE();
	   __HAL_RCC_GPIOG_CLK_ENABLE();
	   __HAL_RCC_GPIOH_CLK_ENABLE();
	   __HAL_RCC_GPIOI_CLK_ENABLE();


	  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
	  GPIO_InitStruct.Pull = GPIO_NOPULL;
	  GPIO_InitStruct.Pin = GPIO_PIN_All;
	  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
	  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
	  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
	  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
	  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);
	  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	  /* Disable GPIOs clock */
	   __HAL_RCC_GPIOA_CLK_DISABLE();
	   __HAL_RCC_GPIOB_CLK_DISABLE();
	   __HAL_RCC_GPIOC_CLK_DISABLE();
	   __HAL_RCC_GPIOD_CLK_DISABLE();
	   __HAL_RCC_GPIOE_CLK_DISABLE();
	   __HAL_RCC_GPIOF_CLK_DISABLE();
	   __HAL_RCC_GPIOG_CLK_DISABLE();
	   __HAL_RCC_GPIOH_CLK_DISABLE();
	   __HAL_RCC_GPIOI_CLK_DISABLE();

	  RTCHandle.Instance = RTC;

	  /* Configure RTC prescaler and RTC data registers as follow:
	  - Hour Format = Format 24
	  - Asynch Prediv = Value according to source clock
	  - Synch Prediv = Value according to source clock
	  - OutPut = Output Disable
	  - OutPutPolarity = High Polarity
	  - OutPutType = Open Drain */
	  RTCHandle.Init.HourFormat = RTC_HOURFORMAT_24;
	  RTCHandle.Init.AsynchPrediv = RTC_ASYNCH_PREDIV;
	  RTCHandle.Init.SynchPrediv = RTC_SYNCH_PREDIV;
	  RTCHandle.Init.OutPut = RTC_OUTPUT_DISABLE;
	  RTCHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	  RTCHandle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

	  if(HAL_RTC_Init(&RTCHandle) != HAL_OK)
	  {
	    /* Initialization Error */
	    Error_Handler();
	  }

	   /*## Configure the Wake up timer ###########################################*/
	  /*  RTC Wakeup Interrupt Generation:
	      Wakeup Time Base = (RTC_WAKEUPCLOCK_RTCCLK_DIV /(LSI))
	      Wakeup Time = Wakeup Time Base * WakeUpCounter
	                  = (RTC_WAKEUPCLOCK_RTCCLK_DIV /(LSI)) * WakeUpCounter
	      ==> WakeUpCounter = Wakeup Time / Wakeup Time Base

	      To configure the wake up timer to 20s the WakeUpCounter is set to 0xA017:
	        RTC_WAKEUPCLOCK_RTCCLK_DIV = RTCCLK_Div16 = 16
	        Wakeup Time Base = 16 /(~32.768KHz) = ~0,488 ms
	        Wakeup Time = ~20s = 0,488ms  * WakeUpCounter
	        ==> WakeUpCounter = ~20s/0,488ms = 40983 = 0xA017 */
	  /* Disable Wake-up timer */
	  if(HAL_RTCEx_DeactivateWakeUpTimer(&RTCHandle) != HAL_OK)
	  {
	    /* Initialization Error */
	    Error_Handler();
	  }

	  /*## Clear all related wakeup flags ########################################*/
	  /* Clear RTC Wake Up timer Flag */
	  __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&RTCHandle, RTC_FLAG_WUTF);

	  /*## Setting the Wake up time ##############################################*/
	  HAL_RTCEx_SetWakeUpTimer_IT(&RTCHandle, 0xA017, RTC_WAKEUPCLOCK_RTCCLK_DIV16);

	  /* FLASH Deep Power Down Mode enabled */
	  HAL_PWREx_EnableFlashPowerDown();

	  /* Enter Stop Mode */
	  HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

	  /* Configures system clock after wake-up from STOP: enable HSE, PLL and select
	  PLL as system clock source (HSE and PLL are disabled in STOP mode) */
	  SYSCLKConfig_STOP();

	  /* Disable Wake-up timer */
	  if(HAL_RTCEx_DeactivateWakeUpTimer(&RTCHandle) != HAL_OK)
	  {
	    /* Initialization Error */
	    Error_Handler();
	  }

	  usbDebug("s- \r\n", __func__);

}
#endif

#if STANDBY_MODE
static void EnterStandbyMode(void)
{
	usbDebug("s+ \r\n", __func__);

	/* Enable Power Clock */
	__HAL_RCC_PWR_CLK_ENABLE();

	/* Re-enable all used wakeup sources: user button (PC.13) */
	HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN4);

	/* Clear all related wakeup flags */
	__HAL_PWR_CLEAR_WAKEUP_FLAG(PWR_WAKEUP_PIN_FLAG4);

	/*## Enter Standby Mode ####################################################*/
	/* Request to enter STANDBY mode  */
	HAL_PWR_EnterSTANDBYMode();

	usbDebug("s+ \r\n", __func__);

}
#endif

static void EnterPowerOffMode(void)
{
	usbDebug("s+ \r\n", __func__);

	/* Enable Power Clock */
	__HAL_RCC_PWR_CLK_ENABLE();

	/* Re-enable all used wakeup sources: user button (PC.13) */
	HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN4);

	/* Clear all related wakeup flags */
	__HAL_PWR_CLEAR_WAKEUP_FLAG(PWR_WAKEUP_PIN_FLAG4);

	/*## Enter Standby Mode ####################################################*/
	/* Request to enter STANDBY mode  */
	HAL_PWR_EnterSTANDBYMode();

	usbDebug("s+ \r\n", __func__);

}

/* Testing code @2026/05/05 by Ludy
 *
 */
static void EnterStandbyFromButton(void)
{
    usbDebug("BUTTON: ENTER_STOP\r\n");
    osDelay(20);
    HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI);
    SystemClock_Config();
    usbDebug("BUTTON: EXIT_STOP\r\n");
}

#endif

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

		/* Ignore any click shortly after STOP wakeup. */
		if ((int32_t)(nowTick - s_wake_lock_until_tick) < 0) {
			break;
		}

		/* Throttle repeated single-click events caused by mechanical bounce. */
		if ((nowTick - s_last_single_click_tick) < 250u) {
			break;
		}
		s_last_single_click_tick = nowTick;

		usbDebug("BUTTON: SINGLE_CLICK\r\n");
		EnterStopmodebyFromButton();
		break;
	}
	case DOUBLE_CLICK:
		usbDebug("BUTTON: DOUBLE_CLICK\r\n");
		break;
	case LONG_PRESS:
		usbDebug("BUTTON: LONG_PRESS\r\n");

		/* Delay 200 ms */
		osDelay(500);

		//EnterPowerOffMode();
		break;
	default:
		break;
	}

	*clickType = NO_CLICK;
}
