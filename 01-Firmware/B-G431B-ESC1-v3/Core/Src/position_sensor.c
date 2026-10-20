/*
 * position_sensor.c
 *
 *  Created on: 08.03.2021
 *      Author: kai
 */
#include <string.h>
#include "position_sensor.h"
#include "as5600.h"
#include "math_tool.h"
#include "control_table.h"

extern I2C_HandleTypeDef hi2c1;

// µs TIMER
extern TIM_HandleTypeDef htim6;

typedef struct {

	int initialized;

	// AS5600 handle
	AS5600_TypeDef *as5600Handle;

	// common sensor values
	uint16_t lastUpdate;
    uint16_t last_angle_data;
    float velocity_deg;
	float velocity_rad;

	float angle_rad;
	float angle_deg;

    float angle_prev_rad;
    float angle_prev_deg;

    float full_rotation_offset;

} positionSensor_t;

static positionSensor_t sensor_storage;
static positionSensor_t *const sensor = &sensor_storage;
static AS5600_TypeDef as5600_storage;

int positionSensor_isReady(void)
{
    return sensor->initialized;
}

int positionSensor_init(void)
{
    uint16_t angle_data;
    memset(sensor, 0, sizeof(*sensor));
    memset(&as5600_storage, 0, sizeof(as5600_storage));
    sensor->as5600Handle = &as5600_storage;
    sensor->as5600Handle->i2cHandle = &hi2c1;
    if (AS5600_Init(sensor->as5600Handle) != HAL_OK ||
        AS5600_GetAngle(sensor->as5600Handle, &angle_data) != HAL_OK)
    {
        hardware_fault_set(1U << HW_ERROR_BIT_POSITION_SENSOR_STATUS_ERROR);
        return 0;
    }
    sensor->last_angle_data = angle_data;
    sensor->angle_rad = (float)angle_data * (M_2PI / 4096.0f);
    sensor->angle_deg = RADIANS_TO_DEGREES(sensor->angle_rad);
    sensor->angle_prev_rad = sensor->angle_rad;
    sensor->angle_prev_deg = sensor->angle_deg;
    sensor->lastUpdate = __HAL_TIM_GET_COUNTER(&htim6);
    sensor->initialized = 1;
    return 1;
}

float positionSensor_getRadiansEstimation(uint16_t time_us)
{
    if (!sensor->initialized) return 0.0f;
    uint16_t const delta_t_us = (uint16_t)(time_us - sensor->lastUpdate);
    if (delta_t_us > 1200U)
    {
        hardware_fault_set(1U << HW_ERROR_BIT_POSITION_SENSOR_NOT_RESPONDING);
        return 0.0f;
    }
    // A fresh sample cannot clear an active timeout until main disarms.
    if (!regs[REG_TORQUE_ENABLE])
        hardware_fault_clear(1U << HW_ERROR_BIT_POSITION_SENSOR_NOT_RESPONDING);
    // Compensate the existing AS5600 slow-filter latency (200 us).
    return sensor->angle_rad + sensor->velocity_rad * (float)(delta_t_us + 200U) / 1000000.0f;
}

void positionSensor_update(void){
    if (!sensor->initialized) return;
    float d_angle;
    uint16_t angle_data;
    float const cpr = 4096.0f;

    // raw data from the sensor
    HAL_StatusTypeDef status = AS5600_CheckMagnet(sensor->as5600Handle);
    if (status == HAL_OK)
        status = AS5600_GetAngle(sensor->as5600Handle, &angle_data);

    if(status!=HAL_OK)
    {    // set encoder error
        hardware_fault_set((uint8_t)(1UL << HW_ERROR_BIT_POSITION_SENSOR_STATUS_ERROR));
        regs[REG_PROTOCOL_CRC_FAIL]++;
        return; // Preserve the complete last valid sample.
    }

    // Timestamp completion consistently with the initial sample.
    uint16_t const now_us = __HAL_TIM_GET_COUNTER(&htim6);
    uint16_t const delta_time_us = (uint16_t)(now_us - sensor->lastUpdate);
    if (delta_time_us == 0) return;
    positionSensor_t next = *sensor;

    // tracking the number of rotations
    // in order to expand angle range form [0,2PI]
    // to basically infinity
    d_angle = (float)(angle_data - next.last_angle_data);
    // if overflow happened track it as full rotation
    if(fabsf(d_angle) > (0.8*cpr) ) next.full_rotation_offset += d_angle > 0 ? -M_2PI : M_2PI;
    // save the current angle value for the next steps
    // in order to know if overflow happened
    next.last_angle_data = angle_data;

    // return the full angle
    // (number of full rotations)*2PI + current sensor angle
    next.angle_rad = next.full_rotation_offset + ( (float)angle_data / (float)cpr) * M_2PI;
    next.angle_deg = RADIANS_TO_DEGREES(next.angle_rad);

    // velocity calculation
    next.velocity_rad = ((next.angle_rad - next.angle_prev_rad)/delta_time_us* 1000000.0f);
    next.velocity_deg = ((next.angle_deg - next.angle_prev_deg)/delta_time_us* 1000000.0f);

    // last angle
    next.angle_prev_rad = next.angle_rad;
    next.angle_prev_deg = next.angle_deg;

    next.lastUpdate = now_us;

    // No blocking work runs with IRQs masked. FOC sees one complete sample.
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    *sensor = next;
    __DMB();
    __set_PRIMASK(primask);
}

float positionSensor_getRadians(void)
{
    return sensor->initialized ? sensor->angle_rad : 0.0f;
}

float positionSensor_getRadiansMultiturn(void)
{
    return sensor->initialized ? sensor->angle_rad : 0.0f;
}

float positionSensor_getDegree(void)
{
    return sensor->initialized ? sensor->angle_deg : 0.0f;
}

float positionSensor_getDegreeMultiturn(void)
{
    return sensor->initialized ? sensor->angle_deg : 0.0f;
}

float positionSensor_getVelocityDegree(void)
{
    return sensor->initialized ? sensor->velocity_deg : 0.0f;
}
