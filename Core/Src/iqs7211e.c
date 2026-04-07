#include "iqs7211e.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "i2c.h"
#include "main.h"
#include "iqs7211e_init.h"
#include "usb.h"

/* ===== binding ===== */
#define IQS_I2C                    (&hi2c3)
#define IQS_ADDR_7BIT              0x56u
#define IQS_ADDR_8BIT              ((uint16_t)(IQS_ADDR_7BIT << 1))

#define IQS_RDY_PORT               TOUCH_RDY_GPIO_Port
#define IQS_RDY_PIN                TOUCH_RDY_Pin

#define IQS_PRODUCT_NUM            0x0458u
#define IQS_MAX_DELAY              200u

/* ===== memory map ===== */
#define IQS_MM_GESTURES            0x0Eu
#define IQS_MM_STATUS              0x0Fu
#define IQS_MM_INFO_FLAGS          0x10u
#define IQS_MM_FINGER_1_X          0x12u /* 0x12..0x15 */
#define IQS_MM_FINGER_2_X          0x16u /* 0x16..0x19 */
#define IQS_MM_SYS_SETTINGS        0x33u
#define IQS_MM_CONFIG_SETTINGS     0x34u

#define IQS_MM_TOUCH_DELTA_BASE16  0xE200u

#define IQS_CFG_COMMS_END_CMD_BIT  6u
#define IQS_CFG_EVENT_MODE_BIT     0u

#define IQS_SYS_ACK_RESET_BIT      7u
#define IQS_SYS_TP_REDO_ATI_BIT    5u
#define IQS_SYS_STREAM_BIT         0u
#define IQS_SYS_SUSPEND_BIT        3u

static inline uint8_t set_bit(uint8_t v, uint8_t b) { return (uint8_t)(v | (1u << b)); }
static inline uint8_t clr_bit(uint8_t v, uint8_t b) { return (uint8_t)(v & (uint8_t)~(1u << b)); }
static inline bool get_bit(uint8_t v, uint8_t b) { return (bool)((v >> b) & 1u); }

typedef struct {
	iqs_state_e state;
	bool        device_present;
	bool        new_data;

	uint8_t     GESTURES[2];
	uint8_t     INFO_FLAGS[2];
	uint8_t     FINGER_1_XY[4];
	uint8_t     FINGER_2_XY[4];
} iqs_ctx_t;

static iqs_ctx_t g_iqs;

static inline bool rdy_level(void)
{
	return (HAL_GPIO_ReadPin(IQS_RDY_PORT, IQS_RDY_PIN) == GPIO_PIN_SET);
}

static inline bool rdy_low(void)
{
	return !rdy_level();
}

static bool wait_rdy_low(uint32_t ms)
{
	TickType_t dl = xTaskGetTickCount() + pdMS_TO_TICKS(ms);

	do {
		if (rdy_low()) {
			return true;
		}
		vTaskDelay(pdMS_TO_TICKS(1));
	} while ((int32_t)(dl - xTaskGetTickCount()) > 0);

	usbDebug("wait_rdy_low TIMEOUT (%lu ms), RDY=%d", (unsigned long)ms, (int)rdy_level());
	return false;
}

static bool wait_rdy_high(uint32_t ms)
{
	TickType_t dl = xTaskGetTickCount() + pdMS_TO_TICKS(ms);

	do {
		if (rdy_level()) {
			return true;
		}
		vTaskDelay(pdMS_TO_TICKS(1));
	} while ((int32_t)(dl - xTaskGetTickCount()) > 0);

	usbDebug("wait_rdy_high TIMEOUT (%lu ms), RDY=%d",
	        (unsigned long)ms,
	        (int)rdy_level());
	return false;
}

