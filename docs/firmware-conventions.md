# Firmware conventions

Applies to `01-Firmware/B-G431B-ESC1-v3` on `professional`. Keep the existing
Makefile and PowerShell regression runner; no extra development framework is
required. Module responsibilities are in [professional-architecture.md](professional-architecture.md),
the wire/operating contract in [servo-drive.md](servo-drive.md), and validation
commands in [tests/README.md](../tests/README.md).

## Names and units

- Application/service APIs use `Module_Action`, with PascalCase after one
  underscore: `Drive_RequestArm`, `Config_ApplyRegisterWrite`, `App_Process`.
  Types use PascalCase, fields/local helpers use snake_case, and constants/enums
  use uppercase names with a module or domain prefix.
- The register/motor adapters retain `control_*` and fault access retains
  `hardware_fault_*`, each with snake_case suffixes. Runtime register reset is
  `control_reset_runtime`; it is startup-only, never a substitute for DISARM.
- Preserve established low-level names (`API_FOC_*`, `positionSensor_*`,
  `AS5600_*`, `HAL_Serial_*`, EEPROM/PID/math helpers), generated STM32 code and
  encoded register names. A broad cosmetic rename of the motor stack adds no
  control benefit. New application code uses its module's declared convention.
- Declare no-argument public C functions with `(void)`. Include the defining
  public header in implementations and linked test doubles so signatures are checked.
- Angles and velocities refer to the motor shaft. Numeric names carry units:
  `_deg`, `_rad`, `_dps`, `_radps`, `_mA`, `_V`, `_C`, `_us`, `_ms`.
  `position_tenths_deg` is the explicit 0.1-degree command representation.
  Sensor angles are unwrapped from boot; timestamps wrap at their integer width.
- `_raw` gain fields are encoded numeric values, not physical gains or packet
  bytes. `ServoCommand.position_kp_raw` multiplies by 2, `position_kd_raw` by
  200 against the existing filtered per-sample difference, and `velocity_kp_raw`
  divides by 10. CAN supplies zero velocity gain. Motor current PI gains retain
  Kp / 100000 and Ki / 100000000. Conversions stay at the consuming boundary.
- Names are independent of CAN packing and serial/flash addresses. Renaming a
  C field/function must not change those representations, scales or signedness.

## Ownership and caller context

Headers state who may call, when initialization occurs, which pointers are
borrowed, and whether work blocks. Unless explicitly allowed, pointer arguments
are nonnull and borrowed only during the call. Snapshots are copied values.

| Owner | Caller and lifetime contract |
| --- | --- |
| App / outer controller / calibration | Single foreground instance, nonreentrant. App retains timer/serial handles; they outlive its processing. Controller inputs are borrowed, never retained. Calibration owns one job and one unread completion. |
| Drive supervisor | ARM/DISARM, command submission and copied reads may run from foreground/CAN IRQ. Controller publication and watchdog service run in foreground; maintenance/calibration entry are foreground-only. Recheck live generation/permission before applying a calculated current. |
| CAN transport | One retained controller handle; bind before start/notifications. IRQ-only RX processing, bounded to entry FIFO occupancy. Buffers are private; queue failure does not undo an accepted command. |
| Configuration service | Pure validation can run from foreground/IRQ. Load is startup-only. Writes/reset/save are synchronous foreground operations with IRQs enabled and explicit startup/maintenance/calibration exclusion. |
| Register/motor/fault adapters | Coherent snapshots, alignment writes and fault updates restore the caller IRQ mask. Bare mirror/byte-copy primitives require their documented caller guard; they do not select drive policy. |
| Encoder / AS5600 | Initialize before ADC/CAN; acquire synchronously in foreground with IRQs enabled. Publish a complete sample atomically. FOC reads prediction/individual published values; acquisition health is reported through faults. |
| FOC | Existing ADC current path owns commutation, protection and immediate coast. Drive owns normal command permission; Calibration owns foreground trajectory and uses guarded FOC excitation primitives. |

Never add blocking I/O inside a critical section, allocation in a control path,
or a second owner/cache of accepted commands. Restore the saved PRIMASK rather
than unconditionally enabling IRQs. I2C and the final calibration flash save are
still synchronous; the save occurs while coasting. Physical timing remains a
bench measurement, even when compilation and host regressions pass.

## Return and failure contracts

| Return type | Interpretation |
| --- | --- |
| `bool` operation | `true` means accepted/succeeded, `false` means rejected/failed. A start result is acceptance, not asynchronous completion. |
| `bool` query | Named condition only. Sensor readiness means successful initialization, not absence of a later encoder fault. `Drive_IsDisarmed` excludes calibration but may coexist with faults/maintenance. |
| `ConfigResult` | `CONFIG_OK` applied/saved; `CONFIG_ACCESS_ERROR` or `CONFIG_BUSY` applies nothing; `CONFIG_STORAGE_ERROR` latches the internal fault and leaves applied RAM values. Successful factory reset retains maintenance until caller reset. |
| `CalibrationResult` | `Calibration_TakeResult` returns one completion: 0 success, 1 no motion, 2 pole mismatch, 3 cancel/fault, 4 storage failure. Rejected start creates no completion. NULL/no pending result returns false without consumption. |
| `HAL_StatusTypeDef` | Low-level I2C/flash convention: `HAL_OK` is zero. Compare explicitly with `HAL_OK`; do not treat it as boolean success. Ignore failed-read output. |
| Serial byte count | Positive queued/read byte count; zero means no progress/rejection. Queue acceptance does not prove physical transmission. |

Do not add a second status enum merely to unify unlike interfaces. Preserve
fault side effects and existing protocol error mappings. Save failures do not
promise flash rollback, and calibration RAM rollback cannot undo a completed
flash write. Header comments give operation-specific details.

## Documentation and local verification

The README, professional architecture, drive contract, roadmap and this guide
describe the current branch. The illustrated architecture and original pseudocode
are explicitly labeled snapshots of `simplified` at `8629fd6`; the cloud handover
and audit preserve their dated history. Use their current-document links when
navigating the edited firmware.

After interface changes, update production callers, test doubles and extraction
signatures together, then run the existing regression suites and ARM builds.
Use numerical/abnormal-condition regressions for behavior; avoid tests that merely
repeat spelling or formatting. Keep one logical change per commit. No new CI,
formatter or lint dependency is required for this project.
