/*
 * control_table.c
 *
 *  Created on: 27 sept. 2020
 *      Author: patrick
 */

#include "control_table.h"
#include "binary_tool.h"
#include "eeprom.h"
#include "telemetry.h"

#include "stm32g4xx_hal.h"
#include <string.h>

uint8_t regs[REG_MAX];

DriveConfig control_read_config(void)
{
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    DriveConfig const result = {
        .min_position_deg = (int16_t)MAKE_SHORT(regs[REG_MIN_POSITION_DEG_L], regs[REG_MIN_POSITION_DEG_H]),
        .max_position_deg = (int16_t)MAKE_SHORT(regs[REG_MAX_POSITION_DEG_L], regs[REG_MAX_POSITION_DEG_H]),
        .max_velocity_dps = (int16_t)MAKE_SHORT(regs[REG_MAX_VELOCITY_DPS_L], regs[REG_MAX_VELOCITY_DPS_H]),
        .max_current_mA = MAKE_SHORT(regs[REG_MAX_CURRENT_MA_L], regs[REG_MAX_CURRENT_MA_H]),
        .encoder_bits = regs[REG_ENCODER_BITS],
        .motor_pole_pairs = regs[REG_MOTOR_POLE_PAIRS],
        .invert_phase_motor = regs[REG_INV_PHASE_MOTOR],
        .low_voltage_V = regs[REG_LOW_VOLTAGE_LIMIT],
        .high_voltage_V = regs[REG_HIGH_VOLTAGE_LIMIT],
        .temperature_limit_C = regs[REG_TEMPERATURE_LIMIT],
        .moving_threshold_dps = regs[REG_MOVING_THRESHOLD_DPS],
        .flux_current_kp_raw = (int16_t)MAKE_SHORT(regs[REG_PID_FLUX_CURRENT_KP_L], regs[REG_PID_FLUX_CURRENT_KP_H]),
        .flux_current_ki_raw = (int16_t)MAKE_SHORT(regs[REG_PID_FLUX_CURRENT_KI_L], regs[REG_PID_FLUX_CURRENT_KI_H]),
        .torque_current_kp_raw = (int16_t)MAKE_SHORT(regs[REG_PID_TORQUE_CURRENT_KP_L], regs[REG_PID_TORQUE_CURRENT_KP_H]),
        .torque_current_ki_raw = (int16_t)MAKE_SHORT(regs[REG_PID_TORQUE_CURRENT_KI_L], regs[REG_PID_TORQUE_CURRENT_KI_H])
    };
    __set_PRIMASK(primask);
    return result;
}

void control_mirror_command(bool enabled, ServoCommand const *command)
{
    regs[REG_TORQUE_ENABLE] = enabled;
    memset(&regs[REG_GOAL_POSITION_DEG_L], 0, REG_PRESENT_POSITION_DEG_L - REG_GOAL_POSITION_DEG_L);
    regs[REG_GOAL_POSITION_DEG_L] = LOW_BYTE(command->position_tenths_deg);
    regs[REG_GOAL_POSITION_DEG_H] = HIGH_BYTE(command->position_tenths_deg);
    regs[REG_GOAL_VELOCITY_DPS_L] = LOW_BYTE(command->velocity_dps);
    regs[REG_GOAL_VELOCITY_DPS_H] = HIGH_BYTE(command->velocity_dps);
    regs[REG_GOAL_TORQUE_CURRENT_MA_L] = LOW_BYTE(command->torque_mA);
    regs[REG_GOAL_TORQUE_CURRENT_MA_H] = HIGH_BYTE(command->torque_mA);
    regs[REG_GOAL_FLUX_CURRENT_MA_L] = LOW_BYTE(command->flux_mA);
    regs[REG_GOAL_FLUX_CURRENT_MA_H] = HIGH_BYTE(command->flux_mA);
    regs[REG_GOAL_POS_KP] = command->pos_kp;
    regs[REG_GOAL_POS_KD] = command->pos_kd;
    regs[REG_GOAL_VEL_KP] = command->vel_kp;
}