static HAL_StatusTypeDef wr8(uint8_t reg, const uint8_t *data, uint16_t len)
{
	return HAL_I2C_Mem_Write(IQS_I2C,
	                         IQS_ADDR_8BIT,
	                         reg,
	                         I2C_MEMADD_SIZE_8BIT,
	                         (uint8_t *)data,
	                         len,
	                         IQS_MAX_DELAY);
}

static HAL_StatusTypeDef rd8(uint8_t reg, uint8_t *data, uint16_t len)
{
	return HAL_I2C_Mem_Read(IQS_I2C,
	                        IQS_ADDR_8BIT,
	                        reg,
	                        I2C_MEMADD_SIZE_8BIT,
	                        data,
	                        len,
	                        IQS_MAX_DELAY);
}

static bool force_comms_window(void)
{
	static const uint8_t ff[2] = { 0xFFu, 0xFFu };

	if (rdy_low()) {
		return true;
	}

	if (HAL_I2C_Master_Transmit(IQS_I2C, IQS_ADDR_8BIT, (uint8_t *)ff, 2, IQS_MAX_DELAY) != HAL_OK) {
		usbDebug("force_comms_window TX fail err=0x%08lx",
		        (unsigned long)HAL_I2C_GetError(IQS_I2C));
		return false;
	}

	if (!wait_rdy_low(300u)) {
		usbDebug("force_comms_window: RDY did not go LOW");
		return false;
	}

	return true;
}

static bool rd_bytes(uint8_t reg, uint8_t *buf, uint8_t n)
{
	if (!buf || !n) {
		return false;
	}
	if (!wait_rdy_low(100u)) {
		return false;
	}
	if (rd8(reg, buf, n) != HAL_OK) {
		usbDebug("rd_bytes reg=0x%02X n=%u fail err=0x%08lx",
		        reg,
		        n,
		        (unsigned long)HAL_I2C_GetError(IQS_I2C));
		return false;
	}
	return true;
}

static bool wr_bytes(uint8_t reg, const uint8_t *buf, uint8_t n)
{
	if (!buf || !n) {
		return false;
	}
	if (!wait_rdy_low(100u)) {
		return false;
	}
	if (wr8(reg, buf, n) != HAL_OK) {
		usbDebug("wr_bytes reg=0x%02X n=%u fail err=0x%08lx",
		        reg,
		        n,
		        (unsigned long)HAL_I2C_GetError(IQS_I2C));
		return false;
	}
	return true;
}

static void write_block(uint8_t start, const uint8_t *blk, uint8_t len)
{
	(void)wr_bytes(start, blk, len);
}

static void set_event_mode(void)
{
	uint8_t cfg[2];

	cfg[0] = CONFIG_SETTINGS0;
	cfg[1] = (uint8_t)(CONFIG_SETTINGS1 | (1u << IQS_CFG_EVENT_MODE_BIT));

	(void)wr_bytes(IQS_MM_CONFIG_SETTINGS, cfg, 2);
}

static void ack_reset(void)
{
	uint8_t b[2];

	if (!rd_bytes(IQS_MM_SYS_SETTINGS, b, 2)) {
		usbDebug("ack_reset read fail");
		return;
	}

	b[0] = set_bit(b[0], IQS_SYS_ACK_RESET_BIT);
	(void)wr_bytes(IQS_MM_SYS_SETTINGS, b, 2);
}

static void queue_updates(void)
{
	uint8_t t[8];

	if (!rd_bytes(IQS_MM_GESTURES, t, 8)) {
		usbDebug("queue read fail");
		return;
	}

	memcpy(g_iqs.GESTURES,    &t[0], 2);
	memcpy(g_iqs.INFO_FLAGS,  &t[2], 2);
	memcpy(g_iqs.FINGER_1_XY, &t[4], 4);

	g_iqs.new_data = true;
}

