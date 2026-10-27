#include "app_fixture.h"
#include "main.h"
#include "drive.h"
#include "foc.h"
#include "position_sensor.h"
#include "protocol.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

AppTestState app_test;
TIM_HandleTypeDef app_test_timer;
HAL_Serial_Handler app_test_serial;
GPIO_TypeDef test_gpio_c;
uint32_t test_primask;
void (*test_irq_restored)(void);

static void record(char event)
{
    if (app_test.trace_length + 1 < sizeof app_test.trace) {
        app_test.trace[app_test.trace_length++] = event;
        app_test.trace[app_test.trace_length] = '\0';
    }
}
void AppTest_ClearTrace(void) { app_test.trace_length = 0; app_test.trace[0] = '\0'; }
void AppTest_Reset(uint16_t start_us)
{
    memset(&app_test, 0, sizeof app_test);
    memset(regs, 0, sizeof regs);
    test_primask = 0;
    test_irq_restored = NULL;
    factory_reset_eeprom_regs();
    reset_ram_regs();
    app_test.position = 123.45f; app_test.velocity = -20.75f;
    app_test.torque = -500; app_test.flux = 123;
    app_test.voltage = 24; app_test.temperature = 30;
    app_test.processing_frequency = 40000;
    app_test.tick = 1000;
    app_test.disarmed = true;
    app_test_timer.counter = start_us;
    App_Init(&app_test_timer, &app_test_serial);
    AppTest_ClearTrace();
}
void AppTest_Poll(uint16_t now_us) { app_test_timer.counter = now_us; App_Process(); }
void AppTest_Advance(uint16_t delta_us) { app_test_timer.counter += delta_us; App_Process(); }

uint16_t test_app_clock_read(TIM_HandleTypeDef *timer)
{
    if (timer != &app_test_timer) ++app_test.errors;
    ++app_test.clock_reads; record('C');
    return timer->counter;
}
uint32_t HAL_GetTick(void) { return app_test.tick; }
void Drive_Service(void)
{
    ++app_test.watchdogs; record('W');
    if (app_test.watchdog_disarms) { app_test.command.enabled = false; app_test.disarmed = true; }
}
DriveCommandSnapshot Drive_Get_Command(void) { return app_test.command; }
void Drive_Disarm(void)
{
    ++app_test.disarms; record('D');
    app_test.command.enabled = false; app_test.disarmed = true;
}
bool Drive_Is_Disarmed(void) { return app_test.disarmed; }
int Drive_Run_Calibration(void)
{
    if (!app_test.disarmed) ++app_test.errors;
    ++app_test.calibrations; record('K');
    app_test_timer.counter += app_test.calibration_delay_us;
    return app_test.calibration_result;
}
void Drive_Publish_Measurements(DriveMeasurements const *measurements)
{
    if (test_primask != 1) ++app_test.errors;
    ++app_test.publications; record('M');
    app_test.published = *measurements;
    control_mirror_measurements(measurements);
}
void ServoController_Init(void) { ++app_test.controller_inits; record('I'); }
ServoSetpoints ServoController_Update(DriveCommandSnapshot const *command,
                                     DriveConfig const *config, float position, float velocity)
{
    ++app_test.servos; record('U');
    app_test.last_command = *command; app_test.last_config = *config;
    app_test.servo_position = position; app_test.servo_velocity = velocity;
    return app_test.setpoints;
}
void positionSensor_update(void)
{
    ++app_test.sensors; record('S');
    app_test_timer.counter += app_test.sensor_delay_us;
}
float positionSensor_getDegreesMultiturn(void) { return app_test.position; }
float positionSensor_getVelocityDps(void) { return app_test.velocity; }
static float measured(float value) { if (test_primask != 1) ++app_test.errors; return value; }
float API_FOC_Get_Present_Torque_Current(void) { return measured(app_test.torque); }
float API_FOC_Get_Present_Flux_Current(void) { return measured(app_test.flux); }
float API_FOC_Get_Present_Voltage(void) { return measured(app_test.voltage); }
float API_FOC_Get_Present_Temp(void) { return measured(app_test.temperature); }
float API_FOC_Get_Processing_Time(void) { return app_test.processing_time; }
float API_FOC_Get_Processing_Frequency(void) { return app_test.processing_frequency; }
void API_FOC_Service_Update(void) { ++app_test.services; record('F'); }
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state)
{
    if (port != STATUS_GPIO_Port || pin != STATUS_Pin) ++app_test.errors;
    app_test.led = state; record('L');
}
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
    if (port != BUTTON_GPIO_Port || pin != BUTTON_Pin) ++app_test.errors;
    record('B');
    return app_test.button_down ? GPIO_PIN_RESET : GPIO_PIN_SET;
}
int HAL_Serial_Available(HAL_Serial_Handler *serial)
{
    if (serial != &app_test_serial) ++app_test.errors;
    ++app_test.serial_checks;
    return app_test.serial_pending;
}
char HAL_Serial_GetChar(HAL_Serial_Handler *serial)
{
    if (serial != &app_test_serial || app_test.serial_pending <= 0) ++app_test.errors;
    --app_test.serial_pending;
    return (char)('a' + app_test.serial_reads++ % 26);
}
void packet_handler(char c)
{
    if (c != (char)('a' + app_test.packets % 26)) ++app_test.errors;
    ++app_test.packets; record('R');
    if (app_test.serial_refill) ++app_test.serial_pending;
    if (app_test.serial_shrink) app_test.serial_pending = 0;
}
int HAL_Serial_Print(HAL_Serial_Handler *serial, char const *format, ...)
{
    if (serial != &app_test_serial) ++app_test.errors;
    ++app_test.prints; record('P');
    va_list args;
    va_start(args, format);
    int const result = vsnprintf(app_test.calibration_text, sizeof app_test.calibration_text, format, args);
    va_end(args);
    return result;
}
HAL_StatusTypeDef eeprom_store(uint8_t const *data, uint32_t size)
{ (void)data; (void)size; return HAL_OK; }
HAL_StatusTypeDef eeprom_restore(uint8_t *data, uint32_t size)
{ (void)data; (void)size; return HAL_OK; }
