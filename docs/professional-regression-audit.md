# Professional firmware regression audit

Recorded 2026-10-27. Scope: `01-Firmware/B-G431B-ESC1-v3`, comparing the
`simplified` baseline `8629fd6` with the completed `professional` refactors at
`d347de1`. Other firmware variants and deferred control policies are excluded.

No confirmed firmware regression was identified in this scoped source review
and the regression scenarios below. One documentation inconsistency was
corrected. This is software evidence, not a hardware equivalence or timing claim.
No production firmware changes were needed for this audit.

## Source comparison

| Area | Reviewed call path and result |
| --- | --- |
| Startup | Configuration/serial/encoder initialize before ADC/CAN use. Runtime reset and Drive initialization start disarmed. Peripheral setup and fatal MOE shutdown remain in their original layer. A failed configuration load now halts before ADC/CAN start, as documented in increment six. |
| Command and permission | Baseline CAN callback/shared goals now map to CanServo -> Drive -> copied command -> ServoController -> guarded FOC publication. Classic standard DLC8, reserved ARM/DISARM, duplicate ARM, clamp semantics, reply layout and accepted-command timestamps are preserved. Drive deliberately owns the command; raw runtime registers are compatibility mirrors. |
| Scheduling | Drive watchdogs precede one 16-bit timer sample; at most one 250 us servo job and 10000 us service job per pass, fixed-phase deadline advancement and bounded serial entry count match the baseline. No new task scheduler is introduced. |
| Controller | Position/velocity/current limits, raw gain multipliers (2 / 200 / 0.1), direction, feed-forward, per-sample filters and zero integral gain match the former main-loop calculation. ARM generations prevent stale current publication, and a rearm resets controller history. |
| Typed motor/fault boundary | Alignment remains signed, trim unsigned, current gains signed and scaled identically. Named flags retain all eight serial bit positions. Coherent views and nested IRQ restoration replace byte decoding; their hardware execution cost still needs measurement. |
| FOC/PWM/encoder | FOC diff is typed input/fault access and calibration extraction; current transforms, PI equations, ADC pairing, current limit, PWM/preload/coast and encoder prediction/fault policy are retained. The AS5600 driver, EEPROM backend, PID/math helpers, serial queue, CORDIC implementation and fatal IRQ handlers have no diff from the baseline. |
| Configuration | Validate/apply/storage boundaries retain the 128-byte image, fields/defaults, reserved bytes, page/doublewords, partial disarmed repairs and failed-write RAM behavior. Maintenance excludes ARM through persistence and successful reset delay. No CRC, flash rollback or transaction was added. |
| Calibration | The original 3 V, 501 forward/reverse positions, dwells, endpoint/pole/direction/offset checks and result codes remain. Motion now advances on foreground passes, with fresh samples after settling, extended dwells after delay, normal services/serial, immediate DISARM and guard lifetime through cleanup/save. App skips outer control so its old disable history cannot cancel the job. Cancellation during an executing save restores RAM and reports cancellation; flash may already contain the new alignment. |

The startup load-failure handling, authoritative command ownership and
nonblocking calibration/service behavior above are intentional refactor changes,
not hidden claims that every instruction sequence is identical to the baseline.
Existing sensor/control/transport and calibration suites check detailed numerical
and abnormal-input behavior alongside this review.

## Confirmed documentation issue

The calibration descriptions said that configuration writes, factory reset and
reboot all return a busy error. Actual `protocol.c` behavior is:

- A configuration write rejected by maintenance/calibration returns the existing
  serial access error (`0x07`).
- Factory reset and reboot rejected by the calibration guard are ignored without
  a reply; no reset or save occurs. No serial busy status is defined.
- Ping/read diagnostics continue during calibration.

Corrected the current drive contract, architecture and roadmap to match the
implementation. Integration scenarios exercise the real serial parser and these
responses. This audit does not introduce a new reply policy or wire status code.

## Tests added at module boundaries

The [integration suite](../tests/integration/README.md) links sixteen real modules
with the real FOC permission/cancellation/watchdog functions extracted from saved
source. It fills the gap between the existing App dependency mocks, independent
controller tests and standalone calibration tests. Production module state is
driven through public APIs, CAN frames, serial packets and actual encoder reads.

Nine scenario groups cover startup/normal control, watchdog boundaries/rollover,
obsolete calculations and acquisition preemption, encoder/TX failures,
recoverable fault/rearm, calibration ownership/serial interlocks, both calibration
directions and persistence, abnormal calibration completion, and configuration/
reset guards including failed saves. CAN ARM is injected during real Config_Save
and DISARM during real I2C acquisition and calibration saving. Command/controller
math, register encoding, configuration guards and calibration state transitions
are not reimplemented in the fixture.

The fixture models hardware register transfers, timestamps/heartbeat, analog
values, ideal rotor response, coast requests and reset/transmit/store outcomes.
It does not execute the ADC ISR/current/PWM body; the FOC/CORDIC suites remain
responsible for those software checks. Selected interrupt arrivals are modeled,
not an exhaustive interleaving search. Physical interrupts and rotor behavior
remain outside the host test guarantee.

## Validation and remaining work

- All fourteen host regression suites pass using native GCC 4.7.0. The nine
  integration scenario groups pass at both `-O0` and `-O2` with warnings treated
  as errors.
- Fresh ARM GCC 14.3.1 builds pass at `-Og` and `-O2`; the inherited CCMRAM/RWX
  linker warning remains. No device was flashed.
- The existing Makefile and `tests/run.ps1` remain the complete local workflow;
  the runner now includes integration. No new dependency was added.

Bench measurements of FOC duration, foreground/I2C timing, actual output levels,
current sensing, calibration under load and disabled back-driving remain pending.
Measured-travel response, armed overspeed, final watchdog thresholds, independent
watchdog design and storage integrity/migration remain explicit later decisions
in the [roadmap](professional-roadmap.md). This audit does not select those policies.
