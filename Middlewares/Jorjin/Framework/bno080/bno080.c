/*
 * Copyright 2015-16 Hillcrest Laboratories, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License and
 * any applicable agreements you may have with Hillcrest Laboratories, Inc.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Sensor Application
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "bno080.h"
#include "sh2.h"
#include "sh2_err.h"
#include "sh2_SensorValue.h"

#include "usb.h"
#include "sensor_hid.h"
//#include "Framework/properties.h"
#include "config.h"

//ckhsu for debug.
int propertySendSensorWithCDC = 0;

// Number of sensor events that can be queued before dropping data.
// A good value would be twice the number of sensors enabled by the app.
#define SENSOR_EVENT_QUEUE_SIZE (6)

#ifndef ARRAY_LEN
#define ARRAY_LEN(a) (sizeof(a)/sizeof(a[0]))
#endif

// Define this to use HMD-appropriate configuration.
// #define CONFIGURE_HMD

// Define this for calibration config appropriate for Robot Vaccuum Cleaners
// #define CONFIGURE_RVC

#define FIX_Q(n, x) ((int32_t)(x * (float)(1 << n)))
static const float scaleDegToRad = 3.14159265358 / 180.0;
//#define M_PI 3.14159265358

// --- Forward declarations -------------------------------------------

static void eventHandler(void * cookie, sh2_AsyncEvent_t *pEvent);
static void sensorHandler(void * cookie, sh2_SensorEvent_t *pEvent);
static void onReset(void);
static void reportProdIds(void);
static void processSensorValue(const sh2_SensorEvent_t * event);

extern void sh2_hal_init();

// --- Private data ---------------------------------------------------

struct SensorContent_t {
    SemaphoreHandle_t wakeSensorTask;
    volatile bool resetPerformed;
    QueueHandle_t eventQueue;
    QueueHandle_t sensorQueue;
};

static struct SensorContent_t ctx;

// --- Public methods -------------------------------------------------

void initSensor() {

  usb_waitUntilInited();
  sh2_hal_init();

  ctx.sensorQueue = xQueueCreate(SENSOR_EVENT_QUEUE_SIZE,
          sizeof(struct SensorMessage));
  if (ctx.sensorQueue == NULL) {
    //printf("Error creating sensor queue.\n");
  }

  ctx.wakeSensorTask = xSemaphoreCreateBinary();
  ctx.eventQueue = xQueueCreate(SENSOR_EVENT_QUEUE_SIZE,
          sizeof(sh2_SensorEvent_t));
  if (ctx.eventQueue == NULL) {
      //printf("Error creating event queue.\n");
  }

  ctx.resetPerformed = false;

  // init SH2 layer
  sh2_initialize(eventHandler, NULL);

  // Register event listener
  sh2_setSensorCallback(sensorHandler, NULL);

  // wait for reset notification, or just go ahead after 100ms
  int waited = 0;
#define WAIT_TIME_MS	800
  //while (!ctx.resetPerformed && (waited < 200)) {
  while (!ctx.resetPerformed && (waited < WAIT_TIME_MS)) {
    vTaskDelay(1);
    waited++;
  }

  // Read and display BNO080 product ids
  //ckhsu
  //vTaskDelay(2000);
  reportProdIds();

  // Perform on-reset setup of BNO080
  onReset();
}

void configSensor(sh2_SensorId_t sensorId, int interval)
{
    static sh2_SensorConfig_t config;
    int status;

    config.changeSensitivityEnabled = false;
    config.wakeupEnabled = false;
    config.changeSensitivityRelative = false;
    config.alwaysOnEnabled = false;
    config.changeSensitivity = 0;
    config.reportInterval_us = interval;
    config.batchInterval_us = 0;

    status = sh2_setSensorConfig(sensorId, &config);

    if (status != 0) {
        //printf("Error while enabling sensor %d\n", sensorId);
    }
}

void sensorLoop()
{
    sh2_SensorEvent_t event;

    // Process sensors forever
    while (1) {
        // Wait until something happens
        xSemaphoreTake(ctx.wakeSensorTask, portMAX_DELAY);

        // Dequeue sensor events
        while (xQueueReceive(ctx.eventQueue, &event, 0) == pdPASS) {
            processSensorValue(&event);
        }

        if (ctx.resetPerformed) {
            onReset();
        }
    }
}

// --- Private methods ----------------------------------------------

static void eventHandler(void * cookie, sh2_AsyncEvent_t *pEvent)
{
    if (pEvent->eventId == SH2_RESET) {
        //printf("SH2 Reset.\n");

        // Signal main loop to handle this.
        ctx.resetPerformed = true;
        xSemaphoreGive(ctx.wakeSensorTask);
    }
}

static void sensorHandler(void * cookie, sh2_SensorEvent_t *pEvent)
{
    xQueueSend(ctx.eventQueue, pEvent, 0);
    xSemaphoreGive(ctx.wakeSensorTask);
}

#define GIRV_REF_6AG  (0x0207)  // 6 axis Game Rotation Vector
#define GIRV_REF_9AGM (0x0204)  // 9 axis Absolute Rotation Vector
#define GIRV_SYNC_INTERVAL (10000)                     // sync interval: 10000 uS (100Hz)
#define GIRV_MAX_ERR FIX_Q(29, (30.0 * scaleDegToRad)) // max error: 30 degrees
#define GIRV_ALPHA FIX_Q(20, 0.303072543909142)        // pred param alpha
#define GIRV_BETA  FIX_Q(20, 0.113295896384921)        // pred param beta
#define GIRV_GAMMA FIX_Q(20, 0.002776219713054)        // pred param gamma
#define ORI_SQRT_2_DIV_2         FIX_Q(30, 0.707106781186548)
#define ORI_MINUS_SQRT_2_DIV_2   FIX_Q(30, -0.707106781186548)
#define ORI_ONE                  FIX_Q(30, 1.0)
#define ORI_MINUS_ONE            FIX_Q(30, -1.0)
#define ORI_ZERO                 FIX_Q(30, 0.0)

#ifdef CONFIGURE_HMD
    // Enable GIRV prediction for 28ms with 100Hz sync
    #define GIRV_PRED_AMT FIX_Q(10, 0.028)             // prediction amt: 28ms
#else
    // Disable GIRV prediction
    #define GIRV_PRED_AMT FIX_Q(10, 0.0)               // prediction amt: 0
#endif


static void configure(void)
{
    int status = SH2_OK;
    uint32_t config[7];
    uint32_t ori[4];

    // Note: The call to sh2_setFrs below updates a non-volatile FRS record
    // so it will remain in effect even after the sensor hub reboots.  It's not strictly
    // necessary to repeat this step every time the system starts up as we are doing
    // in this example code.

    // Configure prediction parameters for Gyro-Integrated Rotation Vector.
    // See section 4.3.24 of the SH-2 Reference Manual for a full explanation.
    // ...
    config[0] = GIRV_REF_6AG;           // Reference Data Type
    config[1] = (uint32_t)GIRV_SYNC_INTERVAL; // Synchronization Interval
    config[2] = (uint32_t)GIRV_MAX_ERR;  // Maximum error
    config[3] = (uint32_t)GIRV_PRED_AMT; // Prediction Amount
    config[4] = (uint32_t)GIRV_ALPHA;    // Alpha
    config[5] = (uint32_t)GIRV_BETA;     // Beta
    config[6] = (uint32_t)GIRV_GAMMA;    // Gamma
    status = sh2_setFrs(GYRO_INTEGRATED_RV_CONFIG, config, ARRAY_LEN(config));

    if (status != SH2_OK) {
        //printf("Error: %d, from sh2_setFrs() in configure().\n", status);
    }

    ori[0] = (uint32_t)ORI_ZERO;
    ori[1] = (uint32_t)ORI_ZERO;
    ori[2] = (uint32_t)ORI_ZERO;
    ori[3] = (uint32_t)ORI_ONE;
    status = sh2_setFrs(SYSTEM_ORIENTATION, ori, ARRAY_LEN(config));

    if (status != SH2_OK) {
        //printf("Error: %d, from sh2_setFrs() in configure().\n", status);
    }

    // The sh2_setCalConfig does not update non-volatile storage.  This
    // only remains in effect until the sensor hub reboots.

#ifdef CONFIGURE_RVC
    // Enable planar calibration mode, which is designed for RVC applications
    status = sh2_setCalConfig(SH2_CAL_PLANAR);
#else
    // Enable dynamic calibration for A, G and M sensors
    status = sh2_setCalConfig(SH2_CAL_ACCEL | SH2_CAL_GYRO | SH2_CAL_MAG);
#endif
    if (status != SH2_OK) {
        //printf("Error: %d, from sh2_setCalConfig() in configure().\n", status);
    }
}

static void onReset(void)
{
    // Configure calibration config as we want it
    configure();

#if SENSOR_DEFAULT_ON
    configSensor(SH2_ACCELEROMETER, 30000); //20000
    configSensor(SH2_GYROSCOPE_CALIBRATED, 30000);
    configSensor(SH2_MAGNETIC_FIELD_CALIBRATED, 20000);
    configSensor(SH2_ROTATION_VECTOR, 20000);
    configSensor(SH2_GRAVITY, 30000);

#else
    //TODO: restore Sensor Report settings.
#endif

    // Toggle reset flag back to false
    ctx.resetPerformed = false;
}

static void reportProdIds(void)
{
    int status;
    sh2_ProductIds_t prodIds;

    memset(&prodIds, 0, sizeof(prodIds));
    status = sh2_getProdIds(&prodIds);

    if (status < 0) {
        //printf("Error from sh2_getProdIds.\n");
        return;
    }

    // Report the results
    for (int n = 0; n < prodIds.numEntries; n++) {
        printf("Part %ld : Version %d.%d.%d Build %ld\r\n",
               prodIds.entry[n].swPartNumber,
               prodIds.entry[n].swVersionMajor, prodIds.entry[n].swVersionMinor,
               prodIds.entry[n].swVersionPatch, prodIds.entry[n].swBuildNumber);
    }
}

static void toEulerAngles(
        struct EulerAngles *angles,
        sh2_RotationVectorWAcc_t *q)
{
    float sinr_cosp = 2 * (q->real * q->i + q->j * q->k);
    float cosr_cosp = 1 - 2 * (q->i * q->i + q->j * q->j);

    angles->roll = atan2(sinr_cosp, cosr_cosp);

    float sinp = 2 * (q->real * q->j - q->k * q->i);

    if (abs(sinp) >= 1) {
        if (sinp < 0) {
            angles->pitch = -M_PI/2.0;
        }
        else {
            angles->pitch = M_PI/2.0;
        }
    }
    else {
        angles->pitch = asin(sinp);
    }

    float siny_cosp = 2 * (q->real * q->k + q->i * q->j);
    float cosy_cosp = 1 - 2 * (q->j * q->j + q->k * q->k);

    angles->yaw = atan2(siny_cosp, cosy_cosp);
}


static void processSensorValue(const sh2_SensorEvent_t * event)
{
    int rc;
    sh2_SensorValue_t value;
    JQueueMessage_t msg;
    ACCEL3_INPUT_REPORT accel3;
    MAG3_INPUT_REPORT tstMag3;
    QUAT_INPUT_REPORT tstQuat;
    struct EulerAngles angles;


    rc = sh2_decodeSensorEvent(&value, event);
    if (rc != SH2_OK) {
        //printf("Error decoding sensor event: %d\n", rc);
        return;
    }

    switch (value.sensorId) {
    case SH2_ACCELEROMETER:

        if (propertySendSensorWithCDC) {
            usbDebug("Accel3 x: %0.6f, y: %0.6f, z: %0.6f\r\n",
                    value.un.accelerometer.x,
                    value.un.accelerometer.y,
                    value.un.accelerometer.z);
        }
        else {
            accel3.ucReportId = REPORT_ID_ACCEL3_INPUT;
            accel3.ucEventType = SENSOR_EVENT_DATA_UPDATED;
            accel3.ucSensorState = SENSOR_STATE_READY;
            accel3.sAccelXValue = (int32_t)(value.un.accelerometer.x*10204082);
            accel3.sAccelYValue = (int32_t)(value.un.accelerometer.y*10204082);
            accel3.sAccelZValue = (int32_t)(value.un.accelerometer.z*10204082);
            msg.type = USB_HID_IMU_INPUT_REPORT; //USB_HID_INPUT_REPORT
            msg.data.inputReport.len = sizeof(ACCEL3_INPUT_REPORT);
            memcpy(msg.data.inputReport.report,
                   (void *)&accel3,
                   msg.data.inputReport.len);
            usbSendMessage(&msg);
        }
        break;
    case SH2_GYROSCOPE_CALIBRATED:
        if (propertySendSensorWithCDC) {
            usbDebug("Gyro3 x: %0.6f, y: %0.6f, z: %0.6f\r\n",
                    value.un.gyroscope.x,
                    value.un.gyroscope.y,
                    value.un.gyroscope.z);
        }
        else {
            accel3.ucReportId = REPORT_ID_GYRO3_INPUT;
            accel3.ucEventType = SENSOR_EVENT_DATA_UPDATED;
            accel3.ucSensorState = SENSOR_STATE_READY;
            accel3.sAccelXValue = (int32_t)(value.un.gyroscope.x*1000000);
            accel3.sAccelYValue = (int32_t)(value.un.gyroscope.y*1000000);
            accel3.sAccelZValue = (int32_t)(value.un.gyroscope.z*1000000);
            msg.type = USB_HID_IMU_INPUT_REPORT; //USB_HID_INPUT_REPORT
            msg.data.inputReport.len = sizeof(ACCEL3_INPUT_REPORT);
            memcpy(msg.data.inputReport.report,
                  (void *)&accel3,
                  msg.data.inputReport.len);
            usbSendMessage(&msg);
        }
        break;
    case SH2_MAGNETIC_FIELD_CALIBRATED:
        if (propertySendSensorWithCDC) {
            usbDebug("Mag x: %0.6f, y: %0.6f, z: %0.6f\r\n",
                    value.un.magneticField.x,
                    value.un.magneticField.y,
                    value.un.magneticField.z);
        }
        else {
            tstMag3.ucReportId = REPORT_ID_MAG3_INPUT;
            tstMag3.ucEventType = SENSOR_EVENT_DATA_UPDATED;
            tstMag3.ucSensorState = SENSOR_STATE_READY;
            tstMag3.sMagXValue = (int32_t)(value.un.magneticField.x*10000);
            tstMag3.sMagYValue = (int32_t)(value.un.magneticField.y*10000);
            tstMag3.sMagZValue = (int32_t)(value.un.magneticField.z*10000);
            msg.type = USB_HID_IMU_INPUT_REPORT; //USB_HID_INPUT_REPORT
            msg.data.inputReport.len = sizeof(MAG3_INPUT_REPORT);
            memcpy(msg.data.inputReport.report,
                  (void *)&tstMag3,
                  msg.data.inputReport.len);
            usbSendMessage(&msg);
        }
        break;
    case SH2_ROTATION_VECTOR:
        if (propertySendSensorWithCDC) {
            toEulerAngles(&angles, &value.un.rotationVector);
            usbDebug("roll: %0.6f, pitch: %0.6f, yaw: %0.6f\r\n",
                                angles.roll,
                                angles.pitch,
                                angles.yaw);
            /*usbDebug("x: %0.6f, y: %0.6f, z: %0.6f, w: %0.6f\r\n",
                    value.un.rotationVector.j,
                    value.un.rotationVector.i,
                    value.un.rotationVector.k,
                    value.un.rotationVector.real);*/
        }
        else {
            tstQuat.ucReportId = REPORT_ID_QUAT_INPUT;
            tstQuat.ucSensorState = SENSOR_STATE_READY;
            tstQuat.ucEventType = SENSOR_EVENT_DATA_UPDATED;
            tstQuat.sQuatIValue = (int32_t)(value.un.rotationVector.i*100000000);
            tstQuat.sQuatJValue = (int32_t)(value.un.rotationVector.j*100000000);
            tstQuat.sQuatKValue = (int32_t)(value.un.rotationVector.k*100000000);
            tstQuat.sRawQuatReal = (int32_t)(value.un.rotationVector.real*100000000);
            tstQuat.sQuatAccuracy = (uint8_t)value.un.rotationVector.accuracy;
            msg.type = USB_HID_IMU_INPUT_REPORT;
            msg.data.inputReport.len = sizeof(QUAT_INPUT_REPORT);
            memcpy(msg.data.inputReport.report, (void *)&tstQuat,
                               msg.data.inputReport.len);
//            usbDebug("x: %0.6f, y: %0.6f, z: %0.6f, w: %0.6f\r\n",
//                    value.un.rotationVector.j,
//                    value.un.rotationVector.i,
//                    value.un.rotationVector.k,
//                    value.un.rotationVector.real);
            usbSendMessage(&msg);

        }
        break;
    case SH2_GRAVITY:
        accel3.ucReportId = REPORT_ID_GRAV3_INPUT;
        accel3.ucEventType = SENSOR_EVENT_DATA_UPDATED;
        accel3.ucSensorState = SENSOR_STATE_READY;
        accel3.sAccelXValue = (int32_t)(value.un.gravity.x*10204082);
        accel3.sAccelYValue = (int32_t)(value.un.gravity.y*10204082);
        accel3.sAccelZValue = (int32_t)(value.un.gravity.z*10204082);
        msg.type = USB_HID_IMU_INPUT_REPORT; //USB_HID_INPUT_REPORT
        msg.data.inputReport.len = sizeof(ACCEL3_INPUT_REPORT);
        memcpy(msg.data.inputReport.report,
               (void *)&accel3,
               msg.data.inputReport.len);
        usbSendMessage(&msg);
        break;
    }
}
