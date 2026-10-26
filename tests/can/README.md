# CAN module regression checks

Run from the repository root with a native GCC or Clang compiler:

```powershell
./tests/can/run.ps1 -Compiler 'C:/path/to/clang.exe'
```

`-Suite strict` and the default `-Suite all` both run the complete current suite,
including explicit DISARM, watchdogs and command publication races.

If `clang` or `gcc` is on `PATH`, the compiler argument is optional. `CC` also
selects a compiler executable.

The runner links production `can_servo.c`, `drive.c` and `control_table.c` as
separate translation units at both `-O0` and `-O2`. Only the thin HAL forwarding
callback and FOC watchdog are extracted from saved source. The command decoder,
transport state and drive policy are not copied into these tests. Generated
source and executables stay in the ignored `tests/can/build/` folder.

The fixture supplies hardware/FOC dependencies and FIFO traffic, including the
bundled HAL driver's DLC-to-byte copy behavior. A GCC/Clang object-size hook
observes the production receive allocation at the HAL call boundary, reporting
undersized/unknown storage before copying. Tests do not access private transport
or supervisor state or deliberately overflow the host process.

Checks cover DLC encodings 0 through 15, wrong identifiers, extended/RTR/FD/BRS
frames, disarmed command rejection, explicit ARM, full command field decoding,
watchdog refresh, and explicit all-zero DISARM. Supervisor checks cover owned
command/status snapshots, register compatibility, maintenance and calibration
entry, fault transitions, generation guards and IRQ-mask preservation.
Transport checks cover callbacks before initialization, controller binding,
complete TX metadata, signed/saturated/NaN reply encoding and queue failures.
Failed replies must not undo accepted commands, delay DISARM or add retries.

These are software checks. They do not validate interrupt timing, physical CAN
reception, motor outputs, or disabled back-driving.
