#ifndef APP_TEST_FIXTURE_H
#define APP_TEST_FIXTURE_H

#include "app.h"
#include "servo_controller.h"
#include "control_table.h"

// Observations and inputs for mocked dependencies, never private app state.
typedef struct {
    float position, velocity, torque, flux, voltage, temperature;
    float processing_time, processing_frequency;
    uint32_t tick;
    DriveCommandSnapshot command, last_command;
    DriveConfig last_config;
    ServoSetpoints setpoints;
    DriveMeasurements published;
    float servo_position, servo_velocity;
    bool disarmed, button_down, watchdog_disarms;
    bool serial_refill, serial_shrink;
    bool calibration_active, calibration_pending, calibration_hold;
    bool calibration_cancelled, calibration_start_rejected, serial_disarms;
    int serial_pending, calibration_result;
    uint16_t sensor_delay_us;
    unsigned errors, controller_inits, clock_reads, watchdogs, sensors, servos;
    unsigned publications, services, disarms, calibrations, prints;
    unsigned calibration_updates, calibration_samples;
    unsigned serial_reads, serial_checks, packets;
    GPIO_PinState led;
    char trace[256], calibration_text[160];
    unsigned trace_length;
} AppTestState;

extern AppTestState app_test;
extern TIM_HandleTypeDef app_test_timer;
extern HAL_Serial_Handler app_test_serial;
void AppTest_Reset(uint16_t start_us);
void AppTest_ClearTrace(void);
void AppTest_Poll(uint16_t now_us);
void AppTest_Advance(uint16_t delta_us);

#endif
