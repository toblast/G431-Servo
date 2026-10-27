#ifndef INC_DRIVE_H_
#define INC_DRIVE_H_

#include "drive_types.h"

#define CAN_COMMAND_TIMEOUT_MS 1000U

// Init is startup-only, before interrupts can accept commands. Other operations
// below are atomic and callable from foreground or CAN IRQ, except maintenance
// transitions and calibration (foreground-only).
// Immediate FOC/CPU fault shutdown remains in the motor-control layer.
void Drive_Init(void);
bool Drive_Request_Arm(void);
void Drive_Disarm(void);
bool Drive_Submit_Command(ServoCommand const *command);
DriveCommandSnapshot Drive_Get_Command(void);
bool Drive_Publish_Currents(DriveCommandSnapshot const *command, bool new_arm,
                          float torque_mA, float flux_mA);
void Drive_Disable_If_Disarmed(void);
void Drive_Service(void);
bool Drive_Is_Disarmed(void);
bool Drive_Is_Maintenance(void);
bool Drive_Begin_Maintenance(bool disarm_first);
void Drive_End_Maintenance(void);
void Drive_Publish_Measurements(DriveMeasurements const *measurements);
DriveStatus Drive_Get_Status(void);

// Foreground-only request. Completion is reported asynchronously by Calibration.
// FOC retains its atomic entry guard and private active/cancelled flags.
bool Drive_Start_Calibration(void);

#endif
