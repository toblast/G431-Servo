#include "cruise_control.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

bool Cruise_ValidateConfig(CruiseConfig const *config)
{
    if (config == NULL) return false;
    float const values[] = { config->wheel_circumference_m,
        config->minimum_speed_mps, config->maximum_speed_mps,
        config->kp, config->ki, config->kd, config->derivative_filter_s };
    for (unsigned i = 0; i < sizeof values / sizeof values[0]; ++i)
        if (!isfinite(values[i]) || values[i] < 0.0f) return false;
    if (config->wheel_circumference_m <= 0 || config->pulses_per_revolution == 0 ||
        config->minimum_speed_mps <= 0 ||
        config->maximum_speed_mps <= config->minimum_speed_mps || config->kp <= 0)
        return false;
    float const distance_m = config->wheel_circumference_m / config->pulses_per_revolution;
    return isfinite(distance_m) && distance_m > 0 &&
        config->period_ms > 0 && config->period_ms <= config->maximum_gap_ms &&
        config->maximum_gap_ms < config->speed_timeout_ms &&
        config->period_ms <= config->speed_window_ms &&
        config->speed_window_ms < config->speed_timeout_ms &&
        config->speed_timeout_ms <= 10000U &&
        config->button_debounce_ms > 0 && config->button_debounce_ms <= 1000U;
}

void Cruise_Cancel(CruiseController *controller, CruiseReason reason)
{
    controller->result.active = false;
    controller->result.target_speed_mps = controller->result.demand = 0;
    controller->result.reason = reason;
    controller->integral = controller->derivative = 0;
    controller->button_started = false;
    controller->button_stable = controller->button_candidate = true;
}

bool Cruise_Init(CruiseController *controller, CruiseConfig const *config)
{
    CruiseConfig const accepted = config != NULL ? *config : (CruiseConfig){ .kp = 0 };
    bool const valid = Cruise_ValidateConfig(&accepted);
    memset(controller, 0, sizeof *controller);
    Cruise_Cancel(controller, CRUISE_UNCONFIGURED);
    if (!valid) return false;
    controller->config = accepted;
    controller->configured = true;
    controller->result.reason = CRUISE_WARMING_UP;
    return true;
}

static void begin_speed(CruiseController *controller, uint32_t count, uint32_t now_ms)
{
    controller->sampled = true;
    controller->last_count = controller->window_count = count;
    controller->last_ms = controller->window_ms = controller->last_pulse_ms = now_ms;
    controller->result.speed_valid = false;
    controller->result.speed_mps = 0;
}

static CruiseReason update_speed(CruiseController *controller, uint32_t count, uint32_t now_ms)
{
    if (!controller->sampled) {
        begin_speed(controller, count, now_ms);
        return CRUISE_WARMING_UP;
    }
    uint32_t const elapsed_ms = now_ms - controller->last_ms;
    uint32_t const pulses = count - controller->last_count;
    if (elapsed_ms > controller->config.maximum_gap_ms || pulses > INT32_MAX) {
        begin_speed(controller, count, now_ms);
        return elapsed_ms > controller->config.maximum_gap_ms ?
            CRUISE_LOOP_GAP : CRUISE_SPEED_INVALID;
    }
    controller->last_ms = now_ms;
    controller->last_count = count;
    if (pulses) controller->last_pulse_ms = now_ms;
    if ((uint32_t)(now_ms - controller->last_pulse_ms) >= controller->config.speed_timeout_ms) {
        begin_speed(controller, count, now_ms);
        return CRUISE_SPEED_TIMEOUT;
    }
    uint32_t const window_ms = now_ms - controller->window_ms;
    if (window_ms >= controller->config.speed_window_ms) {
        uint32_t const window_pulses = count - controller->window_count;
        controller->window_ms = now_ms;
        controller->window_count = count;
        if (window_pulses) {
            float const speed_mps = (float)window_pulses *
                (controller->config.wheel_circumference_m /
                 controller->config.pulses_per_revolution) * (1000.0f / window_ms);
            if (!isfinite(speed_mps) || speed_mps > controller->config.maximum_speed_mps) {
                begin_speed(controller, count, now_ms);
                return CRUISE_SPEED_INVALID;
            }
            controller->result.speed_mps = speed_mps;
            controller->result.speed_valid = true;
        }
    }
    return controller->result.speed_valid ? CRUISE_DISABLED : CRUISE_WARMING_UP;
}

