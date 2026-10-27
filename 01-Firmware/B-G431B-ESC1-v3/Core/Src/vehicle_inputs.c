#include "vehicle_inputs.h"
#include "drive_mode.h"
#include <stddef.h>

static VehicleInputConfig board;
static volatile uint32_t pulse_count;
static volatile bool initialized, pulse_fault;
static uint32_t pulse_tick_ms;
static uint16_t pulses_in_tick;
static IRQn_Type counter_irq;

static bool valid_pin(VehicleInputPin pin)
{
    // Only unused PWM/Hall input and former CAN pins are candidates. In the
    // standalone build CAN is stopped. CAN pad circuitry still requires explicit
    // wiring verification/modification before those pads can become inputs.
    return (pin.port == GPIOA && (pin.pin == GPIO_PIN_11 || pin.pin == GPIO_PIN_15)) ||
           (pin.port == GPIOB && (pin.pin == GPIO_PIN_6 || pin.pin == GPIO_PIN_9));
}

bool VehicleInputs_Init(VehicleInputConfig const *config)
{
    // Reject reinitialization while a counter IRQ could be running.
    if (!DRIVE_STANDALONE || initialized || config == NULL || !config->wiring_verified ||
        !config->polarities_configured || config->maximum_pulses_per_ms == 0 ||
        config->maximum_pulses_per_ms > 100) return false;
    VehicleInputPin const pins[] = { config->speed, config->brake, config->clutch, config->button };
    for (unsigned i = 0; i < 4; ++i) {
        if (!valid_pin(pins[i])) return false;
        if (i != 0 && pins[i].active_level != GPIO_PIN_RESET &&
            pins[i].active_level != GPIO_PIN_SET) return false;
        for (unsigned j = 0; j < i; ++j)
            if (pins[i].port == pins[j].port && pins[i].pin == pins[j].pin) return false;
    }
    board = *config;
    pulse_count = 0;
    pulse_fault = false;
    pulse_tick_ms = HAL_GetTick();
    pulses_in_tick = 0;
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    for (unsigned i = 0; i < 4; ++i) {
        GPIO_InitTypeDef gpio = {0};
        gpio.Pin = pins[i].pin;
        gpio.Mode = i == 0 ? GPIO_MODE_IT_RISING : GPIO_MODE_INPUT;
        gpio.Pull = GPIO_NOPULL; // Defined levels must come from the input circuit.
        HAL_GPIO_Init(pins[i].port, &gpio);
    }
    counter_irq = board.speed.pin <= GPIO_PIN_9 ? EXTI9_5_IRQn : EXTI15_10_IRQn;
    __HAL_GPIO_EXTI_CLEAR_IT(board.speed.pin);
    initialized = true;
    // Lower priority than ADC/FOC, CORDIC, CAN and serial interrupts.
    HAL_NVIC_SetPriority(counter_irq, 5, 0);
    HAL_NVIC_EnableIRQ(counter_irq);
    return true;
}

CruiseInputs VehicleInputs_Read(void)
{
    CruiseInputs sample = {0};
    if (!initialized) return sample;
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    sample.pulse_count = pulse_count;
    sample.valid = !pulse_fault;
    __set_PRIMASK(primask);
    sample.brake_active = HAL_GPIO_ReadPin(board.brake.port, board.brake.pin) == board.brake.active_level;
    sample.clutch_active = HAL_GPIO_ReadPin(board.clutch.port, board.clutch.pin) == board.clutch.active_level;
    sample.button_active = HAL_GPIO_ReadPin(board.button.port, board.button.pin) == board.button.active_level;
    return sample;
}

void VehicleInputs_OnPulse(uint16_t pin)
{
    if (!initialized || pulse_fault || pin != board.speed.pin) return;
    uint32_t const now_ms = HAL_GetTick();
    if (now_ms != pulse_tick_ms) {
        pulse_tick_ms = now_ms;
        pulses_in_tick = 0;
    }
    if (++pulses_in_tick > board.maximum_pulses_per_ms) {
        pulse_fault = true;
        HAL_NVIC_DisableIRQ(counter_irq);
        return;
    }
    ++pulse_count;
}

void VehicleInputs_HandleExti(void)
{
    if (initialized) HAL_GPIO_EXTI_IRQHandler(board.speed.pin);
}

void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    VehicleInputs_OnPulse(pin);
}
