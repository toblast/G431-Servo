#include "standalone.h"

// Fill measured hardware values here after the wiring and actuator are chosen.
// Zero geometry/gains/travel and unverified inputs deliberately prevent ARM.
// These settings are source-only; the existing EEPROM/register ABI is unchanged.
StandaloneConfig const standalone_config = {
    .cruise = {
        .period_ms = 20,
        .maximum_gap_ms = 100,
        .speed_window_ms = 100,
        .speed_timeout_ms = 500,
        .button_debounce_ms = 30,
        .derivative_filter_s = 0.1f
    }
};
