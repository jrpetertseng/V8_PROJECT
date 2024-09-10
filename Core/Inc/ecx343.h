/*
 * ecx343.h
 *
 *  Created on: Dec 20, 2022
 *      Author: User
 */
#include "main.h"
#include "flash_rw_process.h"
#include "FreeRTOS.h"
#include "semphr.h"

#ifndef INC_ECX343_H_
#define INC_ECX343_H_

#define ECX343EN_L_SPI_ENABLE()                 HAL_GPIO_WritePin(PNL_L_NSS_GPIO_Port, PNL_L_NSS_Pin, GPIO_PIN_RESET);
#define ECX343EN_L_SPI_DISABLE()                HAL_GPIO_WritePin(PNL_L_NSS_GPIO_Port, PNL_L_NSS_Pin, GPIO_PIN_SET);
#define ECX343EN_R_SPI_ENABLE()                 HAL_GPIO_WritePin(PNL_R_NSS_GPIO_Port, PNL_R_NSS_Pin, GPIO_PIN_RESET);
#define ECX343EN_R_SPI_DISABLE()                HAL_GPIO_WritePin(PNL_R_NSS_GPIO_Port, PNL_R_NSS_Pin, GPIO_PIN_SET);

#define ECX343EN_1V8_ENABLE()                   HAL_GPIO_WritePin(PNL_1V8_EN_GPIO_Port, PNL_1V8_EN_Pin, GPIO_PIN_SET);
#define ECX343EN_3V3_ENABLE()                   HAL_GPIO_WritePin(PNL_3V3_EN_GPIO_Port, PNL_3V3_EN_Pin, GPIO_PIN_SET);
#define ECX343EN_6V6_N_ENABLE()                 HAL_GPIO_WritePin(PNL_6V6_N_EN_GPIO_Port, PNL_6V6_N_EN_Pin, GPIO_PIN_SET);
#define ECX343EN_L_XCLR_ENABLE()                HAL_GPIO_WritePin(PNL_L_XCLR_GPIO_Port, PNL_L_XCLR_Pin, GPIO_PIN_SET);
#define ECX343EN_R_XCLR_ENABLE()                HAL_GPIO_WritePin(PNL_R_XCLR_GPIO_Port, PNL_R_XCLR_Pin, GPIO_PIN_SET);
#define ECX343EN_LVDS_ENABLE()                  (HAL_GPIO_ReadPin(LT7911_INT_GPIO_Port, LT7911_INT_Pin) == GPIO_PIN_SET)

#define ECX343EN_1V8_DISABLE()                  HAL_GPIO_WritePin(PNL_1V8_EN_GPIO_Port, PNL_1V8_EN_Pin, GPIO_PIN_RESET);
#define ECX343EN_3V3_DISABLE()                  HAL_GPIO_WritePin(PNL_3V3_EN_GPIO_Port, PNL_3V3_EN_Pin, GPIO_PIN_RESET);
#define ECX343EN_6V6_N_DISABLE()                HAL_GPIO_WritePin(PNL_6V6_N_EN_GPIO_Port, PNL_6V6_N_EN_Pin, GPIO_PIN_RESET);
#define ECX343EN_L_XCLR_DISABLE()               HAL_GPIO_WritePin(PNL_L_XCLR_GPIO_Port, PNL_L_XCLR_Pin, GPIO_PIN_RESET);
#define ECX343EN_R_XCLR_DISABLE()               HAL_GPIO_WritePin(PNL_R_XCLR_GPIO_Port, PNL_R_XCLR_Pin, GPIO_PIN_RESET);
#define ECX343EN_LVDS_DISABLE()                 (HAL_GPIO_ReadPin(LT7911_INT_GPIO_Port, LT7911_INT_Pin) == GPIO_PIN_RESET)


//power on sequence number
#define POW_ON_SEQ_NONE                         0
#define POW_ON_SEQ_PNL_1V8                      1
#define POW_ON_SEQ_PNL_3V3                      2
#define POW_ON_SEQ_PNL_6V6_N                    3
#define POW_ON_SEQ_P_XCLR                       1 // 4
#define POW_ON_SEQ_PANEL_REG_SETTING            2 // 5
#define POW_ON_SEQ_PANEL_LUMINANCE_SETTING      3 // 6
#define POW_ON_SEQ_LVDS_EN                      4 // 7
#define POW_ON_SEQ_PS_OFF_SETTING               5 // 8

