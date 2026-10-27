/*
 * position_sensor.h
 *
 *  Created on: 08.03.2021
 *      Author: kai
 */

#ifndef INC_POSITION_SENSOR_H_
#define INC_POSITION_SENSOR_H_

#include "as5600.h"
#include <stdbool.h>

// Startup-only, before ADC/CAN start. True means the first valid sample was
// acquired; failure latches the encoder-read fault and leaves readiness false.
bool positionSensor_init(void);
// Successful initialization only; check live fault flags for acquisition health.
// Readable from foreground or IRQ. Readiness is not cleared by a later bad read.
bool positionSensor_isReady(void);
// FOC/ADC context, after initialization. time_us is the wrapping 1 MHz timer.
// Returns radians with the existing latency prediction. A sample older than
// 1200 us latches the stale fault and returns zero; callers must check faults.
float positionSensor_getRadiansEstimation(uint16_t time_us) __attribute__((section (".ccmram")));
// Foreground-only, nonreentrant, IRQs enabled for synchronous I2C. A failed read
// latches the encoder fault and preserves the complete last valid sample.
void positionSensor_update(void);
// Unwrapped motor-shaft angles; the accumulated turn count starts at boot.
// Getters return the last published sample, or zero before initialization.
// Foreground/IRQ readers may use individual getters; mask IRQs if a coherent
// group is needed across separate calls. They do not acknowledge sensor faults.
float positionSensor_getRadiansMultiturn(void);
float positionSensor_getDegreesMultiturn(void);
// Motor-shaft velocity in degrees per second.
float positionSensor_getVelocityDps(void);

#endif /* INC_POSITION_SENSOR_H_ */
