#ifndef SERVO_EMERGENCY_STOP_H
#define SERVO_EMERGENCY_STOP_H

#include "stm32g4xx_hal.h"

// Fatal paths cannot rely on initialized handles, interrupts or a working tick.
static inline void Servo_Emergency_Stop(void)
{
    __disable_irq();
    CLEAR_BIT(TIM1->BDTR, TIM_BDTR_MOE);
    __DSB();
}

#endif
