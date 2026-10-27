#ifndef INC_CRUISE_CONTROL_H_
#define INC_CRUISE_CONTROL_H_

#include <stdbool.h>
#include <stdint.h>

// Road speed is m/s; demand is a normalized throttle position in [0, 1].
// This integral belongs to the road-speed loop, never the motor position loop.
typedef struct {
    float wheel_circumference_m;
    uint32_t pulses_per_revolution;
    float minimum_speed_mps, maximum_speed_mps;
    float kp, ki, kd, derivative_filter_s;
    uint32_t period_ms, maximum_gap_ms, speed_window_ms, speed_timeout_ms;
    uint32_t button_debounce_ms;
} CruiseConfig;

typedef struct {
    uint32_t pulse_count; // Cumulative rising edges, modulo 2^32.
    bool valid, brake_active, clutch_active, button_active;
} CruiseInputs;

typedef enum {
    CRUISE_UNCONFIGURED, CRUISE_WARMING_UP, CRUISE_DISABLED, CRUISE_ACTIVE,
    CRUISE_BUTTON_OFF, CRUISE_BRAKE, CRUISE_CLUTCH, CRUISE_INPUT_ERROR,
    CRUISE_SPEED_TIMEOUT, CRUISE_SPEED_INVALID, CRUISE_LOOP_GAP,
    CRUISE_DRIVE_UNAVAILABLE, CRUISE_PID_ERROR
} CruiseReason;

typedef struct {
    bool active, speed_valid;
    float speed_mps, target_speed_mps, demand;
    CruiseReason reason;
} CruiseResult;

// Caller-owned storage; foreground-only, no allocation or HAL dependency.
// Fields are module-owned history; callers use the operations below.
typedef struct {
    CruiseConfig config;
    CruiseResult result;
    uint32_t last_ms, last_count, window_ms, window_count, last_pulse_ms;
    uint32_t button_since_ms;
    bool configured, sampled, button_started, button_stable, button_candidate;
    float integral, derivative, previous_speed_mps;
} CruiseController;

bool Cruise_ValidateConfig(CruiseConfig const *config); // Pure, NULL rejected.
// Copies config. Invalid settings leave the instance disabled/unconfigured.
bool Cruise_Init(CruiseController *controller, CruiseConfig const *config);
// Clears PID/engagement and requires release + fresh press, retaining speed.
void Cruise_Cancel(CruiseController *controller, CruiseReason reason);
// Borrowed samples; now_ms is a wrapping millisecond tick. ready includes local
// drive health. initial_demand tracks measured actuator position on engagement.
// Called at config.period_ms; excessive actual gaps cancel rather than catch up.
CruiseResult Cruise_Update(CruiseController *controller, CruiseInputs const *inputs,
                           uint32_t now_ms, bool ready, float initial_demand);

#endif
