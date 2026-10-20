# CAN callback regression checks

Run from the repository root with a native GCC or Clang compiler:

```powershell
./tests/can/run.ps1 -Compiler 'C:/path/to/clang.exe'
```

`-Suite strict` and the default `-Suite all` both run the complete current suite,
including explicit DISARM, watchdogs and command publication races.

If `clang` or `gcc` is on `PATH`, the compiler argument is optional. `CC` also
selects a compiler executable.

The runner extracts the CAN declarations and callback directly from the saved
production `main.c`. The command decoder is not copied into these tests. The
generated source and executable stay in the ignored `tests/can/build/` folder.
The fixture supplies only the HAL/register environment and a single RX FIFO
entry, including the bundled HAL driver's DLC-to-byte copy behavior. It reports
undersized receive storage without deliberately overflowing the host process.

Checks cover DLC encodings 0 through 15, wrong identifiers, extended/RTR/FD/BRS
frames, disarmed command rejection, explicit ARM, full command field decoding,
watchdog refresh, and explicit all-zero DISARM.

These are software checks. They do not validate interrupt timing, physical CAN
reception, motor outputs, or disabled back-driving.
