#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "cordic.h"
#include "control_table.h"
#include "binary_tool.h"
#include "pid.h"
#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "CORDIC check line %d: %s\n", __LINE__, #condition); exit(1); } } while (0)
static CORDIC_TypeDef accelerator;
CORDIC_HandleTypeDef hcordic = { &accelerator, HAL_CORDIC_STATE_READY, 0 };
uint8_t regs[REG_MAX];
TIM_HandleTypeDef htim6;
static unsigned writes, reads, polls, ready_at = 3, disables;
static bool allow_output;
static unsigned phase_updates, pi_calls;
static int32_t last_angle;
static float observed_kp[2], observed_ki[2], observed_limit[2];
HAL_StatusTypeDef HAL_CORDIC_Configure(CORDIC_HandleTypeDef *handle, CORDIC_ConfigTypeDef *config)
{
    CHECK(handle == &hcordic && config->NbWrite == 1 && config->NbRead == 2 && config->Precision == 5);
    return HAL_OK;
}
void LL_CORDIC_WriteData(CORDIC_TypeDef *handle, uint32_t value) { CHECK(handle == &accelerator); last_angle = (int32_t)value; ++writes; }
uint32_t LL_CORDIC_IsActiveFlag_RRDY(CORDIC_TypeDef *handle) { CHECK(handle == &accelerator); return ++polls == ready_at; }
uint32_t LL_CORDIC_ReadData(CORDIC_TypeDef *handle) { CHECK(handle == &accelerator); return ++reads == 1 ? 0x40000000U : 0x20000000U; }
uint32_t HAL_GetTick(void) { return 1; }
float positionSensor_getRadiansEstimation(uint16_t time) { (void)time; return 0; }
float positionSensor_getVelocityDps(void) { return 0; }
void pid_reset(pid_context_t *ctx) { (void)ctx; }
float pi_process_antiwindup_clamp(pid_context_t *ctx, float e, float p, float i, float limit)
{
    (void)ctx; (void)e;
    CHECK(allow_output && pi_calls < 2);
    observed_kp[pi_calls] = p; observed_ki[pi_calls] = i; observed_limit[pi_calls++] = limit;
    return 0;
}
void LL_FOC_coast(void) { }
void LL_FOC_brake(void) { CHECK(0); }
void LL_FOC_set_phase_voltage(float d, float q, float c, float s, float v)
{ (void)d; (void)q; (void)c; (void)s; CHECK(allow_output && v == 24); ++phase_updates; }
/* GLOBALS */
void API_FOC_Torque_Disable(void) { ++disables; foc_state = FOC_STATE_IDLE; }
/* UPDATE */
int main(void)
{
    float c = 8, s = 9;
    CHECK(API_CORDIC_Processor_Init() == HAL_OK);
    CHECK(RadiansToQ31(M_2PI / 2) == INT32_MIN);
    CHECK(API_CORDIC_Processor_Update(0, &c, &s) == HAL_OK);
    CHECK(c == .5f && s == .25f && reads == 2 && writes == 1 && polls == 3);
    c = 8; s = 9; polls = 0; ready_at = 0;
    CHECK(API_CORDIC_Processor_Update(0, &c, &s) == HAL_TIMEOUT);
    CHECK(c == 8 && s == 9 && polls == 64 && reads == 2);
    CHECK(hcordic.State == HAL_CORDIC_STATE_ERROR && hcordic.ErrorCode == HAL_CORDIC_ERROR_TIMEOUT);
    hcordic.State = HAL_CORDIC_STATE_READY;
    CHECK(API_CORDIC_Processor_Update(NAN, &c, &s) == HAL_ERROR);
    CHECK(API_CORDIC_Processor_Update(INFINITY, &c, &s) == HAL_ERROR);
    CHECK(API_CORDIC_Processor_Update(-1, &c, &s) == HAL_ERROR);
    CHECK(API_CORDIC_Processor_Update(7, &c, &s) == HAL_ERROR);
    CHECK(writes == 2);
    unsigned states[] = { FOC_STATE_IDLE, FOC_STATE_TORQUE_CONTROL, FOC_STATE_FLUX_CONTROL };
    for (unsigned i = 0; i < 3; ++i) {
        hardware_fault_clear(0xff);
        hcordic.State = HAL_CORDIC_STATE_BUSY;
        foc_state = states[i];
        API_FOC_Torque_Update();
        CHECK(disables == i + 1 && foc_state == FOC_STATE_IDLE);
        CHECK(hardware_fault_status() == (1U << HW_ERROR_BIT_INTERNAL));
    }
    hardware_fault_clear(0xff);
    regs[REG_MOTOR_SYNCHRO_L] = LOW_BYTE(-30); regs[REG_MOTOR_SYNCHRO_H] = HIGH_BYTE(-30);
    regs[REG_GOAL_SYNCHRO_OFFSET_L] = 60;
    regs[REG_MAX_VELOCITY_DPS_L] = 100;
    regs[REG_PID_FLUX_CURRENT_KP_L] = LOW_BYTE(250); regs[REG_PID_FLUX_CURRENT_KP_H] = HIGH_BYTE(250);
    regs[REG_PID_FLUX_CURRENT_KI_L] = LOW_BYTE(350); regs[REG_PID_FLUX_CURRENT_KI_H] = HIGH_BYTE(350);
    regs[REG_PID_TORQUE_CURRENT_KP_L] = LOW_BYTE(700); regs[REG_PID_TORQUE_CURRENT_KP_H] = HIGH_BYTE(700);
    regs[REG_PID_TORQUE_CURRENT_KI_L] = LOW_BYTE(900); regs[REG_PID_TORQUE_CURRENT_KI_H] = HIGH_BYTE(900);
    allow_output = true;
    present_voltage_V = 24;
    hcordic.State = HAL_CORDIC_STATE_READY; polls = reads = 0; ready_at = 1;
    foc_state = FOC_STATE_TORQUE_CONTROL;
    API_FOC_Torque_Update();
    CHECK(phase_updates == 1 && pi_calls == 2 && disables == 3);
    CHECK(llabs((long long)last_angle - 357913941LL) < 128); // -30 + 60 = 30 degrees.
    CHECK(fabsf(observed_kp[0] - 0.0025f) < 1e-7f && fabsf(observed_kp[1] - 0.007f) < 1e-7f);
    CHECK(fabsf(observed_ki[0] - 0.0000035f) < 1e-10f && fabsf(observed_ki[1] - 0.000009f) < 1e-10f);
    CHECK(observed_limit[0] == 24 && observed_limit[1] == 24);
    regs[REG_PID_FLUX_CURRENT_KP_L] = LOW_BYTE(251);
    pi_calls = polls = reads = 0;
    API_FOC_Torque_Update();
    CHECK(phase_updates == 2 && fabsf(observed_kp[0] - 0.00251f) < 1e-7f);
    puts("CORDIC failure paths and FOC typed gain/angle scaling passed.");
    return 0;
}
