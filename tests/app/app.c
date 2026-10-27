#include "app_fixture.h"
#include "binary_tool.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned assertions;
#define CHECK(condition) do { ++assertions; if (!(condition)) { \
    fprintf(stderr, "App line %d: %s\n", __LINE__, #condition); exit(1); \
} } while (0)

static void test_deadlines_order_and_inputs(void)
{
    AppTest_Reset(0);
    CHECK(app_test.controller_inits == 1 && app_test.clock_reads == 1);
    CHECK(app_test.watchdogs == 0 && app_test.calibrations == 0 && app_test.servos == 0);
    AppTest_Poll(249);
    CHECK(strcmp(app_test.trace, "WC") == 0 && app_test.servos == 0 && app_test.services == 0);
    AppTest_ClearTrace();
    app_test.command.enabled = true; app_test.command.generation = 9;
    app_test.command.goal.torque_mA = 23;
    AppTest_Poll(250);
    CHECK(strcmp(app_test.trace, "WCSUM") == 0);
    CHECK(app_test.last_command.enabled && app_test.last_command.generation == 9 &&
          app_test.last_command.goal.torque_mA == 23);
    CHECK(app_test.last_config.max_current_mA == 500 &&
          fabsf(app_test.servo_position - app_test.position) < 0.001f &&
          app_test.servo_velocity == app_test.velocity);
    CHECK(app_test.publications == 1 && app_test.published.torque_mA == -500 && test_primask == 0);
    AppTest_Poll(499);
    CHECK(app_test.servos == 1);
    AppTest_Poll(500);
    CHECK(app_test.servos == 2);
    AppTest_ClearTrace();
    app_test.serial_pending = 2;
    AppTest_Poll(10000);
    CHECK(strcmp(app_test.trace, "WCSUMFLBRR") == 0);
    CHECK(app_test.servos == 3 && app_test.services == 1 && app_test.packets == 2);
    CHECK(app_test.errors == 0);
}

static void test_phase_catchup_rollover_and_single_time_sample(void)
{
    AppTest_Reset(1000);
    for (unsigned i = 1; i <= 4; ++i) {
        AppTest_Poll(2000);
        CHECK(app_test.servos == i && app_test.services == 0);
    }
    AppTest_Poll(2000);
    CHECK(app_test.servos == 4 && app_test.watchdogs == 5);
    AppTest_Reset(0);
    AppTest_Poll(30000);
    CHECK(app_test.servos == 1 && app_test.services == 1);
    AppTest_Poll(30000);
    CHECK(app_test.servos == 2 && app_test.services == 2);
    AppTest_Poll(30000);
    CHECK(app_test.servos == 3 && app_test.services == 3);
    AppTest_Poll(30000);
    CHECK(app_test.servos == 4 && app_test.services == 3);

    AppTest_Reset(65500);
    AppTest_Poll((uint16_t)(65500U + 249U));
    CHECK(app_test.servos == 0 && app_test.services == 0);
    AppTest_Poll((uint16_t)(65500U + 250U));
    CHECK(app_test.servos == 1 && app_test.services == 0);
    AppTest_Poll((uint16_t)(65500U + 10000U));
    CHECK(app_test.servos == 2 && app_test.services == 1);

    AppTest_Reset(0);
    app_test.sensor_delay_us = 2;
    AppTest_Poll(9999);
    CHECK(app_test_timer.counter == 10001 && app_test.services == 0 && app_test.clock_reads == 2);
    App_Process();
    CHECK(app_test.services == 1 && app_test.clock_reads == 3);
    CHECK(app_test.errors == 0);
}

static void test_watchdog_configuration_and_led(void)
{
    AppTest_Reset(0);
    app_test.command.enabled = true;
    app_test.disarmed = false;
    app_test.watchdog_disarms = true;
    AppTest_Poll(250);
    CHECK(app_test.watchdogs == 1 && !app_test.last_command.enabled && app_test.disarmed);
    AppTest_Reset(0);
    regs[REG_ENCODER_BITS] = 0;
    AppTest_Poll(10000);
    CHECK(strstr(app_test.trace, "MDFLB") != NULL && app_test.disarms == 1);
    CHECK(hardware_fault_status() == (1U << HW_ERROR_BIT_CONFIGURATION) && app_test.led == GPIO_PIN_SET);
    regs[REG_ENCODER_BITS] = 12;
    hardware_fault_set(1U << HW_ERROR_BIT_OVERHEATING);
    AppTest_Poll(20000);
    CHECK(hardware_fault_status() == (1U << HW_ERROR_BIT_OVERHEATING) && app_test.disarms == 1);
    hardware_fault_clear(0xff);
    AppTest_Poll(30000);
    CHECK(app_test.led == GPIO_PIN_RESET && app_test.disarmed);
    regs[REG_LED] = 1;
    AppTest_Poll(40000);
    CHECK(app_test.led == GPIO_PIN_SET && app_test.errors == 0);
}

