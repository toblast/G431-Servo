#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "foc.h"
#include "control_table.h"
#include "math_tool.h"
#include "binary_tool.h"

uint8_t regs[REG_MAX];
TIM_HandleTypeDef htim6;
uint32_t test_primask;
void (*test_irq_restored)(void);
static uint64_t clock_us, last_sample, last_service, cancel_at;
static uint64_t max_sample_gap, max_service_gap;
static unsigned failures, samples, services, stores, fault_after, cancellation_seen;
static float rotor_angle, rotor_direction;
static bool ready, stuck;
static HAL_StatusTypeDef store_status = HAL_OK;
#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "Calibration check failed at line %d: %s\n", __LINE__, #condition); ++failures; \
} } while (0)

/* GLOBALS */
static void LL_FOC_coast(void) { }
int positionSensor_isReady(void) { return ready; }
bool control_configuration_valid(void) { return true; }
float positionSensor_getRadians(void) { return rotor_angle; }
uint32_t HAL_GetTick(void)
{
    clock_us += 50;
    htim6.counter = (uint16_t)clock_us;
    if (calibration_active && !cancellation_seen && clock_us >= cancel_at) {
        ++cancellation_seen;
        API_FOC_Torque_Disable();
        CHECK(API_FOC_Is_Calibrating());
    }
    return (uint32_t)(clock_us / 1000);
}
void positionSensor_update(void)
{
    CHECK(test_primask == 0);
    if (clock_us - last_sample > max_sample_gap) max_sample_gap = clock_us - last_sample;
    clock_us += 80;
    htim6.counter = (uint16_t)clock_us;
    last_sample = clock_us;
    ++samples;
    if (!stuck && foc_state == FOC_STATE_FLUX_CONTROL && setpoint_flux_voltage_V > 0)
        rotor_angle = 1.0f + rotor_direction * setpoint_electrical_angle_rad / 7.0f;
    if (fault_after && samples >= fault_after)
        hardware_fault_set(1U << HW_ERROR_BIT_POSITION_SENSOR_STATUS_ERROR);
}
void API_FOC_Service_Update(void)
{
    CHECK(test_primask == 0);
    if (clock_us - last_service > max_service_gap) max_service_gap = clock_us - last_service;
    last_service = clock_us;
    ++services;
}
HAL_StatusTypeDef store_eeprom_regs(void)
{
    CHECK(API_FOC_Is_Calibrating());
    CHECK(foc_state == FOC_STATE_IDLE && setpoint_flux_voltage_V == 0);
    CHECK(!calibration_cancelled && hardware_fault_status() == 0);
    ++stores;
    if (store_status != HAL_OK) hardware_fault_set(1U << HW_ERROR_BIT_INTERNAL);
    return store_status;
}
/* FUNCTIONS */

static void reset_fixture(void)
{
    memset(regs, 0, sizeof regs);
    regs[REG_MOTOR_POLE_PAIRS] = 7;
    regs[REG_INV_PHASE_MOTOR] = 1;
    regs[REG_MOTOR_SYNCHRO_L] = 0x23;
    regs[REG_MOTOR_SYNCHRO_H] = 1;
    clock_us = last_sample = last_service = 0;
    max_sample_gap = max_service_gap = 0;
    cancel_at = UINT64_MAX;
    samples = services = stores = fault_after = cancellation_seen = 0;
    rotor_angle = rotor_direction = 1;
    ready = true;
    stuck = false;
    calibration_active = calibration_cancelled = false;
    test_primask = 0;
    htim6.counter = 0;
}
static void check_failed(int expected)
{
    CHECK(API_FOC_Calibrate() == expected);
    CHECK(!API_FOC_Is_Calibrating() && foc_state == FOC_STATE_IDLE);
    CHECK(stores == 0 && setpoint_flux_voltage_V == 0);
    CHECK(regs[REG_INV_PHASE_MOTOR] == 1 && regs[REG_MOTOR_SYNCHRO_L] == 0x23 && regs[REG_MOTOR_SYNCHRO_H] == 1);
}
int main(void)
{
    reset_fixture();
    CHECK(API_FOC_Calibrate() == 0);
    CHECK(stores == 1 && samples > 1000 && services > 200);
    CHECK(max_sample_gap < 1200 && max_service_gap < 12000);
    CHECK(!API_FOC_Is_Calibrating() && foc_state == FOC_STATE_IDLE);
    CHECK(regs[REG_INV_PHASE_MOTOR] == 0 && setpoint_flux_voltage_V == 0);

    reset_fixture(); rotor_direction = -1;
    CHECK(API_FOC_Calibrate() == 0 && stores == 1 && regs[REG_INV_PHASE_MOTOR] == 1);
    reset_fixture(); fault_after = 10; check_failed(3);
    reset_fixture(); cancel_at = 500000; check_failed(3);
    CHECK(cancellation_seen == 1);
    reset_fixture(); stuck = true; check_failed(1);
    reset_fixture(); regs[REG_MOTOR_POLE_PAIRS] = 14; check_failed(2);
    reset_fixture(); ready = false; check_failed(3);
    reset_fixture(); store_status = HAL_ERROR;
    CHECK(API_FOC_Calibrate() == 4 && stores == 1);
    CHECK(!API_FOC_Is_Calibrating() && foc_state == FOC_STATE_IDLE);
    CHECK(regs[REG_INV_PHASE_MOTOR] == 1 && regs[REG_MOTOR_SYNCHRO_L] == 0x23 && regs[REG_MOTOR_SYNCHRO_H] == 1);
    CHECK(hardware_fault_status() == (1U << HW_ERROR_BIT_INTERNAL));
    puts(failures ? "Calibration regressions failed." : "Calibration regressions passed.");
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
