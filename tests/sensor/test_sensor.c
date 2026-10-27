/* Production acquisition code runs unchanged against a deterministic I2C bus. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "control_table.h"

#include "../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c"
#include "../../01-Firmware/B-G431B-ESC1-v3/Core/Src/position_sensor.c"

I2C_HandleTypeDef hi2c1;
TIM_HandleTypeDef htim4;
TIM_HandleTypeDef htim6;
uint8_t regs[REG_MAX];
uint32_t test_primask;
void (*test_irq_restored)(void);
static unsigned observed_samples;

static uint8_t bus[256];
static int failed_read_reg = -1;
static int failed_write;
static int ignored_config_write;
static int partial_read;
static uint16_t acquisition_us;
static unsigned failures;

#define CHECK(expression) do { \
    if (!(expression)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expression); \
        ++failures; \
    } \
} while (0)

static int close_to(float a, float b, float tolerance)
{
    return isfinite(a) && fabsf(a - b) <= tolerance;
}

HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *handle, uint16_t address,
    uint16_t reg, uint16_t reg_size, uint8_t *data, uint16_t count, uint32_t timeout)
{
    (void)handle; (void)address; (void)reg_size; (void)timeout;
    CHECK(test_primask == 0); /* Blocking I2C must leave fast control interrupts enabled. */
    htim6.counter = (uint16_t)(htim6.counter + acquisition_us);
    if ((int)reg == failed_read_reg) {
        if (partial_read && count) data[0] = bus[reg];
        return HAL_ERROR;
    }
    memcpy(data, &bus[reg], count);
    return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *handle, uint16_t address,
    uint16_t reg, uint16_t reg_size, uint8_t *data, uint16_t count, uint32_t timeout)
{
    (void)handle; (void)address; (void)reg_size; (void)timeout;
    if (failed_write) return HAL_ERROR;
    if (ignored_config_write && reg == AS5600_CONF_REG_ADDR) return HAL_OK;
    memcpy(&bus[reg], data, count);
    return HAL_OK;
}

/* The unselected sensor is linked only to satisfy the production switch. */

static void set_angle(uint16_t angle)
{
    bus[0x0e] = (uint8_t)(angle >> 8);
    bus[0x0f] = (uint8_t)angle;
    bus[0x0c] = (uint8_t)(angle >> 8);
    bus[0x0d] = (uint8_t)angle;
}

static void fixture(uint16_t angle, uint16_t start_us)
{
    memset(bus, 0, sizeof bus);
    memset(regs, 0, sizeof regs);
    failed_read_reg = -1;
    failed_write = partial_read = 0;
    ignored_config_write = 0;
    acquisition_us = 0;
    test_primask = 0;
    test_irq_restored = NULL;
    observed_samples = 0;
    bus[0x0b] = 0x20; /* magnet detected, field strength in range */
    set_angle(angle);
    htim6.counter = start_us;
}

static void test_driver_failure_keeps_caller_output(void)
{
    fixture(0xabc, 0);
    AS5600_TypeDef encoder = { &hi2c1, 0x6c };
    uint16_t angle = 0x7654;
    failed_read_reg = 0x0e;
    CHECK(AS5600_GetAngle(&encoder, &angle) != HAL_OK);
    CHECK(angle == 0x7654);
    partial_read = 1;
    CHECK(AS5600_GetAngle(&encoder, &angle) != HAL_OK);
    CHECK(angle == 0x7654);

    failed_read_reg = 0x0c;
    CHECK(AS5600_GetRawAngle(&encoder, &angle) != HAL_OK);
    CHECK(angle == 0x7654);
}

static void test_configuration_preserves_zero_and_verifies_readback(void)
{
    fixture(1000, 100);
    bus[0x01] = 0x0a;
    bus[0x02] = 0xbc;
    CHECK(positionSensor_init() == 1);
    CHECK(bus[0x01] == 0x0a && bus[0x02] == 0xbc);
    CHECK(bus[0x07] == 0x03 && bus[0x08] == 0x00); /* 2x filter, nominal, WD off */

    fixture(1000, 100);
    ignored_config_write = 1;
    CHECK(positionSensor_init() == 0);
    CHECK(hardware_fault_status() & (1U << HW_ERROR_BIT_POSITION_SENSOR_STATUS_ERROR));
    fixture(1000, 100);
    failed_read_reg = AS5600_CONF_REG_ADDR;
    CHECK(positionSensor_init() == 0);
}

