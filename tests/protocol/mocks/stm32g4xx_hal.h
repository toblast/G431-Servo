#ifndef SERIAL_PROTOCOL_TEST_HAL_H
#define SERIAL_PROTOCOL_TEST_HAL_H
#include <stdint.h>
typedef enum { HAL_OK, HAL_ERROR } HAL_StatusTypeDef;
typedef struct { int unused; } UART_HandleTypeDef;
typedef struct { int unused; } TIM_HandleTypeDef;
typedef struct { int unused; } GPIO_TypeDef;
typedef enum { GPIO_PIN_RESET, GPIO_PIN_SET } GPIO_PinState;
extern uint32_t test_primask;
extern void (*before_irq_disable)(void);
static inline uint32_t __get_PRIMASK(void) { return test_primask; }
static inline void __disable_irq(void) {
    if (before_irq_disable) {
        void (*interrupt)(void) = before_irq_disable;
        before_irq_disable = 0;
        interrupt();
    }
    test_primask = 1;
}
static inline void __set_PRIMASK(uint32_t value) { test_primask = value; }
void HAL_Delay(uint32_t time);
void HAL_NVIC_SystemReset(void);
#endif
