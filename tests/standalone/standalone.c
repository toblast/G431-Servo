// Synthetic wiring/calibration only; these are not motorcycle defaults.
#include "standalone.h"
#include "app.h"
#include "can_servo.h"
#include "configuration.h"
#include "control_table.h"
#include "drive.h"
#include "foc.h"
#include "servo_controller.h"
#include "position_sensor.h"
#include "calibration.h"
#include "protocol.h"
#include "main.h"
#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(c) do { ++checks; if (!(c)) { \
    fprintf(stderr, "Standalone line %d: %s\n", __LINE__, #c); exit(1); } } while (0)
#define NEAR(a, b) CHECK(fabsf((a) - (b)) < 0.001f)

uint32_t test_primask;
GPIO_TypeDef test_gpio_a, test_gpio_b, test_gpio_c;
static uint64_t clock_us;
static uint16_t level_a, level_b, pending_exti;
static unsigned gpio_setups, irq_enables, clock_enables, coasts, current_publications;
static unsigned irq_disables;
static bool calibration, rx_pending;
static float rotor_deg = 5;
static uint8_t rx_data[8];
static FDCAN_HandleTypeDef can;
static TIM_HandleTypeDef timer;
static HAL_Serial_Handler serial;
#ifndef TEST_REVERSE_TRAVEL
#define TEST_REVERSE_TRAVEL 0
#endif

StandaloneConfig const standalone_config = {
    .cruise = { .wheel_circumference_m = 2, .pulses_per_revolution = 50,
        .minimum_speed_mps = 5, .maximum_speed_mps = 60,
        .kp = 0.1f, .ki = 0.05f, .kd = 0.01f, .derivative_filter_s = 0.1f,
        .period_ms = 20, .maximum_gap_ms = 100, .speed_window_ms = 100,
        .speed_timeout_ms = 500, .button_debounce_ms = 30 },
    .inputs = { .wiring_verified = true, .polarities_configured = true,
        .maximum_pulses_per_ms = 8,
        .speed = { GPIOA, GPIO_PIN_15, GPIO_PIN_SET },
        .brake = { GPIOB, GPIO_PIN_6, GPIO_PIN_SET },
        .clutch = { GPIOA, GPIO_PIN_11, GPIO_PIN_RESET },
        .button = { GPIOB, GPIO_PIN_9, GPIO_PIN_SET } },
    .closed_position_deg = TEST_REVERSE_TRAVEL ? 20 : 0,
    .open_position_deg = TEST_REVERSE_TRAVEL ? 0 : 20,
    .position_kp_raw = 1, .position_kd_raw = 1
};

static void test_controller(void)
{
    CruiseConfig config = standalone_config.cruise;
    CruiseController controller;
    CruiseInputs input = { .valid = true };
    CHECK(!Cruise_ValidateConfig(NULL));
    CruiseConfig bad = config;
    bad.kp = NAN; CHECK(!Cruise_Init(&controller, &bad));
    bad = config; bad.pulses_per_revolution = 0; CHECK(!Cruise_ValidateConfig(&bad));
    bad = config; bad.maximum_gap_ms = bad.speed_timeout_ms; CHECK(!Cruise_ValidateConfig(&bad));
    bad = config; bad.minimum_speed_mps = bad.maximum_speed_mps; CHECK(!Cruise_ValidateConfig(&bad));
    CHECK(Cruise_Init(&controller, &config));
    CHECK(Cruise_Init(&controller, &controller.config)); // Reset from the retained settings.
    uint32_t now = UINT32_MAX - 50U;
    input.pulse_count = UINT32_MAX - 12U;
    CruiseResult result = Cruise_Update(&controller, &input, now, true, 0.25f);
    CHECK(!result.active && !result.speed_valid);
    for (unsigned i = 0; i < 12; ++i) {
        now += 20; input.pulse_count += 5;
        result = Cruise_Update(&controller, &input, now, true, 0.25f);
    }
    NEAR(result.speed_mps, 10);
    CHECK(result.speed_valid && !result.active);
    input.button_active = true;
    for (unsigned i = 0; i < 3; ++i) {
        now += 20; input.pulse_count += 5;
        result = Cruise_Update(&controller, &input, now, true, 0.25f);
    }
    CHECK(result.active); NEAR(result.target_speed_mps, 10); NEAR(result.demand, 0.25f);
    for (unsigned i = 0; i < 10; ++i) {
        now += 20; input.pulse_count += 4;
        result = Cruise_Update(&controller, &input, now, true, 0.25f);
    }
    CHECK(result.active && result.demand > 0.25f && result.demand <= 1);
    input.brake_active = true;
    result = Cruise_Update(&controller, &input, now += 20, true, 0.25f);
    CHECK(!result.active && result.reason == CRUISE_BRAKE && result.demand == 0);
    input.brake_active = false;
    for (unsigned i = 0; i < 10; ++i) {
        now += 20; input.pulse_count += 5;
        CHECK(!Cruise_Update(&controller, &input, now, true, 0.25f).active);
    }
    input.valid = false;
    result = Cruise_Update(&controller, &input, now += 20, true, 0.25f);
    CHECK(result.reason == CRUISE_INPUT_ERROR && !result.speed_valid);
    config.kp = 0.1f; config.ki = 1; config.kd = 0;
    CHECK(Cruise_Init(&controller, &config));
    input = (CruiseInputs){ .valid = true };
    now = 0;
    Cruise_Update(&controller, &input, now, true, 0);
    for (unsigned i = 0; i < 20; ++i) {
        now += 20; input.pulse_count += 5;
        if (i == 17) input.button_active = true;
        result = Cruise_Update(&controller, &input, now, true, 0);
    }
    CHECK(result.active);
    for (unsigned i = 0; i < 5; ++i) {
        now += 20; input.pulse_count += 4;
        result = Cruise_Update(&controller, &input, now, true, 0);
    }
    NEAR(result.speed_mps, 8); NEAR(result.demand, 0.24f);
    for (unsigned i = 0; i < 2000; ++i) {
        now += 20; input.pulse_count += 4;
        result = Cruise_Update(&controller, &input, now, true, 0);
    }
    CHECK(result.active && result.demand <= 1 && controller.integral < 1);
    controller.config.kp = FLT_MAX;
    result = Cruise_Update(&controller, &input, now += 20, true, 0);
    CHECK(!result.active && result.reason == CRUISE_PID_ERROR);
    // Independent abnormal cases, including a reset that is not natural wrap.
    for (unsigned fault = 0; fault < 6; ++fault) {
        CHECK(Cruise_Init(&controller, &standalone_config.cruise));
        input = (CruiseInputs){ .valid = true };
        now = 0;
        for (unsigned i = 0; i < 20; ++i) {
            now += 20; input.pulse_count += 5;
            if (i == 17) input.button_active = true;
            result = Cruise_Update(&controller, &input, now, true, 0.25f);
        }
        CHECK(result.active);
        if (fault == 0) {
            input.pulse_count = 0;
            result = Cruise_Update(&controller, &input, now += 20, true, 0);
            CHECK(result.reason == CRUISE_SPEED_INVALID);
        } else if (fault == 1) {
            input.pulse_count += 10000;
            for (unsigned i = 0; i < 5; ++i)
                result = Cruise_Update(&controller, &input, now += 20, true, 0);
            CHECK(result.reason == CRUISE_SPEED_INVALID ||
                  result.reason == CRUISE_WARMING_UP);
        } else if (fault == 2) {
            result = Cruise_Update(&controller, &input, now += 200, true, 0);
            CHECK(result.reason == CRUISE_LOOP_GAP);
        } else if (fault == 3) {
            for (unsigned i = 0; i < 25; ++i)
                result = Cruise_Update(&controller, &input, now += 20, true, 0);
            CHECK(result.reason == CRUISE_SPEED_TIMEOUT);
        } else if (fault == 4) {
            result = Cruise_Update(&controller, &input, now += 20, false, 0);
            CHECK(result.reason == CRUISE_DRIVE_UNAVAILABLE);
        } else {
            input.valid = false;
            result = Cruise_Update(&controller, &input, now += 20, true, 0);
            CHECK(result.reason == CRUISE_INPUT_ERROR);
        }
        CHECK(!result.active && result.demand == 0);
    }
}

uint32_t HAL_GetTick(void) { return (uint32_t)(clock_us / 1000U); }
void test_clock_enable(void) { ++clock_enables; }
void test_exti_clear(uint16_t pin) { pending_exti &= (uint16_t)~pin; }
void HAL_GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *gpio)
{
    CHECK(port == GPIOA || port == GPIOB);
    CHECK(gpio->Pull == GPIO_NOPULL);
    CHECK(gpio->Mode == GPIO_MODE_INPUT || gpio->Mode == GPIO_MODE_IT_RISING);
    ++gpio_setups;
}
void HAL_NVIC_SetPriority(IRQn_Type irq, uint32_t preempt, uint32_t sub)
{ CHECK(irq == EXTI15_10_IRQn && preempt == 5 && sub == 0); }
void HAL_NVIC_EnableIRQ(IRQn_Type irq) { CHECK(irq == EXTI15_10_IRQn); ++irq_enables; }
void HAL_NVIC_DisableIRQ(IRQn_Type irq) { CHECK(irq == EXTI15_10_IRQn); ++irq_disables; }
void HAL_GPIO_EXTI_IRQHandler(uint16_t pin)
{
    if (pending_exti & pin) {
        test_exti_clear(pin);
        HAL_GPIO_EXTI_Callback(pin);
    }
}
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
    if (port == GPIOC) { CHECK(pin == BUTTON_Pin); return GPIO_PIN_SET; }
    CHECK(port == GPIOA || port == GPIOB);
    return ((port == GPIOA ? level_a : level_b) & pin) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state)
{ (void)state; CHECK(port == STATUS_GPIO_Port && pin == STATUS_Pin); }
bool positionSensor_isReady(void) { return true; }
float positionSensor_getDegreesMultiturn(void) { return rotor_deg; }
float positionSensor_getVelocityDps(void) { return 0; }
void positionSensor_update(void) { }
bool API_FOC_Is_Calibrating(void) { return calibration; }
bool API_FOC_Check_Watchdog(void) { return true; }
void API_FOC_Torque_Enable(void) { CHECK(test_primask == 1); }
void API_FOC_Torque_Disable(void) { CHECK(test_primask == 1); ++coasts; calibration = false; }
void API_FOC_Set_Torque_Flux_Currents_mA(float iq, float id)
{ CHECK(test_primask == 1 && isfinite(iq) && isfinite(id)); ++current_publications; }
float API_FOC_Get_Present_Torque_Current(void) { return 0; }
float API_FOC_Get_Present_Flux_Current(void) { return 0; }
float API_FOC_Get_Present_Voltage(void) { return 24; }
float API_FOC_Get_Present_Temp(void) { return 30; }
float API_FOC_Get_Processing_Time(void) { return 5; }
float API_FOC_Get_Processing_Frequency(void) { return 40000; }
void API_FOC_Service_Update(void) { }
void Calibration_Init(void) { calibration = false; }
bool Calibration_Start(void) { calibration = true; return true; }
void Calibration_Update(bool serviced) { (void)serviced; }
bool Calibration_TakeResult(CalibrationResult *result) { (void)result; return false; }
int HAL_Serial_Available(HAL_Serial_Handler *handler) { (void)handler; return 0; }
char HAL_Serial_GetChar(HAL_Serial_Handler *handler) { (void)handler; return 0; }
int HAL_Serial_Print(HAL_Serial_Handler *handler, char const *format, ...)
{ (void)handler; (void)format; return 0; }
void packet_handler(char c) { (void)c; }
HAL_StatusTypeDef eeprom_store(uint8_t const *data, uint32_t size)
{ (void)data; (void)size; return HAL_OK; }
HAL_StatusTypeDef eeprom_restore(uint8_t *data, uint32_t size)
{ (void)data; (void)size; return HAL_OK; }
uint32_t HAL_FDCAN_GetRxFifoFillLevel(FDCAN_HandleTypeDef *handle, uint32_t fifo)
{ CHECK(handle == &can && fifo == FDCAN_RX_FIFO0); return rx_pending ? 1 : 0; }
HAL_StatusTypeDef test_can_get_rx_message(FDCAN_HandleTypeDef *handle, uint32_t fifo,
    FDCAN_RxHeaderTypeDef *header, uint8_t *data, size_t capacity)
{
    (void)fifo; CHECK(handle == &can && capacity == 64);
    *header = (FDCAN_RxHeaderTypeDef){ .Identifier = regs[REG_ID],
        .IdType = FDCAN_STANDARD_ID, .RxFrameType = FDCAN_DATA_FRAME,
        .FDFormat = FDCAN_CLASSIC_CAN, .BitRateSwitch = FDCAN_BRS_OFF,
        .DataLength = FDCAN_DLC_BYTES_8 };
    memcpy(data, rx_data, 8); rx_pending = false; return HAL_OK;
}
HAL_StatusTypeDef HAL_FDCAN_AddMessageToTxFifoQ(FDCAN_HandleTypeDef *handle,
    FDCAN_TxHeaderTypeDef *header, uint8_t *data)
{ (void)header; (void)data; CHECK(handle == &can); return HAL_OK; }

