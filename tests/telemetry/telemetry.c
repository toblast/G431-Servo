#include "app_fixture.h"
#include "binary_tool.h"
#include "telemetry.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "Telemetry line %d: %s\n", __LINE__, #condition); exit(1); } } while (0)
static int16_t read_word(unsigned address) { return (int16_t)MAKE_SHORT(regs[address], regs[address + 1]); }

int main(void)
{
    AppTest_Reset(0);
    regs[REG_MOVING_THRESHOLD_DPS] = 5;
    // Exercise counters through public passes, rather than injecting app state.
    // The final publication sees 4000 servo ticks and 123000 completed passes.
    for (unsigned i = 0; i < 119001; ++i) App_Process();
    for (unsigned i = 0; i < 4000; ++i) AppTest_Advance(250);
    CHECK(read_word(REG_PRESENT_POSITION_DEG_L) == 1234);
    CHECK(read_word(REG_PRESENT_VELOCITY_DPS_L) == -20 && read_word(REG_PRESENT_TORQUE_CURRENT_MA_L) == -500);
    CHECK(regs[REG_PRESENT_VOLTAGE] == 24 && regs[REG_PID_FREQUENCY] == 4 && regs[REG_MLP_FREQUENCY] == 123);
    app_test.position = 1e20f; app_test.velocity = INFINITY;
    app_test.torque = -INFINITY; app_test.flux = NAN;
    app_test.voltage = INFINITY; app_test.temperature = -30;
    app_test.processing_time = INFINITY; app_test.processing_frequency = INFINITY;
    app_test.setpoints.position_deg = -1e20f; app_test.setpoints.velocity_dps = INFINITY;
    app_test.setpoints.torque_mA = NAN; app_test.setpoints.flux_mA = -INFINITY;
    app_test.tick = 1;
    AppTest_Advance(250);
    CHECK(read_word(REG_PRESENT_POSITION_DEG_L) == INT16_MAX && read_word(REG_PRESENT_VELOCITY_DPS_L) == INT16_MAX);
    CHECK(read_word(REG_PRESENT_TORQUE_CURRENT_MA_L) == INT16_MIN && read_word(REG_PRESENT_FLUX_CURRENT_MA_L) == 0);
    CHECK(read_word(REG_SETPOINT_POSITION_DEG_L) == INT16_MIN && read_word(REG_SETPOINT_VELOCITY_DPS_L) == INT16_MAX);
    CHECK(read_word(REG_SETPOINT_TORQUE_CURRENT_MA_L) == 0 && read_word(REG_SETPOINT_FLUX_CURRENT_MA_L) == INT16_MIN);
    CHECK(regs[REG_PRESENT_VOLTAGE] == 255 && regs[REG_PRESENT_TEMPERATURE] == 0 && regs[REG_PROCESSING_TIME] == 255);
    CHECK(regs[REG_FOC_FREQUENCY] == 255 && regs[REG_PID_FREQUENCY] == 255 && regs[REG_MOVING] == 1);
    app_test.position = NAN; app_test.velocity = NAN;
    app_test.voltage = NAN; app_test.temperature = NAN; app_test.tick = 0;
    AppTest_Advance(250);
    CHECK(read_word(REG_PRESENT_POSITION_DEG_L) == 0 && regs[REG_MOVING] == 0);
    CHECK(regs[REG_PRESENT_VOLTAGE] == 0 && regs[REG_PRESENT_TEMPERATURE] == 0);
    CHECK(regs[REG_PID_FREQUENCY] == 0 && regs[REG_MLP_FREQUENCY] == 0 && test_primask == 0);
    CHECK(telemetry_i16(32767) == INT16_MAX && telemetry_i16(-32768) == INT16_MIN);
    CHECK(telemetry_i16(-1.9f) == -1 && telemetry_u8(254.9f) == 254);
    CHECK(app_test.errors == 0);
    puts("Production application telemetry saturation, nonfinite values and counters passed.");
    return 0;
}
