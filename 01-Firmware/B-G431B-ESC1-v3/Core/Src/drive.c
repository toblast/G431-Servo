#include "drive.h"
#include "configuration.h"
#include "calibration.h"
#include "control_table.h"
#include "foc.h"
#include "position_sensor.h"
#include <math.h>
#include <string.h>

// Private owner of command permission and command history. Register values are
// compatibility mirrors, never the source of an accepted servo command.
static struct {
    bool armed, maintenance;
    uint32_t last_command_ms, generation;
    ServoCommand command;
    DriveMeasurements measured;
} drive;

void Drive_Init(void)
{
    memset(&drive, 0, sizeof drive);
    control_mirror_command(false, &drive.command);
}

bool Drive_Is_Disarmed(void)
{
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    bool const result = !drive.armed && !API_FOC_Is_Calibrating();
    __set_PRIMASK(primask);
    return result;
}

bool Drive_Is_Maintenance(void)
{
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    bool const result = drive.maintenance;
    __set_PRIMASK(primask);
    return result;
}

void Drive_Disarm(void)
{
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    drive.armed = false;
    memset(&drive.command, 0, sizeof drive.command);
    control_mirror_command(false, &drive.command);
    API_FOC_Torque_Disable();
    __set_PRIMASK(primask);
}

bool Drive_Request_Arm(void)
{
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    bool accepted = false;
    if (API_FOC_Is_Calibrating()) goto done;
    if (drive.maintenance || !Config_IsValid()) {
        hardware_fault_set(DRIVE_FAULT_CONFIGURATION);
        goto done;
    }
    if (!positionSensor_isReady() || hardware_fault_status() != 0) goto done;
    if (!drive.armed) {
        DriveConfig const config = control_read_config();
        float const position = positionSensor_getDegreesMultiturn();
        if (!isfinite(position) || position < config.min_position_deg || position > config.max_position_deg)
            goto done;
        memset(&drive.command, 0, sizeof drive.command);
        drive.command.position_tenths_deg = (int16_t)(10.0f * position);
        ++drive.generation;
        drive.armed = true;
        control_mirror_command(true, &drive.command);
    }
    drive.last_command_ms = HAL_GetTick();
    accepted = true;
done:
    __set_PRIMASK(primask);
    return accepted;
}

bool Drive_Submit_Command(ServoCommand const *command)
{
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    bool const accepted = drive.armed && !API_FOC_Is_Calibrating();
    if (accepted) {
        drive.command = *command;
        control_mirror_command(true, &drive.command);
        drive.last_command_ms = HAL_GetTick();
    }
    __set_PRIMASK(primask);
    return accepted;
}

DriveCommandSnapshot Drive_Get_Command(void)
{
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    DriveCommandSnapshot const result = {
        .enabled = drive.armed && hardware_fault_status() == 0,
        .generation = drive.generation,
        .goal = drive.command
    };
    __set_PRIMASK(primask);
    return result;
}

bool Drive_Publish_Currents(DriveCommandSnapshot const *command, bool new_arm,
                          float torque_mA, float flux_mA)
{
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    bool const accepted = command->enabled && drive.armed &&
        command->generation == drive.generation && hardware_fault_status() == 0 &&
        !API_FOC_Is_Calibrating();
    if (accepted) {
        if (new_arm) API_FOC_Torque_Enable();
        API_FOC_Set_Torque_Flux_Currents_mA(torque_mA, flux_mA);
    }
    __set_PRIMASK(primask);
    return accepted;
}

void Drive_Disable_If_Disarmed(void)
{
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    if (!drive.armed || hardware_fault_status() != 0) API_FOC_Torque_Disable();
    __set_PRIMASK(primask);
}

void Drive_Service(void)
{
    if (hardware_fault_status() != 0) Drive_Disarm();
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    uint32_t const now = HAL_GetTick();
    if (drive.armed && (uint32_t)(now - drive.last_command_ms) > CAN_COMMAND_TIMEOUT_MS)
        Drive_Disarm();
    if (!API_FOC_Check_Watchdog()) Drive_Disarm();
    __set_PRIMASK(primask);
}

bool Drive_Begin_Maintenance(bool disarm_first)
{
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    bool const accepted = !drive.maintenance && !API_FOC_Is_Calibrating() &&
                          (disarm_first || !drive.armed);
    if (accepted) {
        drive.maintenance = true;
        if (disarm_first) Drive_Disarm();
    }
    __set_PRIMASK(primask);
    return accepted;
}

void Drive_End_Maintenance(void)
{
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    drive.maintenance = false;
    __set_PRIMASK(primask);
}

bool Drive_Start_Calibration(void)
{
    if (!Drive_Is_Disarmed() || Drive_Is_Maintenance()) return false;
    // FOC repeats its atomic entry check: a CAN ARM arriving here must win or
    // be rejected by FOC's calibration-active flag, never race into motion.
    return Calibration_Start();
}

void Drive_Publish_Measurements(DriveMeasurements const *measurements)
{
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    drive.measured = *measurements;
    control_mirror_measurements(&drive.measured);
    __set_PRIMASK(primask);
}

DriveStatus Drive_Get_Status(void)
{
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    DriveFaultFlags const faults = hardware_fault_status();
    DriveStatus const result = {
        .state = faults ? DRIVE_FAULT : API_FOC_Is_Calibrating() ? DRIVE_CALIBRATING :
                 drive.maintenance ? DRIVE_MAINTENANCE : drive.armed ? DRIVE_ARMED : DRIVE_DISARMED,
        .faults = faults,
        .generation = drive.generation,
        .armed = drive.armed,
        .last_command_ms = drive.last_command_ms,
        .measured = drive.measured
    };
    __set_PRIMASK(primask);
    return result;
}
