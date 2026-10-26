#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../01-Firmware/B-G431B-ESC1-v3/Core/Src/control_table.c"

uint32_t test_primask;
void (*test_irq_restored)(void);
static unsigned failures;
static HAL_StatusTypeDef store_status = HAL_OK;
static uint8_t image[REG_TORQUE_ENABLE];
#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "Configuration check failed at line %d: %s\n", __LINE__, #condition); ++failures; \
} } while (0)
HAL_StatusTypeDef eeprom_store(uint8_t const *data, uint32_t size) { memcpy(image, data, size); return store_status; }
HAL_StatusTypeDef eeprom_restore(uint8_t *data, uint32_t size) { memcpy(data, image, size); return HAL_OK; }
static void defaults(void) { memset(regs, 0, sizeof regs); factory_reset_eeprom_regs(); }
static void word(unsigned address, int value) { regs[address] = LOW_BYTE(value); regs[address + 1] = HIGH_BYTE(value); }
int main(void)
{
    defaults();
    CHECK(control_configuration_valid());
    DriveConfig const config = control_read_config();
    CHECK(config.min_position_deg == -800 && config.max_position_deg == 800);
    CHECK(config.max_current_mA == 500 && config.encoder_bits == 12);
    store_status = HAL_ERROR;
    CHECK(store_eeprom_regs() == HAL_ERROR);
    CHECK(hardware_fault_status() == (1U << HW_ERROR_BIT_INTERNAL));
    hardware_fault_clear(0xff);
    CHECK(factory_reset_eeprom_regs() == HAL_ERROR);
    CHECK(hardware_fault_status() == (1U << HW_ERROR_BIT_INTERNAL));
    hardware_fault_clear(0xff);
    store_status = HAL_OK;
    CHECK(MAKE_SHORT(regs[REG_MAX_CURRENT_MA_L], regs[REG_MAX_CURRENT_MA_H]) == 500);
    CHECK(regs[REG_ENCODER_BITS] == 12);
    word(REG_MAX_CURRENT_MA_L, 0); CHECK(!control_configuration_valid());
    word(REG_MAX_CURRENT_MA_L, 1000); CHECK(control_configuration_valid());
    word(REG_MAX_CURRENT_MA_L, 1001); CHECK(!control_configuration_valid());
    word(REG_MAX_CURRENT_MA_L, 26000); CHECK(!control_configuration_valid());
    defaults(); word(REG_MAX_VELOCITY_DPS_L, 0); CHECK(!control_configuration_valid());
    word(REG_MAX_VELOCITY_DPS_L, -1); CHECK(!control_configuration_valid());
    word(REG_MAX_VELOCITY_DPS_L, 361); CHECK(!control_configuration_valid());
    defaults(); word(REG_MIN_POSITION_DEG_L, 900); CHECK(!control_configuration_valid());
    defaults(); word(REG_MAX_POSITION_DEG_L, 3277); CHECK(!control_configuration_valid());
    defaults(); regs[REG_ENCODER_BITS] = 14; CHECK(!control_configuration_valid());
    defaults(); regs[REG_MOTOR_POLE_PAIRS] = 0; CHECK(!control_configuration_valid());
    defaults(); regs[REG_INV_PHASE_MOTOR] = 2; CHECK(!control_configuration_valid());
    defaults(); regs[REG_LOW_VOLTAGE_LIMIT] = 0; CHECK(!control_configuration_valid());
    defaults(); regs[REG_HIGH_VOLTAGE_LIMIT] = 10; CHECK(!control_configuration_valid());
    defaults(); regs[REG_HIGH_VOLTAGE_LIMIT] = 255; CHECK(!control_configuration_valid());
    defaults(); regs[REG_TEMPERATURE_LIMIT] = 0; CHECK(!control_configuration_valid());
    defaults(); regs[REG_TEMPERATURE_LIMIT] = 71; CHECK(!control_configuration_valid());
    defaults(); word(REG_PID_FLUX_CURRENT_KP_L, -1); CHECK(!control_configuration_valid());
    defaults(); word(REG_PID_TORQUE_CURRENT_KI_L, -1); CHECK(!control_configuration_valid());
    defaults(); word(REG_PID_TORQUE_CURRENT_KP_L, 0); word(REG_PID_TORQUE_CURRENT_KI_L, 0);
    CHECK(!control_configuration_valid());
    word(REG_PID_TORQUE_CURRENT_KI_L, 10); CHECK(control_configuration_valid());
    puts(failures ? "Configuration regressions failed." : "Configuration regressions passed.");
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
