#include <stdio.h>
#include <string.h>

#include "sensor_hid.h"

ALS_FEATURE_REPORT alsFeatureReport = {
		.ucReportId = REPORT_ID_ALS_FEATURE,
		.ucConnectionType = SENSOR_CONNECTION_TYPE_PC_ATTACHED,
		.ucReportingState = SENSOR_REPORTING_STATE_ALL_EVENTS,
		.ucPowerState = SENSOR_POWER_STATE_D4_POWER_OFF,
		.ucSensorState = SENSOR_STATE_READY,
		.ulReportInterval = 10,
		.usChangeSensitivity = 1,
		.sMaximum = 255,
		.sMinimum = 0
};
