#ifndef INC_DRIVE_H_
#define INC_DRIVE_H_

#include "drive_types.h"

#define CAN_COMMAND_TIMEOUT_MS 1000U

// One private supervisor. Input pointers are nonnull, borrowed for the call and
// copied when retained. Snapshots/status are returned by value. Runtime calls
// restore the caller's IRQ mask; maintenance/calibration entry are foreground-
// only. Immediate FOC/CPU fault shutdown remains in the motor-control layer.
// Startup-only, before interrupts can accept commands; always starts disarmed.
void Drive_Init(void);
// Foreground/CAN IRQ. True means accepted (including duplicate ARM); rejection
// may latch the configuration fault. Accepted ARM refreshes the command tick.
bool Drive_RequestArm(void);
// Foreground/CAN IRQ. Clears permission/goals and immediately coasts/cancels FOC.
void Drive_Disarm(void);
// Foreground/CAN IRQ. True means the goal was copied and watchdog refreshed.
// Rejected commands change neither the goal nor its acceptance timestamp.
bool Drive_SubmitCommand(ServoCommand const *command);
// Foreground/CAN IRQ; coherent copied views, not permission to publish later.
DriveCommandSnapshot Drive_GetCommand(void);
DriveStatus Drive_GetStatus(void);
// Foreground controller. True means currents passed the live permission/fault/
// generation check. new_arm initializes FOC only for that accepted generation.
bool Drive_PublishCurrents(DriveCommandSnapshot const *command, bool new_arm,
                          float torque_mA, float flux_mA);
// Foreground controller cleanup; live permission is rechecked before disable.
void Drive_DisableIfDisarmed(void);
// Foreground, every pass before other jobs; disarms on faults/watchdog expiry.
void Drive_Service(void);
// Foreground/CAN IRQ. Disarmed excludes calibration, but does not imply absence
// of faults or maintenance. Check entry operations' return values for permission.
bool Drive_IsDisarmed(void);
bool Drive_IsMaintenance(void);
// Foreground-only, nonnested. True acquires the ARM exclusion guard; when
// disarm_first is true, disarm as part of acceptance. Calibration rejects entry.
bool Drive_BeginMaintenance(bool disarm_first);
// Foreground-only; call only after successful maintenance entry/cleanup.
void Drive_EndMaintenance(void);
// Foreground; copy a complete measurement group (caller masks ADCs for capture).
void Drive_PublishMeasurements(DriveMeasurements const *measurements);

// Foreground-only. True starts a job; false leaves no new completion result.
// Accepted completion is reported asynchronously by Calibration, once.
// FOC retains its atomic entry guard and private active/cancelled flags.
bool Drive_StartCalibration(void);

#endif
