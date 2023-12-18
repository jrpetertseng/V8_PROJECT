/*
 * ecx343.c
 *
 *  Created on: Dec 21, 2022
 *      Author: User
 */
#include "spi.h"
#include "ecx343.h"
#include "main.h"
#include "flash_rw_process.h"
#include "cmd_engine.h"
#include "lt7911.h"
#include "stdbool.h"
#include "math.h"
#include "usb.h"
#define DEBUG_ADC 0

// Define constants used for ADC conversion and temperature processing
#define VREF 1.8f  								// Reference voltage for ADC
#define ADC_RESOLUTION ((1 << 12) - 1)  		// Resolution of ADC, 12 bits in this case
#define MAX_BUFFER_SIZE 10  					// Maximum size of the circular buffer
#define SOME_THRESHOLD 5    					// Threshold for initial temperature readings
#define ALPHA 0.05f          					// Smoothing factor for exponential smoothing
#define MIN_TEMPERATURE 20.0f  					// Minimum valid temperature
#define MAX_TEMPERATURE 100.0f  				// Maximum valid temperature
#define TEMPERATURE_THRESHOLD 5.0f  			// Maximum allowed deviation from smoothed temperature

// Circular buffer structure for storing temperature readings
typedef struct {
    float buffer[MAX_BUFFER_SIZE];  // Array to store temperature values
    int head;  						// Index of the next element to write in the buffer
    int size;  						// Current number of elements in the buffer
} CircularBuffer;

/*Panel Select Right: 0, Left: 1*/

uint8_t pow_on_seq = 1;
uint8_t pow_off_seq = 1;
uint8_t panel_cont_seq = 0;
uint8_t pnl_queue = 0;

SPI_HandleTypeDef *spi_handle_L;
SPI_HandleTypeDef *spi_handle_R;
uint8_t PanelCurrentMap;
uint8_t flag_Freq, flag_2D3D;

uint16_t current_brightness[2] = {0, 0};
const uint16_t minInternalBrightness = 0x64;
const uint16_t maxInternalBrightness = 0x1F4;

// Global variables for storing temperature values and counts
float temperatureLeft;
float temperatureRight;
float smoothed_left;
float smoothed_right;
static int leftTemperatureCount = 0;
static int rightTemperatureCount = 0;
static CircularBuffer leftTemperatureBuffer, rightTemperatureBuffer;
static bool isBufferInitialized = false;

static uint8_t invL_masks[] = {0x80, 0x84, 0x8C, 0x88};
static uint8_t invR_masks[] = {0x80, 0x84, 0x8C, 0x88};

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;

extern ECX343_DATA ecx343_data;
extern ECX343_DATA ecx343_current_data;
extern uint32_t ecx343RW_buf[sizeof(ECX343_DATA)/4];

const setPanelSeqTable PanelContPanelReg60HzSettingTable[];
const setPanelSeqTable PanelContPanelReg120HzSettingTable[];
const setPanelSeqTable LeftPanelContLuminance60HzSettingTable[];
const setPanelSeqTable RightPanelContLuminance60HzSettingTable[];
const setPanelSeqTable LeftPanelContLuminance120HzSettingTable[];
const setPanelSeqTable RightPanelContLuminance120HzSettingTable[];
const setPanelSeqTable PanelContPSReleaseTable[];
const setPanelSeqTable PanelContPSTransitionTable[];

static void ecx343en_l_spi_enable() { ECX343EN_L_SPI_ENABLE(); }
static void ecx343en_l_spi_disable() { ECX343EN_L_SPI_DISABLE(); }
static void ecx343en_r_spi_enable() { ECX343EN_R_SPI_ENABLE(); }
static void ecx343en_r_spi_disable() { ECX343EN_R_SPI_DISABLE(); }

static void panel_reg_rw_init(SPI_HandleTypeDef *hspi_L, SPI_HandleTypeDef *hspi_R);
static PowContStatus pow_on_sequence(void);
static PowContStatus panel_reg_setting(uint8_t pnl_select);
static PowContStatus panel_left_luminance_setting(void);
static PowContStatus panel_right_luminance_setting(void);
static PowContStatus panel_ps_release(uint8_t pnl_select);

