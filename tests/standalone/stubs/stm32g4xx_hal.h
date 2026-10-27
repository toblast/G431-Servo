#ifndef STANDALONE_TEST_HAL_H_
#define STANDALONE_TEST_HAL_H_
#include "../../integration/stubs/stm32g4xx_hal.h"
extern GPIO_TypeDef test_gpio_a, test_gpio_b;
#define GPIOA (&test_gpio_a)
#define GPIOB (&test_gpio_b)
#define GPIO_PIN_9 (1U << 9)
#define GPIO_PIN_15 (1U << 15)
typedef struct { uint32_t Pin, Mode, Pull, Speed, Alternate; } GPIO_InitTypeDef;
typedef enum { EXTI9_5_IRQn, EXTI15_10_IRQn } IRQn_Type;
#define GPIO_MODE_INPUT 0U
#define GPIO_MODE_IT_RISING 1U
#define GPIO_NOPULL 0U
void test_clock_enable(void);
#define __HAL_RCC_GPIOA_CLK_ENABLE() test_clock_enable()
#define __HAL_RCC_GPIOB_CLK_ENABLE() test_clock_enable()
void test_exti_clear(uint16_t pin);
#define __HAL_GPIO_EXTI_CLEAR_IT(pin) test_exti_clear(pin)
void HAL_GPIO_Init(GPIO_TypeDef *, GPIO_InitTypeDef *);
void HAL_NVIC_SetPriority(IRQn_Type, uint32_t, uint32_t);
void HAL_NVIC_EnableIRQ(IRQn_Type);
void HAL_NVIC_DisableIRQ(IRQn_Type);
void HAL_GPIO_EXTI_IRQHandler(uint16_t);
void HAL_GPIO_EXTI_Callback(uint16_t);
#endif
