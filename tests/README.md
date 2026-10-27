# Firmware regression checks

Run from the repository root with PowerShell and a native GCC compiler:

```powershell
./tests/run.ps1 -Compiler 'C:/path/to/gcc.exe'
```

`CC` or `gcc` on `PATH` can select the compiler. Individual suites accept the
same `-Compiler` argument. Generated sources and executables are ignored.
Caller/return/unit conventions are in [firmware-conventions.md](../docs/firmware-conventions.md).

| Suite | Production behavior exercised |
| --- | --- |
| startup | Dependencies before ADC callbacks; seven FOC peripheral start failures, failed timebase/configuration load, CAN binding before start and application initialization after startup dependencies |
| sensor | Configuration address/readback and preserved zero, register byte order, failed/partial I2C reads, initialization, magnet status, sample timing, wraparound, atomic publication and fault flags |
| can | Every DLC encoding, frame type/ID, FIFO bounds, binding/buffer capacity, reply encoding/queue failures, ARM/DISARM, watchdog rollover, stale generations, typed ownership and maintenance/fault policy; links production CAN/drive/register modules at `-O0` and `-O2` |
| servo | Directly linked controller, supervisor, register adapter and PID/math helpers; gain/unit scales, setpoint/current limits, filters, feed-forward, direction, arm history and invalidated publication snapshots at `-O0` and `-O2` |
| app | Directly linked foreground application/register adapter at `-O0` and `-O2`; deadlines, phase/catch-up, rollover, call order, delay handling, watchdog-first dispatch, configuration faults, button edges, ongoing services/measurement publication during calibration, excitation ownership, one-shot results, bounded serial work and IRQ restoration |
| foc | State transitions, measured overcurrent, ADC pairing/missing streams, ADC errors, coast, preload-before-enable and invalid PWM inputs |
| calibration | Directly linked job at `-O0`/`-O2`; full trajectory/both directions, dwell and fresh capture, no catch-up burst, tick rollover, invalid entry, completion/IRQ semantics, no motion/pole mismatch, FOC loss, cancellation, sensor and persistence failures; see [calibration checks](calibration/README.md) |
| protocol | Real configuration service/register/storage path under serial framing; disarmed writes, protected runtime registers, malformed packets, ARM-race/maintenance interlocks and write/reset failures |
| serial | Full TX queue, FIFO wrap and DMA buffer ownership, DMA start/runtime errors, RX reload boundary and bounded formatting |
| telemetry | Directly linked application/register adapter, counter sequencing through public passes, signed/byte saturation, NaN/infinity, negative values and zero uptime |
| configuration | Directly linked validation/service/storage and adapters; defaults, pure validation, typed views, image size/runtime exclusion, load/save failures, staged writes, permissions/reset guard lifetime, reserved bytes, fault flags and IRQ preservation at `-O0`/`-O2` |
| fatal | Six production fatal handlers disable MOE before halting |
| cordic | Bounded polling, invalid angles, Q31 boundary, failure in every FOC state and successful typed FOC gain/angle scaling with live configuration changes |

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
