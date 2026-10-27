#include "standalone.h"
#include "configuration.h"
#include "control_table.h"
#include "drive.h"
#include "position_sensor.h"
#include <math.h>
#include <string.h>

static struct {
    StandaloneConfig config;
    CruiseController controller;
    uint32_t last_update_ms, arm_generation;
    bool configured;
} local;

static bool travel_valid(StandaloneConfig const *config)
{
    DriveConfig const drive_config = control_read_config();
    float const closed = config->closed_position_deg;
    float const open = config->open_position_deg;
    return isfinite(closed) && isfinite(open) && fabsf(open - closed) >= 0.1f &&
        closed >= drive_config.min_position_deg && closed <= drive_config.max_position_deg &&
        open >= drive_config.min_position_deg && open <= drive_config.max_position_deg &&
        closed >= -3276.7f && closed <= 3276.7f && open >= -3276.7f && open <= 3276.7f &&
        config->position_kp_raw != 0;
}

bool Standalone_Init(StandaloneConfig const *config)
{
    memset(&local, 0, sizeof local);
    Cruise_Cancel(&local.controller, CRUISE_UNCONFIGURED);
    if (config == NULL || !Cruise_ValidateConfig(&config->cruise) || !travel_valid(config))
        return false;
    float const maximum_pulses_per_ms = config->cruise.maximum_speed_mps /
        (config->cruise.wheel_circumference_m / config->cruise.pulses_per_revolution) / 1000.0f;
    if (!isfinite(maximum_pulses_per_ms) ||
        ceilf(maximum_pulses_per_ms) > config->inputs.maximum_pulses_per_ms)
        return false;
    if (!VehicleInputs_Init(&config->inputs)) return false;
    local.config = *config;
    Cruise_Init(&local.controller, &config->cruise);
    local.last_update_ms = HAL_GetTick();
    local.configured = true;
    return true;
}

static void cancel(CruiseReason reason)
{
    Cruise_Cancel(&local.controller, reason);
    // Leave an independent, explicitly requested calibration job alone.
    if (Drive_GetStatus().armed) Drive_Disarm();
}

void Standalone_Process(void)
{
    if (!local.configured) {
        cancel(CRUISE_UNCONFIGURED);
        return;
    }
    CruiseInputs const inputs = VehicleInputs_Read();
    if (!inputs.valid) {
        Cruise_Update(&local.controller, &inputs, HAL_GetTick(), false, 0);
        Drive_Disarm();
        return;
    }
    if (inputs.brake_active || inputs.clutch_active) {
        cancel(inputs.brake_active ? CRUISE_BRAKE : CRUISE_CLUTCH);
        // Also cancel manual calibration, but keep sampling road speed below.
        Drive_Disarm();
    }
    DriveStatus const drive = Drive_GetStatus();
    if (local.controller.result.active &&
        (!drive.armed || drive.generation != local.arm_generation)) {
        cancel(CRUISE_DRIVE_UNAVAILABLE);
    }
    bool const ready = Config_IsValid() && travel_valid(&local.config) &&
        drive.faults == 0 &&
        (drive.state == DRIVE_DISARMED || drive.state == DRIVE_ARMED);
    if (!ready) {
        cancel(CRUISE_DRIVE_UNAVAILABLE);
    }
    uint32_t const now_ms = HAL_GetTick();
    if ((uint32_t)(now_ms - local.last_update_ms) < local.config.cruise.period_ms) return;
    local.last_update_ms = now_ms; // No replay of missed controller steps.
    bool const was_active = local.controller.result.active;
    float const initial_demand =
        (positionSensor_getDegreesMultiturn() - local.config.closed_position_deg) /
        (local.config.open_position_deg - local.config.closed_position_deg);
    CruiseResult const result = Cruise_Update(&local.controller, &inputs, now_ms, ready, initial_demand);
    if (!result.active) {
        if (was_active || drive.armed) Drive_Disarm();
        return;
    }
    if (!was_active) {
        if (!Drive_RequestArm()) {
            cancel(CRUISE_DRIVE_UNAVAILABLE);
            return;
        }
        local.arm_generation = Drive_GetStatus().generation;
    }
    float const position_deg = local.config.closed_position_deg + result.demand *
        (local.config.open_position_deg - local.config.closed_position_deg);
    ServoCommand const command = {
        .position_tenths_deg = (int16_t)lroundf(position_deg * 10.0f),
        .position_kp_raw = local.config.position_kp_raw,
        .position_kd_raw = local.config.position_kd_raw
    };
    if (!Drive_SubmitCommand(&command)) cancel(CRUISE_DRIVE_UNAVAILABLE);
}

CruiseResult Standalone_GetStatus(void)
{
    return local.controller.result;
}