static PowContStatus pow_off_sequence(void);
static PowContStatus panel_ps_transition(uint8_t pnl_select);

static PanelRegStatus panel_reg_read_value(uint8_t addr, uint8_t *value, uint8_t pnl_select);
static PanelRegStatus panel_reg_write_value(uint8_t addr, uint8_t value, uint8_t pnl_select);
static bool is_ecx343RW_buf_valid(ECX343_DATA *buf);

void ECX343EN_Init(void)
{
    panel_reg_rw_init(&hspi4, &hspi2);

}

void ECX343EN_Run(void)
{
    ECX343EN_PowerOn();

}

void ECX343EN_PowerOn(void)
{
    while(pow_on_sequence() == PANEL_CONT_CONTINUE) {
        osDelay(10);
    }
    pow_on_seq = 1;
}

void ECX343EN_PowerOff(void)
{
    while(pow_off_sequence() == PANEL_CONT_CONTINUE) {
        osDelay(10);
    }
    pow_off_seq = 1;
}

static PowContStatus pow_on_sequence(void)
{
    PowContStatus pnl_status;

    //power on sequence
    switch(pow_on_seq)
    {
        case POW_ON_SEQ_NONE:
            return POW_CONT_OK;

        case POW_ON_SEQ_PNL_1V8:
            ECX343EN_1V8_ENABLE();
            pow_on_seq = POW_ON_SEQ_PNL_3V3;
            break;

        case POW_ON_SEQ_PNL_3V3:
            ECX343EN_3V3_ENABLE();
            pow_on_seq = POW_ON_SEQ_PNL_6V6_N;
            break;

        case POW_ON_SEQ_PNL_6V6_N:
            ECX343EN_6V6_N_ENABLE();
            pow_on_seq = POW_ON_SEQ_P_XCLR;
            break;

        case POW_ON_SEQ_P_XCLR:
            ECX343EN_L_XCLR_ENABLE();
            ECX343EN_R_XCLR_ENABLE();
            pow_on_seq = POW_ON_SEQ_PANEL_REG_SETTING;
            break;

        case POW_ON_SEQ_PANEL_REG_SETTING:
            pnl_status = panel_reg_setting(pnl_queue);
            if (pnl_status == PANEL_CONT_OK) {
                pnl_queue = !pnl_queue;
                if (!pnl_queue) {
                    pow_on_seq = POW_ON_SEQ_PANEL_LUMINANCE_SETTING;
                }
            } else if (pnl_status == PANEL_CONT_ERR) {
                pow_on_seq = POW_ON_SEQ_NONE;
                return POW_CONT_ERR;
            }
            break;

        case POW_ON_SEQ_PANEL_LUMINANCE_SETTING:
            if (pnl_queue) {
                pnl_status = panel_left_luminance_setting();
            } else {
                pnl_status = panel_right_luminance_setting();
            }

            if (pnl_status == PANEL_CONT_OK) {
                pnl_queue = !pnl_queue;
                if (!pnl_queue) {
                    pow_on_seq = POW_ON_SEQ_LVDS_EN;
                    write_panel_registers(&ecx343_current_data);
                }
            } else if (pnl_status == PANEL_CONT_ERR) {
                pow_on_seq = POW_ON_SEQ_NONE;
                return POW_CONT_ERR;
            }
            break;

        case POW_ON_SEQ_LVDS_EN:
            ECX343EN_LVDS_ENABLE();
            pow_on_seq = POW_ON_SEQ_PS_OFF_SETTING;
            break;

        case POW_ON_SEQ_PS_OFF_SETTING:
            pnl_status = panel_ps_release(pnl_queue);
            if (pnl_status == PANEL_CONT_OK) {
                pnl_queue = !pnl_queue;
                if (!pnl_queue) {
                    pow_on_seq = POW_ON_SEQ_NONE;
                    return POW_CONT_OK;
                }
            } else if (pnl_status == PANEL_CONT_ERR) {
                pow_on_seq = POW_ON_SEQ_NONE;
                return POW_CONT_ERR;
            }
            break;

        default:
            pow_on_seq = POW_ON_SEQ_NONE;
            return POW_CONT_OK;
    }

    return POW_CONT_CONTINUE;
}

