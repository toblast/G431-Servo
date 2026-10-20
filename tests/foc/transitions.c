#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "control_table.h"
#include "pid.h"

uint8_t regs[REG_MAX];
uint32_t test_primask;
void (*test_irq_restored)(void);
static unsigned delays, brakes, observations;
static unsigned failures;
bool control_configuration_valid(void) { return true; }
#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "FOC transition failed at line %d: %s\n", __LINE__, #condition); ++failures; \
} } while (0)

void pid_reset(pid_context_t *ctx)
{
    CHECK(test_primask == 1);
    ctx->err_last_one = ctx->err_integral = ctx->derivative_filtered = 0;
}
static void LL_FOC_coast(void) { ++brakes; }
void HAL_Delay(uint32_t time) { (void)time; ++delays; }
/* GLOBALS */
/* FUNCTIONS */

static void observe_enable(void)
{
    ++observations;
    CHECK(foc_state == FOC_STATE_TORQUE_CONTROL);
    CHECK(setpoint_torque_current_mA == 0 && setpoint_flux_current_mA == 0);
    CHECK(flux_pi.err_integral == 0 && torque_pi.err_integral == 0);
}

static void observe_currents(void)
{
    ++observations;
    CHECK(setpoint_torque_current_mA == 123 && setpoint_flux_current_mA == -45);
}

int main(void)
{
    regs[REG_TORQUE_ENABLE] = 1;
    setpoint_torque_current_mA = 1000;
    setpoint_flux_current_mA = 2000;
    flux_pi.err_integral = torque_pi.err_integral = 12;
    test_irq_restored = observe_enable;
    API_FOC_Torque_Enable();
    CHECK(observations == 1 && test_primask == 0);

    observations = 0;
    test_irq_restored = observe_currents;
    API_FOC_Set_Torque_Flux_Currents_mA(123, -45);
    CHECK(observations == 1);
    test_irq_restored = NULL;

    API_FOC_Torque_Disable();
    CHECK(foc_state == FOC_STATE_IDLE);
    CHECK(setpoint_torque_current_mA == 0 && setpoint_flux_current_mA == 0);
    CHECK(delays == 0 && brakes == 1);
    API_FOC_Set_Torque_Flux_Currents_mA(999, 999);
    CHECK(setpoint_torque_current_mA == 0 && setpoint_flux_current_mA == 0);

    regs[REG_HARDWARE_ERROR_STATUS] = 1;
    API_FOC_Torque_Enable();
    CHECK(foc_state == FOC_STATE_IDLE);
    regs[REG_HARDWARE_ERROR_STATUS] = 0;
    regs[REG_TORQUE_ENABLE] = 0;
    API_FOC_Torque_Enable();
    CHECK(foc_state == FOC_STATE_IDLE);

    test_primask = 1;
    calibration_active = true;
    API_FOC_Torque_Disable();
    CHECK(calibration_active && calibration_cancelled);
    CHECK(test_primask == 1);
    puts(failures ? "FOC transition regressions failed." : "FOC transition regressions passed.");
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