//power off sequence number
#define POW_OFF_SEQ_NONE                        0
#define POW_OFF_SEQ_PS_ON_SETTING               1
#define POW_OFF_SEQ_LVDS_DIS                    2
#define POW_OFF_SEQ_P_XCLR                      3
#define POW_OFF_SEQ_PNL_6V6_N                   4
#define POW_OFF_SEQ_PNL_3V3                     5
#define POW_OFF_SEQ_PNL_1V8                     6

//Power Sequence Status
typedef enum {
    POW_CONT_OK         = 0x00,
    POW_CONT_ERR        = 0x01,
    POW_CONT_CONTINUE   = 0x02,
} PowContStatus;

//Map value
#define PANEL_MAP_0                     (uint8_t)0x00

//Address value
#define PANEL_ADDR_00                   0x00
#define PANEL_ADDR_01                   0x01
#define PANEL_ADDR_02                   0x02
#define PANEL_ADDR_03                   0x03
#define PANEL_ADDR_04                   0x04
#define PANEL_ADDR_05                   0x05
#define PANEL_ADDR_06                   0x06
#define PANEL_ADDR_07                   0x07
#define PANEL_ADDR_08                   0x08
#define PANEL_ADDR_09                   0x09
#define PANEL_ADDR_0A                   0x0A
#define PANEL_ADDR_0B                   0x0B
#define PANEL_ADDR_0C                   0x0C
#define PANEL_ADDR_0D                   0x0D
#define PANEL_ADDR_0E                   0x0E
#define PANEL_ADDR_0F                   0x0F

#define PANEL_ADDR_10                   0x10
#define PANEL_ADDR_11                   0x11
#define PANEL_ADDR_12                   0x12
#define PANEL_ADDR_13                   0x13
#define PANEL_ADDR_14                   0x14
#define PANEL_ADDR_15                   0x15
#define PANEL_ADDR_16                   0x16
#define PANEL_ADDR_17                   0x17
#define PANEL_ADDR_18                   0x18
#define PANEL_ADDR_19                   0x19
#define PANEL_ADDR_1A                   0x1A
#define PANEL_ADDR_1B                   0x1B
#define PANEL_ADDR_1C                   0x1C
#define PANEL_ADDR_1D                   0x1D
#define PANEL_ADDR_1E                   0x1E
#define PANEL_ADDR_1F                   0x1F

#define PANEL_ADDR_20                   0x20
#define PANEL_ADDR_21                   0x21
#define PANEL_ADDR_22                   0x22
#define PANEL_ADDR_23                   0x23
#define PANEL_ADDR_24                   0x24
#define PANEL_ADDR_25                   0x25
#define PANEL_ADDR_26                   0x26
#define PANEL_ADDR_27                   0x27
#define PANEL_ADDR_28                   0x28
#define PANEL_ADDR_29                   0x29
#define PANEL_ADDR_2A                   0x2A
#define PANEL_ADDR_2B                   0x2B
#define PANEL_ADDR_2C                   0x2C
#define PANEL_ADDR_2D                   0x2D
#define PANEL_ADDR_2E                   0x2E
#define PANEL_ADDR_2F                   0x2F

#define PANEL_ADDR_30                   0x30
#define PANEL_ADDR_31                   0x31
#define PANEL_ADDR_32                   0x32
#define PANEL_ADDR_33                   0x33
#define PANEL_ADDR_34                   0x34
#define PANEL_ADDR_35                   0x35
#define PANEL_ADDR_36                   0x36
#define PANEL_ADDR_37                   0x37
#define PANEL_ADDR_38                   0x38
#define PANEL_ADDR_39                   0x39
#define PANEL_ADDR_3A                   0x3A
#define PANEL_ADDR_3B                   0x3B
#define PANEL_ADDR_3C                   0x3C
#define PANEL_ADDR_3D                   0x3D
#define PANEL_ADDR_3E                   0x3E
#define PANEL_ADDR_3F                   0x3F

#define PANEL_ADDR_40                   0x40
#define PANEL_ADDR_41                   0x41
#define PANEL_ADDR_42                   0x42
#define PANEL_ADDR_43                   0x43
#define PANEL_ADDR_44                   0x44
#define PANEL_ADDR_45                   0x45
#define PANEL_ADDR_46                   0x46
#define PANEL_ADDR_47                   0x47
#define PANEL_ADDR_48                   0x48
#define PANEL_ADDR_49                   0x49
#define PANEL_ADDR_4A                   0x4A
#define PANEL_ADDR_4B                   0x4B
#define PANEL_ADDR_4C                   0x4C
#define PANEL_ADDR_4D                   0x4D
#define PANEL_ADDR_4E                   0x4E
#define PANEL_ADDR_4F                   0x4F