static PowContStatus panel_reg_setting(uint8_t pnl_select)
{
    const setPanelSeqTable *table;

    if (flag_Freq) {
        table = PanelContPanelReg120HzSettingTable;
    } else {
        table = PanelContPanelReg60HzSettingTable;
    }

    if (table[panel_cont_seq].map != PANEL_TABLE_END) {
        if (panel_reg_write(table[panel_cont_seq].map,
                            table[panel_cont_seq].addr,
                            table[panel_cont_seq].value,
                            pnl_select) == PANEL_REG_ERR) {
            panel_cont_seq = PANEL_CONT_SEQ_NONE;
            return PANEL_REG_ERR;
        }
        panel_cont_seq++;
    } else {
        panel_cont_seq = PANEL_CONT_SEQ_NONE;
        return PANEL_REG_OK;
    }

    return PANEL_REG_CONTINUE;
}

static PowContStatus panel_left_luminance_setting()
{
    const setPanelSeqTable *table;

    if (flag_Freq) {
        table = LeftPanelContLuminance120HzSettingTable;
    } else {
        table = LeftPanelContLuminance60HzSettingTable;
    }

    if (table[panel_cont_seq].map != PANEL_TABLE_END) {
        if (panel_reg_write(table[panel_cont_seq].map,
                            table[panel_cont_seq].addr,
                            table[panel_cont_seq].value,
                            PANEL_LEFT) == PANEL_REG_ERR) {
            panel_cont_seq = PANEL_CONT_SEQ_NONE;
            return PANEL_REG_ERR;
        }
        panel_cont_seq++;
    } else {
        panel_cont_seq = PANEL_CONT_SEQ_NONE;
        return PANEL_REG_OK;
    }

    return PANEL_REG_CONTINUE;
}

static PowContStatus panel_right_luminance_setting()
{
    const setPanelSeqTable *table;

    if (flag_Freq) {
        table = RightPanelContLuminance120HzSettingTable;
    } else {
        table = RightPanelContLuminance60HzSettingTable;
    }

    if (table[panel_cont_seq].map != PANEL_TABLE_END) {
        if (panel_reg_write(table[panel_cont_seq].map,
                            table[panel_cont_seq].addr,
                            table[panel_cont_seq].value,
                            PANEL_RIGHT) == PANEL_REG_ERR) {
            panel_cont_seq = PANEL_CONT_SEQ_NONE;
            return PANEL_REG_ERR;
        }
        panel_cont_seq++;
    } else {
        panel_cont_seq = PANEL_CONT_SEQ_NONE;
        return PANEL_REG_OK;
    }

    return PANEL_REG_CONTINUE;
}

static PowContStatus panel_ps_release(uint8_t pnl_select)
{
    //Panel register setting
    if(PanelContPSReleaseTable[panel_cont_seq].map != PANEL_TABLE_END) {
        if(panel_reg_write(PanelContPSReleaseTable[panel_cont_seq].map,
                           PanelContPSReleaseTable[panel_cont_seq].addr,
                           PanelContPSReleaseTable[panel_cont_seq].value,
                           pnl_select) == PANEL_REG_ERR) {
            panel_cont_seq = PANEL_CONT_SEQ_NONE;
            return PANEL_REG_ERR;
        }
        panel_cont_seq++;
    } else {
        panel_cont_seq = PANEL_CONT_SEQ_NONE;
        return PANEL_REG_OK;
    }

    return PANEL_REG_CONTINUE;
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
                pnl_status = panel_ps_transition(pnl_queue);
                if(pnl_status == PANEL_CONT_OK) {
                    osDelay(24);
                    pnl_queue = !pnl_queue;
                    if (!pnl_queue) {
                        pow_off_seq = POW_OFF_SEQ_LVDS_DIS;
                    }
                } else if(pnl_status == PANEL_CONT_ERR) {
                    pow_off_seq = POW_OFF_SEQ_NONE;
                    return POW_CONT_ERR;
                }
                break;

            case POW_OFF_SEQ_LVDS_DIS:
                ECX343EN_LVDS_DISABLE();
                pow_off_seq = POW_OFF_SEQ_P_XCLR;
                break;

            case POW_OFF_SEQ_P_XCLR:
                ECX343EN_L_XCLR_DISABLE();
                ECX343EN_R_XCLR_DISABLE();
                pow_off_seq = POW_OFF_SEQ_PNL_6V6_N;
                break;

            case POW_OFF_SEQ_PNL_6V6_N:
                ECX343EN_6V6_N_DISABLE();
                pow_off_seq = POW_OFF_SEQ_PNL_3V3;
                break;

            case POW_OFF_SEQ_PNL_3V3:
                ECX343EN_3V3_DISABLE();
                pow_off_seq = POW_OFF_SEQ_PNL_1V8;
                break;

            case POW_OFF_SEQ_PNL_1V8:
                ECX343EN_1V8_DISABLE();
                pow_off_seq = POW_OFF_SEQ_NONE;
                return POW_CONT_OK;

            default:
                pow_off_seq = POW_OFF_SEQ_NONE;
                break;
        }

        return POW_CONT_CONTINUE;
}

