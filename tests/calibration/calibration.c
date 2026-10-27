#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "calibration.h"
#include "foc.h"
#include "position_sensor.h"
#include "control_table.h"
#include "configuration.h"
#include "math_tool.h"
#include "binary_tool.h"

uint8_t regs[REG_MAX];
uint32_t test_primask;
void (*test_irq_restored)(void);
static uint64_t clock_us, last_sample, last_service, cancel_at, foc_stop_at;
static uint64_t max_sample_gap, max_service_gap, target_since;
static unsigned failures, samples, services, stores, fault_after, cancellation_seen;
static unsigned captures, forward_steps, reverse_steps;
static float rotor_angle, rotor_direction;
static bool ready, stuck, valid_config, cancel_during_store, sampled_this_pass;
static HAL_StatusTypeDef store_status;
#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "Calibration check failed at line %d: %s\n", __LINE__, #condition); ++failures; \
} } while (0)

/* GLOBALS */
static void LL_FOC_coast(void) { }
bool positionSensor_isReady(void) { return ready; }
bool Config_IsValid(void) { return valid_config; }
float positionSensor_getRadiansMultiturn(void)
{
    CHECK(sampled_this_pass);
    ++captures;
    float const expected_angle = captures == 1 ? M_3PI_2 + M_2PI : captures == 2 ? M_3PI_2 : 0.0f;
    CHECK(fabsf(setpoint_electrical_angle_rad - expected_angle) < 0.00001f);
    CHECK(HAL_GetTick() - (uint32_t)(target_since / 1000) >= (captures == 3 ? 1000U : 200U));
    return rotor_angle;
}
uint32_t API_FOC_Get_Timestamp_ms(void)
{
    CHECK(test_primask == 1);
    return (uint32_t)((clock_us < foc_stop_at ? clock_us : foc_stop_at) / 1000);
}
uint32_t HAL_GetTick(void) { return (uint32_t)(clock_us / 1000); }
static void acquire(void)
{
    CHECK(test_primask == 0);
    if (clock_us - last_sample > max_sample_gap) max_sample_gap = clock_us - last_sample;
    last_sample = clock_us;
    ++samples;
    if (!stuck && foc_state == FOC_STATE_FLUX_CONTROL && setpoint_flux_voltage_V > 0)
        rotor_angle = 1.0f + rotor_direction * setpoint_electrical_angle_rad / 7.0f;
    if (fault_after && samples >= fault_after) hardware_fault_set(DRIVE_FAULT_ENCODER_READ);
}
static void service(void)
{
    CHECK(test_primask == 0);
    if (clock_us - last_service > max_service_gap) max_service_gap = clock_us - last_service;
    last_service = clock_us;
    ++services;
}
bool Config_Save(void)
{
    CHECK(test_primask == 0 && API_FOC_Is_Calibrating());
    CHECK(foc_state == FOC_STATE_IDLE && setpoint_flux_voltage_V == 0);
    CHECK(!calibration_cancelled && hardware_fault_status() == 0);
    CHECK(!API_FOC_Calibration_Begin());
    ++stores;
    if (cancel_during_store) API_FOC_Torque_Disable();
    if (store_status != HAL_OK) hardware_fault_set(DRIVE_FAULT_INTERNAL);
    return store_status == HAL_OK;
}
/* FUNCTIONS */

