/*
 * ecx343.c
 *
 *  Created on: Dec 21, 2022
 *      Author: User
 */
#include "stdbool.h"
#include "math.h"
#include "usb.h"
#include "main.h"
#include "ecx343.h"
#include "flash_rw_process.h"
#include "cmd_engine.h"
#include "lt7911.h"
#include <string.h>
#include "semphr.h"
#include <stdarg.h>

/* External Declarations */
extern ECX343_DATA ecx343_data;
extern ECX343_DATA ecx343_current_data;
extern uint32_t ecx343RW_buf[sizeof(ECX343_DATA) / 4];
extern SPI_HandleTypeDef hspi2;
extern SPI_HandleTypeDef hspi4;
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;

/* Constants */
const uint8_t leftInversionMasks[4] = {0x80, 0x84, 0x8C, 0x88};
const uint8_t rightInversionMasks[4] = {0x80, 0x84, 0x8C, 0x88};
const setPanelSeqTable PanelContPanelReg60HzSettingTable[];
const setPanelSeqTable PanelContPanelReg120HzSettingTable[];
const setPanelSeqTable LeftPanelContLuminance60HzSettingTable[];
const setPanelSeqTable RightPanelContLuminance60HzSettingTable[];
const setPanelSeqTable LeftPanelContLuminance120HzSettingTable[];
const setPanelSeqTable RightPanelContLuminance120HzSettingTable[];
const setPanelSeqTable PanelContPSReleaseTable[];
const setPanelSeqTable PanelContPSTransitionTable[];

/* Global Variables */
PanelMode currentPanelMode = MODE_2D_60HZ;
float g_temperatureLeftRaw = 0.0f;
float g_temperatureRightRaw = 0.0f;
float g_temperatureLeftSmoothed = 0.0f;
float g_temperatureRightSmoothed = 0.0f;
SemaphoreHandle_t xMutex;

/* Static Variables */
static SPI_HandleTypeDef *current_spi_handle = NULL;
static uint8_t currentPanelMap;
static PanelSide currentPanelSide;
static uint8_t pow_on_seq = 1;
static uint8_t pow_off_seq = 1;
static uint8_t panel_cont_seq = 0;
static CircularBuffer leftTemperatureBuffer, rightTemperatureBuffer;
static bool isBufferInitialized = false;
static uint16_t previousLeftBrightness;
static uint16_t previousRightBrightness;

/* Static Functions */
static bool is_within_range(int value, int min, int max);
static bool is_within_ranges(int value, int min1, int max1, int min2, int max2);
static bool is_ecx343RW_buf_valid(ECX343_DATA *buf);
static void ECX343EN_ActivatePanel(PanelSide side);
static void ECX343EN_PowerOn(void);
static void ECX343EN_PowerOff(void);
static void ECX343EN_StartPanelPowerSequence(PanelSide side);
static void ECX343EN_StopPanelPowerSequence(PanelSide side);
static void ECX343EN_PanelPSRelease(void);
static PowContStatus pow_on_sequence(void);
static PowContStatus pow_off_sequence(void);
static PowContStatus panel_reg_setting(void);
static PowContStatus panel_luminance_setting(void);
static PowContStatus panel_ps_release(void);
static PowContStatus panel_ps_transition(void);
static PanelRegStatus panel_reg_read(uint8_t map, uint8_t addr, uint8_t *value);
static PanelRegStatus panel_reg_write(uint8_t map, uint8_t addr, uint8_t value);
static PanelRegStatus panel_reg_read_value(uint8_t addr, uint8_t *value);
static PanelRegStatus panel_reg_write_value(uint8_t addr, uint8_t value);
static void writeRegisters(ECX343_DATA *data);
static float exponential_smoothing_sequence(CircularBuffer *cb);
static float readVoltageAndCalculate(PanelSide site, uint8_t voltage, ADC_HandleTypeDef *hadc);
static void ECX343EN_Luminance(PanelSide side, uint16_t brightness);
static void ECX343EN_ArbitraryLuminanceH(PanelSide side, uint8_t value);
static void ECX343EN_ArbitraryLuminanceL(PanelSide side, uint8_t value);

/*Helper Functions */
static void spi_enable(bool enable);

void executeTaskWithMutex(TaskID taskID, ...) {
    va_list args;
    va_start(args, taskID);

    if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
        switch (taskID) {
            case POWER_ON:
                panelPowerOn(va_arg(args, int));
                break;
            case POWER_OFF:
                panelPowerOff(va_arg(args, int));
                break;
            case WRITE_REGISTERS:
                {
                    ECX343_DATA *data = va_arg(args, ECX343_DATA*);
                    ECX343EN_WriteRegisters(data);
                }
                break;
            case INVERSION:
                {
                    int side = va_arg(args, int);
                    uint8_t value = (uint8_t) va_arg(args, int);
                    ECX343EN_Inversion(side, value);
                }
                break;
            case LUMINANCE_MODE:
                {
                    int side = va_arg(args, int);
                    uint8_t value = (uint8_t) va_arg(args, int);
                    ECX343EN_LuminanceModeSetting(side, value);
                }
                break;
            case PRESET_LUMINANCE:
                {
                    int side = va_arg(args, int);
                    uint8_t value = (uint8_t) va_arg(args, int);
                    ECX343EN_PresetLuminanceValue(side, value);
                }
                break;
            case ORBIT_H:
                {
                    int side = va_arg(args, int);
                    uint8_t value = (uint8_t) va_arg(args, int);
                    ECX343EN_OrbitH(side, value);
                }
                break;
            case ORBIT_V:
                {
                    int side = va_arg(args, int);
                    uint8_t value = (uint8_t) va_arg(args, int);
                    ECX343EN_OrbitV(side, value);
                }
                break;
            case ADJUST_BRIGHTNESS:
                adjustBrightness();
                break;
            case UPDATE_TEMPERATURE:
                {
                    int side = va_arg(args, int);
                    uint8_t value = (uint8_t) va_arg(args, int);
                    ECX343EN_TempDetect(side, value);
                }
                break;
            default:
                break;
        }
        xSemaphoreGive(xMutex);
    }
    va_end(args);
}

void ECX343EN_Init(void)
{
	currentPanelMap = PANEL_MAP_0;

    memset(ecx343RW_buf, 0, sizeof(ecx343RW_buf));
    memset((void *)&ecx343_current_data, 0, sizeof(ECX343_DATA));
    Flash_Read_Data(0x08010000 , ecx343RW_buf, (sizeof(ecx343RW_buf)/4)-1);
    memcpy((void *)&ecx343_current_data, ecx343RW_buf, sizeof(ECX343_DATA));

    if (!is_ecx343RW_buf_valid(&ecx343_current_data)) {
        ecx343_current_data = ecx343_data;
    }

    if (!Check_Ecx343_data_checksum(ecx343_current_data)) {
        ecx343_current_data = ecx343_data;
    }

    LT7911_Mode_Switch(ecx343_current_data.uLCD_MODE);
    currentPanelMode = ecx343_current_data.uLCD_MODE;
    previousLeftBrightness = ecx343_current_data.uLCD_LUXL;
    previousRightBrightness = ecx343_current_data.uLCD_LUXR;

	xMutex = xSemaphoreCreateMutex();
	if (xMutex == NULL)	{
        while (1) {
        }
	}
}

void panelPowerOn(PanelSide side)
{
	ECX343EN_PowerOn();
	switch(side)
	{
		case PANEL_LEFT:
			ECX343EN_StartPanelPowerSequence(PANEL_LEFT);
			osDelay(10);
			break;
		case PANEL_RIGHT:
			ECX343EN_StartPanelPowerSequence(PANEL_RIGHT);
			osDelay(10);
			break;
		case PANEL_BOTH:
			ECX343EN_StartPanelPowerSequence(PANEL_LEFT);
			osDelay(10);
			ECX343EN_StartPanelPowerSequence(PANEL_RIGHT);
			osDelay(10);
			break;
		default:
			break;
	}
	ECX343EN_PanelPSRelease();
}

