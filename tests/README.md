# Firmware regression checks

Run from the repository root with PowerShell and a native GCC compiler:

```powershell
./tests/run.ps1 -Compiler 'C:/path/to/gcc.exe'
```

`CC` or `gcc` on `PATH` can select the compiler. Individual suites accept the
same `-Compiler` argument. Generated sources and executables are ignored.

| Suite | Production behavior exercised |
| --- | --- |
| startup | Dependencies before ADC callbacks; seven FOC peripheral start failures, failed timebase and held-button interlock |
| sensor | Configuration address/readback and preserved zero, register byte order, failed/partial I2C reads, initialization, magnet status, sample timing, wraparound, atomic publication and fault flags |
| can | Every DLC encoding, frame type/ID, FIFO bounds, ARM/DISARM, watchdog rollover, stale generations, typed command/status ownership and supervisor maintenance/fault policy; links production drive/register modules directly |
| servo | Directly linked controller, supervisor, register adapter and PID/math helpers; gain/unit scales, setpoint/current limits, filters, feed-forward, direction, arm history and invalidated publication snapshots at `-O0` and `-O2` |
| foc | State transitions, measured overcurrent, ADC pairing/missing streams, ADC errors, coast, preload-before-enable and invalid PWM inputs |
| calibration | Successful alignment, both directions, no motion, pole mismatch, serviced acquisition, FOC loss before/during alignment, cancellation, sensor and persistence failures |
| protocol | Disarmed configuration, protected runtime registers, malformed packets, maintenance interlock and write failures |
| serial | Full TX queue, FIFO wrap and DMA buffer ownership, DMA start/runtime errors, RX reload boundary and bounded formatting |
| telemetry | Saved production publication block and directly linked register adapter, signed/byte saturation, NaN/infinity, negative values and zero-uptime counters |
| configuration | Conservative defaults, invalid settings, maintenance interlock and persistence error propagation |
| fatal | Six production fatal handlers disable MOE before halting |
| cordic | Bounded result polling, invalid angles, Q31 boundary and calculation failure in every FOC state |

The tests include production C files/headers or extract complete functions from
the saved sources. They mock peripherals; they do not reproduce the control
algorithm in a separate implementation. Some tests include private state to
inject faults and exercise interrupt boundaries deterministically.

These checks do not establish physical PWM levels, interrupt execution time,
I2C/CAN timing, current-sense accuracy or motor stability. Follow the
[bench checklist](../docs/servo-drive.md#bench-validation) after an ARM build.

For the selected firmware, install GNU Make and the Arm GNU bare-metal
toolchain, then build without flashing:

```powershell
make -C 01-Firmware/B-G431B-ESC1-v3 -j4 all
```

`arm-none-eabi-*` must be on `PATH`, or pass `GCC_PATH=<toolchain-bin>` to Make.
For a separate optimized build, use
`BUILD_DIR=build/optimized OPT=-O2 DEBUG=0`; the default remains `-Og`.
On Windows, create that nested output directory first with
`New-Item -ItemType Directory -Force 01-Firmware/B-G431B-ESC1-v3/build/optimized`.
Validation used native GCC 4.7.0 and Arm GNU 14.3.rel1 on Windows. A linker
warning about an RWX LOAD segment is inherited from the CCMRAM layout.