static void redo_ati(void)
{
	if (!wait_rdy_high(1000u)) {
		usbDebug("redo_ati: wait H fail");
		return;
	}
	if (!wait_rdy_low(1000u)) {
		usbDebug("redo_ati: wait L fail");
		return;
	}

	{
		uint8_t sys[2];

		if (!rd_bytes(IQS_MM_SYS_SETTINGS, sys, 2)) {
			usbDebug("redo_ati: SYS rd fail");
			return;
		}

		sys[0] = (uint8_t)(sys[0]
		         | (1u << IQS_SYS_ACK_RESET_BIT)
		         | (1u << IQS_SYS_TP_REDO_ATI_BIT));

		if (!wr_bytes(IQS_MM_SYS_SETTINGS, sys, 2)) {
			usbDebug("redo_ati: SYS wr fail");
			return;
		}
	}

	for (uint32_t i = 0; i < 600u; i++) {
		uint8_t stat = 0;

		if (!wait_rdy_high(300u)) {
			break;
		}
		if (!wait_rdy_low(300u)) {
			break;
		}

		if (!rd_bytes(IQS_MM_STATUS, &stat, 1)) {
			usbDebug("redo_ati: STAT rd fail");
			return;
		}

		if ((stat & 0x10u) == 0u) {
			return;
		}

		vTaskDelay(pdMS_TO_TICKS(10));
	}

	usbDebug("redo_ati: timeout");
}