static PowContStatus panel_ps_transition(uint8_t pnl_select)
{
    //Panel register setting
    if(PanelContPSTransitionTable[panel_cont_seq].map != PANEL_TABLE_END) {
        if(panel_reg_write(PanelContPSTransitionTable[panel_cont_seq].map,
                           PanelContPSTransitionTable[panel_cont_seq].addr,
                           PanelContPSTransitionTable[panel_cont_seq].value,
                           pnl_select) == PANEL_REG_ERR) {
            panel_cont_seq = PANEL_CONT_SEQ_NONE;
            return PANEL_REG_ERR;
        }
        panel_cont_seq++;
    } else {
        panel_cont_seq = PANEL_CONT_SEQ_NONE;
        return PANEL_REG_OK;
    }

    return PANEL_REG_CONTINUE;
}

static PanelRegStatus panel_reg_read_value(uint8_t addr, uint8_t *value, uint8_t pnl_select)
{
    uint8_t PanelWriteBuf[10];
    uint8_t PanelReadBuf[10];
    SPI_HandleTypeDef *spi_handle = pnl_select ? spi_handle_L : spi_handle_R;
    void (*spi_enable)() = pnl_select ? ecx343en_l_spi_enable : ecx343en_r_spi_enable;
    void (*spi_disable)() = pnl_select ? ecx343en_l_spi_disable : ecx343en_r_spi_disable;

    PanelWriteBuf[0] = PANEL_ADDR_81;
    PanelWriteBuf[1] = addr;

    spi_enable();
    if (HAL_SPI_Transmit(spi_handle, &PanelWriteBuf[0], PANEL_WRITE_LENGTH, PANEL_REG_RW_TIMEOUT) != HAL_OK) {
        return PANEL_REG_ERR;
    }
    spi_disable();

    PanelWriteBuf[0] = PANEL_ADDR_81;
    PanelWriteBuf[1] = 0xFF;

    spi_enable();
    if (HAL_SPI_TransmitReceive(spi_handle, &PanelWriteBuf[0], &PanelReadBuf[0], PANEL_WRITE_LENGTH, PANEL_REG_RW_TIMEOUT) != HAL_OK) {
        return PANEL_REG_ERR;
    }
    spi_disable();

    *value = PanelReadBuf[1];

    return PANEL_REG_OK;
}

static PanelRegStatus panel_reg_write_value(uint8_t addr, uint8_t value, uint8_t pnl_select)
{
    uint8_t PanelWriteBuf[10];
    SPI_HandleTypeDef *spi_handle = pnl_select ? spi_handle_L : spi_handle_R;
    void (*spi_enable)() = pnl_select ? ecx343en_l_spi_enable : ecx343en_r_spi_enable;
    void (*spi_disable)() = pnl_select ? ecx343en_l_spi_disable : ecx343en_r_spi_disable;

    PanelWriteBuf[0] = addr;
    PanelWriteBuf[1] = value;

    spi_enable();
    if (HAL_SPI_Transmit(spi_handle, &PanelWriteBuf[0], PANEL_WRITE_LENGTH, PANEL_REG_RW_TIMEOUT) != HAL_OK) {
        return PANEL_REG_ERR;
    }
    spi_disable();

    return PANEL_REG_OK;
}