#define PANEL_ADDR_50                   0x50
#define PANEL_ADDR_51                   0x51
#define PANEL_ADDR_52                   0x52
#define PANEL_ADDR_53                   0x53
#define PANEL_ADDR_54                   0x54
#define PANEL_ADDR_55                   0x55
#define PANEL_ADDR_56                   0x56
#define PANEL_ADDR_57                   0x57
#define PANEL_ADDR_58                   0x58
#define PANEL_ADDR_59                   0x59
#define PANEL_ADDR_5A                   0x5A
#define PANEL_ADDR_5B                   0x5B
#define PANEL_ADDR_5C                   0x5C
#define PANEL_ADDR_5D                   0x5D
#define PANEL_ADDR_5E                   0x5E
#define PANEL_ADDR_5F                   0x5F

#define PANEL_ADDR_60                   0x60
#define PANEL_ADDR_61                   0x61
#define PANEL_ADDR_62                   0x62
#define PANEL_ADDR_63                   0x63
#define PANEL_ADDR_64                   0x64
#define PANEL_ADDR_65                   0x65
#define PANEL_ADDR_66                   0x66
#define PANEL_ADDR_67                   0x67
#define PANEL_ADDR_68                   0x68
#define PANEL_ADDR_69                   0x69
#define PANEL_ADDR_6A                   0x6A
#define PANEL_ADDR_6B                   0x6B
#define PANEL_ADDR_6C                   0x6C
#define PANEL_ADDR_6D                   0x6D
#define PANEL_ADDR_6E                   0x6E
#define PANEL_ADDR_6F                   0x6F

#define PANEL_ADDR_70                   0x70
#define PANEL_ADDR_71                   0x71
#define PANEL_ADDR_72                   0x72
#define PANEL_ADDR_73                   0x73
#define PANEL_ADDR_74                   0x74
#define PANEL_ADDR_75                   0x75
#define PANEL_ADDR_76                   0x76
#define PANEL_ADDR_77                   0x77
#define PANEL_ADDR_78                   0x78
#define PANEL_ADDR_79                   0x79
#define PANEL_ADDR_7A                   0x7A
#define PANEL_ADDR_7B                   0x7B
#define PANEL_ADDR_7C                   0x7C
#define PANEL_ADDR_7D                   0x7D
#define PANEL_ADDR_7E                   0x7E
#define PANEL_ADDR_7F                   0x7F

#define PANEL_ADDR_80                   0x80
#define PANEL_ADDR_81                   0x81
#define PANEL_ADDR_82                   0x82
#define PANEL_ADDR_83                   0x83
#define PANEL_ADDR_84                   0x84
#define PANEL_ADDR_85                   0x85
#define PANEL_ADDR_86                   0x86
#define PANEL_ADDR_87                   0x87
#define PANEL_ADDR_88                   0x88
#define PANEL_ADDR_89                   0x89
#define PANEL_ADDR_8A                   0x8A
#define PANEL_ADDR_8B                   0x8B
#define PANEL_ADDR_8C                   0x8C
#define PANEL_ADDR_8D                   0x8D
#define PANEL_ADDR_8E                   0x8E
#define PANEL_ADDR_8F                   0x8F

#define PANEL_ADDR_90                   0x90
#define PANEL_ADDR_91                   0x91
#define PANEL_ADDR_92                   0x92
#define PANEL_ADDR_93                   0x93
#define PANEL_ADDR_94                   0x94
#define PANEL_ADDR_95                   0x95
#define PANEL_ADDR_96                   0x96
#define PANEL_ADDR_97                   0x97
#define PANEL_ADDR_98                   0x98
#define PANEL_ADDR_99                   0x99
#define PANEL_ADDR_9A                   0x9A
#define PANEL_ADDR_9B                   0x9B
#define PANEL_ADDR_9C                   0x9C
#define PANEL_ADDR_9D                   0x9D
#define PANEL_ADDR_9E                   0x9E
#define PANEL_ADDR_9F                   0x9F

#define PANEL_ADDR_A0                   0xA0
#define PANEL_ADDR_A1                   0xA1
#define PANEL_ADDR_A2                   0xA2
#define PANEL_ADDR_A3                   0xA3
#define PANEL_ADDR_A4                   0xA4
#define PANEL_ADDR_A5                   0xA5
#define PANEL_ADDR_A6                   0xA6
#define PANEL_ADDR_A7                   0xA7
#define PANEL_ADDR_A8                   0xA8
#define PANEL_ADDR_A9                   0xA9
#define PANEL_ADDR_AA                   0xAA
#define PANEL_ADDR_AB                   0xAB
#define PANEL_ADDR_AC                   0xAC
#define PANEL_ADDR_AD                   0xAD
#define PANEL_ADDR_AE                   0xAE
#define PANEL_ADDR_AF                   0xAF