void control_mirror_measurements(DriveMeasurements const *measurements)
{
    telemetry_store_i16(regs, REG_PRESENT_POSITION_DEG_L, measurements->position_deg * 10.0f);
    telemetry_store_i16(regs, REG_PRESENT_VELOCITY_DPS_L, measurements->velocity_dps);
    telemetry_store_i16(regs, REG_PRESENT_TORQUE_CURRENT_MA_L, measurements->torque_mA);
    telemetry_store_i16(regs, REG_PRESENT_FLUX_CURRENT_MA_L, measurements->flux_mA);
    regs[REG_PRESENT_VOLTAGE] = telemetry_u8(measurements->voltage_V);
    regs[REG_PRESENT_TEMPERATURE] = telemetry_u8(measurements->temperature_C);
    regs[REG_MOVING] = measurements->moving;
}

bool control_configuration_valid(void)
{
    DriveConfig const config = control_read_config();
    return config.min_position_deg < config.max_position_deg && config.min_position_deg >= -3276 && config.max_position_deg <= 3276 &&
           config.max_velocity_dps > 0 && (uint16_t)config.max_velocity_dps <= SERVO_BENCH_MAX_VELOCITY_DPS &&
           config.max_current_mA > 0 && config.max_current_mA <= SERVO_BENCH_MAX_CURRENT_MA &&
           config.encoder_bits == 12 && config.motor_pole_pairs != 0 && config.invert_phase_motor <= 1 &&
           config.low_voltage_V > 0 && config.low_voltage_V < config.high_voltage_V &&
           config.high_voltage_V <= REG_HIGH_VOLTAGE_LIMIT_VALUE &&
           config.temperature_limit_C > 0 && config.temperature_limit_C <= REG_TEMPERATURE_LIMIT_VALUE &&
           config.flux_current_kp_raw >= 0 && config.flux_current_ki_raw >= 0 &&
           (config.flux_current_kp_raw > 0 || config.flux_current_ki_raw > 0) &&
           config.torque_current_kp_raw >= 0 && config.torque_current_ki_raw >= 0 &&
           (config.torque_current_kp_raw > 0 || config.torque_current_ki_raw > 0);
}