static void update(bool sampled)
{
    uint64_t const before = clock_us;
    unsigned const old_samples = samples, old_services = services;
    float const old_angle = setpoint_electrical_angle_rad;
    bool const was_exciting = foc_state == FOC_STATE_FLUX_CONTROL;
    sampled_this_pass = sampled;
    Calibration_Update(sampled);
    CHECK(clock_us == before && samples == old_samples && services == old_services);
    if (foc_state == FOC_STATE_FLUX_CONTROL && (!was_exciting || old_angle != setpoint_electrical_angle_rad)) {
        CHECK(setpoint_flux_voltage_V == 3.0f);
        if (!was_exciting) CHECK(setpoint_electrical_angle_rad == M_3PI_2);
        if (was_exciting && setpoint_electrical_angle_rad > old_angle) {
            ++forward_steps;
            CHECK(fabsf(setpoint_electrical_angle_rad - old_angle - M_2PI / 500) < 0.00001f);
            CHECK(HAL_GetTick() - (uint32_t)(target_since / 1000) >= 2);
        } else if (was_exciting && setpoint_electrical_angle_rad < old_angle && setpoint_electrical_angle_rad != 0) {
            ++reverse_steps;
            CHECK(fabsf(old_angle - setpoint_electrical_angle_rad - M_2PI / 500) < 0.00001f);
            CHECK(HAL_GetTick() - (uint32_t)(target_since / 1000) >= 2);
        }
        target_since = clock_us;
    }
}
static void poll(void)
{
    clock_us += 100;
    if (API_FOC_Is_Calibrating() && !cancellation_seen && clock_us >= cancel_at) {
        ++cancellation_seen;
        API_FOC_Torque_Disable();
        CHECK(API_FOC_Is_Calibrating());
        CHECK(!API_FOC_Calibration_SetFlux(1, 3));
    }
    bool const sampled = clock_us - last_sample >= 250;
    if (sampled) acquire();
    if (clock_us - last_service >= 10000) service();
    update(sampled);
}
static CalibrationResult finish(void)
{
    CalibrationResult result;
    unsigned polls = 0;
    while (!Calibration_TakeResult(&result)) {
        if (++polls > 100000) { fprintf(stderr, "Calibration did not complete.\n"); exit(1); }
        poll();
    }
    CHECK(!Calibration_TakeResult(&result));
    CHECK(!API_FOC_Is_Calibrating() && foc_state == FOC_STATE_IDLE && setpoint_flux_voltage_V == 0);
    return result;
}
static void reset_fixture(void)
{
    memset(regs, 0, sizeof regs);
    regs[REG_MOTOR_POLE_PAIRS] = 7;
    regs[REG_INV_PHASE_MOTOR] = 1;
    regs[REG_MOTOR_SYNCHRO_L] = 0x23;
    regs[REG_MOTOR_SYNCHRO_H] = 1;
    clock_us = last_sample = last_service = target_since = 0;
    max_sample_gap = max_service_gap = 0;
    cancel_at = foc_stop_at = UINT64_MAX;
    samples = services = stores = fault_after = cancellation_seen = 0;
    forward_steps = reverse_steps = captures = 0;
    rotor_angle = rotor_direction = 1;
    ready = valid_config = true;
    stuck = cancel_during_store = sampled_this_pass = false;
    store_status = HAL_OK;
    calibration_active = calibration_cancelled = false;
    test_primask = 0;
    test_irq_restored = NULL;
    foc_state = FOC_STATE_IDLE;
    setpoint_flux_voltage_V = setpoint_electrical_angle_rad = 0;
    Calibration_Init();
}
static void check_rollback(void)
{
    CHECK(regs[REG_INV_PHASE_MOTOR] == 1 && regs[REG_MOTOR_SYNCHRO_L] == 0x23 && regs[REG_MOTOR_SYNCHRO_H] == 1);
}
static void check_failed(CalibrationResult expected)
{
    CHECK(Calibration_Start());
    CHECK(finish() == expected);
    CHECK(stores == 0);
    check_rollback();
}
static void check_rejected(void)
{
    CHECK(!Calibration_Start() && !API_FOC_Is_Calibrating());
    CalibrationResult result;
    CHECK(!Calibration_TakeResult(&result));
    CHECK(stores == 0 && test_primask == 0);
    check_rollback();
}
int main(void)
{
    reset_fixture();
    CHECK(Calibration_Start() && API_FOC_Is_Calibrating());
    CHECK(!Calibration_Start());
    CHECK(finish() == CALIBRATION_SUCCESS);
    CHECK(stores == 1 && samples > 10000 && services > 300);
    CHECK(max_sample_gap == 300 && max_service_gap == 10000);
    CHECK(forward_steps == 500 && reverse_steps == 500 && captures == 3);
    CHECK(regs[REG_INV_PHASE_MOTOR] == 0 && regs[REG_MOTOR_SYNCHRO_L] == 62 && regs[REG_MOTOR_SYNCHRO_H] == 1);
    CHECK(clock_us >= 3704000 && clock_us < 3720000);

    reset_fixture(); rotor_direction = -1;
    CHECK(Calibration_Start() && finish() == CALIBRATION_SUCCESS);
    CHECK(stores == 1 && regs[REG_INV_PHASE_MOTOR] == 1 && regs[REG_MOTOR_SYNCHRO_L] == 41 && regs[REG_MOTOR_SYNCHRO_H] == 0);
    reset_fixture(); fault_after = 10; check_failed(CALIBRATION_CANCELLED);
    reset_fixture(); fault_after = 2000; check_failed(CALIBRATION_CANCELLED);
    reset_fixture(); cancel_at = 500000; check_failed(CALIBRATION_CANCELLED);
    CHECK(cancellation_seen == 1);
    reset_fixture(); stuck = true; check_failed(CALIBRATION_NO_MOTION);
    reset_fixture(); rotor_direction = NAN; check_failed(CALIBRATION_NO_MOTION);
    reset_fixture(); regs[REG_MOTOR_POLE_PAIRS] = 14; check_failed(CALIBRATION_POLE_MISMATCH);
    reset_fixture(); ready = false; check_rejected();
    reset_fixture(); valid_config = false; check_rejected();
    reset_fixture(); hardware_fault_set(DRIVE_FAULT_INTERNAL); check_rejected();
    reset_fixture(); regs[REG_TORQUE_ENABLE] = 1; check_rejected();
    CHECK(regs[REG_TORQUE_ENABLE] == 1);
    reset_fixture(); foc_stop_at = 0; check_failed(CALIBRATION_CANCELLED);
    CHECK(clock_us < 200000 && hardware_fault_status() == DRIVE_FAULT_FOC_TIMEOUT);
    reset_fixture(); foc_stop_at = 500000; check_failed(CALIBRATION_CANCELLED);
    CHECK(clock_us < 512000 && hardware_fault_status() == DRIVE_FAULT_FOC_TIMEOUT);
    reset_fixture(); foc_stop_at = 3000000; check_failed(CALIBRATION_CANCELLED);
    CHECK(clock_us < 3012000 && hardware_fault_status() == DRIVE_FAULT_FOC_TIMEOUT);
    reset_fixture(); clock_us = ((uint64_t)UINT32_MAX - 100) * 1000;
    last_sample = last_service = clock_us;
    CHECK(Calibration_Start() && finish() == CALIBRATION_SUCCESS && hardware_fault_status() == 0);
    reset_fixture(); store_status = HAL_ERROR;
    CHECK(Calibration_Start() && finish() == CALIBRATION_STORAGE_ERROR && stores == 1);
    check_rollback();
    CHECK(hardware_fault_status() == DRIVE_FAULT_INTERNAL);
    reset_fixture(); cancel_during_store = true;
    CHECK(Calibration_Start() && finish() == CALIBRATION_CANCELLED && stores == 1);
    check_rollback();

    reset_fixture(); CHECK(Calibration_Start());
    while (captures < 3) poll();
    API_FOC_Torque_Disable();
    CHECK(finish() == CALIBRATION_CANCELLED && stores == 0);
    check_rollback();
    reset_fixture(); CHECK(Calibration_Start());
    while (clock_us < 500000) poll();
    ready = false;
    CHECK(finish() == CALIBRATION_CANCELLED && stores == 0);
    check_rollback();

    // No catch-up burst after a late call; each following step gets its own dwell.
    reset_fixture(); CHECK(Calibration_Start());
    clock_us = 200000; update(false);
    clock_us = 300000; update(false);
    clock_us = 800000; update(false);
    CHECK(forward_steps == 1);
    update(false); CHECK(forward_steps == 1);
    clock_us += 1999; update(false); CHECK(forward_steps == 1);
    ++clock_us; update(false); CHECK(forward_steps == 2);
    API_FOC_Torque_Disable(); CHECK(finish() == CALIBRATION_CANCELLED);

    // Settled endpoints wait for a normal acquisition on a later pass.
    reset_fixture(); CHECK(Calibration_Start());
    clock_us = 200000; update(false);
    clock_us = 300000; update(false);
    for (unsigned i = 0; i < 501; ++i) { clock_us += 2000; update(false); }
    clock_us += 200000; update(true); CHECK(captures == 0);
    update(false); CHECK(captures == 0);
    acquire(); update(true); CHECK(captures == 1);
    API_FOC_Torque_Disable(); CHECK(finish() == CALIBRATION_CANCELLED);

    // Cleanup releases the guard last, restores IRQ masks and reports only once.
    reset_fixture(); test_primask = 1;
    CHECK(Calibration_Start() && test_primask == 1);
    API_FOC_Torque_Disable(); update(false);
    CHECK(!API_FOC_Is_Calibrating() && test_primask == 1);
    CHECK(!Calibration_Start() && !Calibration_TakeResult(NULL));
    CalibrationResult result;
    CHECK(Calibration_TakeResult(&result) && result == CALIBRATION_CANCELLED);
    CHECK(!Calibration_TakeResult(&result));
    check_rollback();
    CHECK(Calibration_Start() && test_primask == 1);
    API_FOC_Torque_Disable(); update(false);
    puts(failures ? "Calibration regressions failed." : "Calibration regressions passed (trajectory, scheduling and faults).");
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