void panelPowerOff(PanelSide side)
{
	switch(side)
	{
		case PANEL_LEFT:
			ECX343EN_StopPanelPowerSequence(PANEL_LEFT);
			osDelay(10);
			break;
		case PANEL_RIGHT:
			ECX343EN_StopPanelPowerSequence(PANEL_RIGHT);
			osDelay(10);
			break;
		case PANEL_BOTH:
			ECX343EN_StopPanelPowerSequence(PANEL_LEFT);
			osDelay(10);
			ECX343EN_StopPanelPowerSequence(PANEL_RIGHT);
			osDelay(10);
			break;
		default:
			break;
	}
	ECX343EN_PowerOff();
}

void ECX343EN_WriteRegisters(ECX343_DATA *data) {
	 ECX343EN_ActivatePanel(PANEL_LEFT);
     while (panel_ps_transition() == PANEL_CONT_CONTINUE) {
     }
     panel_reg_write(PANEL_MAP_0, SCAN_MODE, leftInversionMasks[data->uLCD_INVL]);
     while (panel_ps_release() == PANEL_CONT_CONTINUE) {
     }
     panel_reg_write(PANEL_MAP_0, ORBIT_HORIZONTAL, LcdHorbit(data->uLCD_HORBL));
     panel_reg_write(PANEL_MAP_0, ORBIT_VERTICAL, LcdVorbit(data->uLCD_VORBL));
     panel_reg_write(PANEL_MAP_0, ARBITRARY_LUMINANCE_VALUE_LOW, data->uLCD_LUXL & 0x000000FF);
     panel_reg_write(PANEL_MAP_0, ARBITRARY_LUMINANCE_VALUE_HIGH, (data->uLCD_LUXL & 0x00000100) >> 8);

     ECX343EN_ActivatePanel(PANEL_RIGHT);
     while (panel_ps_transition() == PANEL_CONT_CONTINUE) {
     }
     panel_reg_write(PANEL_MAP_0, SCAN_MODE, rightInversionMasks[data->uLCD_INVR]);
     while (panel_ps_release() == PANEL_CONT_CONTINUE) {
     }
     panel_reg_write(PANEL_MAP_0, ORBIT_HORIZONTAL, LcdHorbit(data->uLCD_HORBR));
     panel_reg_write(PANEL_MAP_0, ORBIT_VERTICAL, LcdVorbit(data->uLCD_VORBR));
     panel_reg_write(PANEL_MAP_0, ARBITRARY_LUMINANCE_VALUE_LOW, data->uLCD_LUXR & 0x000000FF);
     panel_reg_write(PANEL_MAP_0, ARBITRARY_LUMINANCE_VALUE_HIGH, (data->uLCD_LUXR & 0x00000100) >> 8);
}

void ECX343EN_Inversion(PanelSide side, uint8_t value) {
    /*
		default: 1000 1100
		[2]
		Selection of rightward / leftward scan
		0: Leftward scan
		1: Rightward scan (Default)
		[3]
		Selection of upward / downward scan
		0: Upward scan
		1: Downward scan (Default)
    */
    ECX343EN_ActivatePanel(side);

    while (panel_ps_transition() == PANEL_CONT_CONTINUE) {
    }

    panel_reg_write(PANEL_MAP_0, SCAN_MODE, side == PANEL_LEFT ? leftInversionMasks[value] : rightInversionMasks[value]);

    while (panel_ps_release() == PANEL_CONT_CONTINUE) {
    }

}

void ECX343EN_LuminanceModeSetting(PanelSide side, uint8_t value) {
    /*
		default: 0000 1111
		[1]
		Luminance setting
		0: Preset Mode
		1: REQUEST_LV (Default)
		[3]
		Auto adjustment of white balance in case this function is valid.
		0: Preset Mode
		1: REQUEST_LV (Default)
    */
	ECX343EN_ActivatePanel(side);
    panel_reg_write(PANEL_MAP_0, LUMINANCE_MODE_SETTING, value);
}

void adjustBrightness(void)
{
    if (previousLeftBrightness == ecx343_current_data.uLCD_LUXL &&
        previousRightBrightness == ecx343_current_data.uLCD_LUXR)
    {
        return;
    }

	if (previousLeftBrightness != ecx343_current_data.uLCD_LUXL) {
		ECX343EN_Luminance(PANEL_LEFT, ecx343_current_data.uLCD_LUXL);
		previousLeftBrightness = ecx343_current_data.uLCD_LUXL;
	}

	if (previousRightBrightness != ecx343_current_data.uLCD_LUXR) {
		ECX343EN_Luminance(PANEL_RIGHT, ecx343_current_data.uLCD_LUXR);
		previousRightBrightness = ecx343_current_data.uLCD_LUXR;
	}
}

static void ECX343EN_Luminance(PanelSide side, uint16_t Brightness) {
    Brightness &= 0x01FF;
    uint8_t highByte = Brightness >> 8;
    uint8_t lowByte = Brightness & 0xFF;

    ECX343EN_ArbitraryLuminanceH(side, highByte);
    ECX343EN_ArbitraryLuminanceL(side, lowByte);
}

static void ECX343EN_ArbitraryLuminanceH(PanelSide side, uint8_t value) {
    /*
		0x12 [1:0] default: 0000 0001
		0x13 [7:0] default: 0101 1110
		Luminance setting register
		0001100100: 1000cd/m^2
		|
		0111110100: 5000cd/m^2
		*Be sure to enter a value between 1000 and 5000cd/ m^2.
		(Default 0101011110: 3500cd/m^2)
    */
	ECX343EN_ActivatePanel(side);
    panel_reg_write(PANEL_MAP_0, ARBITRARY_LUMINANCE_VALUE_HIGH, value);
}

static void ECX343EN_ArbitraryLuminanceL(PanelSide side, uint8_t value) {
    /*
		0x12 [1:0] default: 0000 0001
		0x13 [7:0] default: 0101 1110
		Luminance setting register
		0001100100: 1000cd/m^2
		|
		0111110100: 5000cd/m^2
		*Be sure to enter a value between 1000 and 5000cd/ m^2.
		(Default 0101011110: 3500cd/m^2)
    */
	ECX343EN_ActivatePanel(side);
    panel_reg_write(PANEL_MAP_0, ARBITRARY_LUMINANCE_VALUE_LOW, value);
}

void ECX343EN_PresetLuminanceValue(PanelSide side, uint8_t value) {
    /*
		default: 0010 0000
		[0]
		Luminance preset mode selection
		0: 500cd/m2 (Default)
		1: 1000cd/m2
    */
	ECX343EN_ActivatePanel(side);
    panel_reg_write(PANEL_MAP_0, PRESET_LUMINANCE_VALUE, value);
}

void ECX343EN_OrbitH(PanelSide side, uint8_t H_value) {
    /*
		0x05 [5:0] default: 0000 0000
		Horizontal orbit adjustment
		110000: -16 pixels
		|
		111111: -1  pixels
		000000: center(Default)
		000001: +1  pixels
		|
		010000: +16 pixels
    */
	ECX343EN_ActivatePanel(side);
    panel_reg_write(PANEL_MAP_0, ORBIT_HORIZONTAL, H_value);
}

void ECX343EN_OrbitV(PanelSide side, uint8_t V_value) {
    /*
		0x06 [5:0] default: 0000 0000
		Vertical orbit adjustment
		110000: -16 pixels
		|
		111111: -1  pixels
		000000: center(Default)
		000001: +1  pixels
		|
		010000: +16 pixels
    */
	ECX343EN_ActivatePanel(side);
    panel_reg_write(PANEL_MAP_0, ORBIT_VERTICAL, V_value);
}

void ECX343EN_TempDetect(PanelSide side, uint8_t value) {
	/*
	    Before setting V1 or V2 output (bits [5:4] in register 0x09),
	    set register 0x8E to 0x0C.

	    0x09 [5:4]:
	    10: V1 output
	    11: V2 output
	*/
	ECX343EN_ActivatePanel(side);
    panel_reg_write(PANEL_MAP_0, PANEL_TEMP_DETECTION, value);
}

static bool is_within_range(int value, int min, int max) {
    return (value >= min) && (value <= max);
}

static bool is_within_ranges(int value, int min1, int max1, int min2, int max2) {
    return is_within_range(value, min1, max1) || is_within_range(value, min2, max2);
}

