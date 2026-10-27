#ifndef INTEGRATION_TEST_HAL_H
#define INTEGRATION_TEST_HAL_H

// Share CAN wire definitions and Cortex-M critical-section equivalents.
#include "../../can/stubs/stm32g4xx_hal.h"

typedef struct { uint16_t counter; } TIM_HandleTypeDef;
typedef struct { unsigned unused; } UART_HandleTypeDef;
typedef struct { unsigned unused; } GPIO_TypeDef;
typedef enum { GPIO_PIN_RESET = 0, GPIO_PIN_SET } GPIO_PinState;
extern GPIO_TypeDef test_gpio_c;
#define GPIOC (&test_gpio_c)
#define GPIO_PIN_6 (1U << 6)
#define GPIO_PIN_10 (1U << 10)
#define GPIO_PIN_11 (1U << 11)
#define GPIO_PIN_14 (1U << 14)
#define __HAL_TIM_GET_COUNTER(timer) ((timer)->counter)
#define I2C_MEMADD_SIZE_8BIT 1U
static inline void __DMB(void) { }

HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *, uint16_t, uint16_t,
    uint16_t, uint8_t *, uint16_t, uint32_t);
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *, uint16_t, uint16_t,
    uint16_t, uint8_t *, uint16_t, uint32_t);
void HAL_GPIO_WritePin(GPIO_TypeDef *, uint16_t, GPIO_PinState);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *, uint16_t);
void HAL_Delay(uint32_t);
void HAL_NVIC_SystemReset(void);

#endif
