#ifndef INC_MOTOR_CONFIG_H_
#define INC_MOTOR_CONFIG_H_

#include <stdbool.h>
#include <stdint.h>

// Existing provisional bench caps, independent of register addresses.
#define SERVO_BENCH_MAX_CURRENT_MA 1000U
#define SERVO_BENCH_OVERCURRENT_MA 1500U
#define SERVO_BENCH_MAX_VELOCITY_DPS 360U

typedef struct {
    int16_t phase_offset_deg;
    uint8_t invert_phase_motor;
} MotorAlignment;

// All angles/velocities refer to the motor shaft. Raw gains retain their register
// scale: FOC divides Kp by 100000 and Ki by 100000000 before its existing PI.
typedef struct {
    int16_t phase_offset_deg;
    uint16_t phase_trim_deg;
    uint8_t motor_pole_pairs, invert_phase_motor;
    int16_t flux_current_kp_raw, flux_current_ki_raw;
    int16_t torque_current_kp_raw, torque_current_ki_raw;
    int16_t max_velocity_dps;
} MotorControlConfig;

typedef struct {
    uint8_t low_voltage_V, high_voltage_V, temperature_limit_C;
} MotorProtectionConfig;

// Narrow live views of the existing settings image, not a second cache. Reads
// and alignment writes are coherent and restore the caller's IRQ mask. Fast
// control readers run from CCMRAM. No allocation or persistence in these paths.
// Readable from foreground/IRQ; alignment writes require foreground calibration
// or configuration exclusion. Nonnull alignment pointers are borrowed/copied.
MotorControlConfig control_read_motor_config(void) __attribute__((section(".ccmram")));
MotorProtectionConfig control_read_protection_config(void);
MotorAlignment control_read_motor_alignment(void);
void control_write_motor_alignment(MotorAlignment const *alignment);
bool control_torque_requested(void) __attribute__((section(".ccmram")));

// Foreground encoder diagnostics: preserve the existing wrapping byte counter.
void control_record_sensor_error(void);

#endif
