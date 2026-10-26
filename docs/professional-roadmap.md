# Professional firmware roadmap

Recorded 2026-10-26. Branch `professional` starts at `8629fd6` on `simplified`.
The goal is professional software structure, ownership and testability, while
preserving the selected motor-control implementation and command contract.

## Architecture points saved for later

1. **Typed application model.** Use `DriveConfig`, `ServoCommand` and
   `DriveStatus` rather than reconstructing values from byte addresses throughout
   the application. Keep register addresses and packing at compatibility and
   communication boundaries. Preserve the existing serial/flash layout.
2. **One owner for application drive policy.** A drive supervisor should own
   arming, command acceptance, watchdog decisions and guarded current publication.
   FOC retains its internal operating modes and immediate fault shutdown.
   Calibration and maintenance permissions must remain explicit.
3. **Focused modules.** Gradually separate drive supervision, the outer servo
   controller, FOC, CAN framing, configuration/persistence and application
   scheduling. HAL callbacks should forward work through explicit interfaces.
4. **One foreground scheduler.** Eventually advance calibration as a state
   machine instead of duplicating acquisition/monitoring in blocking waits.
   This is a later behavior-sensitive change, not part of the first extraction.
5. **Tests at module interfaces.** Compile/link production modules directly
   where possible, replacing source-text extraction incrementally. Retain the
   existing abnormal-condition regressions and add a reproducible toolchain/CI.
6. **Consistent engineering conventions.** Make units, ownership, caller
   context, error returns and documentation explicit. Keep existing atomic
   publication and arm-generation protection visible in the interfaces.

## Earlier architecture assessments to retain

- Keep the ESP32/STM32 responsibility split. Cruise logic and vehicle inputs
  belong to ESP32; commutation, current/position control and encoder acquisition
  stay local to STM32.
- Keep the position-to-Iq-to-current-PI-to-PWM chain and initially no position
  integral. Spring feed-forward remains compatible with that architecture.
- Keep explicit ARM/DISARM, disabled startup, fault precedence, coast while
  disabled and acquisition while coasting.
- Measure actual FOC duration, foreground jitter and I2C acquisition time. Rates
  of 40 kHz and 4 kHz are targets, not established execution guarantees.
- Application state and register coupling are the main software organization
  weaknesses. Improve them incrementally without rewriting working FOC math.
- Later control decisions remain explicit: velocity-field purpose, derivative
  per sample versus velocity-error damping, measured-travel violation/recovery,
  armed overspeed behavior and final watchdog thresholds.
- Later reliability/interface work includes independent-watchdog design and
  physical release verification, state/fault CAN feedback after command bench
  validation, and configuration CRC/transactional storage with a migration policy.

## First refactor: typed model and drive supervisor

Current path: `main.c` CAN callback -> shared command registers/arming flags ->
foreground byte decoding -> outer controller -> guarded FOC current publication.
Serial configuration/reset and button calibration also depend on that state.

The first increment will introduce a typed command owner and drive supervisor,
typed views of active configuration and measurements, and a register adapter.
The selected wire format, flash layout, current/position equations, default
limits, watchdog thresholds, calibration trajectory and immediate disable paths
must remain unchanged. FOC's private modes remain separate from application
arming. Low-level FOC configuration/fault register access can be migrated later.

Acceptance checks:

- Startup/reset remains disarmed. ARM/DISARM, duplicate ARM, rejected messages,
  watchdog rollover and stale-generation rejection retain their behavior.
- CAN decode and serial access use supervisor entry points rather than owning
  arming or maintenance flags. Register command/status values remain compatible
  mirrors; changing a command mirror must not change the owned command.
- The outer loop consumes typed command/configuration data; arithmetic and
  update rates remain unchanged.
- Native tests link the new production module directly, including the existing
  CAN regression cases. Existing suites and both ARM optimization builds pass.
- No firmware flashing, mechanical assumptions or vehicle logic are introduced.

## Reference documents

- [Current drive contract](servo-drive.md)
- [Audit and completed fixes](simplified-audit.md)
- [Firmware pseudocode](firmware-pseudocode.md), recorded for the baseline
- [Regression suites](../tests/README.md)
