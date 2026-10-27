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
- [x] Extract foreground application scheduling, acquisition, telemetry and
  periodic services into `app.c`.
- [x] Add typed configuration, alignment, permission and fault interfaces for
  FOC/PWM and encoder consumers, preserving the existing register image.
- [x] Separate configuration validation, guarded application and fixed-image
  persistence, retaining the serial map and flash format.
- [x] Advance calibration motion through explicit foreground states, preserving
  the trajectory, fault/cancellation checks and guarded completion/storage.
- [x] Finish convention and documentation consistency: application API names,
  explicit numeric units/scales, caller/ownership and return contracts, current
  references and clearly labeled historical guides.

The revised software-structure list is complete. Follow-up work remains the
bench measurements and the explicit control/reliability decisions below;
they require separate validation or a selected policy. CI remains optional.

Continuing workflow:

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

These are the original design goals, retained as history. The completed list
above and increments below describe their current implementation status.

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

The later increments below complete CAN handling, foreground scheduling, typed
low-level boundaries and separation of configuration/persistence responsibilities.

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

Application scheduling is extracted in the fourth refactor below. Keep local
build/test instructions simple; CI is optional and deferred.

## Fourth refactor: foreground application scheduling

`app.c` owns the foreground deadlines, counters, acquisition/controller calls,
measurement and diagnostic publication, periodic configuration/FOC service,
LED/button handling and bounded serial processing. `main.c` retains hardware and
motor-dependency startup, CAN setup and HAL forwarding. It calls `App_Init` once
after CAN setup and `App_Process` once per foreground pass.

The scheduling rules are preserved: watchdogs first, one 16-bit microsecond timer
sample per pass, at most one 250 us servo update and one 10000 us service update,
with deadlines advanced by their fixed periods. Delayed work catches up over
later passes. Service uses the original time sample even if acquisition takes
time. Serial processing is limited to the bytes present when that section starts.
At this increment calibration remained blocking, with the same button-edge
interlocks, cancellation and result reporting. Controller, drive, CAN, FOC and
encoder implementations are unchanged. Inactive trace/debug scaffolding was
removed with the old loop.

The application suite directly links `app.c` and the register adapter, checking
deadlines, phase, rollover, call order, delayed acquisition, faults, button edges,
then-blocking calibration, serial refill/shrink and IRQ-mask restoration. Telemetry
tests also link the application directly and exercise counters through public
passes, replacing source-text extraction. Startup checks verify application
initialization after its dependencies. All thirteen regression suites and ARM
builds at `-Og`/`-O2` pass.

Typed configuration/fault boundaries are implemented in the fifth refactor below.

## Fifth refactor: typed motor configuration and faults

`motor_config.h` exposes narrow `MotorControlConfig`, `MotorProtectionConfig` and
`MotorAlignment` views. `motor_config.c` owns their legacy register decoding,
coherent alignment reads/writes, live torque-permission access and encoder error
counter updates. FOC, PWM helpers and the encoder no longer access the control
register map. Gain divisors, signed alignment/unsigned trim, limits, sensor
prediction, current regulation, shutdown behavior and calibration trajectory
remain unchanged. Calibration saves through a boolean bridge to the existing
whole-image storage and retains its failure/rollback behavior.

`drive_faults.h` defines named masks and `DriveFaultFlags` with the same eight
serial bit assignments. Its implementation retains the existing diagnostic byte
as authoritative storage. Set/clear operations preserve unrelated bits and the
caller IRQ mask. Fast motor readers and fault helpers are placed in CCMRAM;
the motor snapshot uses a short critical section. No second configuration cache,
new persistence format or fault-recovery policy is introduced.

All thirteen suites pass. Added checks cover typed signedness/scales, coherent
snapshots, complete alignment publication/restore, permission semantics, wrapping
encoder diagnostics, every fault bit, IRQ masks, persistence error propagation
and successful FOC gain/angle consumption. Existing calibration cancellation,
encoder failure, immediate coast and malformed CAN regressions remain. ARM
builds at `-Og` and `-O2` pass; hardware timing of the added interface calls and
snapshot critical section still requires bench measurement.

Configuration/persistence responsibilities are separated in the sixth refactor
below.

## Sixth refactor: configuration validation, application and persistence

`config_validation.c` owns pure `Config_Validate` and current-value validation.
`configuration.c` owns startup loading, guarded register writes, factory reset
and save/fault handling. `config_storage.c` transfers a fixed 128-byte
`ConfigImage` through the unchanged EEPROM backend. `control_table.c` now owns
only register/default encoding, coherent image capture/application and runtime
mirrors; it performs no flash I/O. Serial framing stays in `protocol.c`, which
delegates configuration operations and retains its LED/reboot handling.

Range and maintenance checks precede write application. Partial disarmed repairs
remain accepted and persisted even when an intermediate image is invalid; safety
validation still gates ARM and periodic operation. Factory reset disarms and
holds maintenance through the reply/reset delay on success, releasing it on
failure. Calibration saves while its existing exclusion guard remains active.
Flash I/O occurs with IRQs enabled after a bounded atomic image snapshot.

