#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "configuration.h"
#include "control_table.h"
#include "binary_tool.h"
#include "drive.h"

uint32_t test_primask;
void (*test_irq_restored)(void);
static unsigned failures;
static unsigned store_calls;
static unsigned load_calls, stored_size, loaded_size;
static HAL_StatusTypeDef store_status = HAL_OK;
static HAL_StatusTypeDef load_status = HAL_OK;
static bool empty_page, armed, calibrating, maintenance;
static void (*on_store)(void);
static uint8_t image[REG_TORQUE_ENABLE];
#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "Configuration check failed at line %d: %s\n", __LINE__, #condition); ++failures; \
} } while (0)
bool eeprom_empty(void) { return empty_page; }
HAL_StatusTypeDef eeprom_store(uint8_t const *data, uint32_t size)
{
    CHECK(test_primask == 0 && size == sizeof image);
    ++store_calls; stored_size = size;
    if (on_store) on_store();
    memcpy(image, data, size); empty_page = false;
    return store_status;
}
HAL_StatusTypeDef eeprom_restore(uint8_t *data, uint32_t size)
{
    CHECK(test_primask == 0 && size == sizeof image);
    ++load_calls; loaded_size = size;
    if (load_status == HAL_OK) memcpy(data, image, size);
    return load_status;
}
bool Drive_BeginMaintenance(bool disarm_first)
{
    if (!disarm_first) CHECK(test_primask == 1);
    if (maintenance || calibrating || (!disarm_first && armed)) return false;
    maintenance = true;
    if (disarm_first) { armed = false; regs[REG_TORQUE_ENABLE] = 0; }
    return true;
}
void Drive_EndMaintenance(void) { maintenance = false; }
static void defaults(void)
{
    memset(regs, 0, sizeof regs);
    armed = calibrating = maintenance = false;
    control_reset_config_defaults();
}
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
    CHECK(!Config_Save() && hardware_fault_status() == DRIVE_FAULT_INTERNAL);
    hardware_fault_clear(0xff);
    store_status = HAL_OK;
    CHECK(Config_Save() && hardware_fault_status() == 0);
}

