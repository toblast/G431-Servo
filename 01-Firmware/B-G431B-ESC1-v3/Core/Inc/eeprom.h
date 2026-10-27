/*
 * eeprom.h
 *
 *  Created on: 13 déc. 2020
 *      Author: Patrick
 */

#ifndef INC_EEPROM_H_
#define INC_EEPROM_H_

#include <stdbool.h>
#include "stm32g4xx_hal.h"

// Existing single-page backend. Startup/foreground only; synchronous I/O with
// IRQs enabled and caller-owned maintenance/calibration exclusion for stores.
// Buffers are nonnull/borrowed, size is bytes (store requires a multiple of 8).
// HAL_OK (zero) means success; a failure does not promise flash rollback.
bool eeprom_empty(void);
HAL_StatusTypeDef eeprom_restore(uint8_t *regs, uint32_t size);
HAL_StatusTypeDef eeprom_store(uint8_t const *regs, uint32_t size);

uint8_t const *eeprom_base_address(void);

#endif /* INC_EEPROM_H_ */