static void test_register_helpers_use_wire_byte_order(void)
{
    fixture(0, 0);
    AS5600_TypeDef encoder = { &hi2c1, 0x6c };
    uint16_t value = 0;
    CHECK(AS5600_SetStartPosition(&encoder, 0xabc) == HAL_OK);
    CHECK(bus[0x01] == 0x0a && bus[0x02] == 0xbc);
    CHECK(AS5600_GetStartPosition(&encoder, &value) == HAL_OK && value == 0xabc);
    CHECK(AS5600_SetStopPosition(&encoder, 0x123) == HAL_OK);
    CHECK(bus[0x03] == 0x01 && bus[0x04] == 0x23);
    CHECK(AS5600_GetStopPosition(&encoder, &value) == HAL_OK && value == 0x123);
    CHECK(AS5600_SetMaxAngle(&encoder, 0x456) == HAL_OK);
    CHECK(bus[0x05] == 0x04 && bus[0x06] == 0x56);
    CHECK(AS5600_GetMaxAngle(&encoder, &value) == HAL_OK && value == 0x456);
    value = 0x7654;
    failed_read_reg = AS5600_MAG_REG_ADDR;
    CHECK(AS5600_GetCORDICMagnitude(&encoder, &value) != HAL_OK && value == 0x7654);
    failed_read_reg = AS5600_ZPOS_REG_ADDR;
    CHECK(AS5600_GetStartPosition(&encoder, &value) != HAL_OK && value == 0x7654);
    failed_read_reg = AS5600_MPOS_REG_ADDR;
    CHECK(AS5600_GetStopPosition(&encoder, &value) != HAL_OK && value == 0x7654);
    failed_read_reg = AS5600_MANG_REG_ADDR;
    CHECK(AS5600_GetMaxAngle(&encoder, &value) != HAL_OK && value == 0x7654);
}

static void test_initialization_requires_valid_encoder(void)
{
    fixture(1000, 100);
    failed_write = 1;
    CHECK(positionSensor_init() == 0);
    CHECK(regs[REG_HARDWARE_ERROR_STATUS] & (1U << HW_ERROR_BIT_POSITION_SENSOR_STATUS_ERROR));

    fixture(1000, 100);
    bus[0x0b] = 0;
    CHECK(positionSensor_init() == 0);
    CHECK(regs[REG_HARDWARE_ERROR_STATUS] & (1U << HW_ERROR_BIT_POSITION_SENSOR_STATUS_ERROR));

    fixture(1000, 100);
    failed_read_reg = 0x0e;
    CHECK(positionSensor_init() == 0);
    CHECK(regs[REG_HARDWARE_ERROR_STATUS] & (1U << HW_ERROR_BIT_POSITION_SENSOR_STATUS_ERROR));
}

static void test_initial_sample_has_zero_velocity(void)
{
    fixture(3072, 1000);
    CHECK(positionSensor_init() == 1);
    CHECK(close_to(positionSensor_getDegreesMultiturn(), 270.0f, 0.01f));
    CHECK(close_to(positionSensor_getVelocityDps(), 0.0f, 0.01f));
    htim6.counter = 1250;
    positionSensor_update();
    CHECK(close_to(positionSensor_getDegreesMultiturn(), 270.0f, 0.01f));
    CHECK(close_to(positionSensor_getVelocityDps(), 0.0f, 0.01f));
}

