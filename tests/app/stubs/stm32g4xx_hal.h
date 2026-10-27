#ifndef APP_TEST_HAL_H
#define APP_TEST_HAL_H

#include "../../sensor/mocks/stm32g4xx_hal.h"

typedef struct { unsigned unused; } UART_HandleTypeDef;
typedef struct { unsigned unused; } GPIO_TypeDef;
typedef enum { GPIO_PIN_RESET = 0, GPIO_PIN_SET } GPIO_PinState;
extern GPIO_TypeDef test_gpio_c;
#define GPIOC (&test_gpio_c)
#define GPIO_PIN_6 (1U << 6)
#define GPIO_PIN_10 (1U << 10)
#define GPIO_PIN_11 (1U << 11)
#define GPIO_PIN_14 (1U << 14)

#undef __HAL_TIM_GET_COUNTER
uint16_t test_app_clock_read(TIM_HandleTypeDef *timer);
#define __HAL_TIM_GET_COUNTER(timer) test_app_clock_read(timer)
uint32_t HAL_GetTick(void);
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin);

#endif
