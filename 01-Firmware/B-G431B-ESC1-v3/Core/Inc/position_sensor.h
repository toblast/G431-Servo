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
float positionSensor_getRadians(void);
float positionSensor_getRadiansMultiturn(void);
float positionSensor_getDegree(void);
float positionSensor_getDegreeMultiturn(void);
float positionSensor_getVelocityDegree(void);

#endif /* INC_POSITION_SENSOR_H_ */
