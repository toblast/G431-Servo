#ifndef SENSOR_TEST_STM32G4XX_HAL_H
#define SENSOR_TEST_STM32G4XX_HAL_H

#include <stdint.h>

typedef enum { HAL_OK = 0, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
typedef struct { unsigned unused; } I2C_HandleTypeDef;
typedef struct { uint16_t counter; } TIM_HandleTypeDef;

#define I2C_MEMADD_SIZE_8BIT 1U
#define __HAL_TIM_GET_COUNTER(timer) ((timer)->counter)

extern uint32_t test_primask;
extern void (*test_irq_restored)(void);
static inline uint32_t __get_PRIMASK(void) { return test_primask; }
static inline void __disable_irq(void) { test_primask = 1U; }
static inline void __enable_irq(void) { test_primask = 0U; }
static inline void __set_PRIMASK(uint32_t value) {
    test_primask = value;
    if (value == 0 && test_irq_restored) test_irq_restored();
}
static inline void __DMB(void) { }

HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *handle, uint16_t address,
    uint16_t reg, uint16_t reg_size, uint8_t *data, uint16_t count, uint32_t timeout);
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *handle, uint16_t address,
    uint16_t reg, uint16_t reg_size, uint8_t *data, uint16_t count, uint32_t timeout);

#endif