static void advance_ms(unsigned duration_ms, unsigned pulse_period_us)
{
    for (unsigned i = 0; i < duration_ms * 4; ++i) {
        clock_us += 250;
        timer.counter = (uint16_t)clock_us;
        if (pulse_period_us && clock_us % pulse_period_us == 0) {
            pending_exti = GPIO_PIN_15;
            VehicleInputs_HandleExti();
        }
        App_Process();
        CHECK(test_primask == 0);
    }
}
static void release_and_engage(void)
{
    level_b &= (uint16_t)~GPIO_PIN_9; advance_ms(250, 4000);
    level_b |= GPIO_PIN_9; advance_ms(80, 4000);
    CHECK(Standalone_GetStatus().active && Drive_GetStatus().armed);
}
static void test_integration(void)
{
    control_reset_config_defaults(); control_reset_runtime(); Drive_Init();
    CHECK(Config_IsValid());
    StandaloneConfig bad;
    memset(&bad, 0, sizeof bad);
    CHECK(!Standalone_Init(&bad)); Standalone_Process();
    CHECK(!Drive_GetStatus().armed && gpio_setups == 0);
    bad = standalone_config; bad.open_position_deg = bad.closed_position_deg;
    CHECK(!Standalone_Init(&bad));
    bad = standalone_config; bad.inputs.wiring_verified = false;
    CHECK(!Standalone_Init(&bad));
    bad = standalone_config; bad.inputs.maximum_pulses_per_ms = 0;
    CHECK(!Standalone_Init(&bad));
    bad = standalone_config; bad.inputs.maximum_pulses_per_ms = 1;
    CHECK(!Standalone_Init(&bad)); // Would reject legitimate pulses at the configured max speed.
    bad = standalone_config; bad.inputs.brake = bad.inputs.speed;
    CHECK(!Standalone_Init(&bad));
    bad = standalone_config; bad.inputs.speed.pin = GPIO_PIN_10;
    CHECK(!Standalone_Init(&bad) && gpio_setups == 0);
    level_a = GPIO_PIN_11; level_b = GPIO_PIN_9;
    App_Init(&timer, &serial);
    CHECK(gpio_setups == 4 && irq_enables == 1 && clock_enables == 2);
    advance_ms(300, 4000);
    CHECK(!Standalone_GetStatus().active && !Drive_GetStatus().armed);
    NEAR(Standalone_GetStatus().speed_mps, 10);
    release_and_engage();
    NEAR(Standalone_GetStatus().demand, TEST_REVERSE_TRAVEL ? 0.75f : 0.25f);
    CHECK(Drive_GetCommand().goal.position_tenths_deg == 50);
    CHECK(current_publications > 0);
    advance_ms(250, 5000);
    CHECK(TEST_REVERSE_TRAVEL ? Drive_GetCommand().goal.position_tenths_deg < 50 :
                               Drive_GetCommand().goal.position_tenths_deg > 50);
    unsigned const before = coasts;
    level_b |= GPIO_PIN_6;
    clock_us += 250; timer.counter = (uint16_t)clock_us; App_Process();
    CHECK(!Drive_GetStatus().armed && coasts > before);
    CHECK(Standalone_GetStatus().reason == CRUISE_BRAKE);
    advance_ms(600, 0);
    CHECK(!Standalone_GetStatus().speed_valid);
    level_b &= (uint16_t)~GPIO_PIN_6; advance_ms(250, 4000);
    CHECK(!Drive_GetStatus().armed); release_and_engage();
    level_a &= (uint16_t)~GPIO_PIN_11; advance_ms(1, 4000);
    CHECK(!Drive_GetStatus().armed && Standalone_GetStatus().reason == CRUISE_CLUTCH);
    level_a |= GPIO_PIN_11; release_and_engage();
    CanServo_Init(&can);
    DriveCommandSnapshot saved = Drive_GetCommand();
    uint32_t const saved_tick = Drive_GetStatus().last_command_ms;
    memset(rx_data, 0xff, sizeof rx_data); rx_pending = true;
    CanServo_OnRxFifo0(&can, FDCAN_IT_RX_FIFO0_NEW_MESSAGE);
    CHECK(Drive_GetStatus().last_command_ms == saved_tick);
    CHECK(Drive_GetCommand().generation == saved.generation);
    memset(rx_data, 1, sizeof rx_data); rx_pending = true;
    CanServo_OnRxFifo0(&can, FDCAN_IT_RX_FIFO0_NEW_MESSAGE);
    CHECK(Drive_GetCommand().goal.position_tenths_deg == saved.goal.position_tenths_deg);
    memset(rx_data, 0, sizeof rx_data); rx_pending = true;
    CanServo_OnRxFifo0(&can, FDCAN_IT_RX_FIFO0_NEW_MESSAGE);
    CHECK(!Drive_GetStatus().armed);
    advance_ms(100, 4000); CHECK(!Standalone_GetStatus().active);
    release_and_engage();
    advance_ms(510, 0);
    CHECK(!Drive_GetStatus().armed && !Standalone_GetStatus().active);
    release_and_engage();
    hardware_fault_set(DRIVE_FAULT_OVERCURRENT); advance_ms(1, 4000);
    CHECK(!Drive_GetStatus().armed && !Standalone_GetStatus().active);
    hardware_fault_clear(DRIVE_FAULT_OVERCURRENT); advance_ms(200, 4000);
    CHECK(!Drive_GetStatus().armed); release_and_engage();
    clock_us += 200000; timer.counter = (uint16_t)clock_us; App_Process();
    CHECK(!Drive_GetStatus().armed && Standalone_GetStatus().reason == CRUISE_LOOP_GAP);
    release_and_engage();
    level_b &= (uint16_t)~GPIO_PIN_9; advance_ms(60, 4000);
    level_b |= GPIO_PIN_9; advance_ms(60, 4000);
    CHECK(!Drive_GetStatus().armed && !Standalone_GetStatus().active);
    CruiseInputs sample = VehicleInputs_Read();
    VehicleInputs_OnPulse(GPIO_PIN_6);
    CHECK(VehicleInputs_Read().pulse_count == sample.pulse_count);
    test_primask = 1; sample = VehicleInputs_Read(); CHECK(test_primask == 1);
    test_primask = 0;
    release_and_engage();
    for (unsigned i = 0; i < 1000; ++i) VehicleInputs_OnPulse(GPIO_PIN_15);
    CHECK(irq_disables == 1 && !VehicleInputs_Read().valid);
    App_Process();
    CHECK(!Drive_GetStatus().armed && Standalone_GetStatus().reason == CRUISE_INPUT_ERROR);
    CHECK(!Standalone_GetStatus().speed_valid);
    advance_ms(100, 4000);
    CHECK(!VehicleInputs_Read().valid && !Standalone_GetStatus().active);
}

int main(void)
{
    test_controller();
    test_integration();
    printf("Standalone production modules: %u checks passed.\n", checks);
    return 0;
}