static void setup_official(void)
{
	vTaskDelay(pdMS_TO_TICKS(100));

	if (rdy_level()) {
		(void)force_comms_window();
	}

	if (!wait_rdy_low(100u)) {
		usbDebug("setup: RDY low timeout");
		return;
	}

	{
		uint8_t sys_cfg_other[6] = {
			SYSTEM_CONTROL_0, SYSTEM_CONTROL_1,
			(uint8_t)(CONFIG_SETTINGS0 | (1u << IQS_CFG_COMMS_END_CMD_BIT)), CONFIG_SETTINGS1,
			OTHER_SETTINGS_0, OTHER_SETTINGS_1
		};
		write_block(IQS_MM_SYS_SETTINGS, sys_cfg_other, sizeof(sys_cfg_other));
	}

	{
		uint8_t alp_comp[4] = {
			ALP_COMPENSATION_A_0, ALP_COMPENSATION_A_1,
			ALP_COMPENSATION_B_0, ALP_COMPENSATION_B_1
		};
		write_block(0x1Fu, alp_comp, sizeof(alp_comp));
	}

	{
		uint8_t ati_blk[14] = {
			TP_ATI_MULTIPLIERS_DIVIDERS_0, TP_ATI_MULTIPLIERS_DIVIDERS_1,
			TP_COMPENSATION_DIV,           TP_REF_DRIFT_LIMIT,
			TP_ATI_TARGET_0,               TP_ATI_TARGET_1,
			TP_MIN_COUNT_REATI_0,          TP_MIN_COUNT_REATI_1,
			ALP_ATI_MULTIPLIERS_DIVIDERS_0, ALP_ATI_MULTIPLIERS_DIVIDERS_1,
			ALP_COMPENSATION_DIV,          ALP_LTA_DRIFT_LIMIT,
			ALP_ATI_TARGET_0,              ALP_ATI_TARGET_1
		};
		write_block(0x21u, ati_blk, sizeof(ati_blk));
	}

	{
		uint8_t rpt_blk[22] = {
			ACTIVE_MODE_REPORT_RATE_0,        ACTIVE_MODE_REPORT_RATE_1,
			IDLE_TOUCH_MODE_REPORT_RATE_0,    IDLE_TOUCH_MODE_REPORT_RATE_1,
			IDLE_MODE_REPORT_RATE_0,          IDLE_MODE_REPORT_RATE_1,
			LP1_MODE_REPORT_RATE_0,           LP1_MODE_REPORT_RATE_1,
			LP2_MODE_REPORT_RATE_0,           LP2_MODE_REPORT_RATE_1,
			ACTIVE_MODE_TIMEOUT_0,            ACTIVE_MODE_TIMEOUT_1,
			IDLE_TOUCH_MODE_TIMEOUT_0,        IDLE_TOUCH_MODE_TIMEOUT_1,
			IDLE_MODE_TIMEOUT_0,              IDLE_MODE_TIMEOUT_1,
			LP1_MODE_TIMEOUT_0,               LP1_MODE_TIMEOUT_1,
			REATI_RETRY_TIME,                 REF_UPDATE_TIME,
			I2C_TIMEOUT_0,                    I2C_TIMEOUT_1
		};
		write_block(0x28u, rpt_blk, sizeof(rpt_blk));
	}

	{
		uint8_t alp_blk[4] = {
			ALP_SETUP_0, ALP_SETUP_1,
			ALP_TX_ENABLE_0, ALP_TX_ENABLE_1
		};
		write_block(0x36u, alp_blk, sizeof(alp_blk));
	}

	{
		uint8_t thr_blk[6] = {
			TRACKPAD_TOUCH_SET_THRESHOLD,   TRACKPAD_TOUCH_CLEAR_THRESHOLD,
			ALP_THRESHOLD_0,                ALP_THRESHOLD_1,
			ALP_SET_DEBOUNCE,               ALP_CLEAR_DEBOUNCE
		};
		write_block(0x38u, thr_blk, sizeof(thr_blk));
	}

	{
		uint8_t betas_blk[4] = {
			ALP_COUNT_BETA_LP1, ALP_LTA_BETA_LP1,
			ALP_COUNT_BETA_LP2, ALP_LTA_BETA_LP2
		};
		write_block(0x3Bu, betas_blk, sizeof(betas_blk));
	}

	{
		uint8_t hw_blk[8] = {
			TP_CONVERSION_FREQUENCY_UP_PASS_LENGTH,  TP_CONVERSION_FREQUENCY_FRACTION_VALUE,
			ALP_CONVERSION_FREQUENCY_UP_PASS_LENGTH, ALP_CONVERSION_FREQUENCY_FRACTION_VALUE,
			TRACKPAD_HARDWARE_SETTINGS_0,            TRACKPAD_HARDWARE_SETTINGS_1,
			ALP_HARDWARE_SETTINGS_0,                 ALP_HARDWARE_SETTINGS_1
		};
		write_block(0x3Du, hw_blk, sizeof(hw_blk));
	}

	{
		uint8_t tp_blk[18] = {
			TRACKPAD_SETTINGS_0_0,                 TRACKPAD_SETTINGS_0_1,
			TRACKPAD_SETTINGS_1_0,                 TRACKPAD_SETTINGS_1_1,
			X_RESOLUTION_0,                        X_RESOLUTION_1,
			Y_RESOLUTION_0,                        Y_RESOLUTION_1,
			XY_DYNAMIC_FILTER_BOTTOM_SPEED_0,      XY_DYNAMIC_FILTER_BOTTOM_SPEED_1,
			XY_DYNAMIC_FILTER_TOP_SPEED_0,         XY_DYNAMIC_FILTER_TOP_SPEED_1,
			XY_DYNAMIC_FILTER_BOTTOM_BETA,         XY_DYNAMIC_FILTER_STATIC_FILTER_BETA,
			STATIONARY_TOUCH_MOV_THRESHOLD,        FINGER_SPLIT_FACTOR,
			X_TRIM_VALUE,                          Y_TRIM_VALUE
		};
		write_block(0x41u, tp_blk, sizeof(tp_blk));
	}

	{
		uint8_t ges_blk[22] = {
			GESTURE_ENABLE_0,          GESTURE_ENABLE_1,
			TAP_TOUCH_TIME_0,          TAP_TOUCH_TIME_1,
			TAP_WAIT_TIME_0,           TAP_WAIT_TIME_1,
			TAP_DISTANCE_0,            TAP_DISTANCE_1,
			HOLD_TIME_0,               HOLD_TIME_1,
			SWIPE_TIME_0,              SWIPE_TIME_1,
			SWIPE_X_DISTANCE_0,        SWIPE_X_DISTANCE_1,
			SWIPE_Y_DISTANCE_0,        SWIPE_Y_DISTANCE_1,
			SWIPE_X_CONS_DIST_0,       SWIPE_X_CONS_DIST_1,
			SWIPE_Y_CONS_DIST_0,       SWIPE_Y_CONS_DIST_1,
			SWIPE_ANGLE,               PALM_THRESHOLD
		};
		write_block(0x4Bu, ges_blk, sizeof(ges_blk));
	}

	{
		uint8_t map_blk[14] = {
			RX_TX_MAP_0, RX_TX_MAP_1, RX_TX_MAP_2,  RX_TX_MAP_3,
			RX_TX_MAP_4, RX_TX_MAP_5, RX_TX_MAP_6,  RX_TX_MAP_7,
			RX_TX_MAP_8, RX_TX_MAP_9, RX_TX_MAP_10, RX_TX_MAP_11,
			RX_TX_MAP_12, RX_TX_MAP_FILLER
		};
		write_block(0x56u, map_blk, sizeof(map_blk));
	}

	{
		uint8_t c0_9[30] = {
			PLACEHOLDER_0,  CH_1_CYCLE_0,  CH_2_CYCLE_0,
			PLACEHOLDER_1,  CH_1_CYCLE_1,  CH_2_CYCLE_1,
			PLACEHOLDER_2,  CH_1_CYCLE_2,  CH_2_CYCLE_2,
			PLACEHOLDER_3,  CH_1_CYCLE_3,  CH_2_CYCLE_3,
			PLACEHOLDER_4,  CH_1_CYCLE_4,  CH_2_CYCLE_4,
			PLACEHOLDER_5,  CH_1_CYCLE_5,  CH_2_CYCLE_5,
			PLACEHOLDER_6,  CH_1_CYCLE_6,  CH_2_CYCLE_6,
			PLACEHOLDER_7,  CH_1_CYCLE_7,  CH_2_CYCLE_7,
			PLACEHOLDER_8,  CH_1_CYCLE_8,  CH_2_CYCLE_8,
			PLACEHOLDER_9,  CH_1_CYCLE_9,  CH_2_CYCLE_9
		};
		write_block(0x5Du, c0_9, sizeof(c0_9));
	}

	{
		uint8_t c10_19[30] = {
			PLACEHOLDER_10, CH_1_CYCLE_10, CH_2_CYCLE_10,
			PLACEHOLDER_11, CH_1_CYCLE_11, CH_2_CYCLE_11,
			PLACEHOLDER_12, CH_1_CYCLE_12, CH_2_CYCLE_12,
			PLACEHOLDER_13, CH_1_CYCLE_13, CH_2_CYCLE_13,
			PLACEHOLDER_14, CH_1_CYCLE_14, CH_2_CYCLE_14,
			PLACEHOLDER_15, CH_1_CYCLE_15, CH_2_CYCLE_15,
			PLACEHOLDER_16, CH_1_CYCLE_16, CH_2_CYCLE_16,
			PLACEHOLDER_17, CH_1_CYCLE_17, CH_2_CYCLE_17,
			PLACEHOLDER_18, CH_1_CYCLE_18, CH_2_CYCLE_18,
			PLACEHOLDER_19, CH_1_CYCLE_19, CH_2_CYCLE_19
		};
		write_block(0x6Cu, c10_19, sizeof(c10_19));
	}

	{
		uint8_t c20[3] = { PLACEHOLDER_20, CH_1_CYCLE_20, CH_2_CYCLE_20 };
		write_block(0x7Bu, c20, sizeof(c20));
	}

	{
		uint8_t cfg0_close = CONFIG_SETTINGS0;
		(void)wr_bytes(IQS_MM_CONFIG_SETTINGS, &cfg0_close, 1);
	}

	vTaskDelay(pdMS_TO_TICKS(1));

	redo_ati();
	set_event_mode();

	{
		uint8_t b2[2];

		if (rd_bytes(IQS_MM_SYS_SETTINGS, b2, 2)) {
			b2[1] = clr_bit(b2[1], IQS_SYS_STREAM_BIT);
			(void)wr_bytes(IQS_MM_SYS_SETTINGS, b2, 2);
		}
	}
}