HAL_StatusTypeDef factory_reset_eeprom_regs()
{
	regs[REG_MODEL_NUMBER_L] = LOW_BYTE(REG_MODEL_NUMBER_VALUE);
	regs[REG_MODEL_NUMBER_H] = HIGH_BYTE(REG_MODEL_NUMBER_VALUE);
	regs[REG_VERSION] = REG_VERSION_VALUE;
	regs[REG_ID] = REG_ID_VALUE;
	regs[REG_BAUD_RATE] = REG_BAUD_RATE_VALUE;
	regs[REG_RETURN_DELAY] = REG_RETURN_DELAY_VALUE;

	regs[REG_MIN_POSITION_DEG_L] = LOW_BYTE(REG_MIN_POSITION_DEG_VALUE);
	regs[REG_MIN_POSITION_DEG_H] = HIGH_BYTE(REG_MIN_POSITION_DEG_VALUE);
	regs[REG_MAX_POSITION_DEG_L] = LOW_BYTE(REG_MAX_POSITION_DEG_VALUE);
	regs[REG_MAX_POSITION_DEG_H] = HIGH_BYTE(REG_MAX_POSITION_DEG_VALUE);
	regs[REG_MAX_VELOCITY_DPS_L] = LOW_BYTE(REG_MAX_VELOCITY_DPS_VALUE);
	regs[REG_MAX_VELOCITY_DPS_H] = HIGH_BYTE(REG_MAX_VELOCITY_DPS_VALUE);
	regs[REG_MAX_ACCELERATION_DPSS_L] = LOW_BYTE(REG_MAX_ACCELERATION_DPSS_VALUE);
	regs[REG_MAX_ACCELERATION_DPSS_H] = HIGH_BYTE(REG_MAX_ACCELERATION_DPSS_VALUE);
	regs[REG_MAX_CURRENT_MA_L] = LOW_BYTE(REG_MAX_CURRENT_MA_VALUE);
	regs[REG_MAX_CURRENT_MA_H] = HIGH_BYTE(REG_MAX_CURRENT_MA_VALUE);
	regs[REG_TEMPERATURE_LIMIT] = REG_TEMPERATURE_LIMIT_VALUE;
	regs[REG_LOW_VOLTAGE_LIMIT] = REG_LOW_VOLTAGE_LIMIT_VALUE;
	regs[REG_HIGH_VOLTAGE_LIMIT] = REG_HIGH_VOLTAGE_LIMIT_VALUE;

	regs[REG_MOVING_THRESHOLD_DPS] = REG_MOVING_THRESHOLD_DPS_VALUE;
	regs[REG_STATUS_RETURN_LVL] = REG_STATUS_RETURN_LVL_VALUE;
	regs[REG_ALARM_LED] = REG_ALARM_LED_VALUE;
	regs[REG_ALARM_SHUTDOWN] = REG_ALARM_SHUTDOWN_VALUE;

	regs[REG_ENCODER_BITS] = REG_ENCODER_BITS_VALUE;
	regs[REG_MOTOR_POLE_PAIRS] = REG_MOTOR_POLE_PAIRS_VALUE;
	regs[REG_MOTOR_SYNCHRO_L] = LOW_BYTE(REG_MOTOR_SYNCHRO_VALUE);
	regs[REG_MOTOR_SYNCHRO_H] = HIGH_BYTE(REG_MOTOR_SYNCHRO_VALUE);
	regs[REG_INV_PHASE_MOTOR] = REG_INV_PHASE_VALUE;
	regs[REG_FIELD_WEAKENING_K] = REG_FIELD_WEAKENING_K_VALUE;


	regs[REG_PID_POSITION_KP_L] = LOW_BYTE(REG_PID_POSITION_KP_VALUE);
	regs[REG_PID_POSITION_KP_H] = HIGH_BYTE(REG_PID_POSITION_KP_VALUE);
	regs[REG_PID_POSITION_KI_L] = LOW_BYTE(REG_PID_POSITION_KI_VALUE);
	regs[REG_PID_POSITION_KI_H] = HIGH_BYTE(REG_PID_POSITION_KI_VALUE);
	regs[REG_PID_POSITION_KD_L] = LOW_BYTE(REG_PID_POSITION_KD_VALUE);
	regs[REG_PID_POSITION_KD_H] = HIGH_BYTE(REG_PID_POSITION_KD_VALUE);

	regs[REG_PID_VELOCITY_KP_L] = LOW_BYTE(REG_PID_VELOCITY_KP_VALUE);
	regs[REG_PID_VELOCITY_KP_H] = HIGH_BYTE(REG_PID_VELOCITY_KP_VALUE);
	regs[REG_PID_VELOCITY_KI_L] = LOW_BYTE(REG_PID_VELOCITY_KI_VALUE);
	regs[REG_PID_VELOCITY_KI_H] = HIGH_BYTE(REG_PID_VELOCITY_KI_VALUE);
	regs[REG_PID_VELOCITY_KD_L] = LOW_BYTE(REG_PID_VELOCITY_KD_VALUE);
	regs[REG_PID_VELOCITY_KD_H] = HIGH_BYTE(REG_PID_VELOCITY_KD_VALUE);
	regs[REG_PID_VELOCITY_KFF_L] = LOW_BYTE(REG_PID_VELOCITY_KFF_VALUE);
	regs[REG_PID_VELOCITY_KFF_H] = HIGH_BYTE(REG_PID_VELOCITY_KFF_VALUE);
	regs[REG_PID_ACCELERATION_KFF_L] = LOW_BYTE(REG_PID_ACCELERATION_KFF_VALUE);
	regs[REG_PID_ACCELERATION_KFF_H] = HIGH_BYTE(REG_PID_ACCELERATION_KFF_VALUE);

	regs[REG_PID_FLUX_CURRENT_KP_L] = LOW_BYTE(REG_PID_FLUX_CURRENT_KP_VALUE);
	regs[REG_PID_FLUX_CURRENT_KP_H] = HIGH_BYTE(REG_PID_FLUX_CURRENT_KP_VALUE);
	regs[REG_PID_FLUX_CURRENT_KI_L] = LOW_BYTE(REG_PID_FLUX_CURRENT_KI_VALUE);
	regs[REG_PID_FLUX_CURRENT_KI_H] = HIGH_BYTE(REG_PID_FLUX_CURRENT_KI_VALUE);
	regs[REG_PID_FLUX_CURRENT_KFF_L] = LOW_BYTE(REG_PID_FLUX_CURRENT_KFF_VALUE);
	regs[REG_PID_FLUX_CURRENT_KFF_H] = HIGH_BYTE(REG_PID_FLUX_CURRENT_KFF_VALUE);

	regs[REG_PID_TORQUE_CURRENT_KP_L] = LOW_BYTE(REG_PID_TORQUE_CURRENT_KP_VALUE);
	regs[REG_PID_TORQUE_CURRENT_KP_H] = HIGH_BYTE(REG_PID_TORQUE_CURRENT_KP_VALUE);
	regs[REG_PID_TORQUE_CURRENT_KI_L] = LOW_BYTE(REG_PID_TORQUE_CURRENT_KI_VALUE);
	regs[REG_PID_TORQUE_CURRENT_KI_H] = HIGH_BYTE(REG_PID_TORQUE_CURRENT_KI_VALUE);
	regs[REG_PID_TORQUE_CURRENT_KFF_L] = LOW_BYTE(REG_PID_TORQUE_CURRENT_KFF_VALUE);
	regs[REG_PID_TORQUE_CURRENT_KFF_H] = HIGH_BYTE(REG_PID_TORQUE_CURRENT_KFF_VALUE);

	regs[REG_CAL_PHASE1_CURRENT_SENSE_MA_L] = LOW_BYTE(REG_CAL_PHASE1_CURRENT_SENSE_MA_VALUE);
	regs[REG_CAL_PHASE1_CURRENT_SENSE_MA_H] = HIGH_BYTE(REG_CAL_PHASE1_CURRENT_SENSE_MA_VALUE);
	regs[REG_CAL_PHASE1_CURRENT_SENSE_OFFSET_L] = LOW_BYTE(REG_CAL_PHASE1_CURRENT_SENSE_OFFSET_VALUE);
	regs[REG_CAL_PHASE1_CURRENT_SENSE_OFFSET_H] = HIGH_BYTE(REG_CAL_PHASE1_CURRENT_SENSE_OFFSET_VALUE);

	regs[REG_CAL_PHASE2_CURRENT_SENSE_MA_L] = LOW_BYTE(REG_CAL_PHASE2_CURRENT_SENSE_MA_VALUE);
	regs[REG_CAL_PHASE2_CURRENT_SENSE_MA_H] = HIGH_BYTE(REG_CAL_PHASE2_CURRENT_SENSE_MA_VALUE);
	regs[REG_CAL_PHASE2_CURRENT_SENSE_OFFSET_L] = LOW_BYTE(REG_CAL_PHASE2_CURRENT_SENSE_OFFSET_VALUE);
	regs[REG_CAL_PHASE2_CURRENT_SENSE_OFFSET_H] = HIGH_BYTE(REG_CAL_PHASE2_CURRENT_SENSE_OFFSET_VALUE);

	regs[REG_CAL_PHASE3_CURRENT_SENSE_MA_L] = LOW_BYTE(REG_CAL_PHASE3_CURRENT_SENSE_MA_VALUE);
	regs[REG_CAL_PHASE3_CURRENT_SENSE_MA_H] = HIGH_BYTE(REG_CAL_PHASE3_CURRENT_SENSE_MA_VALUE);
	regs[REG_CAL_PHASE3_CURRENT_SENSE_OFFSET_L] = LOW_BYTE(REG_CAL_PHASE3_CURRENT_SENSE_OFFSET_VALUE);
	regs[REG_CAL_PHASE3_CURRENT_SENSE_OFFSET_H] = HIGH_BYTE(REG_CAL_PHASE3_CURRENT_SENSE_OFFSET_VALUE);

	regs[REG_CAL_VOLTAGE_SENSOR_L] = LOW_BYTE(REG_CAL_VOLTAGE_SENSOR_VALUE);
	regs[REG_CAL_VOLTAGE_SENSOR_H] = HIGH_BYTE(REG_CAL_VOLTAGE_SENSOR_VALUE);

	regs[REG_EWMA_ENCODER] = REG_EWMA_ENCODER_VALUE;

	return store_eeprom_regs();
}


void load_eeprom_regs()
{
	eeprom_restore(regs, REG_TORQUE_ENABLE); // REG_TORQUE_ENABLE must be 64 bits aligned
}

HAL_StatusTypeDef store_eeprom_regs()
{
	HAL_StatusTypeDef const result = eeprom_store(regs, REG_TORQUE_ENABLE);
	if (result != HAL_OK) hardware_fault_set(DRIVE_FAULT_INTERNAL);
	return result;
}

void reset_ram_regs()
{
	memset(&regs[REG_TORQUE_ENABLE],0,REG_MAX-REG_TORQUE_ENABLE);
}

bool control_persist_config(void)
{
    return store_eeprom_regs() == HAL_OK;
}

