# Outer servo controller regression checks

Run from the repository root:

```powershell
./tests/servo/run.ps1 -Compiler 'C:/path/to/gcc.exe'
```

The runner links production `servo_controller.c`, `drive.c`, `control_table.c`,
`pic.c` and `math_tool.c` as separate translation units, at both `-O0` and `-O2`.
There is no source extraction, copied controller algorithm or private-state
access. FOC, encoder readiness, time and persistence are hardware dependencies
provided by the fixture; the supervisor's permission/generation checks are real.

Checks exercise known numerical outputs and public-interface sequences:

- Command scaling, proportional/derivative gains, per-sample filter history,
  feed-forward, zero integral gain, limits, phase reversal and unchanged Id.
- Commands received before the first servo tick, duplicate ARM, DISARM, and
  rearm with or without an intervening disabled tick.
- Stale enabled snapshots after rearm, stale disabled snapshots after a newer
  ARM, and faults detected after command capture. Guarded publication must
  reject invalidated work; the next accepted generation starts fresh history.

Returned setpoints are diagnostics for the calculation and can differ from
the last published currents when permission is revoked. Peripheral mocks do
not validate real interrupt timing, sensor accuracy, motor response or tuning.
