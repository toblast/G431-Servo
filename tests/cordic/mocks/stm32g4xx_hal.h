#ifndef CORDIC_TEST_HAL_H
#define CORDIC_TEST_HAL_H
#include <stdint.h>
typedef enum { HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
typedef struct { unsigned unused; } CORDIC_TypeDef;
typedef struct { CORDIC_TypeDef *Instance; unsigned State, ErrorCode; } CORDIC_HandleTypeDef;
typedef struct { unsigned Function, Scale, InSize, OutSize, NbWrite, NbRead, Precision; } CORDIC_ConfigTypeDef;
typedef struct { uint16_t counter; } TIM_HandleTypeDef;
#define __HAL_TIM_GET_COUNTER(timer) ((timer)->counter)
#define HAL_CORDIC_STATE_READY 1U
#define HAL_CORDIC_STATE_BUSY 2U
#define HAL_CORDIC_STATE_ERROR 3U
#define HAL_CORDIC_ERROR_TIMEOUT 4U
#define CORDIC_FUNCTION_COSINE 0U
#define CORDIC_SCALE_0 0U
#define CORDIC_INSIZE_32BITS 0U
#define CORDIC_OUTSIZE_32BITS 0U
#define CORDIC_NBWRITE_1 1U
#define CORDIC_NBREAD_2 2U
#define CORDIC_PRECISION_5CYCLES 5U
static inline uint32_t __get_PRIMASK(void) { return 0; }
static inline void __disable_irq(void) { }
static inline void __set_PRIMASK(uint32_t value) { (void)value; }
HAL_StatusTypeDef HAL_CORDIC_Configure(CORDIC_HandleTypeDef *, CORDIC_ConfigTypeDef *);
#endif
