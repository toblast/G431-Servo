#ifndef INC_DRIVE_TYPES_H_
#define INC_DRIVE_TYPES_H_

#include <stdbool.h>
#include <stdint.h>

// Active application settings. Gain fields retain their existing register scale.
typedef struct {
    int16_t min_position_deg, max_position_deg, max_velocity_dps;
    uint16_t max_current_mA;
    uint8_t encoder_bits, motor_pole_pairs, invert_phase_motor;
    uint8_t low_voltage_V, high_voltage_V, temperature_limit_C;
    uint8_t moving_threshold_dps;
    int16_t flux_current_kp_raw, flux_current_ki_raw;
    int16_t torque_current_kp_raw, torque_current_ki_raw;
} DriveConfig;

// Numeric command values, never packet bytes or register addresses. Preserve
// existing units/scales so the refactor does not alter arithmetic or the wire ABI.
typedef struct {
    int16_t position_tenths_deg, velocity_dps, torque_mA, flux_mA;
    uint8_t pos_kp, pos_kd, vel_kp;
} ServoCommand;

typedef struct {
    bool enabled;
    uint32_t generation;
    ServoCommand goal;
} DriveCommandSnapshot;

typedef enum {
    DRIVE_DISARMED,
    DRIVE_ARMED,
    DRIVE_CALIBRATING,
    DRIVE_MAINTENANCE,
    DRIVE_FAULT
} DriveState;

typedef struct {
    float position_deg, velocity_dps;
    float torque_mA, flux_mA, voltage_V, temperature_C;
    bool moving;
} DriveMeasurements;

typedef struct {
    DriveState state;
    uint8_t faults;
    uint32_t generation;
    bool armed; // Command permission, not proof that PWM outputs are enabled.
    uint32_t last_command_ms;
    DriveMeasurements measured;
} DriveStatus;

#endif
