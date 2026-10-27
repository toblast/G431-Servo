#include "app.h"
#include "main.h"
#include "drive.h"
#include "servo_controller.h"
#include "can_servo.h"
#include "calibration.h"
#include "configuration.h"
#include "config_storage.h"
#include "control_table.h"
#include "motor_config.h"
#include "drive_faults.h"
#include "position_sensor.h"
#include "foc.h"
#include "protocol.h"
#include "pid.h"
#include "math_tool.h"
#include "binary_tool.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

I2C_HandleTypeDef hi2c1;
FDCAN_HandleTypeDef hfdcan1;
TIM_HandleTypeDef htim6;
HAL_Serial_Handler serial;
GPIO_TypeDef test_gpio_c;
uint32_t test_primask;

static unsigned scenarios;
#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "Integration scenario %u, line %d: %s\n", scenarios, __LINE__, #condition); exit(1); \
} } while (0)
#define NEAR(actual, expected, tolerance) CHECK(fabsf((actual) - (expected)) <= (tolerance))

static uint64_t clock_us;
static uint32_t stopped_foc_ms;
static bool heartbeat_running, encoder_connected, follow_flux, button_down;
static float rotor_direction, fixed_angle_rad, measured_iq_mA;
static unsigned angle_reads, services, coasts, stores, resets, prints, can_replies, serial_replies;
static uint8_t sensor_bus[256], uart_rx[128], last_serial_reply[128], last_can_reply[4];
static unsigned uart_size, uart_read;
static int last_calibration_result;
static ConfigImage flash_image;
static bool fail_store, fail_tx;
static void (*during_i2c)(void);
static void (*during_store)(void);
static FDCAN_RxHeaderTypeDef queued_header;
static uint8_t queued_data[8];
static bool rx_pending;

/* FOC_GLOBALS */
// Hardware boundary only: observe immediate coast requests, not physical gates.
static void LL_FOC_coast(void) { ++coasts; }
/* FOC_FUNCTIONS */

uint32_t HAL_GetTick(void) { return (uint32_t)(clock_us / 1000); }
uint32_t API_FOC_Get_Timestamp_ms(void)
{ return heartbeat_running ? HAL_GetTick() : stopped_foc_ms; }
float API_FOC_Get_Present_Torque_Current(void) { CHECK(test_primask == 1); return measured_iq_mA; }
float API_FOC_Get_Present_Flux_Current(void) { CHECK(test_primask == 1); return 0; }
float API_FOC_Get_Present_Voltage(void) { CHECK(test_primask == 1); return 24; }
float API_FOC_Get_Present_Temp(void) { CHECK(test_primask == 1); return 30; }
float API_FOC_Get_Processing_Time(void) { return 5; }
float API_FOC_Get_Processing_Frequency(void) { return 40000; }
void API_FOC_Service_Update(void) { CHECK(test_primask == 0); ++services; }

static void set_clock(uint64_t time_us)
{
    clock_us = time_us;
    htim6.counter = (uint16_t)clock_us;
}
static void step(void)
{
    set_clock(clock_us + 250);
    App_Process();
    CHECK(test_primask == 0);
}
static void run_us(uint32_t duration_us)
{
    CHECK(duration_us % 250 == 0);
    while (duration_us) { step(); duration_us -= 250; }
}

HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *handle, uint16_t address,
    uint16_t reg, uint16_t reg_size, uint8_t *data, uint16_t count, uint32_t timeout)
{
    (void)timeout;
    CHECK(test_primask == 0 && handle == &hi2c1 && address == 0x6c);
    CHECK(reg_size == I2C_MEMADD_SIZE_8BIT && reg + count <= sizeof sensor_bus);
    if (during_i2c) {
        void (*interrupt)(void) = during_i2c;
        during_i2c = NULL;
        interrupt(); // CAN can preempt synchronous foreground acquisition.
    }
    if (!encoder_connected) { data[0] = 0xff; return HAL_ERROR; }
    if (reg == 0x0e) {
        ++angle_reads;
        float angle_rad = fixed_angle_rad;
        if (follow_flux && foc_state == FOC_STATE_FLUX_CONTROL)
            angle_rad = 1.0f + rotor_direction * setpoint_electrical_angle_rad / 7.0f;
        uint16_t const counts = (uint16_t)lroundf(normalize_angle(angle_rad) * 4096 / M_2PI) & 4095;
        sensor_bus[0x0e] = HIGH_BYTE(counts);
        sensor_bus[0x0f] = LOW_BYTE(counts);
    }
    memcpy(data, &sensor_bus[reg], count);
    return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *handle, uint16_t address,
    uint16_t reg, uint16_t reg_size, uint8_t *data, uint16_t count, uint32_t timeout)
{
    (void)timeout;
    CHECK(test_primask == 0 && handle == &hi2c1 && address == 0x6c);
    CHECK(reg_size == I2C_MEMADD_SIZE_8BIT && reg + count <= sizeof sensor_bus);
    if (!encoder_connected) return HAL_ERROR;
    memcpy(&sensor_bus[reg], data, count);
    return HAL_OK;
}
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state)
{ (void)state; CHECK(port == STATUS_GPIO_Port && pin == STATUS_Pin); }
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
    CHECK(port == BUTTON_GPIO_Port && pin == BUTTON_Pin);
    return button_down ? GPIO_PIN_RESET : GPIO_PIN_SET;
}