static bool is_ecx343RW_buf_valid(ECX343_DATA *buf) {
    bool invl_valid = is_within_range(buf->uLCD_INVL, 0, 3);
    bool invr_valid = is_within_range(buf->uLCD_INVR, 0, 3);
    bool luxl_valid = is_within_range(buf->uLCD_LUXL, 100, 500);
    bool luxr_valid = is_within_range(buf->uLCD_LUXR, 100, 500);
    bool horbl_valid = is_within_ranges(buf->uLCD_HORBL, 0, 16, 100, 116);
    bool horbr_valid = is_within_ranges(buf->uLCD_HORBR, 0, 16, 100, 116);
    bool vorbl_valid = is_within_ranges(buf->uLCD_VORBL, 0, 16, 100, 116);
    bool vorbr_valid = is_within_ranges(buf->uLCD_VORBR, 0, 16, 100, 116);
    bool mode_valid = is_within_range(buf->uLCD_MODE, 0, 3);

    return invl_valid && invr_valid && luxl_valid && luxr_valid && horbl_valid && horbr_valid && vorbl_valid && vorbr_valid && mode_valid;
}

static void ECX343EN_PowerOn(void) {
    ECX343EN_1V8_ENABLE();
    osDelay(10);
    ECX343EN_3V3_ENABLE();
    osDelay(10);
    ECX343EN_6V6_N_ENABLE();
    osDelay(10);
}

static void ECX343EN_PowerOff(void) {
    ECX343EN_6V6_N_DISABLE();
    osDelay(10);
    ECX343EN_3V3_DISABLE();
    osDelay(10);
    ECX343EN_1V8_DISABLE();
    osDelay(10);
}

static void ECX343EN_StartPanelPowerSequence (PanelSide side) {
	 ECX343EN_ActivatePanel(side);
    while(pow_on_sequence() == PANEL_CONT_CONTINUE) {
    }
    pow_on_seq = 1;
}

static void ECX343EN_StopPanelPowerSequence(PanelSide side) {
	 ECX343EN_ActivatePanel(side);
    while (pow_off_sequence() == PANEL_CONT_CONTINUE) {
    }
    pow_off_seq = 1;
}

static void ECX343EN_PanelPSRelease(void) {
     ECX343EN_ActivatePanel(PANEL_LEFT);
     while (panel_ps_release() == PANEL_CONT_CONTINUE) {
     }

     ECX343EN_ActivatePanel(PANEL_RIGHT);
     while (panel_ps_release() == PANEL_CONT_CONTINUE) {
     }
}

static void ECX343EN_ActivatePanel(PanelSide side) {
    currentPanelSide = side;
    // Set the current SPI handle based on the panel side
    current_spi_handle = (side == PANEL_LEFT) ? &hspi2 : &hspi4;
}

static void spi_enable(bool enable) {
    // Directly control the SPI based on the current panel side
    if (currentPanelSide == PANEL_LEFT) {
        if (enable) {
            ECX343EN_L_SPI_ENABLE();
        } else {
            ECX343EN_L_SPI_DISABLE();
        }
    } else if (currentPanelSide == PANEL_RIGHT) {
        if (enable) {
            ECX343EN_R_SPI_ENABLE();
        } else {
            ECX343EN_R_SPI_DISABLE();
        }
    }
}

static PowContStatus pow_on_sequence(void)
{
    PowContStatus pnl_status;

    //power on sequence
    switch(pow_on_seq)
    {
        case POW_ON_SEQ_NONE:
            return POW_CONT_OK;

//        case POW_ON_SEQ_PNL_1V8:
//            ECX343EN_1V8_ENABLE();
//            pow_on_seq = POW_ON_SEQ_PNL_3V3;
//            break;
//
//        case POW_ON_SEQ_PNL_3V3:
//            ECX343EN_3V3_ENABLE();
//            pow_on_seq = POW_ON_SEQ_PNL_6V6_N;
//            break;
//
//        case POW_ON_SEQ_PNL_6V6_N:
//            ECX343EN_6V6_N_ENABLE();
//            pow_on_seq = POW_ON_SEQ_P_XCLR;
//            break;

        case POW_ON_SEQ_P_XCLR:
            if (currentPanelSide == PANEL_LEFT) {
                ECX343EN_L_XCLR_ENABLE();
            } else {
                ECX343EN_R_XCLR_ENABLE();
            }
            pow_on_seq = POW_ON_SEQ_PANEL_REG_SETTING;
            break;

        case POW_ON_SEQ_PANEL_REG_SETTING:
            pnl_status = panel_reg_setting();
            if (pnl_status == PANEL_CONT_OK) {
				pow_on_seq = POW_ON_SEQ_PANEL_LUMINANCE_SETTING;
            } else if (pnl_status == PANEL_CONT_ERR) {
                pow_on_seq = POW_ON_SEQ_NONE;
                return POW_CONT_ERR;
            }
            break;

        case POW_ON_SEQ_PANEL_LUMINANCE_SETTING:
            pnl_status = panel_luminance_setting();
            if (pnl_status == PANEL_CONT_OK) {
            	writeRegisters(&ecx343_current_data);
                pow_on_seq = POW_ON_SEQ_LVDS_EN;
            } else if (pnl_status == PANEL_CONT_ERR) {
                pow_on_seq = POW_ON_SEQ_NONE;
                return POW_CONT_ERR;
            }
            break;

        case POW_ON_SEQ_LVDS_EN:
//            ECX343EN_LVDS_ENABLE();
//            pow_on_seq = POW_ON_SEQ_PS_OFF_SETTING;
        	  pow_on_seq = POW_ON_SEQ_NONE;
        	  return POW_CONT_OK;
            break;

//        case POW_ON_SEQ_PS_OFF_SETTING:
//            pnl_status = panel_ps_release();
//            if (pnl_status == PANEL_CONT_OK) {
//                pow_on_seq = POW_ON_SEQ_NONE;
//                return POW_CONT_OK;
//            } else if (pnl_status == PANEL_CONT_ERR) {
//                pow_on_seq = POW_ON_SEQ_NONE;
//                return POW_CONT_ERR;
//            }
//            break;

        default:
            pow_on_seq = POW_ON_SEQ_NONE;
    }

    return POW_CONT_CONTINUE;
}

static PowContStatus pow_off_sequence(void)
{
    PowContStatus pnl_status;

        //power on sequence
        switch(pow_off_seq)
        {
            case POW_OFF_SEQ_NONE:
                break;

            case POW_OFF_SEQ_PS_ON_SETTING:
                pnl_status = panel_ps_transition();
                if(pnl_status == PANEL_CONT_OK) {
                    osDelay(34);
					pow_off_seq = POW_OFF_SEQ_LVDS_DIS;
                } else if(pnl_status == PANEL_CONT_ERR) {
                    pow_off_seq = POW_OFF_SEQ_NONE;
                    return POW_CONT_ERR;
                }
                break;

            case POW_OFF_SEQ_LVDS_DIS:
//                ECX343EN_LVDS_DISABLE();
                pow_off_seq = POW_OFF_SEQ_P_XCLR;
                break;

            case POW_OFF_SEQ_P_XCLR:
                if (currentPanelSide == PANEL_LEFT) {
                    ECX343EN_L_XCLR_DISABLE();
                } else {
                    ECX343EN_R_XCLR_DISABLE();
                }
                pow_off_seq = POW_OFF_SEQ_NONE;
                return POW_CONT_OK;
//
//            case POW_OFF_SEQ_PNL_6V6_N:
//                ECX343EN_6V6_N_DISABLE();
//                pow_off_seq = POW_OFF_SEQ_PNL_3V3;
//                break;
//
//            case POW_OFF_SEQ_PNL_3V3:
//                ECX343EN_3V3_DISABLE();
//                pow_off_seq = POW_OFF_SEQ_PNL_1V8;
//                break;
//
//            case POW_OFF_SEQ_PNL_1V8:
//                ECX343EN_1V8_DISABLE();
//                pow_off_seq = POW_OFF_SEQ_NONE;
//                return POW_CONT_OK;

            default:
                pow_off_seq = POW_OFF_SEQ_NONE;
                break;
        }

        return POW_CONT_CONTINUE;
}

