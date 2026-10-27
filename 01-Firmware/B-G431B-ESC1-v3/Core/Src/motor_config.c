#include "motor_config.h"
#include "control_table.h"
#include "binary_tool.h"

MotorControlConfig control_read_motor_config(void)
{
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    MotorControlConfig const result = {
        .phase_offset_deg = (int16_t)MAKE_SHORT(regs[REG_MOTOR_SYNCHRO_L], regs[REG_MOTOR_SYNCHRO_H]),
        .phase_trim_deg = MAKE_SHORT(regs[REG_GOAL_SYNCHRO_OFFSET_L], regs[REG_GOAL_SYNCHRO_OFFSET_H]),
        .motor_pole_pairs = regs[REG_MOTOR_POLE_PAIRS],
        .invert_phase_motor = regs[REG_INV_PHASE_MOTOR],
        .flux_current_kp_raw = (int16_t)MAKE_SHORT(regs[REG_PID_FLUX_CURRENT_KP_L], regs[REG_PID_FLUX_CURRENT_KP_H]),
        .flux_current_ki_raw = (int16_t)MAKE_SHORT(regs[REG_PID_FLUX_CURRENT_KI_L], regs[REG_PID_FLUX_CURRENT_KI_H]),
        .torque_current_kp_raw = (int16_t)MAKE_SHORT(regs[REG_PID_TORQUE_CURRENT_KP_L], regs[REG_PID_TORQUE_CURRENT_KP_H]),
        .torque_current_ki_raw = (int16_t)MAKE_SHORT(regs[REG_PID_TORQUE_CURRENT_KI_L], regs[REG_PID_TORQUE_CURRENT_KI_H]),
        .max_velocity_dps = (int16_t)MAKE_SHORT(regs[REG_MAX_VELOCITY_DPS_L], regs[REG_MAX_VELOCITY_DPS_H])
    };
    __set_PRIMASK(primask);
    return result;
}

MotorProtectionConfig control_read_protection_config(void)
{
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    MotorProtectionConfig const result = {
        .low_voltage_V = regs[REG_LOW_VOLTAGE_LIMIT],
        .high_voltage_V = regs[REG_HIGH_VOLTAGE_LIMIT],
        .temperature_limit_C = regs[REG_TEMPERATURE_LIMIT]
    };
    __set_PRIMASK(primask);
    return result;
}

MotorAlignment control_read_motor_alignment(void)
{
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    MotorAlignment const result = {
        .phase_offset_deg = (int16_t)MAKE_SHORT(regs[REG_MOTOR_SYNCHRO_L], regs[REG_MOTOR_SYNCHRO_H]),
        .invert_phase_motor = regs[REG_INV_PHASE_MOTOR]
    };
    __set_PRIMASK(primask);
    return result;
}

void control_write_motor_alignment(MotorAlignment const *alignment)
{
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    regs[REG_INV_PHASE_MOTOR] = alignment->invert_phase_motor;
    regs[REG_MOTOR_SYNCHRO_L] = LOW_BYTE(alignment->phase_offset_deg);
    regs[REG_MOTOR_SYNCHRO_H] = HIGH_BYTE(alignment->phase_offset_deg);
    __set_PRIMASK(primask);
}

bool control_torque_requested(void)
{
    return *(volatile uint8_t *)&regs[REG_TORQUE_ENABLE] != 0;
}

void control_record_sensor_error(void)
{
    // This legacy byte counts encoder read errors despite its old CRC name.
    ++regs[REG_PROTOCOL_CRC_FAIL];
}
