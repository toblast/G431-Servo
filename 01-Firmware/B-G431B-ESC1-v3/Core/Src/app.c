#include "app.h"
#include "configuration.h"
#include "calibration.h"
#include "main.h"
#include "position_sensor.h"
#include "servo_controller.h"
#include "drive.h"
#include "foc.h"
#include "control_table.h"
#include "protocol.h"
#include "telemetry.h"
#include <math.h>

#define SERVO_PERIOD_US 250U
#define SERVICE_PERIOD_US 10000U

static struct {
    TIM_HandleTypeDef *timer;
    HAL_Serial_Handler *serial;
    uint16_t servo_last_time_us, service_last_time_us;
    uint32_t servo_counter, loop_counter;
    bool calibration_button_was_down;
} app;

void App_Init(TIM_HandleTypeDef *timer, HAL_Serial_Handler *serial)
{
    app.timer = timer;
    app.serial = serial;
    ServoController_Init();
    Calibration_Init();
    // A button held at power-up must be released before it can request motion.
    app.calibration_button_was_down = true;
    uint16_t const now = __HAL_TIM_GET_COUNTER(timer);
    app.servo_last_time_us = app.service_last_time_us = now;
    app.servo_counter = app.loop_counter = 0;
}

static void update_servo(void)
{
    positionSensor_update();
    DriveCommandSnapshot const command = Drive_Get_Command();
    DriveConfig const config = control_read_config();
    ServoSetpoints setpoints = {
        .position_deg = 0.0f, .velocity_dps = 0.0f,
        .torque_mA = 0.0f, .flux_mA = 0.0f
    };
    // Calibration owns excitation until cleanup finishes. In particular, do not
    // let an old outer-controller disable transition cancel a newly started job.
    if (!API_FOC_Is_Calibrating()) {
        setpoints = ServoController_Update(&command, &config,
            positionSensor_getDegreesMultiturn(), positionSensor_getVelocityDps());
    }

    // Publish the complete measurement snapshot with ADC interrupts masked.
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    DriveMeasurements const measurements = {
        .position_deg = positionSensor_getDegreesMultiturn(),
        .velocity_dps = positionSensor_getVelocityDps(),
        .torque_mA = API_FOC_Get_Present_Torque_Current(),
        .flux_mA = API_FOC_Get_Present_Flux_Current(),
        .voltage_V = API_FOC_Get_Present_Voltage(),
        .temperature_C = API_FOC_Get_Present_Temp(),
        .moving = fabsf(positionSensor_getVelocityDps()) > (float)config.moving_threshold_dps
    };
    Drive_Publish_Measurements(&measurements);
    __set_PRIMASK(primask);

    // Preserve diagnostic scaling and the counter values before loop completion.
    telemetry_store_i16(regs, REG_SETPOINT_POSITION_DEG_L, setpoints.position_deg * 10.0f);
    telemetry_store_i16(regs, REG_SETPOINT_VELOCITY_DPS_L, setpoints.velocity_dps * 1.0f);
    telemetry_store_i16(regs, REG_SETPOINT_TORQUE_CURRENT_MA_L, setpoints.torque_mA * 1.0f);
    telemetry_store_i16(regs, REG_SETPOINT_FLUX_CURRENT_MA_L, setpoints.flux_mA * 1.0f);
    regs[REG_PROCESSING_TIME] = telemetry_u8(API_FOC_Get_Processing_Time());
    regs[REG_FOC_FREQUENCY] = telemetry_u8(API_FOC_Get_Processing_Frequency() / 1000.0f);
    uint32_t const uptime_ms = HAL_GetTick();
    regs[REG_PID_FREQUENCY] = uptime_ms != 0 ? telemetry_u8((float)app.servo_counter / (float)uptime_ms) : 0;
    regs[REG_MLP_FREQUENCY] = uptime_ms != 0 ? telemetry_u8((float)app.loop_counter / (float)uptime_ms) : 0;
}

static void service(void)
{
    if (!Config_IsValid()) {
        hardware_fault_set(DRIVE_FAULT_CONFIGURATION);
        Drive_Disarm();
    } else hardware_fault_clear(DRIVE_FAULT_CONFIGURATION);
    API_FOC_Service_Update();

    HAL_GPIO_WritePin(STATUS_GPIO_Port, STATUS_Pin,
        (regs[REG_LED] > 0) || (hardware_fault_status() > 0) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // Update the edge history even while armed, so disarming a held button
    // cannot turn that old press into a new calibration request.
    bool const button_down = HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin) == GPIO_PIN_RESET;
    bool const calibration_requested = button_down && !app.calibration_button_was_down && Drive_Is_Disarmed();
    app.calibration_button_was_down = button_down;
    if (calibration_requested && !Drive_Start_Calibration())
        HAL_Serial_Print(app.serial, "Calibration result: %d (0=success, 1=no motion, 2=pole pairs, 3=cancel/fault, 4=storage)\n", CALIBRATION_CANCELLED);

    // Bound work to the bytes present now so traffic cannot starve watchdogs.
    uint32_t remaining = (uint32_t)HAL_Serial_Available(app.serial);
    while (remaining-- != 0 && HAL_Serial_Available(app.serial)) {
        char const c = HAL_Serial_GetChar(app.serial);
        packet_handler(c);
    }
}

void App_Process(void)
{
    Drive_Service();
    uint16_t const now = __HAL_TIM_GET_COUNTER(app.timer);
    bool acquisition_serviced = false;
    uint16_t const servo_delta_us = now - app.servo_last_time_us;
    if (servo_delta_us >= SERVO_PERIOD_US) {
        app.servo_last_time_us += SERVO_PERIOD_US;
        ++app.servo_counter;
        update_servo();
        acquisition_serviced = true;
    }

    // Use the same time sample even if acquisition takes time.
    uint16_t const service_delta_us = now - app.service_last_time_us;
    if (service_delta_us >= SERVICE_PERIOD_US) {
        app.service_last_time_us += SERVICE_PERIOD_US;
        service();
    }
    Calibration_Update(acquisition_serviced);
    CalibrationResult result;
    if (Calibration_TakeResult(&result))
        HAL_Serial_Print(app.serial, "Calibration result: %d (0=success, 1=no motion, 2=pole pairs, 3=cancel/fault, 4=storage)\n", result);
    ++app.loop_counter;
}
