#ifndef SERIAL_TRANSPORT_TEST_HAL_H
#define SERIAL_TRANSPORT_TEST_HAL_H
#include <stdint.h>
typedef enum { HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
typedef struct { uint32_t counter; } DMA_HandleTypeDef;
typedef struct { DMA_HandleTypeDef *hdmarx; uint32_t gState, ErrorCode; } UART_HandleTypeDef;
typedef enum { GPIO_PIN_RESET, GPIO_PIN_SET } GPIO_PinState;
typedef struct { GPIO_PinState state; } GPIO_TypeDef;
#define HAL_UART_STATE_READY 0U
#define HAL_UART_STATE_BUSY_TX 1U
#define HAL_UART_ERROR_DMA 1U
#define HAL_UART_ERROR_ORE 2U
#define __HAL_DMA_GET_COUNTER(dma) ((dma)->counter)
extern uint32_t test_primask;
static inline uint32_t __get_PRIMASK(void) { return test_primask; }
static inline void __disable_irq(void) { test_primask = 1; }
static inline void __set_PRIMASK(uint32_t value) { test_primask = value; }
HAL_StatusTypeDef HAL_UART_Receive_DMA(UART_HandleTypeDef *, uint8_t *, uint16_t);
HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef *, uint8_t const *, uint16_t);
void HAL_GPIO_WritePin(GPIO_TypeDef *, uint16_t, GPIO_PinState);
#endif