static void test_failed_runtime_sample_preserves_state(void)
{
    fixture(4090, 1000);
    CHECK(positionSensor_init() == 1);
    htim6.counter = 1250;
    set_angle(4094);
    positionSensor_update();
    const float previous_angle = positionSensor_getRadiansMultiturn();
    const float previous_velocity = positionSensor_getVelocityDps();

    htim6.counter = 1500;
    set_angle(2); /* failure must not count this as crossing the turn boundary */
    failed_read_reg = 0x0e;
    partial_read = 1;
    positionSensor_update();
    CHECK(positionSensor_getRadiansMultiturn() == previous_angle);
    CHECK(positionSensor_getVelocityDps() == previous_velocity);
    CHECK(regs[REG_HARDWARE_ERROR_STATUS] & (1U << HW_ERROR_BIT_POSITION_SENSOR_STATUS_ERROR));

    /* 1250+1201 proves the rejected acquisition did not renew freshness. */
    positionSensor_getRadiansEstimation(2451);
    CHECK(regs[REG_HARDWARE_ERROR_STATUS] & (1U << HW_ERROR_BIT_POSITION_SENSOR_NOT_RESPONDING));

    failed_read_reg = -1;
    htim6.counter = 1750;
    set_angle(4);
    positionSensor_update();
    CHECK(close_to(positionSensor_getDegreesMultiturn(), 360.3515625f, 0.01f));
    CHECK(regs[REG_HARDWARE_ERROR_STATUS] & (1U << HW_ERROR_BIT_POSITION_SENSOR_STATUS_ERROR));
}

static void test_equal_timestamps_do_not_make_stale_or_infinite_velocity(void)
{
    fixture(1024, 5000);
    CHECK(positionSensor_init() == 1);
    CHECK(isfinite(positionSensor_getRadiansEstimation(5000)));
    CHECK(!(regs[REG_HARDWARE_ERROR_STATUS] & (1U << HW_ERROR_BIT_POSITION_SENSOR_NOT_RESPONDING)));
    set_angle(1025);
    positionSensor_update();
    CHECK(isfinite(positionSensor_getVelocityDps()));
    CHECK(close_to(positionSensor_getVelocityDps(), 0.0f, 0.01f));
}

static void test_wraparound_uses_modular_elapsed_time(void)
{
    fixture(1024, 65535);
    CHECK(positionSensor_init() == 1);
    htim6.counter = 0;
    set_angle(1025);
    positionSensor_update();
    CHECK(close_to(positionSensor_getVelocityDps(), 87890.625f, 10.0f));
    CHECK(isfinite(positionSensor_getRadiansEstimation(0)));
    CHECK(!(regs[REG_HARDWARE_ERROR_STATUS] & (1U << HW_ERROR_BIT_POSITION_SENSOR_NOT_RESPONDING)));
}

static void test_velocity_includes_acquisition_duration(void)
{
    fixture(1000, 1000);
    acquisition_us = 40;
    CHECK(positionSensor_init() == 1);
    /* Initialization reads status then angle; obtain the actual completion time. */
    const uint16_t previous_completion = htim6.counter;
    htim6.counter = (uint16_t)(previous_completion + 260);
    set_angle(1003);
    positionSensor_update();
    const uint16_t elapsed_us = (uint16_t)(htim6.counter - previous_completion);
    const float expected_dps = (3.0f * 360.0f / 4096.0f) * 1000000.0f / elapsed_us;
    CHECK(close_to(positionSensor_getVelocityDps(), expected_dps, 0.5f));
}

static void test_runtime_magnet_failure_preserves_sample(void)
{
    const uint8_t invalid_status[] = { 0, 0x28, 0x30, 0x20 };
    for (unsigned i = 0; i < sizeof invalid_status; ++i) {
        fixture(1024, 1000);
        CHECK(positionSensor_init() == 1);
        const float angle = positionSensor_getRadiansMultiturn();
        const float velocity = positionSensor_getVelocityDps();
        htim6.counter = 1250;
        set_angle(2048);
        bus[0x0b] = invalid_status[i];
        if (i == 3) failed_read_reg = 0x0b;
        positionSensor_update();
        CHECK(positionSensor_getRadiansMultiturn() == angle);
        CHECK(positionSensor_getVelocityDps() == velocity);
        CHECK(regs[REG_HARDWARE_ERROR_STATUS] & (1U << HW_ERROR_BIT_POSITION_SENSOR_STATUS_ERROR));
        CHECK(sensor->last_sample_us == 1000);
    }
}

static void observe_published_sample(void)
{
    ++observed_samples;
    CHECK(test_primask == 0);
    CHECK(close_to(positionSensor_getDegreesMultiturn(), 90.087890625f, 0.01f));
    CHECK(close_to(positionSensor_getVelocityDps(), 351.5625f, 0.5f));
    CHECK(sensor->last_sample_us == 1250);
    CHECK(sensor->angle_prev_rad == sensor->angle_rad);
}

