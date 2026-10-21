# G431 servo drive — simplified branch

Local servo firmware for the ST B-G431B-ESC1 with an AS5600 encoder over I2C.
The selected project is **`01-Firmware/B-G431B-ESC1-v3`**. Other firmware folders
are upstream experiments and are outside the simplified branch's validation.

The STM32 owns commutation, FOC/current control, encoder acquisition, the outer
servo loop and local faults. An ESP32 owns cruise control and sends commands
over CAN. No vehicle speed, brake, clutch or cruise-control logic runs here.

- Power-up is DISARMED, with motor outputs disabled in coast mode.
- Explicit 8-byte ARM and DISARM messages; one 8-byte servo command format.
- Classic CAN, 11-bit IDs, 1 Mbit/s; default command ID `0x001`.
- Invalid frames do not refresh the 1000 ms command watchdog.
- AS5600 only: SDA = PB7, SCL = PB8; failed reads preserve the last valid sample.
- Existing Clarke/Park, PI current control and CSVPWM are retained.
- Configured PWM/FOC rate: 40 kHz; outer loop target: 4 kHz; service: 100 Hz.
- Provisional bench current defaults: 500 mA command limit, configurable up to
  1000 mA; measured phase-current trip at 1500 mA.

Start with the [visual architecture guide](docs/architecture.md)
([illustrated HTML edition](docs/architecture.html)) for diagrams of the system,
control loops, code modules, execution contexts and state transitions.

See the [drive contract and bench checklist](docs/servo-drive.md),
[audit findings and patch plan](docs/simplified-audit.md),
[system architecture](docs/system-architecture.md) and
[regression instructions](tests/README.md).

Build with GNU Make and an Arm GNU bare-metal compiler on `PATH`:

```powershell
make -C 01-Firmware/B-G431B-ESC1-v3 -j4 all
./tests/run.ps1 -Compiler 'C:/path/to/native/gcc.exe'
```

This branch has software tests and an ARM build. Hardware timing, current
calibration, tuning and coast/back-drive behavior still require bench validation.
Old EEPROM settings can fail the new checks; configure the 12-bit encoder,
actual motor pole pairs and conservative limits while disarmed. The factory
pole-pair default is inherited and is not a verified specification for the GB5215.

The legacy GUI's torque controls, runtime-register writes and abbreviated CAN
formats are unsupported. Serial access is for disarmed configuration and
diagnostics; manual calibration is requested by the board button.

Based on the community
[Smart Brushless Servo project](https://hackaday.io/project/177578-b-g431b-esc-brushless-servo-controller).
