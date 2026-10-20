#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define assert(condition) do { if (!(condition)) { \
    fprintf(stderr, "Startup dependency check failed at line %d: %s\n", __LINE__, #condition); \
    exit(1); \
} } while (0)

typedef struct { uint32_t CCER, BDTR; } TIM_TypeDef;
typedef struct { TIM_TypeDef *Instance; } TIM_HandleTypeDef;
typedef int ADC_HandleTypeDef;
typedef int OPAMP_HandleTypeDef;
static TIM_TypeDef timer1, timer6;
TIM_HandleTypeDef htim1 = { &timer1 }, htim6 = { &timer6 };
ADC_HandleTypeDef hadc1, hadc2;
OPAMP_HandleTypeDef hopamp1, hopamp2, hopamp3;
int huart2, serial;
uint32_t ADC1_DMA[5], ADC2_DMA[3];
uint8_t regs[1];
#define REG_TORQUE_ENABLE 0
static bool configuration_loaded, ram_reset, sensor_ready, cordic_ready;
static bool foc_ready, service_ready, serial_ready, can_ready;
static unsigned adc_callbacks;
#define SENSOR_TYPE 0
#define TIM_CHANNEL_1 1
#define TIM_CHANNEL_2 2
#define TIM_CHANNEL_3 3
#define HAL_OK 0
#define TIM_CCER_CC1E 1U
#define TIM_CCER_CC1NE 4U
#define TIM_CCER_CC2E 16U
#define TIM_CCER_CC2NE 64U
#define TIM_CCER_CC3E 256U
#define TIM_CCER_CC3NE 1024U
#define SET_BIT(reg, bits) ((reg) |= (bits))
#define __HAL_TIM_SET_COMPARE(timer, channel, value) ((void)0)

static int HAL_TIM_Base_Start(TIM_HandleTypeDef *timer) { assert(timer->Instance->BDTR == 0); return HAL_OK; }
static void LL_FOC_coast(void) { htim1.Instance->BDTR = 0; }
static void Error_Handler(void) { exit(1); }
static void HAL_OPAMP_Start(OPAMP_HandleTypeDef *opamp) { (void)opamp; }
static void API_CORDIC_Processor_Init(void) { cordic_ready = true; }
static void API_FOC_Torque_Disable(void) { foc_ready = true; }
static void HAL_ADC_Start_DMA(ADC_HandleTypeDef *adc, uint32_t *data, unsigned count)
{
    (void)adc; (void)data; (void)count;
    /* A DMA completion may preempt startup as soon as acquisition starts. */
    assert(configuration_loaded && ram_reset && sensor_ready && cordic_ready);
    assert(!can_ready);
    adc_callbacks++;
}
static bool eeprom_empty(void) { return true; }
static void factory_reset_eeprom_regs(void) { assert(!can_ready); }
static void load_eeprom_regs(void) { configuration_loaded = true; }
static void reset_ram_regs(void) { ram_reset = true; }
static void HAL_Serial_Init(int *uart, int *handler) { (void)uart; (void)handler; serial_ready = true; }
static int positionSensor_init(int type) { (void)type; sensor_ready = true; return 1; }
static void positionSensor_update(void) { assert(sensor_ready); }
static void API_FOC_Service_Update(void) { assert(foc_ready); service_ready = true; }
static void FDCAN_Config(void)
{
    assert(sensor_ready && cordic_ready && foc_ready && service_ready && serial_ready);
    can_ready = true;
}

/* FOC_INIT */

int main(void)
{
    /* STARTUP */
    assert(adc_callbacks == 2 && can_ready);
    puts("Startup callback-order regression passed.");
    return 0;
}