uint32_t HAL_FDCAN_GetRxFifoFillLevel(FDCAN_HandleTypeDef *handle, uint32_t fifo)
{ CHECK(handle == &hfdcan1 && fifo == FDCAN_RX_FIFO0); return rx_pending ? 1 : 0; }
HAL_StatusTypeDef test_can_get_rx_message(FDCAN_HandleTypeDef *handle, uint32_t fifo,
    FDCAN_RxHeaderTypeDef *header, uint8_t *data, size_t capacity)
{
    CHECK(handle == &hfdcan1 && fifo == FDCAN_RX_FIFO0 && capacity >= 8 && rx_pending);
    *header = queued_header;
    memcpy(data, queued_data, queued_header.DataLength >> 16);
    rx_pending = false;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_FDCAN_AddMessageToTxFifoQ(FDCAN_HandleTypeDef *handle,
    FDCAN_TxHeaderTypeDef *header, uint8_t *data)
{
    CHECK(handle == &hfdcan1 && header->Identifier == 0x10U + regs[REG_ID]);
    CHECK(header->DataLength == FDCAN_DLC_BYTES_4);
    memcpy(last_can_reply, data, sizeof last_can_reply);
    ++can_replies;
    return fail_tx ? HAL_ERROR : HAL_OK;
}
static void receive(uint8_t const data[8], uint32_t id, uint32_t dlc)
{
    CHECK(!rx_pending);
    memcpy(queued_data, data, sizeof queued_data);
    queued_header = (FDCAN_RxHeaderTypeDef){
        .Identifier = id, .DataLength = dlc, .IdType = FDCAN_STANDARD_ID,
        .RxFrameType = FDCAN_DATA_FRAME, .FDFormat = FDCAN_CLASSIC_CAN,
        .BitRateSwitch = FDCAN_BRS_OFF
    };
    rx_pending = true;
    CanServo_OnRxFifo0(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE);
    CHECK(!rx_pending);
}
static void can_arm(void)
{
    uint8_t const data[8] = {255,255,255,255,255,255,255,255};
    receive(data, regs[REG_ID], FDCAN_DLC_BYTES_8);
}
static void can_disarm(void)
{
    uint8_t const data[8] = {0};
    receive(data, regs[REG_ID], FDCAN_DLC_BYTES_8);
}
static void command(int16_t position, int16_t velocity, int16_t feedforward,
                    uint8_t kp, uint8_t kd)
{
    uint8_t const data[8] = {LOW_BYTE(position), HIGH_BYTE(position),
        LOW_BYTE(velocity), HIGH_BYTE(velocity), LOW_BYTE(feedforward),
        HIGH_BYTE(feedforward), kp, kd};
    receive(data, regs[REG_ID], FDCAN_DLC_BYTES_8);
}
static void arm_during_store(void)
{
    can_arm();
    CHECK(!Drive_GetStatus().armed);
}

bool eeprom_empty(void) { return false; }
HAL_StatusTypeDef eeprom_restore(uint8_t *data, uint32_t size)
{ CHECK(size == sizeof flash_image); memcpy(data, flash_image.bytes, size); return HAL_OK; }
HAL_StatusTypeDef eeprom_store(uint8_t const *data, uint32_t size)
{
    CHECK(test_primask == 0 && size == sizeof flash_image && !Drive_GetStatus().armed);
    CHECK(Drive_IsMaintenance() || API_FOC_Is_Calibrating());
    if (API_FOC_Is_Calibrating()) CHECK(foc_state == FOC_STATE_IDLE && setpoint_flux_voltage_V == 0);
    ++stores;
    if (during_store) during_store();
    if (fail_store) return HAL_ERROR;
    memcpy(flash_image.bytes, data, size);
    return HAL_OK;
}

int HAL_Serial_Available(HAL_Serial_Handler *handler)
{ CHECK(handler == &serial); return (int)(uart_size - uart_read); }
char HAL_Serial_GetChar(HAL_Serial_Handler *handler)
{ CHECK(handler == &serial && uart_read < uart_size); return (char)uart_rx[uart_read++]; }
int HAL_Serial_Write(HAL_Serial_Handler *handler, uint8_t const *data, int length)
{
    CHECK(handler == &serial && length >= 11 && (unsigned)length <= sizeof last_serial_reply);
    memcpy(last_serial_reply, data, length);
    ++serial_replies;
    return length;
}
int HAL_Serial_Print(HAL_Serial_Handler *handler, char const *format, ...)
{
    CHECK(handler == &serial);
    char buffer[160];
    va_list args;
    va_start(args, format);
    int const length = vsnprintf(buffer, sizeof buffer, format, args);
    va_end(args);
    CHECK(sscanf(buffer, "Calibration result: %d ", &last_calibration_result) == 1);
    ++prints;
    return length;
}
void HAL_Delay(uint32_t ms)
{
    CHECK(!Drive_GetStatus().armed && Drive_IsMaintenance() && foc_state == FOC_STATE_IDLE);
    set_clock(clock_us + ms * 1000U); // The foreground really blocks in this existing reset delay.
}
void HAL_NVIC_SystemReset(void)
{ CHECK(!Drive_GetStatus().armed && Drive_IsMaintenance()); ++resets; }
static void serial_request(uint8_t instruction, uint8_t const *parameters, unsigned count)
{
    CHECK(uart_read == uart_size && count + 10 <= sizeof uart_rx);
    uint8_t const header[] = {255,255,253,0,1,0,0};
    memcpy(uart_rx, header, sizeof header);
    uart_rx[4] = regs[REG_ID];
    uart_rx[5] = LOW_BYTE(count + 3);
    uart_rx[6] = HIGH_BYTE(count + 3);
    uart_rx[7] = instruction;
    if (count) memcpy(&uart_rx[8], parameters, count);
    uint16_t const crc = updateCRC(0, uart_rx, count + 8);
    uart_rx[count + 8] = LOW_BYTE(crc);
    uart_rx[count + 9] = HIGH_BYTE(crc);
    uart_read = 0; uart_size = count + 10;
    run_us(10000);
    CHECK(uart_read == uart_size);
}

static void reset_fixture(bool connected)
{
    if (API_FOC_Is_Calibrating()) { Drive_Disarm(); Calibration_Update(false); }
    test_primask = 0;
    during_i2c = during_store = NULL;
    heartbeat_running = true;
    encoder_connected = connected;
    follow_flux = button_down = fail_store = fail_tx = rx_pending = false;
    fixed_angle_rad = rotor_direction = 1.0f;
    measured_iq_mA = -45;
    uart_size = uart_read = 0;
    prints = can_replies = serial_replies = services = angle_reads = stores = resets = 0;
    last_calibration_result = -1;
    memset(sensor_bus, 0, sizeof sensor_bus);
    sensor_bus[0x0b] = 0x20;
    set_clock(0);
    control_reset_config_defaults();
    regs[REG_MOTOR_POLE_PAIRS] = 7; // Simulated rotor, not a hardware default.
    regs[REG_INV_PHASE_MOTOR] = 0;
    regs[REG_MOTOR_SYNCHRO_L] = 0x23; regs[REG_MOTOR_SYNCHRO_H] = 1;
    control_capture_config(&flash_image);
    CHECK(Config_Load());
    control_reset_runtime();
    Drive_Init();
    API_FOC_Torque_Disable();
    CHECK(positionSensor_init() == connected);
    CanServo_Init(&hfdcan1);
    App_Init(&htim6, &serial);
    coasts = 0;
}
static void idle(void)
{
    CHECK(!Drive_GetCommand().enabled && foc_state == FOC_STATE_IDLE);
    CHECK(setpoint_torque_current_mA == 0 && setpoint_flux_current_mA == 0);
}
static void alignment_restored(void)
{
    MotorAlignment const alignment = control_read_motor_alignment();
    CHECK(alignment.phase_offset_deg == 0x123 && alignment.invert_phase_motor == 0);
}
static void finish_calibration(int expected)
{
    unsigned limit = 20000;
    while (!prints && limit--) step();
    CHECK(prints == 1 && last_calibration_result == expected && limit != 0);
    CHECK(!API_FOC_Is_Calibrating());
    idle();
    run_us(10000);
    CHECK(prints == 1);
}
static void start_calibration(void)
{
    run_us(10000); // Release the startup button interlock.
    button_down = true;
    run_us(10000);
    CHECK(API_FOC_Is_Calibrating() && !Drive_GetStatus().armed && prints == 0);
}

static void test_startup_and_command_flow(void)
{
    ++scenarios;
    reset_fixture(false);
    can_arm(); run_us(20000);
    CHECK(Drive_GetStatus().state == DRIVE_FAULT && hardware_fault_status() == DRIVE_FAULT_ENCODER_READ);
    idle();
    reset_fixture(true);
    button_down = true; run_us(20000);
    CHECK(!API_FOC_Is_Calibrating() && prints == 0); idle();
    can_arm();
    CHECK(Drive_GetStatus().armed && foc_state == FOC_STATE_IDLE);
    int16_t const target = (int16_t)(positionSensor_getDegreesMultiturn() * 10) + 100;
    command(target, 1000, 25, 3, 0);
    step();
    NEAR(setpoint_torque_current_mA, 6 * (target / 10.0f - positionSensor_getDegreesMultiturn()) + 25, 0.001f);
    CHECK(foc_state == FOC_STATE_TORQUE_CONTROL && regs[REG_TORQUE_ENABLE] == 1);
    CHECK((int16_t)MAKE_SHORT(regs[REG_SETPOINT_VELOCITY_DPS_L], regs[REG_SETPOINT_VELOCITY_DPS_H]) == 360);
    uint32_t const generation = Drive_GetCommand().generation;
    can_arm();
    CHECK(Drive_GetCommand().generation == generation && Drive_GetCommand().goal.position_tenths_deg == target);
    CHECK((int16_t)MAKE_SHORT(last_can_reply[0], last_can_reply[1]) == (int16_t)(positionSensor_getDegreesMultiturn() * 10));
    CHECK((int16_t)MAKE_SHORT(last_can_reply[2], last_can_reply[3]) == -45);
    command(32767, 0, 0, 255, 0); step();
    CHECK(setpoint_torque_current_mA == 500);
    unsigned const before = coasts;
    can_disarm(); idle(); CHECK(coasts > before);
    command(0, 0, 100, 0, 0); step(); idle();
}
static void test_timeout_and_rollover(void)
{
    ++scenarios;
    reset_fixture(true);
    set_clock(((uint64_t)UINT32_MAX - 500) * 1000);
    // Reinitialize time origins through startup APIs, without touching private deadlines.
    CHECK(positionSensor_init()); App_Init(&htim6, &serial);
    can_arm(); command(100, 0, 75, 0, 0);
    uint32_t const accepted_ms = Drive_GetStatus().last_command_ms;
    uint8_t const invalid[8] = {1,2,3,4,5,6,7,8};
    for (unsigned i = 0; i < 100; ++i) {
        run_us(10000);
        receive(invalid, regs[REG_ID], FDCAN_DLC_BYTES_4);
        receive(invalid, regs[REG_ID] + 1, FDCAN_DLC_BYTES_8);
    }
    CHECK(Drive_GetStatus().armed && Drive_GetStatus().last_command_ms == accepted_ms);
    run_us(1000);
    idle(); CHECK(!Drive_GetStatus().armed && hardware_fault_status() == 0);
    can_arm(); step(); CHECK(Drive_GetStatus().armed);
    stopped_foc_ms = HAL_GetTick(); heartbeat_running = false;
    run_us(11000);
    idle(); CHECK(hardware_fault_status() == DRIVE_FAULT_FOC_TIMEOUT);
    heartbeat_running = true; can_arm(); CHECK(!Drive_GetStatus().armed);
}
static void test_stale_calculation_and_acquisition_interrupt(void)
{
    ++scenarios;
    reset_fixture(true);
    can_arm(); command(100, 0, 100, 0, 1); step();
    DriveCommandSnapshot const stale = Drive_GetCommand();
    can_disarm(); can_arm(); command(200, 0, 50, 0, 0);
    DriveConfig const config = control_read_config();
    ServoController_Update(&stale, &config, positionSensor_getDegreesMultiturn(), 0);
    CHECK(setpoint_torque_current_mA == 0 && Drive_GetCommand().generation != stale.generation);
    step(); CHECK(setpoint_torque_current_mA == 50);
    during_i2c = can_disarm;
    step(); idle(); CHECK(!Drive_GetStatus().armed);
    can_arm(); command(300, 0, 75, 0, 0); step();
    CHECK(setpoint_torque_current_mA == 75); // Old derivative/filter history was reset.
}
static void test_encoder_failure_and_tx_failure(void)
{
    ++scenarios;
    reset_fixture(true);
    can_arm(); fail_tx = true; command(100, 0, 123, 0, 0); step();
    CHECK(setpoint_torque_current_mA == 123 && Drive_GetStatus().armed);
    float const position = positionSensor_getDegreesMultiturn();
    unsigned const before = coasts;
    encoder_connected = false; step();
    idle(); CHECK(coasts > before && positionSensor_getDegreesMultiturn() == position);
    step(); CHECK(!Drive_GetStatus().armed && hardware_fault_status() == DRIVE_FAULT_ENCODER_READ);
    encoder_connected = true; run_us(10000); can_arm();
    CHECK(!Drive_GetStatus().armed); // Restored traffic does not clear a latched sensor fault.
}
static void test_recoverable_fault_needs_explicit_rearm(void)
{
    ++scenarios;
    reset_fixture(true);
    can_arm(); command(100, 0, 123, 0, 0); step();
    hardware_fault_set(DRIVE_FAULT_VOLTAGE); // Model a protection observation between passes.
    step(); idle(); CHECK(!Drive_GetStatus().armed && Drive_GetStatus().state == DRIVE_FAULT);
    hardware_fault_clear(DRIVE_FAULT_VOLTAGE); // Model the protection service's recovery.
    command(100, 0, 123, 0, 0); run_us(10000);
    idle(); CHECK(Drive_GetStatus().state == DRIVE_DISARMED);
    can_arm(); step();
    CHECK(Drive_GetStatus().armed && setpoint_torque_current_mA == 0);
}
static void test_calibration_ownership_and_serial_interlocks(void)
{
    ++scenarios;
    reset_fixture(true);
    run_us(10000); // Button released.
    can_arm(); command(100, 0, 100, 0, 1); step();
    can_disarm();
    CHECK(Drive_StartCalibration()); // No disabled controller tick between ARM and calibration.
    follow_flux = true;
    run_us(250000);
    CHECK(API_FOC_Is_Calibrating() && foc_state == FOC_STATE_FLUX_CONTROL);
    CHECK(setpoint_flux_voltage_V == 3 && prints == 0);
    unsigned const reads_before = angle_reads, services_before = services;
    can_arm(); command(100, 0, 50, 0, 0);
    CHECK(!Drive_GetStatus().armed && hardware_fault_status() == 0);
    uint8_t const write[] = {REG_MAX_CURRENT_MA_L, 0, 42};
    serial_request(3, write, sizeof write);
    CHECK(last_serial_reply[8] == 7 && regs[REG_MAX_CURRENT_MA_L] == LOW_BYTE(500) && stores == 0);
    unsigned const replies = serial_replies;
    serial_request(6, NULL, 0); serial_request(8, NULL, 0);
    CHECK(serial_replies == replies && resets == 0 && stores == 0 && API_FOC_Is_Calibrating());
    serial_request(1, NULL, 0);
    CHECK(serial_replies == replies + 1 && last_serial_reply[8] == 0);
    CHECK(angle_reads > reads_before && services > services_before);
    can_disarm();
    CHECK(foc_state == FOC_STATE_IDLE && API_FOC_Is_Calibrating());
    finish_calibration(CALIBRATION_CANCELLED); alignment_restored();
    can_arm(); command(100, 0, 75, 0, 0); step();
    CHECK(Drive_GetStatus().armed && setpoint_torque_current_mA == 75);
}
static void test_calibration_completion_and_storage(void)
{
    ++scenarios;
    for (int direction = 1; direction >= -1; direction -= 2) {
        reset_fixture(true); follow_flux = true; rotor_direction = (float)direction;
        start_calibration();
        during_store = arm_during_store;
        finish_calibration(CALIBRATION_SUCCESS);
        CHECK(stores == 1 && hardware_fault_status() == 0);
        MotorAlignment const alignment = control_read_motor_alignment();
        CHECK(alignment.invert_phase_motor == (direction < 0));
        CHECK(abs(alignment.phase_offset_deg - (direction > 0 ? 318 : 41)) <= 1);
        CHECK(flash_image.bytes[REG_MOTOR_SYNCHRO_L] == LOW_BYTE(alignment.phase_offset_deg));
        CHECK(flash_image.bytes[REG_INV_PHASE_MOTOR] == alignment.invert_phase_motor);
        CHECK(!Drive_GetStatus().armed);
        during_store = NULL;
        can_arm(); step(); CHECK(Drive_GetStatus().armed && hardware_fault_status() == 0);
    }
    reset_fixture(true); follow_flux = true; fail_store = true;
    start_calibration(); finish_calibration(CALIBRATION_STORAGE_ERROR);
    CHECK(stores == 1 && hardware_fault_status() == DRIVE_FAULT_INTERNAL); alignment_restored();
    reset_fixture(true); follow_flux = true; during_store = can_disarm;
    start_calibration(); finish_calibration(CALIBRATION_CANCELLED);
    CHECK(stores == 1 && hardware_fault_status() == 0); alignment_restored();
    CHECK(flash_image.bytes[REG_MOTOR_SYNCHRO_L] != LOW_BYTE(0x123)); // No flash rollback claimed.
}
static void test_calibration_failures(void)
{
    ++scenarios;
    reset_fixture(true); start_calibration();
    finish_calibration(CALIBRATION_NO_MOTION);
    CHECK(stores == 0); alignment_restored();
    reset_fixture(true); follow_flux = true; regs[REG_MOTOR_POLE_PAIRS] = 14;
    start_calibration(); finish_calibration(CALIBRATION_POLE_MISMATCH);
    CHECK(stores == 0); alignment_restored();
    reset_fixture(true); follow_flux = true; start_calibration(); run_us(250000);
    encoder_connected = false;
    finish_calibration(CALIBRATION_CANCELLED);
    CHECK(stores == 0 && hardware_fault_status() == DRIVE_FAULT_ENCODER_READ); alignment_restored();
    reset_fixture(true); follow_flux = true; start_calibration(); run_us(250000);
    stopped_foc_ms = HAL_GetTick(); heartbeat_running = false;
    finish_calibration(CALIBRATION_CANCELLED);
    CHECK(stores == 0 && hardware_fault_status() == DRIVE_FAULT_FOC_TIMEOUT); alignment_restored();
}
static void test_configuration_reset_and_storage_fault(void)
{
    ++scenarios;
    reset_fixture(true);
    uint8_t const write[] = {REG_MAX_CURRENT_MA_L, 0, 42};
    can_arm(); command(100, 0, 50, 0, 0); step();
    serial_request(3, write, sizeof write);
    CHECK(last_serial_reply[8] == 7 && stores == 0 && Drive_GetStatus().armed);
    can_disarm();
    during_store = arm_during_store;
    serial_request(3, write, sizeof write);
    CHECK(stores == 1 && last_serial_reply[8] == 0 && !Drive_GetStatus().armed && !Drive_IsMaintenance());
    CHECK(control_read_config().max_current_mA == 298 && flash_image.bytes[REG_MAX_CURRENT_MA_L] == 42);
    during_store = NULL;
    run_us(10000); can_arm(); CHECK(Drive_GetStatus().armed);
    command(100, 0, 50, 0, 0); step();
    serial_request(8, NULL, 0);
    CHECK(resets == 1 && Drive_IsMaintenance()); idle();
    can_arm(); CHECK(!Drive_GetStatus().armed);
    reset_fixture(true); fail_store = true;
    serial_request(3, write, sizeof write);
    CHECK(stores == 1 && last_serial_reply[8] == 7 && !Drive_IsMaintenance());
    CHECK(control_read_config().max_current_mA == 298 && hardware_fault_status() == DRIVE_FAULT_INTERNAL);
    can_arm(); CHECK(!Drive_GetStatus().armed);
    reset_fixture(true); fail_store = true; can_arm(); step();
    serial_request(6, NULL, 0);
    CHECK(last_serial_reply[8] == 1 && stores == 1 && resets == 0 && !Drive_IsMaintenance());
    CHECK(hardware_fault_status() == DRIVE_FAULT_INTERNAL); idle();
    reset_fixture(true); can_arm(); step();
    serial_request(6, NULL, 0);
    CHECK(stores == 1 && resets == 1 && Drive_IsMaintenance()); idle();
    CHECK(flash_image.bytes[REG_ENCODER_BITS] == 12 && flash_image.bytes[REG_MAX_CURRENT_MA_L] == LOW_BYTE(500));
}

int main(void)
{
    test_startup_and_command_flow();
    test_timeout_and_rollover();
    test_stale_calculation_and_acquisition_interrupt();
    test_encoder_failure_and_tx_failure();
    test_recoverable_fault_needs_explicit_rearm();
    test_calibration_ownership_and_serial_interlocks();
    test_calibration_completion_and_storage();
    test_calibration_failures();
    test_configuration_reset_and_storage_fault();
    printf("Firmware integration: %u scenario groups passed.\n", scenarios);
    return 0;
}