PanelRegStatus panel_reg_read(uint8_t map, uint8_t addr, uint8_t *value, uint8_t pnl_select)
{
    //map check
    if(PanelCurrentMap != map) {
        //change reg map
        if(panel_reg_write_value(PANEL_ADDR_82, map, pnl_select) == PANEL_REG_ERR) {
            return PANEL_REG_ERR;
        }
        PanelCurrentMap = map;

    }

    //read panel register
    if(panel_reg_read_value(addr, value, pnl_select) == PANEL_REG_ERR) {
        return PANEL_REG_ERR;
    }

    return PANEL_REG_OK;
}

PanelRegStatus panel_reg_write(uint8_t map, uint8_t addr, uint8_t value, uint8_t pnl_select)
{
    //map check
    if(PanelCurrentMap != map) {
        //change reg map
        if(panel_reg_write_value(PANEL_ADDR_82, map, pnl_select) == PANEL_REG_ERR) {
            return PANEL_REG_ERR;
        }
        PanelCurrentMap = map;
    }

    //write panel register
    if(panel_reg_write_value(addr, value, pnl_select) == PANEL_REG_ERR) {
        return PANEL_REG_ERR;
    }

    return PANEL_REG_OK;
}

void ECX343EN_Inversion(uint8_t value, uint8_t pnl_select)
{
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
    while(panel_ps_transition(pnl_select) == PANEL_CONT_CONTINUE) {
        osDelay(10);
    }

    if(pnl_select)
    {
        panel_reg_write(PANEL_MAP_0, SCAN_MODE, invL_masks[value], pnl_select);
        osDelay(10);
    }
    else
    {
        panel_reg_write(PANEL_MAP_0, SCAN_MODE, invR_masks[value], pnl_select);
        osDelay(10);
    }

    while(panel_ps_release(pnl_select) == PANEL_CONT_CONTINUE) {
        osDelay(10);
    }

}

void ECX343EN_LuminanceModeSetting(uint8_t value, uint8_t pnl_select)
{
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
    panel_reg_write(PANEL_MAP_0, LUMINANCE_MODE_SETTING, value, pnl_select);
}

void ECX343EN_ArbitraryLuminanceH(uint8_t value, uint8_t pnl_select)
{
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
    panel_reg_write(PANEL_MAP_0, ARBITRARY_LUMINANCE_VALUE_HIGH, value, pnl_select);
}

void ECX343EN_ArbitraryLuminanceL(uint8_t value, uint8_t pnl_select)
{
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
    panel_reg_write(PANEL_MAP_0, ARBITRARY_LUMINANCE_VALUE_LOW, value, pnl_select);
}

void ECX343EN_PresetLuminanceValue(uint8_t value, uint8_t pnl_select)
{
    /*
    default: 0010 0000
    [0]
    Luminance preset mode selection
    0: 500cd/m2 (Default)
    1: 1000cd/m2
    */
    panel_reg_write(PANEL_MAP_0, PRESET_LUMINANCE_VALUE, value, pnl_select);
}

void ECX343EN_OrbitH(uint8_t H_value, uint8_t pnl_select)
{
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
    panel_reg_write(PANEL_MAP_0, ORBIT_HORIZONTAL, H_value, pnl_select);
}

void ECX343EN_OrbitV(uint8_t V_value, uint8_t pnl_select)
{
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
    panel_reg_write(PANEL_MAP_0, ORBIT_VERTICAL, V_value, pnl_select);
}

void ECX343EN_TempDetect(uint8_t value, uint8_t pnl_select)
{
    panel_reg_write(PANEL_MAP_0, PANEL_TEMPERATURE_DETECTION, value, pnl_select);
}

