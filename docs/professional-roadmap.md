# Professional firmware roadmap

Recorded 2026-10-26. Branch `professional` starts at `8629fd6` on `simplified`.
The goal is professional software structure, ownership and testability, while
preserving the selected motor-control implementation and command contract.

## Revised working to-do list

Priorities revised 2026-10-27 for a personal project. The goal is readable,
maintainable firmware with a simple local workflow. Keep the existing Makefile
and regression runner; CI is optional and deferred, not a prerequisite for the
next refactor. This list sets the current order; the original goals and completed
increments below remain the reference history.

Completed:

- [x] Introduce typed command/status interfaces, typed configuration views and
  the drive supervisor.
- [x] Extract the outer servo controller and its private control history.
- [x] Extract CAN frame handling and private transport buffers.

Next software work, in order:

1. [ ] **Extract foreground application scheduling.** Move acquisition, servo
   updates, telemetry and periodic services from `main.c` into an application
   module. Keep the current execution order, timing rules, watchdog checks and
   bounded serial work. Leave hardware setup and HAL forwarding in `main.c`.
2. [ ] **Complete typed configuration and fault boundaries.** Replace remaining
   raw-register dependencies in FOC and encoder code with explicit interfaces.
   Preserve units/scales, fast control calculations and fault/shutdown behavior.
3. [ ] **Clarify configuration and persistence responsibilities.** Give
   validation, application and storage clear interfaces while preserving the
   existing serial addresses and flash format. CRC, transactional storage and
   migration remain separate later reliability work.
4. [ ] **Make calibration nonblocking.** Advance explicit calibration states
   through the normal foreground scheduler, preserving the trajectory,
   cancellation, fault checks and completion/storage behavior. This is a
   behavior-sensitive change and needs corresponding abnormal-condition tests.
5. [ ] **Finish convention and documentation consistency.** Apply clear names,
   explicit units, caller/ownership contracts and consistent error returns as
   modules are touched, then review the remaining inconsistencies. Preserve
   working FOC code when the proposed change is only cosmetic.

Throughout these steps:

- Keep small logical changes and directly test new module interfaces. Retain
  the existing abnormal-condition regressions and build after meaningful code
  changes; replace source extraction where a module boundary makes that useful.
- Keep build/test instructions short: required tools, the compiler versions
  used for validation and the local commands. Add setup machinery only when a
  concrete need appears. CI remains an optional future convenience.
- Update this roadmap and the architecture guide after each extraction.

Timing measurements, bench checks and the control/reliability decisions listed
below remain pending. Structural refactors do not select new motor-control
policies or establish hardware validation results.

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
   existing abnormal-condition regressions. Document the simple local build and
   test process; defer optional CI and elaborate toolchain setup.
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

Implemented as the first increment on `professional`. Details, remaining
boundaries and verification are in [professional-architecture.md](professional-architecture.md).

Baseline path: `main.c` CAN callback -> shared command registers/arming flags ->
foreground byte decoding -> outer controller -> guarded FOC current publication.
Serial configuration/reset and button calibration also depend on that state.

The first increment introduces a typed command owner and drive supervisor,
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

## Second refactor: outer servo controller

The outer controller now lives in `servo_controller.c`, with one private owner
for PD history, filtered velocity error and the last applied arm generation.
`main.c` acquires the sensor sample, captures typed inputs and invokes the module
on the existing schedule. Diagnostic setpoints are returned as `ServoSetpoints`;
the supervisor remains the final authority for current publication and disable.

Gain scales, per-sample filters, feed-forward, current/travel/velocity setpoint
limits, phase reversal and zero position-integral gain are preserved. No CAN
field, rate, watchdog threshold, FOC or encoder implementation changed.
The directly linked servo suite covers numerical behavior and publication races.

Roadmap item 3 remains partial: foreground scheduling remains in `main.c`;
configuration/persistence separation and low-level register coupling remain later
increments. CAN handling is extracted in the third refactor below.

## Third refactor: CAN transport and protocol handling

`can_servo.c` owns the bound FDCAN handle, private receive/transmit buffers,
bounded FIFO processing, command-frame validation/decoding and feedback encoding.
`main.c` binds it before CAN start and forwards the HAL receive callback.
Peripheral configuration, filters, transceiver pins and HAL start stay in
`main.c`; drive policy and watchdog history remain in the supervisor.

The 8-byte command format, reserved ARM/DISARM values, accepted identifiers and
frame types, four-byte feedback, acceptance timestamps, calibration permissions
and entry-time FIFO work bound are preserved. TX queue failure retains its
existing behavior: no retries or command rollback, including immediate DISARM.

Tests link the production CAN module directly and exercise binding, private
receive capacity, reply metadata/encoding and TX failures alongside the existing
malformed-frame, watchdog and supervisor regressions, at both host optimizations.
All twelve regression suites and ARM builds at `-Og`/`-O2` pass.

Next planned work is foreground application scheduling separation, followed by
the revised software list above. Keep local build/test instructions simple;
CI is optional and deferred. Blocking calibration, remaining register coupling,
configuration/persistence separation, wider convention consistency and the
earlier control/hardware decisions above remain pending.

## Reference documents

- [Current drive contract](servo-drive.md)
- [Audit and completed fixes](simplified-audit.md)
- [Firmware pseudocode](firmware-pseudocode.md), recorded for the baseline
- [Regression suites](../tests/README.md)
