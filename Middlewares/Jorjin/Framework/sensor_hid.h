#ifndef _JORJIN_FRAMEWORK_SENSOR_HID_H_
#define _JORJIN_FRAMEWORK_SENSOR_HID_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/*
 * Sensor Connection Type
 */
#define SENSOR_CONNECTION_TYPE_PC_INTEGRATED  0x00  //0x01
#define SENSOR_CONNECTION_TYPE_PC_ATTACHED    0x01
#define SENSOR_CONNECTION_TYPE_PC_EXTERNAL    0x02

/*
 * Sensor Reporting States
 */
#define SENSOR_REPORTING_STATE_NO_EVENTS              0x00 //0x01
#define SENSOR_REPORTING_STATE_ALL_EVENTS             0x01
#define SENSOR_REPORTING_STATE_THRESHOLD_EVENTS       0x02
#define SENSOR_REPORTING_STATE_NO_EVENTS_WAKE         0x03
#define SENSOR_REPORTING_STATE_ALL_EVENTS_WAKE        0x04
#define SENSOR_REPORTING_STATE_THRESHOLD_EVENTS_WAKE  0x05

/*
 * Sensor Power States
 */
#define SENSOR_POWER_STATE_UNDEFINED            0x00    //0x01
#define SENSOR_POWER_STATE_D0_FULL_POWER        0x01
#define SENSOR_POWER_STATE_D1_LOW_POWER         0x02
#define SENSOR_POWER_STATE_D2_STANDBY_WITH_WAKE 0x03
#define SENSOR_POWER_STATE_D3_SLEEP_WITH_WAKE   0x04
#define SENSOR_POWER_STATE_D4_POWER_OFF         0x05

/*
 * Sensor States
 */
#define SENSOR_STATE_UNKNOWN        0x00    //0x01
#define SENSOR_STATE_READY          0x01
#define SENSOR_STATE_NOT_AVAILABLE  0x02
#define SENSOR_STATE_NO_DATA        0x03
#define SENSOR_STATE_INITIALIZING   0x04
#define SENSOR_STATE_ACCESS_DENIED  0x05
#define SENSOR_STATE_ERROR          0x06

/*
 * Sensor Events
 */
#define SENSOR_EVENT_UNKNOWN        0x00    //0x01
#define SENSOR_EVENT_STATE_CHANGED  0x01
#define SENSOR_EVENT_PROPERTY_CHANGED 0x02
#define SENSOR_EVENT_DATA_UPDATED   0x03
#define SENSOR_EVENT_POLL_RESPONSE  0x04
#define SENSOR_EVENT_CHANGE_SENSITIVITY 0x05

/* Legacy IMU HID report structs are kept for build compatibility. */
typedef struct _ACCEL3_FEATURE_REPORT
{
    uint8_t   ucReportId;
    uint8_t   ucConnectionType;
    uint8_t   ucReportingState;
    uint8_t   ucPowerState;
    uint8_t   ucSensorState;
    uint32_t  ulReportInterval;
    uint16_t  usAccelChangeSensitivity;
    int32_t   sAccelMaximum;
    int32_t   sAccelMinimum;
} __packed ACCEL3_FEATURE_REPORT;

typedef struct _ACCEL3_INPUT_REPORT
{
    uint8_t   ucReportId;
    uint8_t   ucSensorState;
    uint8_t   ucEventType;
    int32_t   sAccelXValue;
    int32_t   sAccelYValue;
    int32_t   sAccelZValue;
} __packed ACCEL3_INPUT_REPORT;

typedef struct _GRAV3_INPUT_REPORT
{
    uint8_t   ucReportId;
    uint8_t   ucSensorState;
    uint8_t   ucEventType;
    int32_t   sGRAVXValue;
    int32_t   sGRAVYValue;
    int32_t   sGRAVZValue;
} __packed GRAV3_INPUT_REPORT;

typedef struct _MAG3_INPUT_REPORT
{
    uint8_t   ucReportId;
    uint8_t   ucSensorState;
    uint8_t   ucEventType;
    int32_t   sMagXValue;
    int32_t   sMagYValue;
    int32_t   sMagZValue;
    uint8_t   sMagAccuracy;
} __packed MAG3_INPUT_REPORT;

typedef struct _QUAT_INPUT_REPORT
{
    uint8_t   ucReportId;
    uint8_t   ucSensorState;
    uint8_t   ucEventType;
    int32_t   sQuatIValue;
    int32_t   sQuatJValue;
    int32_t   sQuatKValue;
    int32_t   sRawQuatReal;
    uint8_t   sQuatAccuracy;
} __packed QUAT_INPUT_REPORT;

typedef struct _ALS_FEATURE_REPORT
{
    //common properties
    uint8_t   ucReportId;
    uint8_t   ucConnectionType;
    uint8_t   ucReportingState;
    uint8_t   ucPowerState;
    uint8_t   ucSensorState;
    uint32_t  ulReportInterval;
    //properties specific to this sensor
    uint16_t  usChangeSensitivity;
    int16_t   sMaximum;
    int16_t   sMinimum;
} __packed ALS_FEATURE_REPORT;

extern ALS_FEATURE_REPORT alsFeatureReport;

#define REPORT_ID_ACCEL3_FEATURE 1
#define REPORT_ID_ACCEL3_INPUT 2
#define REPORT_ID_GYRO3_FEATURE 3
#define REPORT_ID_GYRO3_INPUT 4
#define REPORT_ID_MAG3_FEATURE 5
#define REPORT_ID_MAG3_INPUT 6
#define REPORT_ID_QUAT_FEATURE 7
#define REPORT_ID_QUAT_INPUT 8
#define REPORT_ID_ALS_FEATURE 9
#define REPORT_ID_ALS_INPUT   10
#define REPORT_ID_GRAV3_FEATURE 13
#define REPORT_ID_GRAV3_INPUT 14

#endif // _JORJIN_FRAMEWORK_SENSOR_HID_H_
