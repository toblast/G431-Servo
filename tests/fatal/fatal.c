#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "emergency_stop.h"
TIM_TypeDef test_tim1;
unsigned irq_disabled, barriers;
static jmp_buf stopped;
#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "Fatal regression line %d\n", __LINE__); exit(1); } } while (0)
static int fatal_loop(void)
{
    CHECK(irq_disabled && barriers == 1);
    CHECK(test_tim1.BDTR == 0x400U);
    longjmp(stopped, 1);
}
/* HANDLERS */
int main(void)
{
    void (*handlers[])(void) = { Error_Handler, NMI_Handler, HardFault_Handler,
        MemManage_Handler, BusFault_Handler, UsageFault_Handler };
    for (unsigned i = 0; i < sizeof handlers / sizeof *handlers; ++i) {
        test_tim1.BDTR = TIM_BDTR_MOE | 0x400U;
        irq_disabled = barriers = 0;
        if (!setjmp(stopped)) handlers[i]();
    }
    puts("All six fatal handlers cut MOE before halting.");
    return 0;
}
