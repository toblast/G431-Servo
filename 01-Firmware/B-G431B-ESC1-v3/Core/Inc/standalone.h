#ifndef INC_STANDALONE_H_
#define INC_STANDALONE_H_

#include "vehicle_inputs.h"
#include "drive_mode.h"

typedef struct {
    CruiseConfig cruise;
    VehicleInputConfig inputs;
    // Measured motor-shaft positions for closed/open actuator travel. Either
    // direction is supported. No final gear ratio is assumed.
    float closed_position_deg, open_position_deg;
    uint8_t position_kp_raw, position_kd_raw;
} StandaloneConfig;

extern StandaloneConfig const standalone_config; // Board-specific source settings.

// Single foreground instance, no allocation; config copied on startup.
// Invalid settings keep local cruise disabled and inputs uninitialized.
bool Standalone_Init(StandaloneConfig const *config);
// Every App pass before the servo; reads cancel inputs every pass, runs road PID
// on its own millisecond deadline without catch-up bursts. Uses Drive's guarded
// ARM/command/DISARM APIs; never writes FOC currents directly.
void Standalone_Process(void);
CruiseResult Standalone_GetStatus(void); // Foreground diagnostic copy.

#endif