static void test_publication_restores_interrupt_mask(void)
{
    fixture(1024, 1000);
    CHECK(positionSensor_init() == 1);
    htim6.counter = 1250;
    set_angle(1025);
    test_irq_restored = observe_published_sample;
    positionSensor_update();
    test_irq_restored = NULL;
    CHECK(observed_samples == 1);
    CHECK(test_primask == 0);
    CHECK(close_to(positionSensor_getDegreesMultiturn(), 90.087890625f, 0.01f));
}

static int selected(const char *group, const char *name)
{
    return strcmp(group, "all") == 0 || strcmp(group, name) == 0;
}

static void test_stale_fault_requires_disarm(void)
{
    fixture(1024, 1000);
    CHECK(positionSensor_init() == 1);
    regs[REG_TORQUE_ENABLE] = 1;
    positionSensor_getRadiansEstimation(2201);
    CHECK(hardware_fault_status() & (1U << HW_ERROR_BIT_POSITION_SENSOR_NOT_RESPONDING));
    htim6.counter = 2250;
    positionSensor_update();
    positionSensor_getRadiansEstimation(2251);
    CHECK(hardware_fault_status() & (1U << HW_ERROR_BIT_POSITION_SENSOR_NOT_RESPONDING));
    regs[REG_TORQUE_ENABLE] = 0;
    positionSensor_getRadiansEstimation(2252);
    CHECK(!(hardware_fault_status() & (1U << HW_ERROR_BIT_POSITION_SENSOR_NOT_RESPONDING)));
}

static void interrupt_sets_sensor_fault(void)
{
    test_irq_restored = NULL;
    hardware_fault_set(1U << HW_ERROR_BIT_POSITION_SENSOR_STATUS_ERROR);
}

static void interrupt_sets_overload(void)
{
    test_irq_restored = NULL;
    hardware_fault_set(1U << HW_ERROR_BIT_OVERLOAD);
}

static void test_fault_updates_preserve_other_sources(void)
{
    fixture(1024, 1000);
    test_irq_restored = interrupt_sets_sensor_fault;
    hardware_fault_set(1U << HW_ERROR_BIT_VOLTAGE);
    CHECK(hardware_fault_status() == ((1U << HW_ERROR_BIT_VOLTAGE) |
                                     (1U << HW_ERROR_BIT_POSITION_SENSOR_STATUS_ERROR)));
    test_irq_restored = interrupt_sets_overload;
    hardware_fault_clear(1U << HW_ERROR_BIT_VOLTAGE);
    CHECK(hardware_fault_status() == ((1U << HW_ERROR_BIT_OVERLOAD) |
                                     (1U << HW_ERROR_BIT_POSITION_SENSOR_STATUS_ERROR)));
    test_primask = 1;
    hardware_fault_set(1U << HW_ERROR_BIT_OVERHEATING);
    CHECK(test_primask == 1);
    test_primask = 0;
}

int main(int argc, char **argv)
{
    test_stale_fault_requires_disarm();
    test_fault_updates_preserve_other_sources();
    const char *group = argc > 1 ? argv[1] : "all";
    if (selected(group, "read")) {
        test_driver_failure_keeps_caller_output();
        test_failed_runtime_sample_preserves_state();
    }
    if (selected(group, "init")) {
        test_configuration_preserves_zero_and_verifies_readback();
        test_register_helpers_use_wire_byte_order();
        test_initialization_requires_valid_encoder();
    }
    if (selected(group, "timing")) {
        test_initial_sample_has_zero_velocity();
        test_equal_timestamps_do_not_make_stale_or_infinite_velocity();
        test_wraparound_uses_modular_elapsed_time();
        test_velocity_includes_acquisition_duration();
    }
    if (selected(group, "snapshot")) {
        test_publication_restores_interrupt_mask();
    }
    if (selected(group, "health")) {
        test_runtime_magnet_failure_preserves_sample();
    }
    if (failures) fprintf(stderr, "%u sensor regression assertion(s) failed\n", failures);
    else puts("Sensor regression checks passed");
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
