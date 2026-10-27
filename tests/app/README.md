# Foreground application regression checks

Run from the repository root:

```powershell
./tests/app/run.ps1 -Compiler 'C:/path/to/gcc.exe'
```

The runner links production `app.c` and `control_table.c` as separate translation
units at `-O0` and `-O2`. Hardware, drive/controller calls and protocol parsing are
mocked. The real register adapter validates configuration and encodes telemetry.
Tests use `App_Init`/`App_Process` and observe dependency calls and registers;
there is no scheduler source extraction or private-state access.

Coverage includes 250 us/10000 us deadline boundaries, fixed-phase catch-up with
one update per pass, 16-bit rollover, a single time sample despite acquisition
delay, watchdog/servo/service ordering, configuration fault/LED behavior, startup
and armed-held button interlocks, calibration start/rejection, continued services,
serial and measured telemetry during motion, skipped outer control, asynchronous
one-shot reporting, serial cancellation, serial refill/shrink, and caller IRQ-mask
preservation. Reinitialization must preserve a
command already accepted by the drive layer.

The telemetry suite reuses this hardware fixture to drive real application
publication and counters through public passes. It retains signed/byte saturation,
NaN/infinity and zero-uptime checks.

These are software checks. Measured timing, motor behavior and physical faults
still require bench validation. The real calibration job is tested by the
[calibration suite](../calibration/README.md); its final flash save remains
synchronous while coasting.
