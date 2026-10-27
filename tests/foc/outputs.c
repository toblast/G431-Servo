#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "foc_utils.h"
#include "control_table.h"

uint8_t regs[REG_MAX];
static TIM_TypeDef timer;
TIM_HandleTypeDef htim1 = { &timer };
static unsigned failures, writes, latched, enables;
#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "FOC output check failed at line %d: %s\n", __LINE__, #condition); ++failures; \
} } while (0)

void test_set_compare(TIM_HandleTypeDef *handle, unsigned channel, uint32_t value)
{
    if (channel == TIM_CHANNEL_1) writes = 0;
    ++writes;
    handle->Instance->preload[channel] = value;
}

void test_enable_outputs(TIM_HandleTypeDef *handle)
{
    CHECK(writes == 3);
    CHECK(latched != 0);
    for (unsigned i = 0; i < 3; ++i)
        CHECK(handle->Instance->active[i] >= 119 && handle->Instance->active[i] <= 1879);
    ++enables;
    handle->Instance->BDTR |= TIM_BDTR_MOE;
}

static void natural_timer_update(void)
{
    memcpy(timer.active, timer.preload, sizeof timer.active);
    ++latched;
}

int main(void)
{
    timer.CR1 = TIM_CR1_CEN;
    timer.CCER = 0x555;
    timer.ARR = 1999;
    timer.BDTR = TIM_BDTR_MOE;
    LL_FOC_coast();
    CHECK(!(timer.BDTR & TIM_BDTR_MOE));
    CHECK(timer.CR1 == TIM_CR1_CEN && timer.CCER == 0x555);

    LL_FOC_set_phase_voltage(2, 1, 1, 0, 12);
    CHECK(!(timer.BDTR & TIM_BDTR_MOE) && enables == 0);
    natural_timer_update();
    LL_FOC_set_phase_voltage(1, 0, 1, 0, 12);
    CHECK((timer.BDTR & TIM_BDTR_MOE) && enables == 1);

    regs[REG_HARDWARE_ERROR_STATUS] = 1;
    LL_FOC_set_phase_voltage(1, 0, 1, 0, 12);
    CHECK(!(timer.BDTR & TIM_BDTR_MOE));
    regs[REG_HARDWARE_ERROR_STATUS] = 0;
    LL_FOC_set_phase_voltage(1, 0, 1, 0, 12);
    CHECK(!(timer.BDTR & TIM_BDTR_MOE));
    natural_timer_update();
    LL_FOC_set_phase_voltage(1, 0, 1, 0, 12);
    CHECK(timer.BDTR & TIM_BDTR_MOE);

    regs[REG_HARDWARE_ERROR_STATUS] = 1U << HW_ERROR_BIT_POSITION_SENSOR_NOT_RESPONDING;
    LL_FOC_brake();
    CHECK(!(timer.BDTR & TIM_BDTR_MOE));

    const float invalid_bus[] = { 0, -1, NAN, INFINITY };
    for (unsigned i = 0; i < sizeof invalid_bus / sizeof invalid_bus[0]; ++i) {
        regs[REG_HARDWARE_ERROR_STATUS] = 0;
        writes = 0;
        LL_FOC_set_phase_voltage(1, 0, 1, 0, invalid_bus[i]);
        CHECK(!(timer.BDTR & TIM_BDTR_MOE) && writes == 0);
        CHECK(hardware_fault_status() & (1U << HW_ERROR_BIT_INTERNAL));
        CHECK(timer.CR1 == TIM_CR1_CEN && timer.CCER == 0x555);
    }
    LL_FOC_set_phase_voltage(NAN, 0, 1, 0, 12);
    CHECK(!(timer.BDTR & TIM_BDTR_MOE));
    puts(failures ? "FOC output regressions failed." : "FOC output regressions passed.");
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
