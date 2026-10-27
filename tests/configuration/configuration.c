#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../01-Firmware/B-G431B-ESC1-v3/Core/Src/control_table.c"

uint32_t test_primask;
void (*test_irq_restored)(void);
static unsigned failures;
static unsigned store_calls;
static HAL_StatusTypeDef store_status = HAL_OK;
static uint8_t image[REG_TORQUE_ENABLE];
#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "Configuration check failed at line %d: %s\n", __LINE__, #condition); ++failures; \
} } while (0)
HAL_StatusTypeDef eeprom_store(uint8_t const *data, uint32_t size) { ++store_calls; memcpy(image, data, size); return store_status; }
HAL_StatusTypeDef eeprom_restore(uint8_t *data, uint32_t size) { memcpy(data, image, size); return HAL_OK; }
static void defaults(void) { memset(regs, 0, sizeof regs); factory_reset_eeprom_regs(); }
static void word(unsigned address, int value) { regs[address] = LOW_BYTE(value); regs[address + 1] = HIGH_BYTE(value); }

static void change_after_read(void)
{
    test_irq_restored = NULL;
    word(REG_MOTOR_SYNCHRO_L, 77);
    word(REG_PID_FLUX_CURRENT_KP_L, 321);
}
static void observe_alignment(void)
{
    test_irq_restored = NULL;
    CHECK(regs[REG_INV_PHASE_MOTOR] == 1 && regs[REG_MOTOR_SYNCHRO_L] == 0x85 && regs[REG_MOTOR_SYNCHRO_H] == 0xff);
}
static void test_typed_motor_and_fault_boundaries(void)
{
    defaults();
    word(REG_MOTOR_SYNCHRO_L, -123);
    word(REG_GOAL_SYNCHRO_OFFSET_L, 65535);
    word(REG_PID_FLUX_CURRENT_KP_L, -32768);
    word(REG_PID_FLUX_CURRENT_KI_L, 32767);
    word(REG_PID_TORQUE_CURRENT_KP_L, 250);
    word(REG_PID_TORQUE_CURRENT_KI_L, 900);
    word(REG_MAX_VELOCITY_DPS_L, -2);
    regs[REG_MOTOR_POLE_PAIRS] = 7; regs[REG_INV_PHASE_MOTOR] = 1;
    test_irq_restored = change_after_read;
    MotorControlConfig const motor = control_read_motor_config();
    CHECK(motor.phase_offset_deg == -123 && motor.phase_trim_deg == 65535);
    CHECK(motor.motor_pole_pairs == 7 && motor.invert_phase_motor == 1 && motor.max_velocity_dps == -2);
    CHECK(motor.flux_current_kp_raw == -32768 && motor.flux_current_ki_raw == 32767 &&
          motor.torque_current_kp_raw == 250 && motor.torque_current_ki_raw == 900);
    CHECK(control_read_motor_config().phase_offset_deg == 77 && control_read_motor_config().flux_current_kp_raw == 321);
    CHECK(test_primask == 0);
    MotorProtectionConfig const protection = control_read_protection_config();
    CHECK(protection.low_voltage_V == REG_LOW_VOLTAGE_LIMIT_VALUE &&
          protection.high_voltage_V == REG_HIGH_VOLTAGE_LIMIT_VALUE &&
          protection.temperature_limit_C == REG_TEMPERATURE_LIMIT_VALUE);

    MotorAlignment const alignment = { .phase_offset_deg = -123, .invert_phase_motor = 1 };
    unsigned const before = store_calls;
    test_irq_restored = observe_alignment;
    control_write_motor_alignment(&alignment);
    CHECK(store_calls == before && test_primask == 0);
    MotorAlignment const saved = control_read_motor_alignment();
    MotorAlignment const zero = { .phase_offset_deg = 0, .invert_phase_motor = 0 };
    control_write_motor_alignment(&zero);
    control_write_motor_alignment(&saved);
    CHECK(control_read_motor_alignment().phase_offset_deg == -123 && regs[REG_INV_PHASE_MOTOR] == 1);
    test_primask = 1;
    control_read_motor_config(); control_read_motor_alignment(); control_read_protection_config();
    control_write_motor_alignment(&zero);
    CHECK(test_primask == 1);
    test_primask = 0;

    regs[REG_TORQUE_ENABLE] = 0; CHECK(!control_torque_requested());
    regs[REG_TORQUE_ENABLE] = 2; CHECK(control_torque_requested());
    regs[REG_PROTOCOL_CRC_FAIL] = 255;
    control_record_sensor_error(); CHECK(regs[REG_PROTOCOL_CRC_FAIL] == 0);
    control_record_sensor_error(); CHECK(regs[REG_PROTOCOL_CRC_FAIL] == 1);

    DriveFaultFlags const flags[] = { DRIVE_FAULT_VOLTAGE, DRIVE_FAULT_ENCODER_READ,
        DRIVE_FAULT_ENCODER_STALE, DRIVE_FAULT_INTERNAL, DRIVE_FAULT_FOC_TIMEOUT,
        DRIVE_FAULT_OVERCURRENT, DRIVE_FAULT_OVERHEATING, DRIVE_FAULT_CONFIGURATION };
    hardware_fault_clear(0xff);
    for (unsigned i = 0; i < sizeof flags / sizeof *flags; ++i) {
        CHECK(flags[i] == (1U << i));
        hardware_fault_set(flags[i]);
    }
    CHECK(hardware_fault_status() == 0xff && regs[REG_HARDWARE_ERROR_STATUS] == 0xff);
    test_primask = 1;
    hardware_fault_clear(DRIVE_FAULT_VOLTAGE | DRIVE_FAULT_OVERCURRENT);
    CHECK(test_primask == 1 && hardware_fault_status() == 0xde);
    hardware_fault_set(DRIVE_FAULT_VOLTAGE);
    CHECK(test_primask == 1 && hardware_fault_status() == 0xdf);
    test_primask = 0;
    hardware_fault_clear(0xff);
    store_status = HAL_ERROR;
    CHECK(!control_persist_config() && hardware_fault_status() == DRIVE_FAULT_INTERNAL);
    hardware_fault_clear(0xff);
    store_status = HAL_OK;
    CHECK(control_persist_config() && hardware_fault_status() == 0);
}
int main(void)
{
    test_typed_motor_and_fault_boundaries();
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
