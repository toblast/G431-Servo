#ifndef INC_CAN_SERVO_H_
#define INC_CAN_SERVO_H_

#include "stm32g4xx_hal.h"

// Bind one controller and initialize transport state at startup, before FDCAN
// start/notifications. Peripheral filters, transceiver pins and HAL start remain
// the caller's responsibility. This operation does not change drive permission.
void CanServo_Init(FDCAN_HandleTypeDef *hfdcan);

// Forward the bound controller's FIFO0 callback here. IRQ-only, nonreentrant;
// drains at most the FIFO entries present at entry. Buffers belong to this
// module; command acceptance, watchdog history and motor state belong to Drive.
void CanServo_OnRxFifo0(FDCAN_HandleTypeDef *hfdcan, uint32_t events);

#endif
