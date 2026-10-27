#ifndef INC_SERVO_CONTROLLER_H_
#define INC_SERVO_CONTROLLER_H_

#include "drive_types.h"

// Calculated setpoints for diagnostics, not proof that publication was accepted.
typedef struct {
    float position_deg, velocity_dps;
    float torque_mA, flux_mA;
} ServoSetpoints;

// One foreground-owned instance. Initialize before the first Update; neither
// operation is reentrant or callable from an interrupt. Update consumes the
// caller's current motor-shaft sample and does not acquire sensors or schedule.
// Input pointers are nonnull, borrowed for this call and never retained. Enabled
// commands require validated config; disabled snapshots return zero diagnostics
// even during staged configuration repairs. These APIs do not change arming.
void ServoController_Init(void);
ServoSetpoints ServoController_Update(DriveCommandSnapshot const *command,
                                     DriveConfig const *config,
                                     float position_deg, float velocity_dps);

#endif