static PowContStatus panel_reg_setting(void) {
    const setPanelSeqTable *table = (currentPanelMode == MODE_2D_120HZ || currentPanelMode == MODE_3D_120HZ) ?
		PanelContPanelReg120HzSettingTable : PanelContPanelReg60HzSettingTable;

    if (table[panel_cont_seq].map != PANEL_TABLE_END) {
        if (panel_reg_write(table[panel_cont_seq].map,
        					table[panel_cont_seq].addr,
							table[panel_cont_seq].value
							) == PANEL_REG_ERR) {
            panel_cont_seq = PANEL_CONT_SEQ_NONE;
            return PANEL_REG_ERR;
        }
        panel_cont_seq++;
    } else {
        panel_cont_seq = PANEL_CONT_SEQ_NONE;
        return PANEL_REG_OK;
    }

    return PANEL_CONT_CONTINUE;
}

static PowContStatus panel_luminance_setting(void) {
    const setPanelSeqTable *table = (currentPanelSide == PANEL_LEFT) ?
		((currentPanelMode == MODE_2D_120HZ || currentPanelMode == MODE_3D_120HZ) ? LeftPanelContLuminance120HzSettingTable : LeftPanelContLuminance60HzSettingTable) :
		((currentPanelMode == MODE_2D_120HZ || currentPanelMode == MODE_3D_120HZ) ? RightPanelContLuminance120HzSettingTable : RightPanelContLuminance60HzSettingTable);

    if (table[panel_cont_seq].map != PANEL_TABLE_END) {
        if (panel_reg_write(table[panel_cont_seq].map,
							table[panel_cont_seq].addr,
							table[panel_cont_seq].value
							) == PANEL_REG_ERR) {
            panel_cont_seq = PANEL_CONT_SEQ_NONE;
            return PANEL_REG_ERR;
        }
        panel_cont_seq++;
    } else {
        panel_cont_seq = PANEL_CONT_SEQ_NONE;
        return PANEL_REG_OK;
    }

    return PANEL_CONT_CONTINUE;
}

static PowContStatus panel_ps_release(void) {
    if (PanelContPSReleaseTable[panel_cont_seq].map != PANEL_TABLE_END) {
        if (panel_reg_write(PanelContPSReleaseTable[panel_cont_seq].map,
							PanelContPSReleaseTable[panel_cont_seq].addr,
							PanelContPSReleaseTable[panel_cont_seq].value
							) == PANEL_REG_ERR) {
            panel_cont_seq = PANEL_CONT_SEQ_NONE;
            return PANEL_REG_ERR;
        }
        panel_cont_seq++;
    } else {
        panel_cont_seq = PANEL_CONT_SEQ_NONE;
        return PANEL_REG_OK;
    }

    return PANEL_CONT_CONTINUE;
}

static PowContStatus panel_ps_transition(void) {
    if (PanelContPSTransitionTable[panel_cont_seq].map != PANEL_TABLE_END) {
        if (panel_reg_write(PanelContPSTransitionTable[panel_cont_seq].map,
							PanelContPSTransitionTable[panel_cont_seq].addr,
							PanelContPSTransitionTable[panel_cont_seq].value
							) == PANEL_REG_ERR) {
            panel_cont_seq = PANEL_CONT_SEQ_NONE;
            return PANEL_REG_ERR;
        }
        panel_cont_seq++;
    } else {
        panel_cont_seq = PANEL_CONT_SEQ_NONE;
        return PANEL_REG_OK;
    }

    return PANEL_CONT_CONTINUE;
}

static PanelRegStatus panel_reg_read(uint8_t map, uint8_t addr, uint8_t *value)
{
    // Map check
    if (currentPanelMap != map) {
        // Change register map
        if (panel_reg_write_value(PANEL_ADDR_82, map) == PANEL_REG_ERR) {
            return PANEL_REG_ERR;
        }
        currentPanelMap = map;
    }

    // Read panel register
    if (panel_reg_read_value(addr, value) == PANEL_REG_ERR) {
        return PANEL_REG_ERR;
    }

    return PANEL_REG_OK;
}

static PanelRegStatus panel_reg_write(uint8_t map, uint8_t addr, uint8_t value)
{
    // Map check
    if (currentPanelMap != map) {
        // Change register map
        if (panel_reg_write_value(PANEL_ADDR_82, map) == PANEL_REG_ERR) {
            return PANEL_REG_ERR;
        }
        currentPanelMap = map;
    }

    // Write panel register
    if (panel_reg_write_value(addr, value) == PANEL_REG_ERR) {
        return PANEL_REG_ERR;
    }

    return PANEL_REG_OK;
}

static PanelRegStatus panel_reg_read_value(uint8_t addr, uint8_t *value) {
    uint8_t PanelWriteBuf[10];
    uint8_t PanelReadBuf[10];

    PanelWriteBuf[0] = PANEL_ADDR_81;
    PanelWriteBuf[1] = addr;

    spi_enable(true);
    if (HAL_SPI_Transmit(current_spi_handle, &PanelWriteBuf[0], PANEL_WRITE_LENGTH, PANEL_REG_RW_TIMEOUT) != HAL_OK) {
        spi_enable(false);
        return PANEL_REG_ERR;
    }
    spi_enable(false);


    PanelWriteBuf[0] = PANEL_ADDR_81;
    PanelWriteBuf[1] = 0xFF;

    spi_enable(true);
    if (HAL_SPI_TransmitReceive(current_spi_handle, &PanelWriteBuf[0], &PanelReadBuf[0], PANEL_WRITE_LENGTH, PANEL_REG_RW_TIMEOUT) != HAL_OK) {
        spi_enable(false);
        return PANEL_REG_ERR;
    }
    spi_enable(false);

    *value = PanelReadBuf[1];

    return PANEL_REG_OK;
}

static PanelRegStatus panel_reg_write_value(uint8_t addr, uint8_t value) {
    uint8_t PanelWriteBuf[10];

    PanelWriteBuf[0] = addr;
    PanelWriteBuf[1] = value;

    spi_enable(true);
    if (HAL_SPI_Transmit(current_spi_handle, &PanelWriteBuf[0], PANEL_WRITE_LENGTH, PANEL_REG_RW_TIMEOUT) != HAL_OK) {
        spi_enable(false);
        return PANEL_REG_ERR;
    }
    spi_enable(false);

    return PANEL_REG_OK;
}

static void writeRegisters(ECX343_DATA *data) {
	if(currentPanelSide == PANEL_LEFT) {
		panel_reg_write(PANEL_MAP_0, SCAN_MODE, leftInversionMasks[data->uLCD_INVL]);
		panel_reg_write(PANEL_MAP_0, ORBIT_HORIZONTAL, LcdHorbit(data->uLCD_HORBL));
		panel_reg_write(PANEL_MAP_0, ORBIT_VERTICAL, LcdVorbit(data->uLCD_VORBL));
		panel_reg_write(PANEL_MAP_0, ARBITRARY_LUMINANCE_VALUE_LOW, data->uLCD_LUXL & 0x000000FF);
		panel_reg_write(PANEL_MAP_0, ARBITRARY_LUMINANCE_VALUE_HIGH, (data->uLCD_LUXL & 0x00000100) >> 8);
	} else{
		panel_reg_write(PANEL_MAP_0, SCAN_MODE, rightInversionMasks[data->uLCD_INVR]);
		panel_reg_write(PANEL_MAP_0, ORBIT_HORIZONTAL, LcdHorbit(data->uLCD_HORBR));
		panel_reg_write(PANEL_MAP_0, ORBIT_VERTICAL, LcdVorbit(data->uLCD_VORBR));
		panel_reg_write(PANEL_MAP_0, ARBITRARY_LUMINANCE_VALUE_LOW, data->uLCD_LUXR & 0x000000FF);
		panel_reg_write(PANEL_MAP_0, ARBITRARY_LUMINANCE_VALUE_HIGH, (data->uLCD_LUXR & 0x00000100) >> 8);
    }
}