static iqs_gesture_e decode_gesture(uint8_t g0, uint8_t g1)
{
	if (get_bit(g0, 0)) return IQS_G_SINGLE_TAP;
	if (get_bit(g0, 1)) return IQS_G_DOUBLE_TAP;
	if (get_bit(g0, 2)) return IQS_G_TRIPLE_TAP;
	if (get_bit(g0, 3)) return IQS_G_PRESS_HOLD;
	if (get_bit(g0, 4)) return IQS_G_PALM;
	if (get_bit(g1, 0)) return IQS_G_SWIPE_X_POS;
	if (get_bit(g1, 1)) return IQS_G_SWIPE_X_NEG;
	if (get_bit(g1, 2)) return IQS_G_SWIPE_Y_POS;
	if (get_bit(g1, 3)) return IQS_G_SWIPE_Y_NEG;
	if (get_bit(g1, 4)) return IQS_G_SWIPE_HOLD_X_POS;
	if (get_bit(g1, 5)) return IQS_G_SWIPE_HOLD_X_NEG;
	if (get_bit(g1, 6)) return IQS_G_SWIPE_HOLD_Y_POS;
	if (get_bit(g1, 7)) return IQS_G_SWIPE_HOLD_Y_NEG;

	return IQS_G_NONE;
}

