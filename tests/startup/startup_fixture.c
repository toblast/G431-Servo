#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

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
static bool mock_button_down;
static bool mock_servo_disarmed = true;
static bool Drive_Is_Disarmed(void) { return mock_servo_disarmed; }
#define BUTTON_GPIO_Port 0
#define BUTTON_Pin 0
#define GPIO_PIN_RESET 0
static int HAL_GPIO_ReadPin(int port, int pin) { (void)port; (void)pin; return mock_button_down ? 0 : 1; }
static int failure_step, step;
static jmp_buf fatal_return;
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

static int next_status(void) { return ++step == failure_step ? 1 : HAL_OK; }
static int HAL_TIM_Base_Start(TIM_HandleTypeDef *timer) { assert(timer->Instance->BDTR == 0); return timer == &htim6 && failure_step == 8 ? 1 : next_status(); }
static void LL_FOC_coast(void) { htim1.Instance->BDTR = 0; }
static void Error_Handler(void) { assert(failure_step != 0); longjmp(fatal_return, 1); }
static int HAL_OPAMP_Start(OPAMP_HandleTypeDef *opamp) { (void)opamp; return next_status(); }
static int API_CORDIC_Processor_Init(void) { cordic_ready = true; return next_status(); }
static void API_FOC_Torque_Disable(void) { foc_ready = true; }
static int HAL_ADC_Start_DMA(ADC_HandleTypeDef *adc, uint32_t *data, unsigned count)
{
    (void)adc; (void)data; (void)count;
    /* A DMA completion may preempt startup as soon as acquisition starts. */
    assert(configuration_loaded && ram_reset && sensor_ready && cordic_ready);
    assert(!can_ready);
    adc_callbacks++;
    return next_status();
}
static bool eeprom_empty(void) { return true; }
static int factory_reset_eeprom_regs(void) { assert(!can_ready); return HAL_OK; }
static void load_eeprom_regs(void) { configuration_loaded = true; }
static void reset_ram_regs(void) { ram_reset = true; }
static void Drive_Init(void) { assert(ram_reset && !can_ready); }
static bool control_configuration_valid(void) { return true; }
#define HW_ERROR_BIT_CONFIGURATION 7
static void hardware_fault_set(uint8_t bits) { (void)bits; }
static void HAL_Serial_Init(int *uart, int *handler) { (void)uart; (void)handler; serial_ready = true; }
static int positionSensor_init(void) { sensor_ready = true; return 1; }
static void positionSensor_update(void) { assert(sensor_ready); }
static void API_FOC_Service_Update(void) { assert(foc_ready); service_ready = true; }
static void FDCAN_Config(void)
{
    assert(sensor_ready && cordic_ready && foc_ready && service_ready && serial_ready);
    can_ready = true;
}

/* FOC_INIT */

static void run_startup(void)
{
    /* STARTUP */
}

int main(void)
{
    run_startup();
    assert(adc_callbacks == 2 && can_ready);
    for (failure_step = 1; failure_step <= 7; ++failure_step) {
        step = 0;
        can_ready = false;
        if (setjmp(fatal_return) == 0) {
            API_FOC_Init();
            assert(false);
        }
        assert(step == failure_step);
        assert(htim1.Instance->BDTR == 0);
    }
    failure_step = 8;
    can_ready = false;
    if (setjmp(fatal_return) == 0) {
        run_startup();
        assert(false);
    }
    assert(!can_ready);
    /* BUTTON_STATE */
    bool const button_states[] = { true, true, false, true, true, false, true };
    bool const requests[] = { false, false, false, true, false, false, true };
    for (unsigned i = 0; i < sizeof button_states / sizeof *button_states; ++i) {
        mock_button_down = button_states[i];
        /* BUTTON_EDGE */
        assert(calibration_requested == requests[i]);
    }
    bool const armed_button_states[] = { false, true, true, false, true };
    bool const disarmed_states[] = { false, false, true, true, true };
    bool const armed_requests[] = { false, false, false, false, true };
    for (unsigned i = 0; i < sizeof armed_button_states / sizeof *armed_button_states; ++i) {
        mock_button_down = armed_button_states[i];
        mock_servo_disarmed = disarmed_states[i];
        /* BUTTON_EDGE */
        assert(calibration_requested == armed_requests[i]);
    }
    puts("Startup order, peripheral/timebase failures and held-button interlock passed.");
    return 0;
}
