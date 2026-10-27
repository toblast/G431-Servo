# ESP32 cruise controller: inputs and speed PID

This is the high-level ESP32-WROOM-32D counterpart to the STM32 servo drive.
It uses MicroPython, as selected for this project. This first increment reads
conditioned motorcycle inputs, estimates road speed, and computes a normalized
throttle demand for bench diagnostics.

**It does not send CAN commands or operate the actuator.** The STM32 firmware
is unchanged. Throttle-to-motor-angle calibration, CAN/TWAI transport, drive
ARM/DISARM and fault-feedback integration are subsequent work. There is no
assumption about motor reduction ratio or the motorcycle's electrical signals.

## Reading route and execution

Read `settings.py`, then `inputs.py`, `speed.py`, `pid.py`, `controller.py`,
`app.py` and finally `main.py`. The hardware boundary is small; all control
logic runs in the host tests without MicroPython or a connected board.

```text
main.run(config)
  -> Application.step()
       -> GpioInputs.read(): cumulative pulses + brake/clutch/button states
       -> WheelSpeed.update(): pulses and elapsed time -> measured m/s
       -> CruiseController.update(): interlocks, button and target capture
            -> SpeedPid.update(): target/measurement/time -> throttle [0, 1]
  -> print diagnostic state, measured speed, target speed and demand
```

`main.py` runs one pass then sleeps for `loop_period_ms`. Calculations use
actual elapsed time, including work and printing. Timing comparisons use
`ticks_diff()` so the clock can wrap. An excessive loop gap cancels computation.
This is a foreground loop; it makes no claim of deterministic scheduling.

## Hardware contract

The four inputs are speed pulses, brake active, clutch active and the one
cruise ON/OFF button. Every input must already be conditioned to defined
ESP32-compatible logic levels, with suitable electrical protection and bias.
The code sets pins to input mode without inferring pull resistors or polarity.
A valid logic level alone cannot reveal every disconnected or stuck signal.

Speed uses the native `machine.Counter` peripheral rather than Python pin
callbacks. It counts **rising edges** in the positive direction. Calibration
therefore means rising pulses per wheel revolution, including any intermediate
sensor gearing. The cumulative counter is not reset between samples.