int mapLuxToPanelBrightness(int lux, int *currentIndex) {
    // Define lux thresholds to separate different brightness intervals.
    const int thresholds[] = {100, 280, 460, 640, 820, 1000};
    // Define the overlap value for smoother transitions between intervals.
    const int overlap = 50;
    // Define brightness ranges corresponding to each lux interval.
    const int brightnessRanges[][2] = {{100, 200}, {160, 280}, {240, 360}, {320, 440}, {400, 500}};
    // Calculate the number of thresholds.
    const int numThresholds = sizeof(thresholds) / sizeof(thresholds[0]);

    // Check if lux value is below the first threshold minus the overlap.
    if (lux < thresholds[0] - overlap) {
        *currentIndex = -2; // Set currentIndex to -2 indicating below the first interval.
        return brightnessRanges[0][0]; // Return the minimum brightness.
    }
    // Check if lux value is above the last threshold plus the overlap.
    else if (lux >= thresholds[numThresholds - 1] + overlap) {
        *currentIndex = 2; // Set currentIndex to 2 indicating above the last interval.
        return brightnessRanges[numThresholds - 2][1]; // Return the maximum brightness.
    }
    // Iterate through the thresholds to find the correct interval for the lux value.
    else {
        for (int i = 0; i < numThresholds - 1; ++i) {
            // Check if lux is within the current interval including overlap.
            if (lux >= thresholds[i] - overlap && lux < thresholds[i + 1] + overlap) {
                *currentIndex = i - 2; // Update currentIndex to the corresponding interval index.
                // Calculate the fraction of the position within the current interval.
                float fraction = (float)(lux - (thresholds[i] - overlap)) /
                                 (thresholds[i + 1] + overlap - (thresholds[i] - overlap));
                // Linearly interpolate the brightness value for the current interval.
                return brightnessRanges[i][0] +
                       fraction * (brightnessRanges[i][1] - brightnessRanges[i][0]);
            }
        }
    }

    // Default case: Set currentIndex to -2 and return the minimum brightness.
    *currentIndex = -2;
    return brightnessRanges[0][0];
}

void smoothlyChangeBrightness(uint16_t targetBrightness) {
    const int adjustmentRate = 10;  // Define the rate at which brightness changes.

    // Check if the current brightness is different from the target brightness.
    if (ecx343_current_data.uLCD_LUXL != targetBrightness ||
        ecx343_current_data.uLCD_LUXR != targetBrightness) {

        // Calculate the difference between the target brightness and the current brightness for both left and right screens.
        int deltaL = targetBrightness - ecx343_current_data.uLCD_LUXL;
        int deltaR = targetBrightness - ecx343_current_data.uLCD_LUXR;

        // Adjust left screen brightness.
        if (abs(deltaL) <= adjustmentRate) {
            // If the difference is less than or equal to the adjustment rate, set the brightness directly to the target.
            ecx343_current_data.uLCD_LUXL = targetBrightness;
        } else {
            // Otherwise, increase or decrease the brightness by the adjustment rate.
            ecx343_current_data.uLCD_LUXL += (deltaL > 0) ? adjustmentRate : -adjustmentRate;
        }

        // Adjust right screen brightness.
        if (abs(deltaR) <= adjustmentRate) {
            // If the difference is less than or equal to the adjustment rate, set the brightness directly to the target.
            ecx343_current_data.uLCD_LUXR = targetBrightness;
        } else {
            // Otherwise, increase or decrease the brightness by the adjustment rate.
            ecx343_current_data.uLCD_LUXR += (deltaR > 0) ? adjustmentRate : -adjustmentRate;
        }

        // Ensure the left screen brightness is within the allowed range.
        ecx343_current_data.uLCD_LUXL = (ecx343_current_data.uLCD_LUXL < MIN_BRIGHTNESS) ? MIN_BRIGHTNESS :
        (ecx343_current_data.uLCD_LUXL > MAX_BRIGHTNESS) ? MAX_BRIGHTNESS : ecx343_current_data.uLCD_LUXL;

        // Ensure the right screen brightness is within the allowed range.
        ecx343_current_data.uLCD_LUXR = (ecx343_current_data.uLCD_LUXR < MIN_BRIGHTNESS) ? MIN_BRIGHTNESS :
        (ecx343_current_data.uLCD_LUXR > MAX_BRIGHTNESS) ? MAX_BRIGHTNESS : ecx343_current_data.uLCD_LUXR;

        // Adjust the physical brightness of the panel.
        executeTaskWithMutex(ADJUST_BRIGHTNESS);
    }
}

// Initialize the circular buffer
static inline void initBuffer(CircularBuffer *cb) {
    cb->head = 0;
    cb->size = 0;
    cb->temperatureCount = 0;
    cb->smoothedTemperature = 0.0f;
    cb->invalidTemperatureCount = 0;
}

// Add a value to the circular buffer
static inline void addToBuffer(CircularBuffer *cb, float value) {
    cb->buffer[cb->head] = value;
    cb->head = (cb->head + 1) % MAX_BUFFER_SIZE;
    if (cb->size < MAX_BUFFER_SIZE) {
        cb->size++;
    }
}

// Get an element from the circular buffer by index
static inline float getBufferElement(CircularBuffer *cb, int index) {
    if (index < 0 || index >= cb->size) {
        return 0;
    }
    int realIndex = (cb->head - cb->size + index + MAX_BUFFER_SIZE) % MAX_BUFFER_SIZE;
    return cb->buffer[realIndex];
}

// Calculate the exponential smoothing of the buffer
static float exponential_smoothing_sequence(CircularBuffer *cb) {
    if (cb->size == 0) {
        return 0;
    }

    float smoothed_result = getBufferElement(cb, 0);
    for (int i = 1; i < cb->size; i++) {
        smoothed_result = ALPHA * getBufferElement(cb, i) + (1 - ALPHA) * smoothed_result;
    }
    return smoothed_result;
}

// Read the voltage from the ADC and calculate the temperature
static float readVoltageAndCalculate(PanelSide side, uint8_t voltage, ADC_HandleTypeDef *hadc) {
    uint16_t adcVal = 0x0000;
    float volt = 0.0;

    executeTaskWithMutex(UPDATE_TEMPERATURE, side, voltage);
    osDelay(100);
    HAL_ADC_Start(hadc);

    if (HAL_ADC_PollForConversion(hadc, 0xFF) == HAL_OK) {
        if (HAL_IS_BIT_SET(HAL_ADC_GetState(hadc), HAL_ADC_STATE_REG_EOC)) {
            adcVal = HAL_ADC_GetValue(hadc);
            volt = (float)adcVal / ADC_RESOLUTION * VREF * 2;
        }
    } else {
        volt = -1.0f;
    }

    HAL_ADC_Stop(hadc);
    return volt;
}

// Calculate the temperature based on two voltage readings
static inline float calculateTemperature(float voltage1, float voltage2) {
    return ((voltage2 - voltage1) - 0.6796) * 1.0 / 0.0025;
}

// Check if the temperature is within the valid range
static inline bool isTemperatureInRange(float temperature) {
    return temperature >= MIN_TEMPERATURE && temperature <= MAX_TEMPERATURE;
}

// Check if the temperature value is valid
static inline bool isTemperatureValueValid(float temperature, CircularBuffer *cb) {
    return isTemperatureInRange(temperature) &&
           (cb->temperatureCount < SOME_THRESHOLD || fabs(temperature - cb->smoothedTemperature) <= TEMPERATURE_THRESHOLD);
}