The image remains exactly bytes 0x00..0x7f, with the same fields, defaults,
reserved-byte behavior, flash page and doubleword writes. Runtime values are not
persisted or changed by loading configuration. No CRC, transaction metadata,
migration, RAM rollback on failed serial writes or new configuration cache is
introduced. Startup now checks a failed storage read before applying the image,
latching the internal fault and halting before ADC/CAN startup.

All thirteen suites pass. Configuration tests directly link the real validation,
service, register, motor/fault and storage modules. They cover empty/existing
images, invalid stored settings, read/write failures, runtime exclusion, staged
repairs, guard lifetime, reset failure and preserved reserved bytes. Serial tests
also link the real configuration path, retaining ARM-race, range and reply/error
coverage. Both ARM builds pass. Calibration scheduling is implemented in the
seventh refactor below; conventions are completed in the eighth. The earlier
bench/control/reliability decisions remain pending.
Keep the existing local workflow; CI is optional and deferred.

## Seventh refactor: foreground calibration state machine

`calibration.c` owns one foreground job with explicit coast, settling, forward,
reverse, endpoint capture, alignment and save states. `Drive_StartCalibration`
checks entry permission; FOC retains atomic entry/excitation guards and immediate
coast/cancellation. `App_Process` advances at most one transition after its normal
jobs and reports completion once. The outer servo calculation is bypassed while
calibration owns excitation; normal sensor acquisition, measurement publication,
protection services and bounded serial processing continue.

The 3 V trajectory, 501 positions in each direction, 2 ms step dwell, settling
intervals, direction/pole checks, offset calculation and result codes are retained.
Each endpoint capture waits for a subsequent normal acquisition after settling.
Delayed passes extend a dwell instead of publishing a burst of targets. DISARM,
encoder faults and FOC loss cancel; failures restore the original RAM alignment.
ARM/configuration maintenance stay excluded through cleanup and persistence.
Serial requests now execute during motion: configuration writes return the
existing access error; reset/reboot requests are ignored without a reply while
the calibration guard is active. They no longer wait for motion to finish.

The final whole-image flash save remains synchronous while coasting; I2C reads
also retain their synchronous implementation. No asynchronous flash driver or
power-loss transaction is introduced. Cancellation during an already executing
save restores RAM and reports cancellation, but cannot undo a flash write.

All thirteen suites and ARM builds at `-Og`/`-O2` pass. Tests link the calibration
job directly at `-O0`/`-O2` and check the full trajectory, fresh endpoint capture,
no catch-up bursts, rollover, invalid entry, cancellation, sensor/FOC failures,
storage failure and completion/IRQ semantics. Application tests check continued
services/serial/measurements, excitation ownership, button interlocks and one-shot
reporting. Hardware timing and motor behavior still require bench validation.

## Eighth refactor: conventions and documentation consistency

Application/service functions use `Module_Action` with a PascalCase action:
the Drive API now matches App, Config, Calibration and ServoController. The
register adapter's startup reset is `control_reset_runtime`. Command gain fields
identify their encoded scale (`position_kp_raw`, `position_kd_raw`,
`velocity_kp_raw`); sensor timestamps, velocity and accumulated turns carry their
actual units. Sensor initialization/readiness now return `bool` with the same
true/false behavior. Production callers and existing test fixtures use these
interfaces consistently.

Public headers state caller context, pointer lifetime, snapshot/IRQ obligations,
success/rejection semantics and synchronous work. No-argument public declarations
use `(void)`. The focused [conventions guide](firmware-conventions.md) records
names, units, return families, low-level compatibility exceptions and the existing
local workflow. Current architecture/operating guides link it; illustrated and
handover documents explicitly identify their historical baseline and link current
references. Old code maps are not presented as the current implementation.

FOC calculations/implementation, gains, limits, CAN/serial/flash layouts,
interrupt policy and calibration behavior are unchanged. Existing numerical,
malformed-input, fault, cancellation and storage regressions validate the rename
and type cleanup; all thirteen suites and ARM builds at `-Og`/`-O2` pass. No
new tooling or hardware validation claim is introduced. The software roadmap is
complete; bench measurements and deferred policy decisions remain pending.

## Post-refactor regression audit and integration checks

Compared baseline `8629fd6` with the completed refactors at `d347de1`, focusing
on startup, arithmetic/scales, permissions/generations, watchdogs, configuration
exclusion and calibration/fault completion. No confirmed firmware regression was
identified by the scoped review and tests. Corrected one documentation mismatch:
calibration rejects serial configuration writes with access error, while
factory-reset/reboot requests are ignored without a reply.

Added nine integration scenario groups linking sixteen real modules and extracted
FOC guards at `-O0`/`-O2`, using the existing native compiler/runner. They exercise
CAN/App/controller and calibration/encoder/configuration/serial handoffs, selected
interrupt arrivals, rollover, faults and storage/reset guards. All fourteen
regression suites and fresh ARM builds at `-Og`/`-O2` pass. No production firmware
change or new tooling was needed. Details and limitations are in the
[regression audit](professional-regression-audit.md); bench validation and the
deferred control/reliability decisions above remain pending.

## Reference documents

- [Current drive contract](servo-drive.md)
- [Audit and completed fixes](simplified-audit.md)
- [Firmware pseudocode](firmware-pseudocode.md), recorded for the baseline
- [Regression suites](../tests/README.md)
- [Firmware conventions](firmware-conventions.md)
- [Post-refactor regression audit](professional-regression-audit.md)
