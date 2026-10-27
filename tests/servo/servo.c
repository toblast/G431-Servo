#include "servo_controller.h"
#include "drive.h"
#include "control_table.h"
#include "configuration.h"
#include "binary_tool.h"
#include "foc.h"
#include "position_sensor.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

uint32_t test_primask;
static unsigned assertions, enables, disables, publications;
static float published_torque, published_flux;
static float position_deg, velocity_dps;
static uint32_t tick;

#define CHECK(condition) do { \
    ++assertions; \
    if (!(condition)) { \
        fprintf(stderr, "Servo line %d: %s\n", __LINE__, #condition); exit(1); \
    } \
} while (0)
#define NEAR(actual, expected) CHECK(fabsf((actual) - (expected)) < 0.001f)

void API_FOC_Torque_Enable(void) { CHECK(test_primask == 1); ++enables; }
void API_FOC_Torque_Disable(void) { CHECK(test_primask == 1); ++disables; }
void API_FOC_Set_Torque_Flux_Currents_mA(float torque, float flux)
{
    CHECK(test_primask == 1);
    ++publications;
    published_torque = torque;
    published_flux = flux;
}
bool API_FOC_Is_Calibrating(void) { return false; }
bool API_FOC_Check_Watchdog(void) { return true; }
int API_FOC_Calibrate(void) { return 0; }
int positionSensor_isReady(void) { return 1; }
float positionSensor_getDegreesMultiturn(void) { return position_deg; }
uint32_t HAL_GetTick(void) { return tick; }
HAL_StatusTypeDef eeprom_store(uint8_t const *data, uint32_t size)
{ (void)data; (void)size; return HAL_OK; }
HAL_StatusTypeDef eeprom_restore(uint8_t *data, uint32_t size)
{ (void)data; (void)size; return HAL_OK; }

static void store_word(unsigned address, int16_t value)
{
    regs[address] = LOW_BYTE(value);
    regs[address + 1] = HIGH_BYTE(value);
}

static void reset_fixture(bool reversed)
{
    test_primask = 0;
    control_reset_config_defaults();
    CHECK(Config_IsValid());
    reset_ram_regs();
    store_word(REG_MIN_POSITION_DEG_L, -30);
    store_word(REG_MAX_POSITION_DEG_L, 30);
    store_word(REG_MAX_VELOCITY_DPS_L, 100);
    regs[REG_INV_PHASE_MOTOR] = reversed;
    CHECK(Config_IsValid());
    position_deg = velocity_dps = 0;
    tick = 100;
    enables = disables = publications = 0;
    published_torque = published_flux = 0;
    Drive_Init();
    ServoController_Init();
}

static ServoSetpoints update(void)
{
    DriveCommandSnapshot const command = Drive_Get_Command();
    DriveConfig const config = control_read_config();
    return ServoController_Update(&command, &config, position_deg, velocity_dps);
}

static void submit(ServoCommand goal)
{
    CHECK(Drive_Submit_Command(&goal));
}

static void check_zero(ServoSetpoints result)
{
    NEAR(result.position_deg, 0);
    NEAR(result.velocity_dps, 0);
    NEAR(result.torque_mA, 0);
    NEAR(result.flux_mA, 0);
}

static void test_startup_gains_and_disable(void)
{
    reset_fixture(false);
    check_zero(update());
    CHECK(enables == 0 && disables == 0 && publications == 0);
    position_deg = 8;
    CHECK(Drive_Request_Arm());
    // A command arriving between ARM and the first servo tick must survive.
    submit((ServoCommand){ .position_tenths_deg = 100, .pos_kp = 3,
                          .torque_mA = 25, .flux_mA = -11 });
    ServoSetpoints const result = update();
    NEAR(result.position_deg, 10);
    NEAR(result.torque_mA, 37); // 2 degrees * (3 * 2) + 25 mA.
    NEAR(published_torque, 37);
    NEAR(published_flux, -11);
    CHECK(enables == 1 && publications == 1);
    update();
    CHECK(enables == 1 && publications == 2);
    Drive_Disarm();
    check_zero(update());
    CHECK(disables == 2 && publications == 2);
    check_zero(update());
    CHECK(disables == 2 && !Drive_Get_Status().armed);
}

static void test_limits_direction_and_feed_forward(void)
{
    reset_fixture(false);
    CHECK(Drive_Request_Arm());
    submit((ServoCommand){ .position_tenths_deg = 32767, .velocity_dps = 32767, .pos_kp = 255 });
    ServoSetpoints result = update();
    NEAR(result.position_deg, 30);
    NEAR(result.velocity_dps, 100);
    NEAR(result.torque_mA, 500);
    submit((ServoCommand){ .position_tenths_deg = -32768, .velocity_dps = -32768, .pos_kp = 255 });
    result = update();
    NEAR(result.position_deg, -30);
    NEAR(result.velocity_dps, -100);
    NEAR(result.torque_mA, -500);
    submit((ServoCommand){ .torque_mA = 700 });
    NEAR(update().torque_mA, 500);
    submit((ServoCommand){ .torque_mA = -700 });
    NEAR(update().torque_mA, -500);

    reset_fixture(true);
    CHECK(Drive_Request_Arm());
    submit((ServoCommand){ .position_tenths_deg = 100, .pos_kp = 3,
                          .torque_mA = 25, .flux_mA = -11 });
    result = update();
    NEAR(result.torque_mA, -85); // Reverse is applied to the complete Iq result.
    NEAR(published_torque, -85);
    NEAR(result.flux_mA, -11); // Id is passed through unchanged.
    submit((ServoCommand){ .torque_mA = 700 });
    NEAR(update().torque_mA, -500);
}