// Update the temperature of the panel
void updatePanelTemperature(void) {
    if (!isBufferInitialized) {
        initBuffer(&leftTemperatureBuffer);
        initBuffer(&rightTemperatureBuffer);
        isBufferInitialized = true;
    }

    float leftOrigVolt[2], rightOrigVolt[2];
    rightOrigVolt[0] = readVoltageAndCalculate(PANEL_RIGHT, PANEL_TEMP_VOLT1, &hadc2);
    rightOrigVolt[1] = readVoltageAndCalculate(PANEL_RIGHT, PANEL_TEMP_VOLT2, &hadc2);
    leftOrigVolt[0] = readVoltageAndCalculate(PANEL_LEFT, PANEL_TEMP_VOLT1, &hadc1);
    leftOrigVolt[1] = readVoltageAndCalculate(PANEL_LEFT, PANEL_TEMP_VOLT2, &hadc1);

    bool isError = (leftOrigVolt[0] == -1.0f || rightOrigVolt[0] == -1.0f ||
                    leftOrigVolt[1] == -1.0f || rightOrigVolt[1] == -1.0f);

    if (!isError) {
        float tempLeft = calculateTemperature(leftOrigVolt[0], leftOrigVolt[1]);
        float tempRight = calculateTemperature(rightOrigVolt[0], rightOrigVolt[1]);

        if (!isTemperatureInRange(tempLeft)) {
            tempLeft = -273.15f; // Set to absolute zero if out of range
        }

        if (!isTemperatureInRange(tempRight)) {
            tempRight = -273.15f; // Set to absolute zero if out of range
        }

        g_temperatureLeftRaw = tempLeft;
        g_temperatureRightRaw = tempRight;
    } else {
        g_temperatureLeftRaw = -273.15f; // Set to absolute zero if error
        g_temperatureRightRaw = -273.15f; // Set to absolute zero if error
    }

    // Update the temperature buffers and smoothed temperatures
    if (isTemperatureValueValid(g_temperatureLeftRaw, &leftTemperatureBuffer)) {
        addToBuffer(&leftTemperatureBuffer, g_temperatureLeftRaw);
        leftTemperatureBuffer.smoothedTemperature = exponential_smoothing_sequence(&leftTemperatureBuffer);
        g_temperatureLeftSmoothed = leftTemperatureBuffer.smoothedTemperature;
    }

    if (isTemperatureValueValid(g_temperatureRightRaw, &rightTemperatureBuffer)) {
        addToBuffer(&rightTemperatureBuffer, g_temperatureRightRaw);
        rightTemperatureBuffer.smoothedTemperature = exponential_smoothing_sequence(&rightTemperatureBuffer);
        g_temperatureRightSmoothed = rightTemperatureBuffer.smoothedTemperature;
    }
}

void CheckPanelState(void)
{
	uint8_t data;

    ECX343EN_ActivatePanel(PANEL_LEFT);
    panel_reg_write(0, 0x80, 1);
    osDelay(10);
    usbDebug("---------PANEL_LEFT---------\r\n");
	for (uint16_t i = 0; i <= 0xFF; i++)
	{
	    panel_reg_read(0, i, &data);
	    usbDebug("Address[0x%02X]: 0x%02X\r\n", i, data);
	    osDelay(10);
	}
    ECX343EN_ActivatePanel(PANEL_RIGHT);
    panel_reg_write(0, 0x80, 1);
    osDelay(10);
	usbDebug("---------PANEL_RIGHT---------\r\n");
	for (uint16_t i = 0; i <= 0xFF; i++)
	{
	    panel_reg_read(0, i, &data);
	    usbDebug("Address[0x%02X]: 0x%02X\r\n", i, data);
	    osDelay(10);
	}
}

void switchMode(void)
{
	ecx343_current_data.uLCD_MODE = currentPanelMode;
	executeTaskWithMutex(POWER_OFF, PANEL_BOTH);

	LT7911_Mode_Switch(ecx343_current_data.uLCD_MODE);
	osDelay(1000);

	executeTaskWithMutex(POWER_ON, PANEL_BOTH);
}

const setPanelSeqTable PanelContPanelReg60HzSettingTable[] = {
//    {PANEL_MAP_0, PANEL_ADDR_01, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_02, 0x8C, 0},
//    {PANEL_MAP_0, PANEL_ADDR_03, 0x58, 0},
//    {PANEL_MAP_0, PANEL_ADDR_04, 0x03, 0},
//    {PANEL_MAP_0, PANEL_ADDR_05, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_06, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_07, 0x20, 0},
//    {PANEL_MAP_0, PANEL_ADDR_08, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_09, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_0A, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_0B, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_0C, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_0D, 0x10, 0},
//    {PANEL_MAP_0, PANEL_ADDR_0E, 0x44, 0},
//    {PANEL_MAP_0, PANEL_ADDR_0F, 0x00, 0},

//    {PANEL_MAP_0, PANEL_ADDR_10, 0x0F, 0},
//    {PANEL_MAP_0, PANEL_ADDR_11, 0x04, 0},
//    {PANEL_MAP_0, PANEL_ADDR_12, 0x01, 0},
//    {PANEL_MAP_0, PANEL_ADDR_13, 0x5E, 0},
//    {PANEL_MAP_0, PANEL_ADDR_14, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_15, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_16, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_17, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_18, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_19, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_1A, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_1B, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_1C, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_1D, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_1E, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_1F, 0x, 0},
//
//    {PANEL_MAP_0, PANEL_ADDR_20, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_21, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_22, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_23, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_24, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_25, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_26, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_27, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_28, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_29, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_2A, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_2B, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_2C, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_2D, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_2E, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_2F, 0x, 0},
//
//    {PANEL_MAP_0, PANEL_ADDR_30, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_31, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_32, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_33, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_34, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_35, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_36, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_37, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_38, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_39, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_3A, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_3B, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_3C, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_3D, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_3E, 0x21, 0},
//    {PANEL_MAP_0, PANEL_ADDR_3F, 0x, 0},
//
//    {PANEL_MAP_0, PANEL_ADDR_40, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_41, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_42, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_43, 0x40, 0},
//    {PANEL_MAP_0, PANEL_ADDR_44, 0x29, 0},
//    {PANEL_MAP_0, PANEL_ADDR_45, 0xD9, 0},
//    {PANEL_MAP_0, PANEL_ADDR_46, 0x02, 0},
    {PANEL_MAP_0, PANEL_ADDR_47, 0x33, 0},
//    {PANEL_MAP_0, PANEL_ADDR_48, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_49, 0x18, 0},
//    {PANEL_MAP_0, PANEL_ADDR_4A, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_4B, 0x22, 0},
    {PANEL_MAP_0, PANEL_ADDR_4C, 0x34, 0},
    {PANEL_MAP_0, PANEL_ADDR_4D, 0x33, 0},
//    {PANEL_MAP_0, PANEL_ADDR_4E, 0x04, 0},
    {PANEL_MAP_0, PANEL_ADDR_4F, 0x0D, 0},

//    {PANEL_MAP_0, PANEL_ADDR_50, 0x18, 0},
//    {PANEL_MAP_0, PANEL_ADDR_51, 0x22, 0},
    {PANEL_MAP_0, PANEL_ADDR_52, 0x35, 0},
    {PANEL_MAP_0, PANEL_ADDR_53, 0x34, 0},
//    {PANEL_MAP_0, PANEL_ADDR_54, 0x04, 0},
    {PANEL_MAP_0, PANEL_ADDR_55, 0x0D, 0},
//    {PANEL_MAP_0, PANEL_ADDR_56, 0x18, 0},
//    {PANEL_MAP_0, PANEL_ADDR_57, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_58, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_59, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_5A, 0x10, 0},
//    {PANEL_MAP_0, PANEL_ADDR_5B, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_5C, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_5D, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_5E, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_5F, 0x, 0},
//
//    {PANEL_MAP_0, PANEL_ADDR_60, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_61, 0x11, 0},
//    {PANEL_MAP_0, PANEL_ADDR_62, 0x03, 0},
//    {PANEL_MAP_0, PANEL_ADDR_63, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_64, 0xED, 0},
//    {PANEL_MAP_0, PANEL_ADDR_65, 0x06, 0},
//    {PANEL_MAP_0, PANEL_ADDR_66, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_67, 0x17, 0},
//    {PANEL_MAP_0, PANEL_ADDR_68, 0xF9, 0},
//    {PANEL_MAP_0, PANEL_ADDR_69, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_6A, 0x09, 0},
//    {PANEL_MAP_0, PANEL_ADDR_6B, 0xE7, 0},
//    {PANEL_MAP_0, PANEL_ADDR_6C, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_6D, 0xEE, 0},
//    {PANEL_MAP_0, PANEL_ADDR_6E, 0xF5, 0},
//    {PANEL_MAP_0, PANEL_ADDR_6F, 0x10, 0},

//    {PANEL_MAP_0, PANEL_ADDR_70, 0x19, 0},
//    {PANEL_MAP_0, PANEL_ADDR_71, 0x11, 0},
//    {PANEL_MAP_0, PANEL_ADDR_72, 0x06, 0},
//    {PANEL_MAP_0, PANEL_ADDR_73, 0x16, 0},
//    {PANEL_MAP_0, PANEL_ADDR_74, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_75, 0x0E, 0},
//    {PANEL_MAP_0, PANEL_ADDR_76, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_77, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_78, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_79, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_7A, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_7B, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_7C, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_7D, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_7E, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_7F, 0x, 0},

//    {PANEL_MAP_0, PANEL_ADDR_80, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_81, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_82, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_83, 0x03, 0},
//    {PANEL_MAP_0, PANEL_ADDR_84, 0x84, 0},
    {PANEL_MAP_0, PANEL_ADDR_85, 0x12, 0},
//    {PANEL_MAP_0, PANEL_ADDR_86, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_87, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_88, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_89, 0x04, 0},
    {PANEL_MAP_0, PANEL_ADDR_8A, 0x65, 0},
//    {PANEL_MAP_0, PANEL_ADDR_8B, 0xDA, 0},
//    {PANEL_MAP_0, PANEL_ADDR_8C, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_8D, 0x, 0},
    {PANEL_MAP_0, PANEL_ADDR_8E, 0x0C, 0},
//    {PANEL_MAP_0, PANEL_ADDR_8F, 0x, 0},
//
//    {PANEL_MAP_0, PANEL_ADDR_90, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_91, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_92, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_93, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_94, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_95, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_96, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_97, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_98, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_99, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_9A, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_9B, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_9C, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_9D, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_9E, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_9F, 0x, 0},
//
//    {PANEL_MAP_0, PANEL_ADDR_A0, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_A1, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_A2, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_A3, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_A4, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_A5, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_A6, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_A7, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_A8, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_A9, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_AA, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_AB, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_AC, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_AD, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_AE, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_AF, 0x, 0},
//
//    {PANEL_MAP_0, PANEL_ADDR_B0, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_B1, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_B2, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_B3, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_B4, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_B5, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_B6, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_B7, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_B8, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_B9, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_BA, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_BB, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_BC, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_BD, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_BE, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_BF, 0x08, 0},

//    {PANEL_MAP_0, PANEL_ADDR_C0, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_C1, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_C2, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_C3, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_C4, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_C5, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_C6, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_C7, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_C8, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_C9, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_CA, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_CB, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_CC, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_CD, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_CE, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_CF, 0x, 0},
//
//    {PANEL_MAP_0, PANEL_ADDR_D0, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_D1, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_D2, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_D3, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_D4, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_D5, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_D6, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_D7, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_D8, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_D9, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_DA, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_DB, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_DC, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_DD, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_DE, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_DF, 0x, 0},
//
//    {PANEL_MAP_0, PANEL_ADDR_E0, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_E1, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_E2, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_E3, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_E4, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_E5, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_E6, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_E7, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_E8, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_E9, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_EA, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_EB, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_EC, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_ED, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_EE, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_EF, 0x, 0},
//
//    {PANEL_MAP_0, PANEL_ADDR_F0, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_F1, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_F2, 0x01, 0},
    {PANEL_MAP_0, PANEL_ADDR_F3, 0x19, 0},
//    {PANEL_MAP_0, PANEL_ADDR_F4, 0x40, 0},
    {PANEL_MAP_0, PANEL_ADDR_F5, 0x01, 0},
//    {PANEL_MAP_0, PANEL_ADDR_F6, 0x42, 0},
    {PANEL_MAP_0, PANEL_ADDR_F7, 0x32, 0},
    {PANEL_MAP_0, PANEL_ADDR_F8, 0x64, 0},
//    {PANEL_MAP_0, PANEL_ADDR_F9, 0x42, 0},
    {PANEL_MAP_0, PANEL_ADDR_FA, 0x32, 0},
    {PANEL_MAP_0, PANEL_ADDR_FB, 0x64, 0},
//    {PANEL_MAP_0, PANEL_ADDR_FC, 0x70, 0},
    {PANEL_MAP_0, PANEL_ADDR_FD, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_FE, 0xFF, 0},
    {PANEL_TABLE_END, 0, 0, 0},

};