static void check_maintenance_during_store(void)
{
    CHECK(maintenance && !armed && !calibrating);
}
static void test_configuration_operations(void)
{
    defaults();
    store_status = load_status = HAL_OK;
    store_calls = load_calls = 0;
    DriveConfig candidate = control_read_config();
    CHECK(Config_Validate(&candidate) && !Config_Validate(NULL));
    candidate.max_current_mA = 1001;
    CHECK(!Config_Validate(&candidate) && Config_IsValid());

    // Only the 128-byte configuration prefix is persisted or applied.
    regs[REG_LED] = 9; regs[REG_GOAL_POS_KP] = 31;
    CHECK(Config_Save() && stored_size == CONFIG_IMAGE_SIZE);
    uint8_t const runtime_led = regs[REG_LED];
    word(REG_MAX_CURRENT_MA_L, 777);
    CHECK(Config_Load() && loaded_size == CONFIG_IMAGE_SIZE && store_calls == 1);
    CHECK(control_read_config().max_current_mA == 500 && regs[REG_LED] == runtime_led && regs[REG_GOAL_POS_KP] == 31);
    word(REG_MAX_CURRENT_MA_L, 26000);
    CHECK(Config_Save());
    control_reset_config_defaults();
    CHECK(Config_Load() && !Config_IsValid() && control_read_config().max_current_mA == 26000);

    defaults(); empty_page = true; store_calls = load_calls = 0;
    CHECK(Config_Load() && Config_IsValid() && store_calls == 1 && load_calls == 1);
    defaults(); empty_page = true; store_status = HAL_ERROR; load_calls = 0;
    CHECK(!Config_Load() && load_calls == 0 && hardware_fault_status() == DRIVE_FAULT_INTERNAL);
    defaults(); empty_page = false; load_status = HAL_ERROR; store_status = HAL_OK;
    word(REG_MAX_CURRENT_MA_L, 731);
    CHECK(!Config_Load() && control_read_config().max_current_mA == 731 && hardware_fault_status() == DRIVE_FAULT_INTERNAL);
    load_status = HAL_OK;

    defaults();
    uint8_t value = 0;
    on_store = check_maintenance_during_store;
    CHECK(Config_ApplyRegisterWrite(REG_ENCODER_BITS, 1, &value) == CONFIG_OK);
    CHECK(!Config_IsValid() && !maintenance); // Intermediate repair values remain supported.
    value = 12;
    CHECK(Config_ApplyRegisterWrite(REG_ENCODER_BITS, 1, &value) == CONFIG_OK && Config_IsValid());
    unsigned const before = store_calls;
    CHECK(Config_ApplyRegisterWrite(0, 0, &value) == CONFIG_ACCESS_ERROR);
    CHECK(Config_ApplyRegisterWrite(0, 1, NULL) == CONFIG_ACCESS_ERROR);
    CHECK(Config_ApplyRegisterWrite(REG_TORQUE_ENABLE - 1, 2, &value) == CONFIG_ACCESS_ERROR);
    CHECK(Config_ApplyRegisterWrite(REG_LED, 1, &value) == CONFIG_ACCESS_ERROR);
    armed = true;
    CHECK(Config_ApplyRegisterWrite(REG_ID, 1, &value) == CONFIG_BUSY);
    armed = false; calibrating = true;
    CHECK(Config_ApplyRegisterWrite(REG_ID, 1, &value) == CONFIG_BUSY);
    calibrating = false;
    CHECK(store_calls == before && regs[REG_ID] == REG_ID_VALUE);
    store_status = HAL_ERROR; value = 27;
    CHECK(Config_ApplyRegisterWrite(REG_ID, 1, &value) == CONFIG_STORAGE_ERROR);
    CHECK(regs[REG_ID] == 27 && !maintenance && hardware_fault_status() == DRIVE_FAULT_INTERNAL);

    defaults(); store_status = HAL_OK;
    regs[REG_RESERVED_RAM_3] = 0x5a; regs[REG_TORQUE_ENABLE - 1] = 0xab;
    regs[REG_LED] = 9; armed = true; regs[REG_TORQUE_ENABLE] = 1;
    CHECK(Config_FactoryReset() == CONFIG_OK && maintenance && !armed && !regs[REG_TORQUE_ENABLE]);
    CHECK(Config_IsValid() && regs[REG_LED] == 9 && regs[REG_RESERVED_RAM_3] == 0x5a && image[REG_TORQUE_ENABLE - 1] == 0xab);
    CHECK(Config_ApplyRegisterWrite(REG_ID, 1, &value) == CONFIG_BUSY);
    Drive_EndMaintenance();
    store_status = HAL_ERROR;
    CHECK(Config_FactoryReset() == CONFIG_STORAGE_ERROR && !maintenance && hardware_fault_status() == DRIVE_FAULT_INTERNAL);
    on_store = NULL;
    store_status = HAL_OK;
}
int main(void)
{
    test_typed_motor_and_fault_boundaries();
    defaults();
    CHECK(Config_IsValid());
    DriveConfig const config = control_read_config();
    CHECK(config.min_position_deg == -800 && config.max_position_deg == 800);
    CHECK(config.max_current_mA == 500 && config.encoder_bits == 12);
    store_status = HAL_ERROR;
    CHECK(!Config_Save());
    CHECK(hardware_fault_status() == (1U << HW_ERROR_BIT_INTERNAL));
    hardware_fault_clear(0xff);
    CHECK(Config_FactoryReset() == CONFIG_STORAGE_ERROR);
    CHECK(hardware_fault_status() == (1U << HW_ERROR_BIT_INTERNAL));
    hardware_fault_clear(0xff);
    store_status = HAL_OK;
    CHECK(MAKE_SHORT(regs[REG_MAX_CURRENT_MA_L], regs[REG_MAX_CURRENT_MA_H]) == 500);
    CHECK(regs[REG_ENCODER_BITS] == 12);
    word(REG_MAX_CURRENT_MA_L, 0); CHECK(!Config_IsValid());
    word(REG_MAX_CURRENT_MA_L, 1000); CHECK(Config_IsValid());
    word(REG_MAX_CURRENT_MA_L, 1001); CHECK(!Config_IsValid());
    word(REG_MAX_CURRENT_MA_L, 26000); CHECK(!Config_IsValid());
    defaults(); word(REG_MAX_VELOCITY_DPS_L, 0); CHECK(!Config_IsValid());
    word(REG_MAX_VELOCITY_DPS_L, -1); CHECK(!Config_IsValid());
    word(REG_MAX_VELOCITY_DPS_L, 361); CHECK(!Config_IsValid());
    defaults(); word(REG_MIN_POSITION_DEG_L, 900); CHECK(!Config_IsValid());
    defaults(); word(REG_MAX_POSITION_DEG_L, 3277); CHECK(!Config_IsValid());
    defaults(); regs[REG_ENCODER_BITS] = 14; CHECK(!Config_IsValid());
    defaults(); regs[REG_MOTOR_POLE_PAIRS] = 0; CHECK(!Config_IsValid());
    defaults(); regs[REG_INV_PHASE_MOTOR] = 2; CHECK(!Config_IsValid());
    defaults(); regs[REG_LOW_VOLTAGE_LIMIT] = 0; CHECK(!Config_IsValid());
    defaults(); regs[REG_HIGH_VOLTAGE_LIMIT] = 10; CHECK(!Config_IsValid());
    defaults(); regs[REG_HIGH_VOLTAGE_LIMIT] = 255; CHECK(!Config_IsValid());
    defaults(); regs[REG_TEMPERATURE_LIMIT] = 0; CHECK(!Config_IsValid());
    defaults(); regs[REG_TEMPERATURE_LIMIT] = 71; CHECK(!Config_IsValid());
    defaults(); word(REG_PID_FLUX_CURRENT_KP_L, -1); CHECK(!Config_IsValid());
    defaults(); word(REG_PID_TORQUE_CURRENT_KI_L, -1); CHECK(!Config_IsValid());
    defaults(); word(REG_PID_TORQUE_CURRENT_KP_L, 0); word(REG_PID_TORQUE_CURRENT_KI_L, 0);
    CHECK(!Config_IsValid());
    word(REG_PID_TORQUE_CURRENT_KI_L, 10); CHECK(Config_IsValid());
    test_configuration_operations();
    puts(failures ? "Configuration regressions failed." : "Configuration validation, application and storage regressions passed.");
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