static void test_derivative_and_arm_history(void)
{
    reset_fixture(false);
    CHECK(Drive_Request_Arm());
    submit((ServoCommand){ .position_tenths_deg = 10, .pos_kd = 1 });
    NEAR(update().torque_mA, 20); // First sample: 200 * 0.1 * 1 degree.
    CHECK(Drive_Request_Arm()); // Duplicate ARM must retain derivative history.
    NEAR(update().torque_mA, 18);
    submit((ServoCommand){ .position_tenths_deg = 20, .pos_kd = 1 });
    NEAR(update().torque_mA, 36.2f);
    CHECK(enables == 1);

    // No disabled servo tick occurs between DISARM and the next ARM.
    Drive_Disarm();
    CHECK(Drive_Request_Arm());
    submit((ServoCommand){ .position_tenths_deg = 10, .pos_kd = 1 });
    NEAR(update().torque_mA, 20);
    CHECK(enables == 2);
    Drive_Disarm();
    check_zero(update());
    CHECK(Drive_Request_Arm());
    submit((ServoCommand){ .position_tenths_deg = 10, .pos_kd = 1 });
    NEAR(update().torque_mA, 20);
    CHECK(enables == 3);

    submit((ServoCommand){ .position_tenths_deg = 100, .pos_kp = 2 });
    for (unsigned i = 0; i < 64; ++i) NEAR(update().torque_mA, 40);
}

static void test_velocity_filter_and_can_compatibility(void)
{
    reset_fixture(false);
    CHECK(Drive_Request_Arm());
    velocity_dps = 2;
    // The internal numeric interface retains the existing velocity filter.
    submit((ServoCommand){ .velocity_dps = 10, .vel_kp = 10 });
    NEAR(update().torque_mA, 1.92f);
    NEAR(update().torque_mA, 3.3792f);
    Drive_Disarm();
    CHECK(Drive_Request_Arm());
    submit((ServoCommand){ .velocity_dps = 10, .vel_kp = 10 });
    NEAR(update().torque_mA, 1.92f);
    // CAN still supplies zero vel_kp: velocity alone adds no torque.
    submit((ServoCommand){ .velocity_dps = 100, .torque_mA = 7 });
    NEAR(update().torque_mA, 7);
}

static void test_invalidated_snapshots(void)
{
    reset_fixture(false);
    CHECK(Drive_Request_Arm());
    submit((ServoCommand){ .position_tenths_deg = 10, .pos_kd = 1 });
    update();
    DriveCommandSnapshot stale = Drive_Get_Command();
    DriveConfig const config = control_read_config();
    Drive_Disarm();
    CHECK(Drive_Request_Arm());
    submit((ServoCommand){ .position_tenths_deg = 10, .pos_kd = 1 });
    unsigned const previous_publications = publications;
    ServoController_Update(&stale, &config, position_deg, velocity_dps);
    CHECK(publications == previous_publications && enables == 1);
    NEAR(update().torque_mA, 20); // Rejected publication cannot commit old history.
    CHECK(enables == 2);

    // A disabled snapshot must not disable a newer ARM.
    Drive_Disarm();
    stale = Drive_Get_Command();
    CHECK(Drive_Request_Arm());
    submit((ServoCommand){ .torque_mA = 23 });
    unsigned const previous_disables = disables;
    check_zero(ServoController_Update(&stale, &config, position_deg, velocity_dps));
    CHECK(disables == previous_disables && Drive_Get_Status().armed);
    NEAR(update().torque_mA, 23);
    CHECK(enables == 3);

    // Faults arriving after the snapshot must still block current publication.
    stale = Drive_Get_Command();
    hardware_fault_set(1U << HW_ERROR_BIT_OVERLOAD);
    unsigned const before_fault = publications;
    ServoController_Update(&stale, &config, position_deg, velocity_dps);
    CHECK(publications == before_fault && enables == 3);
    Drive_Service();
    hardware_fault_clear(0xff);
    check_zero(update());
    CHECK(!Drive_Get_Status().armed && publications == before_fault);
    CHECK(test_primask == 0);
}

int main(void)
{
    test_startup_gains_and_disable();
    test_limits_direction_and_feed_forward();
    test_derivative_and_arm_history();
    test_velocity_filter_and_can_compatibility();
    test_invalidated_snapshots();
    printf("Servo controller: %u assertions passed.\n", assertions);
    return 0;
}