const setPanelSeqTable PanelContPanelReg120HzSettingTable[] = {
//    {PANEL_MAP_0, PANEL_ADDR_01, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_02, 0x8C, 0},
//    {PANEL_MAP_0, PANEL_ADDR_03, 0xD9, 0},
//    {PANEL_MAP_0, PANEL_ADDR_04, 0x05, 0},
//    {PANEL_MAP_0, PANEL_ADDR_05, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_06, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_07, 0x10, 0},
//    {PANEL_MAP_0, PANEL_ADDR_08, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_09, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_0A, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_0B, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_0C, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_0D, 0x10, 0},
//    {PANEL_MAP_0, PANEL_ADDR_0E, 0x44, 0},
//    {PANEL_MAP_0, PANEL_ADDR_0F, 0x00, 0},

//    {PANEL_MAP_0, PANEL_ADDR_10, 0x0F, 0},
//    {PANEL_MAP_0, PANEL_ADDR_11, 0x04, 0},
//    {PANEL_MAP_0, PANEL_ADDR_12, 0x01, 0},
//    {PANEL_MAP_0, PANEL_ADDR_13, 0x5E, 0},
//    {PANEL_MAP_0, PANEL_ADDR_14, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_15, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_16, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_17, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_18, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_19, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_1A, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_1B, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_1C, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_1D, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_1E, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_1F, 0x, 0},
//
//    {PANEL_MAP_0, PANEL_ADDR_20, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_21, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_22, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_23, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_24, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_25, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_26, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_27, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_28, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_29, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_2A, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_2B, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_2C, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_2D, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_2E, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_2F, 0x, 0},
//
//    {PANEL_MAP_0, PANEL_ADDR_30, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_31, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_32, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_33, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_34, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_35, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_36, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_37, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_38, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_39, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_3A, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_3B, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_3C, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_3D, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_3E, 0x21, 0},
//    {PANEL_MAP_0, PANEL_ADDR_3F, 0x, 0},
//
//    {PANEL_MAP_0, PANEL_ADDR_40, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_41, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_42, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_43, 0x40, 0},
//    {PANEL_MAP_0, PANEL_ADDR_44, 0x29, 0},
//    {PANEL_MAP_0, PANEL_ADDR_45, 0xD9, 0},
//    {PANEL_MAP_0, PANEL_ADDR_46, 0x02, 0},
    {PANEL_MAP_0, PANEL_ADDR_47, 0x33, 0},
//    {PANEL_MAP_0, PANEL_ADDR_48, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_49, 0x18, 0},
//    {PANEL_MAP_0, PANEL_ADDR_4A, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_4B, 0x22, 0},
    {PANEL_MAP_0, PANEL_ADDR_4C, 0x34, 0},
    {PANEL_MAP_0, PANEL_ADDR_4D, 0x33, 0},
//    {PANEL_MAP_0, PANEL_ADDR_4E, 0x04, 0},
    {PANEL_MAP_0, PANEL_ADDR_4F, 0x0D, 0},

//    {PANEL_MAP_0, PANEL_ADDR_50, 0x18, 0},
//    {PANEL_MAP_0, PANEL_ADDR_51, 0x22, 0},
    {PANEL_MAP_0, PANEL_ADDR_52, 0x35, 0},
    {PANEL_MAP_0, PANEL_ADDR_53, 0x34, 0},
//    {PANEL_MAP_0, PANEL_ADDR_54, 0x04, 0},
    {PANEL_MAP_0, PANEL_ADDR_55, 0x0D, 0},
//    {PANEL_MAP_0, PANEL_ADDR_56, 0x18, 0},
//    {PANEL_MAP_0, PANEL_ADDR_57, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_58, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_59, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_5A, 0x19, 0},
//    {PANEL_MAP_0, PANEL_ADDR_5B, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_5C, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_5D, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_5E, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_5F, 0x, 0},
//
//    {PANEL_MAP_0, PANEL_ADDR_60, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_61, 0x1F, 0},
//    {PANEL_MAP_0, PANEL_ADDR_62, 0x06, 0},
//    {PANEL_MAP_0, PANEL_ADDR_63, 0x01, 0},
//    {PANEL_MAP_0, PANEL_ADDR_64, 0xD7, 0},
//    {PANEL_MAP_0, PANEL_ADDR_65, 0x0B, 0},
//    {PANEL_MAP_0, PANEL_ADDR_66, 0x10, 0},
//    {PANEL_MAP_0, PANEL_ADDR_67, 0x2A, 0},
//    {PANEL_MAP_0, PANEL_ADDR_68, 0xEF, 0},
//    {PANEL_MAP_0, PANEL_ADDR_69, 0x10, 0},
//    {PANEL_MAP_0, PANEL_ADDR_6A, 0x0E, 0},
//    {PANEL_MAP_0, PANEL_ADDR_6B, 0xCB, 0},
//    {PANEL_MAP_0, PANEL_ADDR_6C, 0x11, 0},
//    {PANEL_MAP_0, PANEL_ADDR_6D, 0xDA, 0},
//    {PANEL_MAP_0, PANEL_ADDR_6E, 0xE7, 0},
//    {PANEL_MAP_0, PANEL_ADDR_6F, 0x10, 0},
//
//    {PANEL_MAP_0, PANEL_ADDR_70, 0x32, 0},
//    {PANEL_MAP_0, PANEL_ADDR_71, 0x22, 0},
//    {PANEL_MAP_0, PANEL_ADDR_72, 0x0B, 0},
//    {PANEL_MAP_0, PANEL_ADDR_73, 0x2C, 0},
//    {PANEL_MAP_0, PANEL_ADDR_74, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_75, 0x1C, 0},
//    {PANEL_MAP_0, PANEL_ADDR_76, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_77, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_78, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_79, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_7A, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_7B, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_7C, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_7D, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_7E, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_7F, 0x, 0},

//    {PANEL_MAP_0, PANEL_ADDR_80, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_81, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_82, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_83, 0x03, 0},
//    {PANEL_MAP_0, PANEL_ADDR_84, 0x84, 0},
    {PANEL_MAP_0, PANEL_ADDR_85, 0x12, 0},
//    {PANEL_MAP_0, PANEL_ADDR_86, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_87, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_88, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_89, 0x04, 0},
    {PANEL_MAP_0, PANEL_ADDR_8A, 0x65, 0},
//    {PANEL_MAP_0, PANEL_ADDR_8B, 0xDA, 0},
//    {PANEL_MAP_0, PANEL_ADDR_8C, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_8D, 0x, 0},
    {PANEL_MAP_0, PANEL_ADDR_8E, 0x0C, 0},
//    {PANEL_MAP_0, PANEL_ADDR_8F, 0x, 0},
//
//    {PANEL_MAP_0, PANEL_ADDR_90, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_91, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_92, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_93, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_94, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_95, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_96, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_97, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_98, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_99, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_9A, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_9B, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_9C, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_9D, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_9E, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_9F, 0x, 0},
//
//    {PANEL_MAP_0, PANEL_ADDR_A0, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_A1, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_A2, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_A3, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_A4, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_A5, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_A6, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_A7, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_A8, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_A9, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_AA, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_AB, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_AC, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_AD, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_AE, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_AF, 0x, 0},
//
//    {PANEL_MAP_0, PANEL_ADDR_B0, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_B1, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_B2, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_B3, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_B4, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_B5, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_B6, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_B7, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_B8, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_B9, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_BA, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_BB, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_BC, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_BD, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_BE, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_BF, 0x08, 0},

//    {PANEL_MAP_0, PANEL_ADDR_C0, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_C1, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_C2, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_C3, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_C4, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_C5, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_C6, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_C7, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_C8, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_C9, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_CA, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_CB, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_CC, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_CD, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_CE, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_CF, 0x, 0},
//
//    {PANEL_MAP_0, PANEL_ADDR_D0, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_D1, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_D2, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_D3, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_D4, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_D5, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_D6, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_D7, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_D8, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_D9, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_DA, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_DB, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_DC, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_DD, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_DE, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_DF, 0x, 0},
//
//    {PANEL_MAP_0, PANEL_ADDR_E0, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_E1, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_E2, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_E3, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_E4, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_E5, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_E6, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_E7, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_E8, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_E9, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_EA, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_EB, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_EC, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_ED, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_EE, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_EF, 0x, 0},
//
//    {PANEL_MAP_0, PANEL_ADDR_F0, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_F1, 0x, 0},
//    {PANEL_MAP_0, PANEL_ADDR_F2, 0x01, 0},
    {PANEL_MAP_0, PANEL_ADDR_F3, 0x19, 0},
//    {PANEL_MAP_0, PANEL_ADDR_F4, 0x40, 0},
    {PANEL_MAP_0, PANEL_ADDR_F5, 0x01, 0},
//    {PANEL_MAP_0, PANEL_ADDR_F6, 0x42, 0},
    {PANEL_MAP_0, PANEL_ADDR_F7, 0x32, 0},
    {PANEL_MAP_0, PANEL_ADDR_F8, 0x64, 0},
//    {PANEL_MAP_0, PANEL_ADDR_F9, 0x42, 0},
    {PANEL_MAP_0, PANEL_ADDR_FA, 0x32, 0},
    {PANEL_MAP_0, PANEL_ADDR_FB, 0x64, 0},
//    {PANEL_MAP_0, PANEL_ADDR_FC, 0x70, 0},
    {PANEL_MAP_0, PANEL_ADDR_FD, 0x00, 0},
//    {PANEL_MAP_0, PANEL_ADDR_FE, 0xFF, 0},
    {PANEL_TABLE_END, 0, 0, 0},

};