/* ===== public API ===== */
HAL_StatusTypeDef IQS7211E_Init(void)
{
	uint8_t  tmp[2];
	uint16_t prod;

	memset(&g_iqs, 0, sizeof(g_iqs));
	g_iqs.state = IQS_STATE_FAIL;

	vTaskDelay(pdMS_TO_TICKS(100));

	if (rdy_level()) {
		(void)force_comms_window();
	}

	if (!wait_rdy_low(100u)) {
		g_iqs.device_present = false;
		g_iqs.state          = IQS_STATE_FAIL;
		return HAL_TIMEOUT;
	}

	if (!rd_bytes(0x00u, tmp, 2)) {
		g_iqs.device_present = false;
		g_iqs.state          = IQS_STATE_FAIL;
		return HAL_ERROR;
	}

	prod = (uint16_t)((tmp[1] << 8) | tmp[0]);

	g_iqs.device_present = (prod == IQS_PRODUCT_NUM);
	g_iqs.state          = g_iqs.device_present ? IQS_STATE_INIT : IQS_STATE_FAIL;

	return g_iqs.device_present ? HAL_OK : HAL_ERROR;
}

void IQS7211E_Run(void)
{
	if (!g_iqs.device_present) {
		return;
	}

	switch (g_iqs.state) {
	case IQS_STATE_INIT:
		setup_official();
		g_iqs.state    = IQS_STATE_RUN;
		g_iqs.new_data = true;
		break;

	case IQS_STATE_RUN:
		if (rdy_low()) {
			queue_updates();

			if (get_bit(g_iqs.INFO_FLAGS[0], 7)) {
				ack_reset();
				g_iqs.state = IQS_STATE_INIT;
			}
		}
		break;

	default:
		break;
	}
}

iqs_state_e IQS7211E_GetState(void) { return g_iqs.state; }
bool IQS7211E_IsPresent(void) { return g_iqs.device_present; }
bool IQS7211E_HasNewData(void) { return g_iqs.new_data; }
void IQS7211E_ClearNewData(void) { g_iqs.new_data = false; }