bool is_ecx343RW_buf_valid(ECX343_DATA *buf) {
    // 檢查 ecx343RW_buf[0] 和 ecx343RW_buf[1] 是否在範圍 0 到 3 之間
    bool invl_valid = (buf->uLCD_INVL == 0) || (buf->uLCD_INVL == 1) || (buf->uLCD_INVL == 2) || (buf->uLCD_INVL == 3);
    bool invr_valid = (buf->uLCD_INVR == 0) || (buf->uLCD_INVR == 1) || (buf->uLCD_INVR == 2) || (buf->uLCD_INVR == 3);

    // 檢查 ecx343RW_buf[2] 和 ecx343RW_buf[3] 的值是否在範圍 100 到 500 之間
    bool luxl_valid = (buf->uLCD_LUXL >= 100) && (buf->uLCD_LUXL <= 500);
    bool luxr_valid = (buf->uLCD_LUXR >= 100) && (buf->uLCD_LUXR <= 500);

    // 檢查 ecx343RW_buf[4]、ecx343RW_buf[5]、ecx343RW_buf[6] 和 ecx343RW_buf[7] 是否在範圍 0~16 或 100~116 之間
    bool horbl_valid = ((buf->uLCD_HORBL >= 0) && (buf->uLCD_HORBL <= 16)) || ((buf->uLCD_HORBL >= 100) && (buf->uLCD_HORBL <= 116));
    bool horbr_valid = ((buf->uLCD_HORBR >= 0) && (buf->uLCD_HORBR <= 16)) || ((buf->uLCD_HORBR >= 100) && (buf->uLCD_HORBR <= 116));
    bool vorbl_valid = ((buf->uLCD_VORBL >= 0) && (buf->uLCD_VORBL <= 16)) || ((buf->uLCD_VORBL >= 100) && (buf->uLCD_VORBL <= 116));
    bool vorbr_valid = ((buf->uLCD_VORBR >= 0) && (buf->uLCD_VORBR <= 16)) || ((buf->uLCD_VORBR >= 100) && (buf->uLCD_VORBR <= 116));

    // 檢查 ecx343RW_buf[8] 是否在範圍 0 到 3 之間
    bool mode_valid = (buf->uLCD_MODE >= 0) && (buf->uLCD_MODE <= 3);

    // 返回檢查結果
    return invl_valid && invr_valid && luxl_valid && luxr_valid && horbl_valid && horbr_valid && vorbl_valid && vorbr_valid && mode_valid;
}

static void panel_reg_rw_init(SPI_HandleTypeDef *hspi_L, SPI_HandleTypeDef *hspi_R)
{
    PanelCurrentMap = PANEL_MAP_0;
    spi_handle_L = hspi_L;
    spi_handle_R = hspi_R;
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
    flag_Freq = ecx343_current_data.uLCD_MODE & 0x01;
    flag_2D3D = (ecx343_current_data.uLCD_MODE & 0x02) >> 1;
}

void write_panel_registers(ECX343_DATA *data) {
    panel_reg_write(PANEL_MAP_0, SCAN_MODE, invL_masks[data->uLCD_INVL], 1);
    panel_reg_write(PANEL_MAP_0, ARBITRARY_LUMINANCE_VALUE_LOW, data->uLCD_LUXL & 0xFF, 1);
    panel_reg_write(PANEL_MAP_0, ARBITRARY_LUMINANCE_VALUE_HIGH, (data->uLCD_LUXL & 0x100) >> 8, 1);
    panel_reg_write(PANEL_MAP_0, ORBIT_HORIZONTAL, LcdHorbit(data->uLCD_HORBL), 1);
    panel_reg_write(PANEL_MAP_0, ORBIT_VERTICAL, LcdVorbit(data->uLCD_VORBL), 1);

    panel_reg_write(PANEL_MAP_0, SCAN_MODE, invR_masks[data->uLCD_INVR], 0);
    panel_reg_write(PANEL_MAP_0, ARBITRARY_LUMINANCE_VALUE_LOW, data->uLCD_LUXR & 0xFF, 0);
    panel_reg_write(PANEL_MAP_0, ARBITRARY_LUMINANCE_VALUE_HIGH, (data->uLCD_LUXR & 0x100) >> 8, 0);
    panel_reg_write(PANEL_MAP_0, ORBIT_HORIZONTAL, LcdHorbit(data->uLCD_HORBR), 0);
    panel_reg_write(PANEL_MAP_0, ORBIT_VERTICAL, LcdVorbit(data->uLCD_VORBR), 0);

    current_brightness[0] = data->uLCD_LUXR;
    current_brightness[1] = data->uLCD_LUXL;
}