#define PANEL_ADDR_B0                   0xB0
#define PANEL_ADDR_B1                   0xB1
#define PANEL_ADDR_B2                   0xB2
#define PANEL_ADDR_B3                   0xB3
#define PANEL_ADDR_B4                   0xB4
#define PANEL_ADDR_B5                   0xB5
#define PANEL_ADDR_B6                   0xB6
#define PANEL_ADDR_B7                   0xB7
#define PANEL_ADDR_B8                   0xB8
#define PANEL_ADDR_B9                   0xB9
#define PANEL_ADDR_BA                   0xBA
#define PANEL_ADDR_BB                   0xBB
#define PANEL_ADDR_BC                   0xBC
#define PANEL_ADDR_BD                   0xBD
#define PANEL_ADDR_BE                   0xBE
#define PANEL_ADDR_BF                   0xBF

#define PANEL_ADDR_C0                   0xC0
#define PANEL_ADDR_C1                   0xC1
#define PANEL_ADDR_C2                   0xC2
#define PANEL_ADDR_C3                   0xC3
#define PANEL_ADDR_C4                   0xC4
#define PANEL_ADDR_C5                   0xC5
#define PANEL_ADDR_C6                   0xC6
#define PANEL_ADDR_C7                   0xC7
#define PANEL_ADDR_C8                   0xC8
#define PANEL_ADDR_C9                   0xC9
#define PANEL_ADDR_CA                   0xCA
#define PANEL_ADDR_CB                   0xCB
#define PANEL_ADDR_CC                   0xCC
#define PANEL_ADDR_CD                   0xCD
#define PANEL_ADDR_CE                   0xCE
#define PANEL_ADDR_CF                   0xCF

#define PANEL_ADDR_D0                   0xD0
#define PANEL_ADDR_D1                   0xD1
#define PANEL_ADDR_D2                   0xD2
#define PANEL_ADDR_D3                   0xD3
#define PANEL_ADDR_D4                   0xD4
#define PANEL_ADDR_D5                   0xD5
#define PANEL_ADDR_D6                   0xD6
#define PANEL_ADDR_D7                   0xD7
#define PANEL_ADDR_D8                   0xD8
#define PANEL_ADDR_D9                   0xD9
#define PANEL_ADDR_DA                   0xDA
#define PANEL_ADDR_DB                   0xDB
#define PANEL_ADDR_DC                   0xDC
#define PANEL_ADDR_DD                   0xDD
#define PANEL_ADDR_DE                   0xDE
#define PANEL_ADDR_DF                   0xDF

#define PANEL_ADDR_E0                   0xE0
#define PANEL_ADDR_E1                   0xE1
#define PANEL_ADDR_E2                   0xE2
#define PANEL_ADDR_E3                   0xE3
#define PANEL_ADDR_E4                   0xE4
#define PANEL_ADDR_E5                   0xE5
#define PANEL_ADDR_E6                   0xE6
#define PANEL_ADDR_E7                   0xE7
#define PANEL_ADDR_E8                   0xE8
#define PANEL_ADDR_E9                   0xE9
#define PANEL_ADDR_EA                   0xEA
#define PANEL_ADDR_EB                   0xEB
#define PANEL_ADDR_EC                   0xEC
#define PANEL_ADDR_ED                   0xED
#define PANEL_ADDR_EE                   0xEE
#define PANEL_ADDR_EF                   0xEF

#define PANEL_ADDR_F0                   0xF0
#define PANEL_ADDR_F1                   0xF1
#define PANEL_ADDR_F2                   0xF2
#define PANEL_ADDR_F3                   0xF3
#define PANEL_ADDR_F4                   0xF4
#define PANEL_ADDR_F5                   0xF5
#define PANEL_ADDR_F6                   0xF6
#define PANEL_ADDR_F7                   0xF7
#define PANEL_ADDR_F8                   0xF8
#define PANEL_ADDR_F9                   0xF9
#define PANEL_ADDR_FA                   0xFA
#define PANEL_ADDR_FB                   0xFB
#define PANEL_ADDR_FC                   0xFC
#define PANEL_ADDR_FD                   0xFD
#define PANEL_ADDR_FE                   0xFE
#define PANEL_ADDR_FF                   0xFF
#define PANEL_TABLE_END                 0xFF

