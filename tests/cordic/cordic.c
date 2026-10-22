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
HAL_StatusTypeDef HAL_CORDIC_Configure(CORDIC_HandleTypeDef *handle, CORDIC_ConfigTypeDef *config)
{
    CHECK(handle == &hcordic && config->NbWrite == 1 && config->NbRead == 2 && config->Precision == 5);
    return HAL_OK;
}
void LL_CORDIC_WriteData(CORDIC_TypeDef *handle, uint32_t value) { CHECK(handle == &accelerator); (void)value; ++writes; }
uint32_t LL_CORDIC_IsActiveFlag_RRDY(CORDIC_TypeDef *handle) { CHECK(handle == &accelerator); return ++polls == ready_at; }
uint32_t LL_CORDIC_ReadData(CORDIC_TypeDef *handle) { CHECK(handle == &accelerator); return ++reads == 1 ? 0x40000000U : 0x20000000U; }
uint32_t HAL_GetTick(void) { return 1; }
float positionSensor_getRadiansEstimation(uint16_t time) { (void)time; return 0; }
float positionSensor_getVelocityDps(void) { return 0; }
void pid_reset(pid_context_t *ctx) { (void)ctx; }
float pi_process_antiwindup_clamp(pid_context_t *ctx, float e, float p, float i, float limit) { (void)ctx; (void)e; (void)p; (void)i; (void)limit; return 0; }
void LL_FOC_coast(void) { }
void LL_FOC_brake(void) { CHECK(0); }
void LL_FOC_set_phase_voltage(float d, float q, float c, float s, float v) { (void)d; (void)q; (void)c; (void)s; (void)v; CHECK(0); }
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
    puts("CORDIC bounded polling, invalid inputs and all three FOC failure paths passed.");
    return 0;
}
