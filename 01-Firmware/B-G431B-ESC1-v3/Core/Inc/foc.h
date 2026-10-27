/*
 * foc.h
 *
 *  Created on: 15 janv. 2021
 *      Author: Patrick, Kai
 */

#ifndef INC_FOC_H_
#define INC_FOC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32g4xx_hal.h"
#include <stdbool.h>

#define FOC_TIMEOUT_MS 10U

// Startup-only, after config/encoder/timebase setup, before CAN acceptance.
void API_FOC_Init(void);

bool API_FOC_Is_Calibrating(void);
// Foreground watchdog: false latches the FOC timeout and immediately coasts.
// Both queries preserve caller IRQ state; the active flag includes save/cleanup.
bool API_FOC_Check_Watchdog(void);
// Foreground calibration primitives. Begin/SetFlux guard against ARM, faults
// and cancellation atomically. Idle coasts; End releases the active guard only
// after the job has completed rollback/result publication. True means entry/
// excitation accepted; false changes no excitation. Job state stays in
// Calibration; only that foreground job calls these primitives. No wait loops.
bool API_FOC_Calibration_Begin(void);
bool API_FOC_Calibration_SetFlux(float angle_rad, float voltage_V);
bool API_FOC_Calibration_Cancelled(void);
void API_FOC_Calibration_Idle(void);
void API_FOC_Calibration_End(void);

// Drive supervisor owns normal permission. Disable is also safe from IRQ and
// immediately coasts/cancels calibration; Enable alone is not an ARM request.
void API_FOC_Torque_Enable(void);
void API_FOC_Torque_Disable(void);

void API_FOC_Set_Torque_Flux_Currents_mA(float Iq_mA, float Id_mA);	// 	Setpoint Iq & Id

// Wrapping timestamp of the last complete FOC iteration, in HAL ms / timer us.
uint32_t API_FOC_Get_Timestamp_ms(void);
uint16_t API_FOC_Get_Timestamp_us(void);
// Last FOC observations: mA (Iq/Id), V and degrees Celsius, respectively.
// Getters do not acquire sensors. Mask ADC IRQs to capture a coherent group.
float API_FOC_Get_Present_Torque_Current(void);
float API_FOC_Get_Present_Flux_Current(void);
float API_FOC_Get_Present_Voltage(void);
float API_FOC_Get_Present_Temp(void);
float API_FOC_Get_Processing_Time(void); // Averaged execution time, microseconds.
float API_FOC_Get_Processing_Frequency(void); // Completed iterations/uptime, Hz.

// Foreground periodic analog/protection service, scheduled by App (10 ms).
void API_FOC_Service_Update(void)  __attribute__((section (".ccmram")));


#ifdef __cplusplus
}
#endif


#endif /* INC_FOC_H_ */
