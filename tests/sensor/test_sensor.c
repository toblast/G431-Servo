/* Production acquisition code runs unchanged against a deterministic I2C bus. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c"
#include "../../01-Firmware/B-G431B-ESC1-v3/Core/Src/position_sensor.c"

I2C_HandleTypeDef hi2c1;
TIM_HandleTypeDef htim4;
TIM_HandleTypeDef htim6;
uint8_t regs[REG_MAX];
uint32_t test_primask;

static uint8_t bus[256];
static int failed_read_reg = -1;
static int failed_write;
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
    memcpy(&bus[reg], data, count);
    return HAL_OK;
}

/* The unselected sensor is linked only to satisfy the production switch. */
void API_AS5048A_Position_Sensor_Init(TIM_HandleTypeDef *timer) { (void)timer; }
float API_AS5048A_Position_Sensor_Get_Radians(void) { abort(); }
float API_AS5048A_Position_Sensor_Get_Radians_Estimation(uint16_t time) { (void)time; abort(); }
float API_AS5048A_Position_Sensor_Get_Multiturn_Radians(void) { abort(); }
float API_AS5048A_Position_Sensor_Get_DPS(void) { abort(); }
uint16_t API_AS5048A_Position_Sensor_Get_DeltaTimestamp(void) { abort(); }
int16_t API_AS5048A_Position_Sensor_Get_DeltaTimeEstimation(void) { abort(); }

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
    acquisition_us = 0;
    test_primask = 0;
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





static void test_failed_runtime_sample_preserves_state(void)
{
    fixture(4090, 1000);
    CHECK(positionSensor_init(AS5600_I2C) == 1);
    htim6.counter = 1250;
    set_angle(4094);
    positionSensor_update();
    const float previous_angle = positionSensor_getRadiansMultiturn();
    const float previous_velocity = positionSensor_getVelocityDegree();

    htim6.counter = 1500;
    set_angle(2); /* failure must not count this as crossing the turn boundary */
    failed_read_reg = 0x0e;
    partial_read = 1;
    positionSensor_update();
    CHECK(positionSensor_getRadiansMultiturn() == previous_angle);
    CHECK(positionSensor_getVelocityDegree() == previous_velocity);
    CHECK(regs[REG_HARDWARE_ERROR_STATUS] & (1U << HW_ERROR_BIT_POSITION_SENSOR_STATUS_ERROR));

    /* 1250+1201 proves the rejected acquisition did not renew freshness. */
    positionSensor_getRadiansEstimation(2451);
    CHECK(regs[REG_HARDWARE_ERROR_STATUS] & (1U << HW_ERROR_BIT_POSITION_SENSOR_NOT_RESPONDING));

    failed_read_reg = -1;
    htim6.counter = 1750;
    set_angle(4);
    positionSensor_update();
    CHECK(close_to(positionSensor_getDegreeMultiturn(), 360.3515625f, 0.01f));
    CHECK(regs[REG_HARDWARE_ERROR_STATUS] & (1U << HW_ERROR_BIT_POSITION_SENSOR_STATUS_ERROR));
}











static int selected(const char *group, const char *name)
{
    return strcmp(group, "all") == 0 || strcmp(group, name) == 0;
}

int main(int argc, char **argv)
{
    const char *group = argc > 1 ? argv[1] : "all";
    if (selected(group, "read")) {
        test_driver_failure_keeps_caller_output();
        test_failed_runtime_sample_preserves_state();
    }
    if (failures) fprintf(stderr, "%u sensor regression assertion(s) failed\n", failures);
    else puts("Sensor regression checks passed");
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
