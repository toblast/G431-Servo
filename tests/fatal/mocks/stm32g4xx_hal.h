#include <stdint.h>
typedef struct { uint32_t BDTR; } TIM_TypeDef;
extern TIM_TypeDef test_tim1;
#define TIM1 (&test_tim1)
#define TIM_BDTR_MOE 0x8000U
#define CLEAR_BIT(reg, bits) ((reg) &= ~(bits))
extern unsigned irq_disabled, barriers;
static inline void __disable_irq(void) { irq_disabled = 1; }
static inline void __DSB(void) { ++barriers; }