static bool button_pressed(CruiseController *controller, bool raw, uint32_t now_ms)
{
    if (!controller->button_started || raw != controller->button_candidate) {
        controller->button_started = true;
        controller->button_candidate = raw;
        controller->button_since_ms = now_ms;
    }
    if (raw != controller->button_stable &&
        (uint32_t)(now_ms - controller->button_since_ms) >= controller->config.button_debounce_ms) {
        controller->button_stable = raw;
        return raw;
    }
    return false;
}

static bool update_pid(CruiseController *controller, uint32_t elapsed_ms)
{
    if (elapsed_ms == 0) return true;
    float const dt_s = (float)elapsed_ms / 1000.0f;
    float const measured = controller->result.speed_mps;
    float const error = controller->result.target_speed_mps - measured;
    float const slope = -(measured - controller->previous_speed_mps) / dt_s;
    float const alpha = dt_s / (controller->config.derivative_filter_s + dt_s);
    float const derivative = controller->derivative + alpha * (slope - controller->derivative);
    float integral = controller->integral + controller->config.ki * error * dt_s;
    float const proportional = controller->config.kp * error;
    float const candidate = proportional + integral + controller->config.kd * derivative;
    if (!isfinite(derivative) || !isfinite(integral) || !isfinite(candidate)) return false;
    if ((candidate > 1 && error > 0) || (candidate < 0 && error < 0))
        integral = controller->integral; // Do not accumulate further into saturation.
    float const output = proportional + integral + controller->config.kd * derivative;
    if (!isfinite(output)) return false;
    controller->integral = integral;
    controller->derivative = derivative;
    controller->previous_speed_mps = measured;
    controller->result.demand = fmaxf(0, fminf(1, output));
    return true;
}

CruiseResult Cruise_Update(CruiseController *controller, CruiseInputs const *inputs,
                           uint32_t now_ms, bool ready, float initial_demand)
{
    if (!controller->configured) return controller->result;
    uint32_t const elapsed_ms = controller->sampled ? now_ms - controller->last_ms : 0;
    if (!inputs->valid) {
        controller->sampled = false;
        controller->result.speed_valid = false;
        controller->result.speed_mps = 0;
        Cruise_Cancel(controller, CRUISE_INPUT_ERROR);
        return controller->result;
    }
    CruiseReason const speed_reason = update_speed(controller, inputs->pulse_count, now_ms);
    CruiseReason reason;
    if (inputs->brake_active) reason = CRUISE_BRAKE;
    else if (inputs->clutch_active) reason = CRUISE_CLUTCH;
    else if (!ready) reason = CRUISE_DRIVE_UNAVAILABLE;
    else if (!controller->result.speed_valid) reason = speed_reason;
    else if (controller->result.speed_mps < controller->config.minimum_speed_mps)
        reason = CRUISE_SPEED_INVALID;
    else {
        if (button_pressed(controller, inputs->button_active, now_ms)) {
            if (controller->result.active) {
                Cruise_Cancel(controller, CRUISE_BUTTON_OFF);
                return controller->result;
            }
            if (!isfinite(initial_demand) || initial_demand < 0 || initial_demand > 1) {
                Cruise_Cancel(controller, CRUISE_DRIVE_UNAVAILABLE);
                return controller->result;
            }
            controller->result.active = true;
            controller->result.target_speed_mps = controller->result.speed_mps;
            controller->previous_speed_mps = controller->result.speed_mps;
            controller->integral = controller->result.demand = initial_demand;
            controller->derivative = 0;
        } else if (controller->result.active && !update_pid(controller, elapsed_ms)) {
            Cruise_Cancel(controller, CRUISE_PID_ERROR);
            return controller->result;
        }
        controller->result.reason = controller->result.active ? CRUISE_ACTIVE : CRUISE_DISABLED;
        return controller->result;
    }
    Cruise_Cancel(controller, reason);
    return controller->result;
}
