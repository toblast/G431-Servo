#ifndef INC_VEHICLE_INPUTS_H_
#define INC_VEHICLE_INPUTS_H_

#include "stm32g4xx_hal.h"
#include "cruise_control.h"

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin; // Exactly one pin, already electrically conditioned.
    GPIO_PinState active_level; // Switches only; speed always counts rising edges.
} VehicleInputPin;

typedef struct {
    bool wiring_verified, polarities_configured;
    uint16_t maximum_pulses_per_ms; // Required burst guard, 1..100; bench-verified.
    VehicleInputPin speed, brake, clutch, button;
} VehicleInputConfig;

// One foreground owner; init once before processing, with IRQs enabled.
// Copies settings. Invalid/unconfirmed wiring touches no GPIO/IRQ configuration.
bool VehicleInputs_Init(VehicleInputConfig const *config);
CruiseInputs VehicleInputs_Read(void); // Coherent counter, raw switch assertions.
// EXTI5..9/10..15 IRQ dispatcher and HAL callback; bounded integer work, no PID.
// Excessive pulses in one HAL tick latch invalid input and disable this IRQ
// until restart, so an edge storm cannot keep preempting foreground indefinitely.
void VehicleInputs_HandleExti(void);
void VehicleInputs_OnPulse(uint16_t pin);

#endif