void ECX343EN_SetLuminance(uint16_t internalBrightness, uint8_t pnl_select)
{
    if (internalBrightness < minInternalBrightness || internalBrightness > maxInternalBrightness) {
        return;
    }

    uint8_t highByte = (internalBrightness & 0x0100) >> 8;
    uint8_t lowByte = internalBrightness & 0x00FF;

    ECX343EN_ArbitraryLuminanceH(highByte, pnl_select);
    ECX343EN_ArbitraryLuminanceL(lowByte, pnl_select);
}

int map_lux_to_internal_brightness(int lux)
{
    const int minLux = 100;
    const int maxLux = 1000;

    if (lux <= minLux) return minInternalBrightness;
    if (lux >= maxLux) return maxInternalBrightness;

    return minInternalBrightness +
    		(lux - minLux) * (maxInternalBrightness - minInternalBrightness) / (maxLux - minLux);
}

void smoothly_change_brightness(int target_brightness, uint8_t panel)
{
    const int adjustment_rate = 50;

    if (abs(current_brightness[panel] - target_brightness) < adjustment_rate)
    {
        current_brightness[panel] = target_brightness;
    }
    else if (current_brightness[panel] < target_brightness)
    {
        current_brightness[panel] += adjustment_rate;
    }
    else
    {
        current_brightness[panel] -= adjustment_rate;
    }

    current_brightness[panel] = current_brightness[panel] < minInternalBrightness ?
    		minInternalBrightness : current_brightness[panel];
    current_brightness[panel] = current_brightness[panel] > maxInternalBrightness ?
    		maxInternalBrightness : current_brightness[panel];

    ECX343EN_SetLuminance(current_brightness[panel], panel);
    osDelay(100);
}

// Initialize the circular buffer
void initBuffer(CircularBuffer *cb) {
    cb->head = 0;
    cb->size = 0;
}

// Add a new value to the circular buffer
void addToBuffer(CircularBuffer *cb, float value) {
    cb->buffer[cb->head] = value;  // Store the value in the buffer
    cb->head = (cb->head + 1) % MAX_BUFFER_SIZE;  // Move the head to the next position
    if (cb->size < MAX_BUFFER_SIZE) {
        cb->size++;  // Increase the size until it reaches the maximum
    }
}

// Retrieve an element from the circular buffer at a specific index
float getBufferElement(CircularBuffer *cb, int index) {
    if (index < 0 || index >= cb->size) {
        return 0;  // Return 0 for invalid index
    }
    int realIndex = (cb->head - cb->size + index + MAX_BUFFER_SIZE) % MAX_BUFFER_SIZE;
    return cb->buffer[realIndex];  // Return the value at the computed real index
}

// Apply exponential smoothing to the data in the circular buffer
float exponential_smoothing_sequence(CircularBuffer *cb) {
    if (cb->size == 0) {
        return 0;  // Return 0 if the buffer is empty
    }

    float smoothed_result = getBufferElement(cb, 0);  // Start with the first element
    for (int i = 1; i < cb->size; i++) {
        // Apply exponential smoothing formula
        smoothed_result = ALPHA * getBufferElement(cb, i) + (1 - ALPHA) * smoothed_result;
    }
    return smoothed_result;  // Return the smoothed result
}

// Read voltage from ADC and calculate the temperature
float readVoltageAndCalculate(uint8_t voltage, uint8_t panel, ADC_HandleTypeDef *hadc) {
    uint16_t adcVal;
    float volt = 0;

    ECX343EN_TempDetect(voltage, panel);  // Detect temperature from the specified panel
    osDelay(100);  // Wait for the sensor to stabilize
    HAL_ADC_Start(hadc);  // Start ADC conversion

    // Check if ADC conversion is completed successfully
    if (HAL_ADC_PollForConversion(hadc, 0xFF) == HAL_OK) {
        if (HAL_IS_BIT_SET(HAL_ADC_GetState(hadc), HAL_ADC_STATE_REG_EOC)) {
            adcVal = HAL_ADC_GetValue(hadc);  // Read the ADC value
            volt = (float)adcVal / ADC_RESOLUTION * VREF * 2;  // Calculate the voltage
        }
        HAL_ADC_Stop(hadc);  // Stop ADC conversion
    }
    return volt;  // Return the calculated voltage
}

