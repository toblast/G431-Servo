#include <stdio.h>
#include <stdlib.h>
#include "control_table.h"

uint8_t regs[REG_MAX];
static TIM_TypeDef timer;
TIM_HandleTypeDef htim1 = { &timer };
ADC_HandleTypeDef hadc1, hadc2, other_adc;
#define FOC_STATE_IDLE 0U
#define FOC_STATE_TORQUE_CONTROL 1U
#define FOC_STATE_FLUX_CONTROL 10U
#define ALPHA_CURRENT_SENSE_OFFSET 0.001f
static uint32_t foc_state;
static unsigned updates, trips;
/* ACQUISITION_GLOBALS */
static void API_FOC_Torque_Disable(void) { ++trips; foc_state = FOC_STATE_IDLE; }
static void FOC_Check_Current_Protection(void) { }
static void API_FOC_Torque_Update(void) { ++updates; }
/* ACQUISITION_FUNCTION */
#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "ADC acquisition line %d: %s\n", __LINE__, #condition); exit(1); } } while (0)

static void reset_fixture(uint32_t state)
{
    regs[REG_HARDWARE_ERROR_STATUS] = 0;
    current_sample_mask = 0;
    updates = trips = 0;
    foc_state = state;
    timer.CR1 = TIM_CR1_DIR;
}

int main(void)
{
    reset_fixture(FOC_STATE_TORQUE_CONTROL);
    API_FOC_It(&hadc1); CHECK(updates == 0);
    API_FOC_It(&hadc2); CHECK(updates == 1 && trips == 0);
    timer.CR1 = 0; API_FOC_It(&hadc1); API_FOC_It(&hadc2);
    timer.CR1 = TIM_CR1_DIR;
    API_FOC_It(&hadc2); API_FOC_It(&hadc1);
    CHECK(updates == 2 && trips == 0);
    API_FOC_It(&other_adc); CHECK(updates == 2);

    ADC_HandleTypeDef *missing_pairs[] = { &hadc1, &hadc2 };
    for (unsigned i = 0; i < 2; ++i) {
        reset_fixture(FOC_STATE_TORQUE_CONTROL);
        API_FOC_It(missing_pairs[i]); API_FOC_It(missing_pairs[i]);
        CHECK(updates == 0 && trips == 1 && hardware_fault_status() == (1U << HW_ERROR_BIT_INTERNAL));
        reset_fixture(FOC_STATE_FLUX_CONTROL);
        API_FOC_It(missing_pairs[i]);
        timer.CR1 = 0; API_FOC_It(missing_pairs[i]);
        CHECK(updates == 0 && trips == 1 && current_sample_mask == 0);
    }
    reset_fixture(FOC_STATE_IDLE);
    API_FOC_It(&hadc1);
    timer.CR1 = 0; API_FOC_It(&hadc1);
    CHECK(updates == 0 && trips == 0 && current_sample_mask == 0);
    timer.CR1 = TIM_CR1_DIR; API_FOC_It(&hadc2);
    CHECK(updates == 0); /* No pairing with ADC1 from the preceding cycle. */
    API_FOC_It(&hadc1); CHECK(updates == 1);
    puts("ADC pairing, missing streams and startup synchronization passed.");
    return 0;
}
