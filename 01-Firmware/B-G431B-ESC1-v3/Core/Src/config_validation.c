#include "configuration.h"
#include "control_table.h"
#include <stddef.h>

bool Config_Validate(DriveConfig const *config)
{
    return config != NULL &&
           config->min_position_deg < config->max_position_deg && config->min_position_deg >= -3276 && config->max_position_deg <= 3276 &&
           config->max_velocity_dps > 0 && (uint16_t)config->max_velocity_dps <= SERVO_BENCH_MAX_VELOCITY_DPS &&
           config->max_current_mA > 0 && config->max_current_mA <= SERVO_BENCH_MAX_CURRENT_MA &&
           config->encoder_bits == 12 && config->motor_pole_pairs != 0 && config->invert_phase_motor <= 1 &&
           config->low_voltage_V > 0 && config->low_voltage_V < config->high_voltage_V &&
           config->high_voltage_V <= REG_HIGH_VOLTAGE_LIMIT_VALUE &&
           config->temperature_limit_C > 0 && config->temperature_limit_C <= REG_TEMPERATURE_LIMIT_VALUE &&
           config->flux_current_kp_raw >= 0 && config->flux_current_ki_raw >= 0 &&
           (config->flux_current_kp_raw > 0 || config->flux_current_ki_raw > 0) &&
           config->torque_current_kp_raw >= 0 && config->torque_current_ki_raw >= 0 &&
           (config->torque_current_kp_raw > 0 || config->torque_current_ki_raw > 0);
}

bool Config_IsValid(void)
{
    DriveConfig const config = control_read_config();
    return Config_Validate(&config);
}