iqs_gesture_e IQS7211E_GetGesture(void)
{
	return decode_gesture(g_iqs.GESTURES[0], g_iqs.GESTURES[1]);
}

HAL_StatusTypeDef IQS7211E_GetAbsXY(uint8_t finger, uint16_t *x, uint16_t *y)
{
	if (!x || !y) {
		return HAL_ERROR;
	}

	if (finger == 1u) {
		*x = (uint16_t)((g_iqs.FINGER_1_XY[1] << 8) | g_iqs.FINGER_1_XY[0]);
		*y = (uint16_t)((g_iqs.FINGER_1_XY[3] << 8) | g_iqs.FINGER_1_XY[2]);
		return HAL_OK;
	}

	if (finger == 2u) {
		*x = (uint16_t)((g_iqs.FINGER_2_XY[1] << 8) | g_iqs.FINGER_2_XY[0]);
		*y = (uint16_t)((g_iqs.FINGER_2_XY[3] << 8) | g_iqs.FINGER_2_XY[2]);
		return HAL_OK;
	}

	return HAL_ERROR;
}

HAL_StatusTypeDef IQS7211E_Suspend(bool enable)
{
	uint8_t b[2];

	if (!wait_rdy_low(100u)) {
		return HAL_TIMEOUT;
	}

	if (!rd_bytes(IQS_MM_SYS_SETTINGS, b, 2)) {
		return HAL_ERROR;
	}

	b[1] = enable ? set_bit(b[1], IQS_SYS_SUSPEND_BIT)
	              : clr_bit(b[1], IQS_SYS_SUSPEND_BIT);

	return wr_bytes(IQS_MM_SYS_SETTINGS, b, 2) ? HAL_OK : HAL_ERROR;
}

static HAL_StatusTypeDef i2c16_repeat_read(uint16_t mem16, uint8_t *dst, uint16_t len)
{
	uint8_t a[2];

	a[0] = (uint8_t)((mem16 >> 8) & 0xFFu);
	a[1] = (uint8_t)(mem16 & 0xFFu);

	HAL_StatusTypeDef st;

	st = HAL_I2C_Master_Transmit(IQS_I2C, IQS_ADDR_8BIT, a, 2, IQS_MAX_DELAY);
	if (st != HAL_OK) {
		return st;
	}

	st = HAL_I2C_Master_Receive(IQS_I2C, IQS_ADDR_8BIT, dst, len, IQS_MAX_DELAY);
	return st;
}

HAL_StatusTypeDef IQS7211E_ReadDeltaAll(int16_t out_delta[42])
{
	if (!out_delta) {
		return HAL_ERROR;
	}

	if (!wait_rdy_low(100u)) {
		return HAL_TIMEOUT;
	}

	{
		uint8_t cfg0_open = (uint8_t)(CONFIG_SETTINGS0 | (1u << IQS_CFG_COMMS_END_CMD_BIT));
		if (!wr_bytes(IQS_MM_CONFIG_SETTINGS, &cfg0_open, 1)) {
			return HAL_ERROR;
		}
	}

	{
		uint8_t buf[84];

		for (uint8_t k = 0; k < 7u; k++) {
			uint16_t base = (uint16_t)(IQS_MM_TOUCH_DELTA_BASE16 + (uint16_t)(k * 6u));
			uint16_t off  = (uint16_t)(k * 12u);

			if (i2c16_repeat_read(base, &buf[off], 12) != HAL_OK) {
				goto close_window_fail;
			}
		}

		for (uint8_t i = 0; i < 42u; i++) {
			uint8_t j   = (uint8_t)(i * 2u);
			out_delta[i] = (int16_t)((buf[j + 1] << 8) | buf[j]);
		}
	}

close_window_fail:
	{
		uint8_t cfg0_close = CONFIG_SETTINGS0;
		(void)wr_bytes(IQS_MM_CONFIG_SETTINGS, &cfg0_close, 1);
	}

	return HAL_OK;
}
