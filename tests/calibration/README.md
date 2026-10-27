# Calibration regression checks

Run from the repository root:

```powershell
./tests/calibration/run.ps1 -Compiler 'C:/path/to/gcc.exe'
```

The runner directly links production `calibration.c`, `motor_config.c`,
`drive_faults.c` and math helpers at `-O0` and `-O2`. It extracts complete FOC
entry/excitation/coast/cancellation and watchdog functions from saved source.
Time, encoder observations and storage are mocked. Tests use public job calls
and observe excitation/alignment/results without accessing private job state.

Coverage includes the full forward/reverse trajectory and both rotor directions,
step dwells, settling before fresh endpoint captures, no target catch-up bursts,
32-bit tick rollover, invalid/armed/faulted entry, no motion/pole mismatch,
DISARM/readiness/sensor failures, FOC loss before/during motion and alignment,
cancellation before/during save, storage failure and RAM rollback. IRQ masks,
active-guard lifetime, unread completion exclusion and one-shot result consumption
are checked. Calibration updates cannot advance virtual time or call acquisition
or protection service; the simulated foreground supplies those observations.

The application suite separately verifies foreground scheduling/ownership. These
software tests do not establish physical timing or motor behavior. The existing
synchronous flash save, while coasting, is retained and has no rollback or
power-loss transaction guarantee.
