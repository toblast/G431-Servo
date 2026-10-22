/*
 * position_sensor.h
 *
 *  Created on: 08.03.2021
 *      Author: kai
 */

#ifndef INC_POSITION_SENSOR_H_
#define INC_POSITION_SENSOR_H_

#include "as5600.h"

int positionSensor_init(void);
int positionSensor_isReady(void);
float positionSensor_getRadiansEstimation(uint16_t time_us) __attribute__((section (".ccmram")));
void positionSensor_update(void);
// Unwrapped motor-shaft angles; the accumulated turn count starts at boot.
float positionSensor_getRadiansMultiturn(void);
float positionSensor_getDegreesMultiturn(void);
// Motor-shaft velocity in degrees per second.
float positionSensor_getVelocityDps(void);

#endif /* INC_POSITION_SENSOR_H_ */