//return value
#define PANEL_CONT_OK                   0x00
#define PANEL_CONT_ERR                  0x01
#define PANEL_CONT_CONTINUE             0x02

//sequence initial value
#define PANEL_CONT_SEQ_NONE             0x00

//panel setting value table
typedef struct {
    uint8_t     map;
    uint8_t     addr;
    uint8_t     value;
    uint16_t    timer;
} setPanelSeqTable, *pstPanelSeqTable;

//panel r/w status
typedef enum {
    PANEL_REG_OK       = 0x00,
    PANEL_REG_ERR      = 0x01,
    PANEL_REG_CONTINUE = 0x02,
} PanelRegStatus;

typedef enum {
    PANEL_LEFT,
    PANEL_RIGHT,
	PANEL_BOTH
} PanelSide;

typedef enum {
    MODE_2D_60HZ =  0,
    MODE_2D_120HZ = 1,
    MODE_3D_60HZ =  2,
    MODE_3D_120HZ = 3,
} PanelMode;

typedef enum {
    PANEL_STATUS_OK = 0,
    PANEL_STATUS_ERROR,
} PanelStatus;

typedef struct {
    PanelSide side;
    uint8_t value;
} PanelControlData;

typedef enum {
    POWER_ON,
    POWER_OFF,
	WRITE_REGISTERS,
    INVERSION,
    LUMINANCE_MODE,
    PRESET_LUMINANCE,
    ORBIT_H,
    ORBIT_V,
    ADJUST_BRIGHTNESS,
	UPDATE_TEMPERATURE,
	POWER_SAVING,
} TaskID;

#define PANEL_WRITE_LENGTH                   (uint16_t)2
#define PANEL_REG_RW_TIMEOUT                 (uint32_t)1000

#define SCAN_MODE                            0x02
#define ORBIT_HORIZONTAL                     0x05
#define ORBIT_VERTICAL                       0x06
#define PRESET_LUMINANCE_VALUE               0x07
#define PANEL_TEMP_DETECTION          		 0x09
#define LUMINANCE_MODE_SETTING               0x10
#define ARBITRARY_LUMINANCE_VALUE_HIGH       0x12
#define ARBITRARY_LUMINANCE_VALUE_LOW        0x13
#define PANEL_TEMP_VOLT1					 0x20
#define PANEL_TEMP_VOLT2					 0x30

#define MIN_BRIGHTNESS 			0x64
#define MAX_BRIGHTNESS 			0x1F4

#define VREF                 	1.8f
#define ADC_RESOLUTION       	((1 << 12) - 1)
#define MAX_BUFFER_SIZE      	10
#define SOME_THRESHOLD       	5
#define ALPHA                	0.05f
#define MIN_TEMPERATURE      	-40.0f
#define MAX_TEMPERATURE      	90.0f
#define TEMPERATURE_THRESHOLD 	10.0f

typedef struct {
    float buffer[MAX_BUFFER_SIZE];
    int head;
    int size;
    int temperatureCount;
    float smoothedTemperature;
    int invalidTemperatureCount;
} CircularBuffer;

// Global variables
extern PanelMode currentPanelMode;
extern float g_temperatureLeftRaw;
extern float g_temperatureRightRaw;
extern float g_temperatureLeftSmoothed;
extern float g_temperatureRightSmoothed;
extern SemaphoreHandle_t xMutex;

// Initialization and power control
void ECX343EN_Init(void);
void panelPowerOn(PanelSide side);
void panelPowerOff(PanelSide side);
void ECX343EN_WriteRegisters(ECX343_DATA *data);

// Panel configuration and control
void ECX343EN_Inversion(PanelSide side, uint8_t value);
void ECX343EN_LuminanceModeSetting(PanelSide side, uint8_t value);
void ECX343EN_PresetLuminanceValue(PanelSide side, uint8_t value);
void ECX343EN_OrbitH(PanelSide side, uint8_t h_value);
void ECX343EN_OrbitV(PanelSide side, uint8_t v_value);
void ECX343EN_TempDetect(PanelSide side, uint8_t value);
void adjustBrightness(void);
void checkPanelState(void);
void switchMode(void);

// Temperature processing
void updatePanelTemperature(void);

// Brightness adjustment based on ambient light
int mapLuxToPanelBrightness(int lux, int *currentIndex);
void smoothlyChangeBrightness(uint16_t targetBrightness);

void executeTaskWithMutex(TaskID taskID, ...);

#endif /* INC_ECX343_H_ */

