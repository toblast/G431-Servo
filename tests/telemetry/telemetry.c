#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "control_table.h"
#include "binary_tool.h"
#include "telemetry.h"

uint8_t regs[REG_MAX];
uint32_t test_primask;
void (*test_irq_restored)(void);
static float position, velocity, torque, flux, voltage, temperature, processing_time, processing_frequency;
static float setpoint_position_deg, setpoint_velocity_dps, setpoint_torque_current_mA, setpoint_flux_current_mA;
static uint32_t pid_counter, mlp_counter, tick;
float positionSensor_getDegreeMultiturn(void) { return position; }
float positionSensor_getVelocityDegree(void) { return velocity; }
float API_FOC_Get_Present_Torque_Current(void) { return torque; }
float API_FOC_Get_Present_Flux_Current(void) { return flux; }
float API_FOC_Get_Present_Voltage(void) { return voltage; }
float API_FOC_Get_Present_Temp(void) { return temperature; }
float API_FOC_Get_Processing_Time(void) { return processing_time; }
float API_FOC_Get_Processing_Frequency(void) { return processing_frequency; }
uint32_t HAL_GetTick(void) { return tick; }
static void publish(void)
{
    /* UPDATE */
}
#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "Telemetry line %d: %s\n", __LINE__, #condition); exit(1); } } while (0)
static int16_t read_word(unsigned address) { return (int16_t)MAKE_SHORT(regs[address], regs[address + 1]); }

int main(void)
{
    position = 123.45f; velocity = -20.75f; torque = -500; flux = 123;
    voltage = 24; temperature = 30; tick = 1000;
    pid_counter = 4000; mlp_counter = 123000; processing_frequency = 40000;
    regs[REG_MOVING_THRESHOLD_DPS] = 5;
    publish();
    CHECK(read_word(REG_PRESENT_POSITION_DEG_L) == 1234);
    CHECK(read_word(REG_PRESENT_VELOCITY_DPS_L) == -20 && read_word(REG_PRESENT_TORQUE_CURRENT_MA_L) == -500);
    CHECK(regs[REG_PRESENT_VOLTAGE] == 24 && regs[REG_PID_FREQUENCY] == 4 && regs[REG_MLP_FREQUENCY] == 123);
    position = 1e20f; velocity = INFINITY; torque = -INFINITY; flux = NAN;
    voltage = INFINITY; temperature = -30; processing_time = INFINITY; processing_frequency = INFINITY;
    setpoint_position_deg = -1e20f; setpoint_velocity_dps = INFINITY;
    setpoint_torque_current_mA = NAN; setpoint_flux_current_mA = -INFINITY;
    tick = 1;
    publish();
    CHECK(read_word(REG_PRESENT_POSITION_DEG_L) == INT16_MAX && read_word(REG_PRESENT_VELOCITY_DPS_L) == INT16_MAX);
    CHECK(read_word(REG_PRESENT_TORQUE_CURRENT_MA_L) == INT16_MIN && read_word(REG_PRESENT_FLUX_CURRENT_MA_L) == 0);
    CHECK(read_word(REG_SETPOINT_POSITION_DEG_L) == INT16_MIN && read_word(REG_SETPOINT_VELOCITY_DPS_L) == INT16_MAX);
    CHECK(read_word(REG_SETPOINT_TORQUE_CURRENT_MA_L) == 0 && read_word(REG_SETPOINT_FLUX_CURRENT_MA_L) == INT16_MIN);
    CHECK(regs[REG_PRESENT_VOLTAGE] == 255 && regs[REG_PRESENT_TEMPERATURE] == 0 && regs[REG_PROCESSING_TIME] == 255);
    CHECK(regs[REG_FOC_FREQUENCY] == 255 && regs[REG_PID_FREQUENCY] == 255 && regs[REG_MOVING] == 1);
    position = NAN; velocity = NAN; voltage = NAN; temperature = NAN; tick = 0;
    publish();
    CHECK(read_word(REG_PRESENT_POSITION_DEG_L) == 0 && regs[REG_MOVING] == 0);
    CHECK(regs[REG_PRESENT_VOLTAGE] == 0 && regs[REG_PRESENT_TEMPERATURE] == 0);
    CHECK(regs[REG_PID_FREQUENCY] == 0 && regs[REG_MLP_FREQUENCY] == 0 && test_primask == 0);
    CHECK(telemetry_i16(32767) == INT16_MAX && telemetry_i16(-32768) == INT16_MIN);
    CHECK(telemetry_i16(-1.9f) == -1 && telemetry_u8(254.9f) == 254);
    puts("Production telemetry saturation, nonfinite values and startup counters passed.");
    return 0;
}
