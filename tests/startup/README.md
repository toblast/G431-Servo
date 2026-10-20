# Startup regression

Run `./tests/startup/run.ps1 -Compiler <native-gcc-path>` from PowerShell.
The runner compiles the saved production startup block and FOC initialization
function with mocked peripherals. DMA startup immediately checks the state that
a preempting callback would need. CAN startup checks all dependencies are ready.
Generated sources and executables stay under the ignored root `build/` directory.

`-FirmwareRoot <path>` can check another source snapshot with the same layout.
