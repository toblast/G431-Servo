Run `./tests/sensor/run.ps1` from PowerShell with a native GCC C compiler.
Pass `-Compiler <path>` or set `CC` if GCC is not on PATH.
Use `-Group read`, `init`, `timing`, `snapshot`, or `health` to check an individual
patch; the default `all` runs every group.

The harness includes the selected firmware's actual `as5600.c` and
`position_sensor.c`, using their real headers with only the STM32 HAL surface
mocked. A deterministic I2C register image supplies successful, failed and
partially completed transactions. The mock timer advances during acquisition.

The checks cover failed-read output preservation, failed initialization/magnet
validation, seeded velocity, rejected runtime samples and latched sensor faults,
timestamp equality and rollover, and velocity timing across acquisition latency.
They do not verify real I2C timing, encoder filter delay, interrupt timing or
electrical output behavior; these require bench measurements.
