#include <stdio.h>
#include <string.h>

#include "sensor_hid.h"

ACCEL3_FEATURE_REPORT accel3FeatureReport = {
		.ucReportId = REPORT_ID_ACCEL3_FEATURE,
		.ucConnectionType = SENSOR_CONNECTION_TYPE_PC_ATTACHED,
		.ucReportingState = SENSOR_REPORTING_STATE_ALL_EVENTS,
		.ucPowerState = SENSOR_POWER_STATE_D4_POWER_OFF,
		.ucSensorState = SENSOR_STATE_READY,
		.ulReportInterval = 10,
		.usAccelChangeSensitivity = 1,
		.sAccelMaximum = 400,
		.sAccelMinimum = -400
};
