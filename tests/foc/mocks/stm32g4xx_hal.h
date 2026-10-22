#ifndef FOC_OUTPUT_TEST_HAL_H
#define FOC_OUTPUT_TEST_HAL_H
#include <stdint.h>
typedef enum { HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
typedef int ADC_HandleTypeDef;
typedef struct { uint32_t CR1, CCER, BDTR, ARR, preload[3], active[3]; } TIM_TypeDef;
typedef struct { TIM_TypeDef *Instance; } TIM_HandleTypeDef;
#define TIM_CHANNEL_1 0U
#define TIM_CHANNEL_2 1U
#define TIM_CHANNEL_3 2U
#define TIM_BDTR_MOE 0x8000U
#define TIM_CR1_CEN 1U
#define TIM_CR1_DIR 0x10U
#define __HAL_TIM_IS_TIM_COUNTING_DOWN(timer) (((timer)->Instance->CR1 & TIM_CR1_DIR) != 0)
#define __HAL_TIM_GET_AUTORELOAD(timer) ((timer)->Instance->ARR)
#define __HAL_TIM_MOE_DISABLE_UNCONDITIONALLY(timer) ((timer)->Instance->BDTR &= ~TIM_BDTR_MOE)
#define __HAL_TIM_MOE_ENABLE(timer) test_enable_outputs(timer)
#define __HAL_TIM_SET_COMPARE(timer, channel, value) test_set_compare(timer, channel, value)
void test_enable_outputs(TIM_HandleTypeDef *timer);
void test_set_compare(TIM_HandleTypeDef *timer, unsigned channel, uint32_t value);
static inline uint32_t __get_PRIMASK(void) { return 0; }
static inline void __disable_irq(void) { }
static inline void __set_PRIMASK(uint32_t value) { (void)value; }
#endif