The API is documented for [MicroPython v1.27.0 Counter](https://docs.micropython.org/en/v1.27.0/library/machine.Counter.html).
Use an ESP32 firmware build exposing that API; startup checks its presence
before configuring GPIOs. This dependency has been checked against the
documentation, but the adapter has not been exercised on an actual ESP32.

The configurable GPIO list excludes flash pins and the usual REPL UART.
Board wiring, boot-strapping pins and any other peripheral use still need
checking when pins are chosen. The speed sensor and brake/clutch interface
circuits remain undecided; this code does not interface directly with ABS.

## Required configuration

Set the `settings` fields after `Settings()` in `main.py` once hardware values
are known. `None` deliberately prevents the full loop from starting.

| Fields | Meaning |
| --- | --- |
| `speed_pin`, `brake_pin`, `clutch_pin`, `button_pin` | Four distinct GPIO numbers on the selected board |
| `brake_active_level`, `clutch_active_level`, `button_active_level` | Explicit `0` or `1` for the asserted state |
| `counter_id` | Available native ESP32 PCNT unit; default `0` |
| `wheel_circumference_m` | Measured rolling wheel circumference in metres |
| `pulses_per_revolution` | Positive integer count of rising pulses per wheel revolution |
| `minimum_speed_mps`, `maximum_speed_mps` | Allowed speed range for engaging and remaining active |
| `kp` | Proportional gain: normalized throttle / (m/s); must be positive |
| `ki` | Integral gain: normalized throttle / ((m/s) × s); zero disables accumulation |
| `kd` | Derivative gain: normalized throttle / (m/s²); zero disables its output contribution |

Use m/s internally; multiply by 3.6 for km/h. There are no production wheel,
speed-range or gain defaults. Test values are synthetic and are not a suggested
motorcycle calibration.

The initial timing values are provisional bench choices:

| Field | Default | Purpose |
| --- | --- | --- |
| `loop_period_ms` | 20 | Delay between foreground passes |
| `maximum_loop_gap_ms` | 100 | Cancel when actual sampling is delayed beyond this |
| `speed_window_ms` | 100 | Pulse-count averaging window |
| `speed_timeout_ms` | 500 | Invalidate after this long without observed pulses |
| `button_debounce_ms` | 30 | Stable button level required before accepting an edge |
| `derivative_filter_s` | 0.1 | First-order derivative filter time constant in seconds |

Choose the speed window and timeout against the actual pulses per revolution
and minimum speed. Too few pulses per window produce coarse measurements;
the last valid speed is held across empty windows until the timeout expires.
The estimator cannot infer the precise speed between pulses.

## State and control behavior

Startup is disabled. Once speed is valid and in range, releasing the button
and pressing it captures the current speed and enables PID computation.
Another press cancels. A button held at startup cannot engage the controller.

Brake or clutch assertion cancels on the next input pass without a debounce
delay. Invalid/stale speed, counter reset, excessive loop delay, input read
failure or invalid PID arithmetic also cancels. Recovery requires a release
and a fresh button press; it never resumes automatically.

```text
error = target_speed_mps - measured_speed_mps
P = kp * error
I = previous_I + ki * error * elapsed_seconds
D = filtered negative derivative of measured speed
throttle = clamp(P + I + kd * D, 0, 1)
```

Conditional integration prevents accumulation further into output saturation.
The derivative acts on measurement to avoid a target-change kick. A PID reset
clears derivative history and can track an explicitly supplied initial output.
The current application starts that output at zero. Actual actuator integration
will need a deliberate engagement/tracking policy and throttle calibration.
This integral is in the **ESP32 road-speed loop**; the STM32 position PD
controller is unaffected.

`ControlResult.active` means computation is active, not that the servo is armed.
A zero diagnostic demand is not a CAN DISARM message. The next CAN integration
must explicitly map cancellation to the STM32's DISARM contract.

## Modules, functions and types

| Module / callable | Responsibility and calls |
| --- | --- |
| `settings.finite(value)` | Rejects booleans, missing and nonfinite numeric values; uses `math.isfinite` |
| `Settings.__init__()` | Declares required hardware/calibration fields and bench timing |
| `Settings.validate(require_pins=False)` | Calls `validate_speed`, `validate_pid`, optionally `validate_inputs` |
| `Settings.validate_speed()` | Checks wheel/speed/timing values and their relationships |
| `Settings.validate_pid()` | Checks finite, nonnegative gains/filter and a positive proportional gain |
| `Settings.validate_inputs()` | Checks distinct usable GPIOs, active levels and PCNT unit |
| `InputSample.__init__(...)` | Holds cumulative integer pulse count and boolean active states; rejects invalid state types |
| `GpioInputs.__init__(settings, machine_module=None)` | Validates input settings, checks Counter API, then constructs `Pin` and `Counter`; injectable hardware module for tests |
| `GpioInputs._active(pin, level)` | Reads `Pin.value`, checks the digital value and applies polarity |
| `GpioInputs.read()` | Calls `Counter.value` and `_active` for each switch; returns `InputSample` |
| `GpioInputs.close()` | Calls `Counter.deinit` to release the counter |
| `SpeedSample.__init__(speed_mps, valid, reason)` | Carries estimated m/s, validity and diagnostic reason |
| `WheelSpeed.__init__(settings, ticks_diff)` | Validates speed settings, converts geometry to metres per pulse and calls `reset` |
| `WheelSpeed.reset()` | Discards previous measurement validity and requests a fresh window |
| `WheelSpeed._begin(count, now_ms, reason)` | Establishes a new counter/time baseline; returns an invalid `SpeedSample` |
| `WheelSpeed.update(count, now_ms)` | Checks count continuity/time, computes window speed, applies timeout/plausibility checks; calls `_begin` or `reset` when invalid |
| `pid.clamp(value, low, high)` | Bounds a value |
| `SpeedPid.__init__(kp, ki, kd, filter_s)` | Validates gains, calls `reset` |
| `SpeedPid.reset(measurement_mps, initial_output=0)` | Validates and initializes tracking and controller history |
| `SpeedPid.update(target_mps, measured_mps, dt_s)` | Computes PID, filtered derivative and anti-windup, calls `clamp`; commits state only after arithmetic checks |
| `Button.__init__(debounce_ms, ticks_diff)` / `reset()` | Initializes stable/candidate history with a release interlock |
| `Button.pressed(raw, now_ms)` | Debounces and returns one event per accepted press |
| `ControlResult.__init__(active, target_speed_mps, throttle, reason)` | Carries computation state, captured target, normalized demand and reason |
| `CruiseController.__init__(settings, ticks_diff)` | Validates control settings and constructs `SpeedPid` and `Button` |
| `CruiseController.disable(reason, reset_button=False)` | Clears active state/target/PID; optionally restores the release interlock |
| `CruiseController.update(now_ms, inputs, speed, initial_throttle=0)` | Checks interlocks, handles button edges and target capture, calls PID or `disable` |
| `Application.__init__(settings, input_source, clock)` | Wires injected input/clock objects to `WheelSpeed` and `CruiseController` |
| `Application.step()` | Reads inputs, handles read exceptions, updates speed and controller; exposes the latest read error |
| `main.run(config)` | Validates all configuration, constructs hardware/application, runs diagnostics and releases counter in `finally` |

Reasons include `warming_up`, `disabled`, `engaged`, `active`, `button_off`,
`brake`, `clutch`, `speed_timeout`, `counter_reset`, `sample_gap`,
`invalid_counter`, `implausible_speed`, `speed_out_of_range`, `loop_gap`,
`input_error`, `invalid_initial_throttle` and `pid_arithmetic`.
The main loop prints on reason changes and every 500 ms as loop timing allows.

## Running and checking

No package dependencies are required. On a PC, from the repository root:

```powershell
python -m unittest discover -s 01-Firmware/ESP32-Cruise/tests -v
```

The host checks use the production Python modules with fake GPIO/counter/time.
They cover calibration and configuration rejection, native-counter mapping,
pulse windows, clock rollover, stale/reset/implausible measurements, startup
and held buttons, brake/clutch cancellation, input exceptions, PID equations,
saturation, anti-windup and invalid arithmetic. They do not test electrical
behavior, ESP32 timing or closed-loop operation with the motor.

For board diagnostics, install a compatible MicroPython ESP32 build and copy
the seven production `.py` files in this directory to its filesystem root.
Fill the required `settings` fields in `main.py`; starting it will print
measured/target m/s and computed demand. Missing configuration stops it before
input GPIO setup. Stopping the loop releases the pulse counter.

Input reading can be checked before tuning the PID: construct `GpioInputs`
with configured pins/polarities, call `read()`, then `close()` when done.
`WheelSpeed` additionally needs wheel/speed/timing settings, but no PID gains.
No project framework, package manager or separate build system is needed.

Remaining steps are to select and bench-check input conditioning and pin
assignments, measure speed calibration, choose/tune the control envelope,
then implement CAN transport plus explicit drive state/fault handling and
actuator travel calibration. These are not implied by the passing host tests.

Further API references: [Pin](https://docs.micropython.org/en/v1.27.0/library/machine.Pin.html)
and [wrapping time functions](https://docs.micropython.org/en/v1.27.0/library/time.html).