static void test_button_edges_and_calibration_job(void)
{
    AppTest_Reset(0);
    bool const buttons[] = { true, true, false, true, true, false, true };
    unsigned const requests[] = { 0, 0, 0, 1, 1, 1, 2 };
    for (unsigned i = 0; i < sizeof buttons / sizeof *buttons; ++i) {
        app_test.button_down = buttons[i];
        AppTest_Poll((uint16_t)((i + 1) * 10000));
        CHECK(app_test.calibrations == requests[i]);
    }
    CHECK(app_test.prints == 2 && app_test.errors == 0);

    AppTest_Reset(0);
    bool const armed_buttons[] = { false, true, true, false, true };
    bool const disarmed[] = { false, false, true, true, true };
    for (unsigned i = 0; i < sizeof armed_buttons / sizeof *armed_buttons; ++i) {
        app_test.button_down = armed_buttons[i]; app_test.disarmed = disarmed[i];
        AppTest_Poll((uint16_t)((i + 1) * 10000));
        CHECK(app_test.calibrations == (i == 4 ? 1U : 0U));
    }

    AppTest_Reset(0);
    AppTest_Poll(10000); // Release the startup button interlock.
    AppTest_ClearTrace();
    app_test.button_down = true;
    app_test.calibration_hold = true;
    app_test.calibration_result = 3;
    app_test.serial_pending = 1;
    AppTest_Poll(20000);
    CHECK(strcmp(app_test.trace, "WCSUMFLBKRT") == 0 && app_test_timer.counter == 20000);
    CHECK(app_test.calibration_active && app_test.prints == 0 && app_test.packets == 1);
    unsigned const servos = app_test.servos;
    AppTest_ClearTrace();
    App_Process();
    CHECK(strcmp(app_test.trace, "WCSMT") == 0 && app_test.calibrations == 1);
    CHECK(app_test.servos == servos && app_test.publications == app_test.sensors);
    app_test.button_down = false;
    AppTest_Poll(30000);
    app_test.button_down = true;
    app_test.serial_pending = 2;
    AppTest_ClearTrace();
    AppTest_Poll(40000);
    CHECK(strcmp(app_test.trace, "WCSMFLBRRT") == 0 && app_test.calibrations == 1);
    CHECK(app_test.servos == servos && app_test.services == 4 && app_test.packets == 3);
    CHECK(app_test.calibration_updates == 4 && app_test.calibration_samples == 4);
    app_test.calibration_hold = false;
    AppTest_ClearTrace();
    App_Process();
    CHECK(strcmp(app_test.trace, "WCSMTP") == 0 && app_test.prints == 1);
    CHECK(strstr(app_test.calibration_text, "Calibration result: 3 ") != NULL);
    AppTest_ClearTrace();
    App_Process();
    CHECK(strcmp(app_test.trace, "WCSUM") == 0 && app_test.servos == servos + 1 && app_test.prints == 1);
    CHECK(app_test.errors == 0);

    AppTest_Reset(0);
    AppTest_Poll(10000);
    app_test.button_down = true; app_test.calibration_hold = true;
    AppTest_Poll(20000);
    app_test.serial_pending = 1; app_test.serial_disarms = true;
    AppTest_ClearTrace();
    AppTest_Poll(30000);
    CHECK(strcmp(app_test.trace, "WCSMFLBRDTP") == 0);
    CHECK(!app_test.calibration_active && app_test.prints == 1 && app_test.disarms == 1);
    CHECK(strstr(app_test.calibration_text, "Calibration result: 3 ") != NULL);
    CHECK(app_test.errors == 0);

    AppTest_Reset(0);
    AppTest_Poll(10000);
    app_test.button_down = true; app_test.calibration_start_rejected = true;
    AppTest_Poll(20000);
    CHECK(!app_test.calibration_active && app_test.prints == 1);
    CHECK(strstr(app_test.calibration_text, "Calibration result: 3 ") != NULL);
    CHECK(app_test.errors == 0);
}

static void test_bounded_serial_and_irq_state(void)
{
    AppTest_Reset(0);
    app_test.serial_pending = 3; app_test.serial_refill = true;
    AppTest_Poll(10000);
    CHECK(app_test.packets == 3 && app_test.serial_reads == 3 && app_test.serial_pending == 3);
    AppTest_Poll(10000);
    CHECK(app_test.watchdogs == 2 && app_test.packets == 3);
    AppTest_Poll(20000);
    CHECK(app_test.packets == 6 && app_test.serial_pending == 3);
    AppTest_Reset(0);
    app_test.serial_pending = 3; app_test.serial_shrink = true;
    AppTest_Poll(10000);
    CHECK(app_test.packets == 1 && app_test.serial_pending == 0);

    AppTest_Reset(0);
    test_primask = 1;
    AppTest_Poll(250);
    CHECK(test_primask == 1 && app_test.errors == 0);
    test_primask = 0;
    // Reinitializing scheduler history must not clear a command accepted by CAN.
    app_test.command.enabled = true; app_test.command.generation = 17;
    App_Init(&app_test_timer, &app_test_serial);
    AppTest_Advance(250);
    CHECK(app_test.last_command.enabled && app_test.last_command.generation == 17 && app_test.disarms == 0);
    CHECK(app_test.errors == 0);
}

int main(void)
{
    test_deadlines_order_and_inputs();
    test_phase_catchup_rollover_and_single_time_sample();
    test_watchdog_configuration_and_led();
    test_button_edges_and_calibration_job();
    test_bounded_serial_and_irq_state();
    printf("Application scheduler: %u assertions passed.\n", assertions);
    return 0;
}