// Validate the temperature value
bool isTemperatureValueValid(float temperature, float smoothed, int temperatureCount) {
    // Check if the temperature is within the valid range
    if (temperature < MIN_TEMPERATURE || temperature > MAX_TEMPERATURE) {
        return false;
    }
    // Accept the temperature if the count is below the threshold or within the smoothed range
    return temperatureCount < SOME_THRESHOLD || fabs(temperature - smoothed) <= TEMPERATURE_THRESHOLD;
}

// Update panel temperature readings
void updatePanelTemperature(void) {
    if (!isBufferInitialized) {
        initBuffer(&leftTemperatureBuffer);  // Initialize left temperature buffer
        initBuffer(&rightTemperatureBuffer);  // Initialize right temperature buffer
        isBufferInitialized = true;
    }

    // Read and calculate the temperature for both left and right panels
    float leftOrigVolt[2] = {0.0};
    float rightOrigVolt[2] = {0.0};

    leftOrigVolt[0] = readVoltageAndCalculate(PANEL_TEMP_VOLT1, PANEL_LEFT, &hadc1);
    rightOrigVolt[0] = readVoltageAndCalculate(PANEL_TEMP_VOLT1, PANEL_RIGHT, &hadc2);

    leftOrigVolt[1] = readVoltageAndCalculate(PANEL_TEMP_VOLT2, PANEL_LEFT, &hadc1);
    rightOrigVolt[1] = readVoltageAndCalculate(PANEL_TEMP_VOLT2, PANEL_RIGHT, &hadc2);
    // Calculate the panel temperatures using a specific formula
    // This formula is likely based on the characteristics or calibration data of the temperature sensors
    // It converts voltage readings into temperature values
    temperatureLeft = ((leftOrigVolt[1] - leftOrigVolt[0]) - 0.6796) * 1.0 / 0.0025;
    temperatureRight = ((rightOrigVolt[1] - rightOrigVolt[0]) - 0.6796) * 1.0 / 0.0025;

	// Check if the temperature readings are valid and update the buffers
	if (isTemperatureValueValid(temperatureLeft, smoothed_left, leftTemperatureCount)) {
		addToBuffer(&leftTemperatureBuffer, temperatureLeft);  // Add to left buffer
		leftTemperatureCount++;  // Increment left temperature count
	}

	if (isTemperatureValueValid(temperatureRight, smoothed_right, rightTemperatureCount)) {
		addToBuffer(&rightTemperatureBuffer, temperatureRight);  // Add to right buffer
		rightTemperatureCount++;  // Increment right temperature count
	}

	// Apply exponential smoothing to the temperature data in the buffers
	smoothed_left = exponential_smoothing_sequence(&leftTemperatureBuffer);
	smoothed_right = exponential_smoothing_sequence(&rightTemperatureBuffer);


}

void ECX343EN_CalculateTemperatures(float *temperatureResult) {
    float rightOrigVolt[2] = {0.0};
    float leftOrigVolt[2] = {0.0};

    // Read voltage and calculate temperature for left and right panels
    leftOrigVolt[0] = readVoltageAndCalculate(PANEL_TEMP_VOLT1, PANEL_LEFT, &hadc1);
    rightOrigVolt[0] = readVoltageAndCalculate(PANEL_TEMP_VOLT1, PANEL_RIGHT, &hadc2);
    leftOrigVolt[1] = readVoltageAndCalculate(PANEL_TEMP_VOLT2, PANEL_LEFT, &hadc1);
    rightOrigVolt[1] = readVoltageAndCalculate(PANEL_TEMP_VOLT2, PANEL_RIGHT, &hadc2);

    // Calculate and store the temperatures based on voltage readings
    // Ensure these calculations are based on the specific characteristics of your temperature sensors
    temperatureResult[0] = ((leftOrigVolt[1] - leftOrigVolt[0]) - 0.6796) * 1.0 / 0.0025;
    temperatureResult[1] = ((rightOrigVolt[1] - rightOrigVolt[0]) - 0.6796) * 1.0 / 0.0025;
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
//    {PANEL_MAP_0, PANEL_ADDR_8E, 0x00, 0},
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
//    {PANEL_MAP_0, PANEL_ADDR_8E, 0x00, 0},
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
