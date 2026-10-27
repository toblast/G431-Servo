#ifndef INC_CONFIGURATION_H_
#define INC_CONFIGURATION_H_

#include "drive_types.h"

typedef enum {
    CONFIG_OK,
    CONFIG_ACCESS_ERROR,
    CONFIG_BUSY,
    CONFIG_STORAGE_ERROR
} ConfigResult;

// Safety validation is independent of persistence and arming permission.
// Validate is pure and returns false for NULL; IsValid snapshots live settings
// coherently. Both may run in foreground or IRQ and retain no caller pointer.
bool Config_Validate(DriveConfig const *config);
bool Config_IsValid(void);

// Startup-only, before drive/ADC/CAN startup: provision an empty page, otherwise
// load the existing image. Invalid settings are retained for disarmed repair.
// I/O failure latches the internal fault and returns false to the startup caller.
bool Config_Load(void);

// Foreground/startup only. Caller owns exclusion: startup, maintenance, or FOC's
// calibration guard, with IRQs enabled. Capture a bounded image atomically;
// synchronous I/O runs outside the snapshot critical section. True means saved.
// Failure latches the internal fault; it does not promise flash rollback or
// restoration of the applied RAM values.
bool Config_Save(void);

// Foreground-only, IRQs enabled, synchronous I/O; data is borrowed for the call.
// CONFIG_OK means applied and saved. CONFIG_ACCESS_ERROR/CONFIG_BUSY leave
// settings unchanged; CONFIG_STORAGE_ERROR leaves applied RAM values and
// latches the internal fault.
// Range/permission checks precede atomic application. Partial updates remain
// supported: safety validation gates ARM, not each intermediate repair write.
ConfigResult Config_ApplyRegisterWrite(uint16_t address, uint16_t length, uint8_t const *data);
// Foreground-only, IRQs enabled. Acquire maintenance, disarm and apply/persist
// defaults; CONFIG_BUSY makes no change. Success keeps maintenance through the
// caller's reply/reset delay; failure releases it and reports the storage fault.
ConfigResult Config_FactoryReset(void);

#endif
