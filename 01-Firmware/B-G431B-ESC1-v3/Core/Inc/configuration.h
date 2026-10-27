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
bool Config_Validate(DriveConfig const *config);
bool Config_IsValid(void);

// Startup-only, before drive/ADC/CAN startup: provision an empty page, otherwise
// load the existing image. Invalid settings are retained for disarmed repair.
// I/O failure latches the internal fault and returns false to the startup caller.
bool Config_Load(void);

// Foreground/startup only. Caller owns exclusion: startup, maintenance, or FOC's
// calibration guard, with IRQs enabled. Capture a bounded image atomically; I/O runs outside
// the snapshot critical section. Failure latches the internal fault, with no
// promise of flash rollback or restoration of the applied RAM values.
bool Config_Save(void);

// Range/permission checks precede atomic application. Partial updates remain
// supported: safety validation gates ARM, not each intermediate repair write.
ConfigResult Config_ApplyRegisterWrite(uint16_t address, uint16_t length, uint8_t const *data);
// Disarm and apply/persist defaults. Success keeps maintenance through the
// caller's reply/reset delay; failure releases it and reports the storage fault.
ConfigResult Config_FactoryReset(void);

#endif
