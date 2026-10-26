#include "servo_controller.h"
#include "drive.h"
#include "pid.h"
#include "math_tool.h"
#include <string.h>

// Preserve the existing per-sample filters and gain scales. No elapsed-time
// normalization or new position integral is introduced by this extraction.
#define ALPHA_VELOCITY 0.24f

static struct {
    pid_context_t pd_position;
    float error_velocity_dps;
    uint32_t applied_arm_generation;
    bool currents_applied;
} controller;

void ServoController_Init(void)
{
    memset(&controller, 0, sizeof controller);
    pid_reset(&controller.pd_position);
}

ServoSetpoints ServoController_Update(DriveCommandSnapshot const *command,
                                     DriveConfig const *config,
                                     float position_deg, float velocity_dps)
{
    ServoSetpoints setpoints = {
        .position_deg = 0.0f, .velocity_dps = 0.0f,
        .torque_mA = 0.0f, .flux_mA = 0.0f
    };
    if (!command->enabled) {
        if (controller.currents_applied) {
            controller.currents_applied = false;
            // Recheck live permission: an IRQ may have armed since the snapshot.
            Drive_Disable_If_Disarmed();
        }
        controller.error_velocity_dps = 0.0f;
        // Drive_Disarm owns command reset; never erase a newly received ARM.
        return setpoints;
    }

    bool const new_arm = !controller.currents_applied ||
                        controller.applied_arm_generation != command->generation;
    if (new_arm) {
        // Keep commands accepted after ARM; reset only controller history.
        controller.error_velocity_dps = 0.0f;
        pid_reset(&controller.pd_position);
    }

    float const goal_position_deg = (float)command->goal.position_tenths_deg / 10.0f;
    float const min_position_deg = (float)config->min_position_deg;
    float const max_position_deg = (float)config->max_position_deg;
    setpoints.position_deg = fconstrain(goal_position_deg, min_position_deg, max_position_deg);

    float const goal_velocity_dps = command->goal.velocity_dps;
    float const max_velocity_dps = config->max_velocity_dps;
    setpoints.velocity_dps = fconstrain(goal_velocity_dps, -max_velocity_dps, max_velocity_dps);

    float const torque_feed_forward_mA = command->goal.torque_mA;
    float const error_position_deg = setpoints.position_deg - position_deg;
    float const pos_kp = (float)command->goal.pos_kp * 2.0f;
    float const pos_kd = (float)command->goal.pos_kd * 100.0f * 2.0f;
    float const max_current_mA = config->max_current_mA;
    float const vel_kp = (float)command->goal.vel_kp / 10.0f;
    controller.error_velocity_dps = ALPHA_VELOCITY * (setpoints.velocity_dps - velocity_dps) +
                                   (1.0f - ALPHA_VELOCITY) * controller.error_velocity_dps;
    float const reverse = config->invert_phase_motor == 0 ? 1.0f : -1.0f;
    setpoints.torque_mA = reverse * pid_process_antiwindup_clamp_with_ff(
        &controller.pd_position, error_position_deg, pos_kp, 0.0f, pos_kd,
        max_current_mA, 0.1f, vel_kp * controller.error_velocity_dps + torque_feed_forward_mA);
    setpoints.flux_mA = command->goal.flux_mA;

    // A DISARM, fault or new arm generation can invalidate this calculation.
    controller.currents_applied = Drive_Publish_Currents(command, new_arm,
                                                       setpoints.torque_mA, setpoints.flux_mA);
    if (controller.currents_applied) controller.applied_arm_generation = command->generation;
    return setpoints;
}