const setPanelSeqTable LeftPanelContLuminance60HzSettingTable[] = {
    {PANEL_MAP_0, PANEL_ADDR_10, 0x0F, 0},
//    {PANEL_MAP_0, PANEL_ADDR_02, 0x80, 0},
    {PANEL_MAP_0, PANEL_ADDR_03, 0x58, 0},
    {PANEL_MAP_0, PANEL_ADDR_04, 0x03, 0},
    {PANEL_TABLE_END, 0, 0, 0},
};

const setPanelSeqTable RightPanelContLuminance60HzSettingTable[] = {
    {PANEL_MAP_0, PANEL_ADDR_10, 0x0F, 0},
//    {PANEL_MAP_0, PANEL_ADDR_02, 0x8C, 0},
    {PANEL_MAP_0, PANEL_ADDR_03, 0x58, 0},
    {PANEL_MAP_0, PANEL_ADDR_04, 0x03, 0},
    {PANEL_TABLE_END, 0, 0, 0},
};

const setPanelSeqTable LeftPanelContLuminance120HzSettingTable[] = {
    {PANEL_MAP_0, PANEL_ADDR_10, 0x0F, 0},
//    {PANEL_MAP_0, PANEL_ADDR_02, 0x80, 0},
    {PANEL_MAP_0, PANEL_ADDR_03, 0xD9, 0},
    {PANEL_MAP_0, PANEL_ADDR_04, 0x05, 0},
    {PANEL_TABLE_END, 0, 0, 0},
};
const setPanelSeqTable RightPanelContLuminance120HzSettingTable[] = {
    {PANEL_MAP_0, PANEL_ADDR_10, 0x0F, 0},
//    {PANEL_MAP_0, PANEL_ADDR_02, 0x8C, 0},
    {PANEL_MAP_0, PANEL_ADDR_03, 0xD9, 0},
    {PANEL_MAP_0, PANEL_ADDR_04, 0x05, 0},
    {PANEL_TABLE_END, 0, 0, 0},
};

const setPanelSeqTable PanelContPSReleaseTable[] = {
    {PANEL_MAP_0, PANEL_ADDR_00, 0x01, 0},
    {PANEL_MAP_0, PANEL_ADDR_00, 0x03, 0},
    {PANEL_MAP_0, PANEL_ADDR_00, 0x07, 0},
    {PANEL_TABLE_END, 0, 0, 0},
};

const setPanelSeqTable PanelContPSTransitionTable[] = {
    {PANEL_MAP_0, PANEL_ADDR_00, 0x03, 0},
    {PANEL_MAP_0, PANEL_ADDR_00, 0x01, 0},
    {PANEL_MAP_0, PANEL_ADDR_00, 0x00, 0},
    {PANEL_TABLE_END, 0, 0, 0},
};
