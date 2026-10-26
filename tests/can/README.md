# CAN callback regression checks

Run from the repository root with a native GCC or Clang compiler:

```powershell
./tests/can/run.ps1 -Compiler 'C:/path/to/clang.exe'
```

`-Suite strict` and the default `-Suite all` both run the complete current suite,
including explicit DISARM, watchdogs and command publication races.

If `clang` or `gcc` is on `PATH`, the compiler argument is optional. `CC` also
selects a compiler executable.

The runner extracts the CAN transport declarations/callback and FOC watchdog
directly from saved production source. It links production `drive.c` and
`control_table.c` as separate translation units. The command decoder and drive
policy are not copied into these tests. Generated source and executables stay
in the ignored `tests/can/build/` folder. The fixture supplies hardware/FOC
dependencies and a single RX FIFO entry, including the bundled HAL driver's
DLC-to-byte copy behavior. It reports
undersized receive storage without deliberately overflowing the host process.

Checks cover DLC encodings 0 through 15, wrong identifiers, extended/RTR/FD/BRS
frames, disarmed command rejection, explicit ARM, full command field decoding,
watchdog refresh, and explicit all-zero DISARM. Supervisor checks cover owned
command/status snapshots, register compatibility, maintenance and calibration
entry, fault transitions, generation guards and IRQ-mask preservation.

These are software checks. They do not validate interrupt timing, physical CAN
reception, motor outputs, or disabled back-driving.
