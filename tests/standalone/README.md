# Standalone cruise regression checks

Run `./tests/standalone/run.ps1 -Compiler 'C:/path/to/gcc.exe'` from the repository
root, or use the root runner. No extra test framework is required.

The suite links production cruise control, GPIO input adapter, standalone
service, application, supervisor, position controller, CAN transport, faults,
register adapter, configuration validation and existing PID/math helpers.
Only HAL, encoder acquisition, FOC hardware/calibration and serial I/O are mocked.
The constant configuration in the fixture is synthetic and is never compiled
into the production ARM image.

The same cases run at `-O0` and `-O2` with increasing and decreasing motor-angle
travel. Numerical cases check pulse geometry, cumulative-count and time wrap,
actual elapsed-time integration, derivative history, saturation, anti-windup,
nonfinite arithmetic, stale speed, counter reset and implausible pulses.

Integration passes call production `App_Process`: button capture -> local ARM
-> motor-position command -> existing position controller -> guarded current
publication. They check disabled startup, held button, raw brake/clutch
cancellation before a PID deadline, release/fresh-press recovery, speed
loss, drive fault, foreground delay, rejected CAN ARM/goal and accepted DISARM.
Input setup rejects unverified/duplicate/occupied pins before GPIO side effects;
counter snapshots preserve the incoming IRQ mask and ignore unrelated EXTI pins.
A noisy pulse burst latches invalid input and disables the speed IRQ until
restart; cancellation clears speed validity. Speed sampling continues while
brake/clutch is asserted, so stale speed cannot remain marked valid.
Startup tests separately compile both modes and verify CAN is not started in
standalone mode.

These are software checks. GPIO voltage levels, edge noise/loss, cancellation
latency under real FOC load, PID tuning, actuator travel and physical coast
behavior still need bench measurement. See [standalone setup](../../docs/standalone.md).
