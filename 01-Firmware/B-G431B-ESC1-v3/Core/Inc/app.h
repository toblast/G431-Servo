#ifndef INC_APP_H_
#define INC_APP_H_

#include "serial.h"

// One foreground-owned application. Initialize after startup dependencies and
// CAN setup, before the first Process call. The timer must already run at 1 MHz
// with a 16-bit counter; serial must be the initialized protocol serial handler.
// Initialization resets scheduler/controller history, not accepted drive goals.
void App_Init(TIM_HandleTypeDef *timer, HAL_Serial_Handler *serial);

// Run one foreground pass. Not reentrant or callable from an IRQ. Watchdogs run
// first; one timer sample schedules at most one servo and one service update.
// Calibration remains blocking; serial work is bounded to its entry byte count.
void App_Process(void);

#endif
