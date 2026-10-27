# Firmware integration regression checks

Run through `tests/run.ps1`, or individually from the repository root:

```powershell
./tests/integration/run.ps1 -Compiler 'C:/path/to/gcc.exe'
```

This suite links sixteen production modules directly: App, Drive, the outer
controller, CAN transport, calibration, the sensor/AS5600 drivers, configuration
validation/service/storage, register/motor/fault adapters, serial protocol and
PID/math helpers. It runs at `-O0` and `-O2`, using the ARM target's unsigned-char
behavior for serial bytes. No additional framework or setup tool is required.

Complete FOC enable/disable/current-publication, watchdog and calibration guard
functions are extracted from the saved source, with their actual private FOC
variables. The test observes FOC modes/targets and coast requests at this boundary.
It never includes or edits private App, Drive, controller, calibration, CAN or
configuration state. The existing peripheral-start, FOC and CORDIC suites cover
the current/PWM/ADC math and interrupt body separately.

Nine scenario groups exercise:

- Disarmed startup with/without the encoder, startup-held button, real CAN
  command decoding, gain scales, clamping, telemetry and duplicate ARM.
- Invalid DLC/wrong ID without watchdog refresh, exact 1000 ms timeout boundary,
  millisecond/microsecond rollover and loss of FOC heartbeat.
- An obsolete calculation crossing CAN DISARM/rearm, controller-history reset,
  and CAN DISARM preempting an actual I2C acquisition.
- TX failure without command rollback, failed encoder reads preserving the sample
  and latching faults, and recovery from a voltage fault requiring explicit ARM.
- Calibration after active control without a disabled tick, exclusive excitation,
  continued acquisition/protection service/serial diagnostics, configuration
  access errors and ignored reset/reboot requests, cancellation and rearm.
- Successful calibration in both rotor directions using quantized AS5600 reads,
  actual image persistence, ARM exclusion during save, storage failure, and
  cancellation during save with RAM rollback but no flash rollback guarantee.
- No motion, incorrect pole count, lost encoder and FOC loss during calibration.
- Real serial configuration/maintenance with ARM arriving during saving,
  retained RAM on failed persistence, reboot and factory-reset guard lifetime,
  and factory-reset failure without reboot.

Hardware boundaries are deterministic mocks: CAN RX/TX, UART bytes/replies,
I2C register transactions, flash backend, GPIO, analog values, coast and FOC
heartbeat. A simple ideal rotor follows the requested calibration angle; it is
not a motor dynamics model. One selected CAN interrupt is injected inside an I2C
transaction, and others during saves. This does not explore all interrupt
interleavings or establish physical PWM, execution time, control stability,
back-driving or power-loss storage behavior. The real reset delay blocks virtual
foreground time; the mock reset records the request and returns only for testing.
