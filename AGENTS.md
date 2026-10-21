# AGENTS.md

# Project: Kawasaki Ninja 300 Cruise-Control Servo Drive

## Purpose

This repository is being adapted for a custom electronic cruise-control actuator
for a 2015 Kawasaki Ninja 300.

The current working branch is:

    simplified

The goal of this branch is not only to fix bugs, but also to simplify the
upstream firmware by removing code paths and features that are not required by
this project.

The STM32G431 motor controller is only the local servo drive.

A separate ESP32-WROOM-32D is the high-level cruise-control controller.

---

# Selected hardware

## Motor

SteadyWin GB5215 BLDC gimbal motor.

Current reference specifications:

- Nominal voltage: 24 V
- Nominal torque: approximately 0.76 Nm
- Peak/stall torque: approximately 1.15 Nm
- Torque constant: approximately 0.68 Nm/A
- No internal gearbox

An external reduction may be used between the motor and throttle pulley.

Current mechanical concept:

- spur gears
- motor and throttle shafts parallel
- likely ratio approximately 3:1
- final ratio is not yet fixed

Do not hard-code assumptions about the final gear ratio into the firmware.

## Rotor position sensor

AS5600 magnetic absolute encoder.

Interface:

    I2C

The AS5600 is connected to the STM32 motor drive, not to the ESP32.

## Motor controller

ST B-G431B-ESC1.

The STM32G431 is responsible for:

- BLDC commutation
- FOC
- current control
- motor position/velocity control
- AS5600 acquisition
- local motor faults
- CAN command reception
- CAN servo telemetry

## High-level controller

ESP32-WROOM-32D.

The ESP32 is responsible for:

- motorcycle speed measurement
- cruise-control logic
- cruise ON/OFF button
- brake input
- clutch input
- desired throttle-position calculation
- communication with the STM32 servo drive over CAN

The ESP32 must not perform FOC.

## CAN physical layer

ESP32 CAN controller:

    ESP32 TWAI peripheral

External CAN transceiver:

    SN65HVD230

The STM32 drive communicates with the ESP32 over CAN.

---

# Control architecture

The STM32 motor drive should behave conceptually like a small industrial servo
drive.

The ESP32 sends a servo command.

The STM32 performs all fast local motor control.

Approximate control architecture:

    Motorcycle speed controller
        ESP32
          |
          v
    desired throttle position
          |
         CAN
          |
          v
    STM32 position controller
          |
          v
    desired Iq / torque
          |
          v
    FOC current controller
          |
          v
        GB5215

The existing outer servo controller is expected to behave approximately as:

    Iq_command =
        Kp * position_error
      + Kd * velocity_error
      + Iq_feedforward

A position integral term is not required initially.

The ESP32 may later use a calibrated throttle-spring feed-forward table:

    throttle_angle -> Iq_feedforward

This allows the feed-forward torque to approximately compensate for the
throttle return spring while the PD controller handles the residual error.

Do not add an integral controller unless specifically requested.

---

# Firmware strategy

Do not rewrite the motor-control stack unless necessary.

The existing FOC/current-control implementation appears usable and should be
preserved unless a concrete defect is identified.

The initial goal is to harden and simplify the integration layer around the
existing motor-control implementation.

Prefer:

- minimal changes
- small patches
- deterministic behavior
- explicit state transitions
- simple CAN protocol
- conservative limits
- easy-to-review code

Avoid:

- large refactors without justification
- unrelated cleanup
- speculative architecture changes
- dynamic allocation in real-time control paths
- modifying working FOC code merely for style

---

# Known issues already identified

The following issues were identified during manual review and must be verified
against the actual code before changing anything.

Do not assume they are correct until the relevant source code has been
inspected.

## CAN parsing

Some short CAN command formats appear to have invalid payload indexing.

Examples previously observed include code paths where:

- a frame declared as 3 bytes accesses RxData[4]
- a frame declared as 5 bytes accesses RxData[5]

These paths should either be corrected or preferably removed if they are not
required.

For this project, a single explicit full command format is preferable.

The expected primary command frame is 8 bytes.

Do not retain multiple abbreviated command formats unless they provide a clear
benefit.

## ARM / DISARM behavior

The current firmware documentation and implementation may not agree on DISARM
semantics.

The project requires:

- explicit ARM command
- explicit DISARM command
- unambiguous state transition
- startup always DISARMED
- no automatic motor enabling

A zero command must not accidentally be interpreted as a valid active servo
command if it is intended to mean DISARM.

## CAN watchdog

The CAN watchdog must only be refreshed after a fully valid command has been
accepted.

Receiving a frame with the correct CAN ID but invalid payload or invalid DLC
must not refresh the watchdog.

CAN timeout must move the motor controller into a defined disabled state.

## AS5600 error handling

An invalid I2C read must never be used as a valid rotor-position update.

On failed AS5600 acquisition:

- preserve the last valid position if appropriate
- mark the sensor fault
- do not process uninitialized or invalid angle data
- transition to a safe defined servo state if the fault persists

## Disabled motor behavior

The current firmware may electrically brake the motor in IDLE or disabled
states.

This project is expected to prefer low-resistance back-driving of the actuator
when the motor is disabled.

Therefore investigate whether the disabled state should use:

    COAST / high impedance

instead of:

    BRAKE / phase shorting

Do not change this behavior until the existing implementation has been traced
and understood.

## Telemetry

The current CAN feedback is likely too limited.

Useful eventual telemetry includes:

- motor position
- Iq / torque current
- servo state
- fault flags
- DC bus voltage
- motor/controller temperature

Do not expand telemetry until the basic command protocol is stable.

---

# Required startup behavior

On power-up:

    state = DISARMED

The motor must not generate intentional torque automatically at startup.

Enabling the servo must require an explicit valid command.

Unexpected resets must return to DISARMED.

---

# Fault philosophy

The following faults must eventually result in a defined motor state:

- encoder failure
- CAN timeout
- over-current
- bus under-voltage
- bus over-voltage
- over-temperature
- invalid configuration
- internal motor-control fault

Do not silently ignore faults.

The exact behavior for each fault should be documented.

During initial development, conservative disabling is preferred over automatic
recovery.

---

# Development workflow

Before modifying any subsystem:

1. identify the relevant files
2. identify the call path
3. explain current behavior
4. distinguish confirmed bugs from design preferences
5. propose the smallest change
6. modify only the requested subsystem
7. build after each meaningful change
8. show the resulting diff
9. explain behavior changes

Prefer one logical change per commit.

Suggested commit style:

    fix: validate CAN frame lengths
    fix: ignore invalid AS5600 reads
    feat: implement explicit disarm command
    feat: use coast mode when disabled
    refactor: remove unused CAN command formats
    feat: add servo fault telemetry

Do not combine unrelated fixes into one commit.

---

# Testing expectations

Initial development is bench-only.

Do not assume the motorcycle throttle load is known.

Do not assume the final mechanical gear ratio is known.

Tests should include abnormal conditions, not only normal operation.

Important bench tests include:

- power-up with motor connected
- power-up without encoder
- unplug encoder while running
- stop CAN traffic
- send malformed CAN frame
- send incorrect DLC
- send command to wrong CAN ID
- block motor mechanically at low current limit
- cycle power
- restart ESP32 while STM32 remains powered
- restart STM32 while ESP32 remains powered
- verify disabled motor back-drive behavior

---

# Scope boundaries

This repository is the STM32 servo-drive firmware.

It is not responsible for:

- vehicle cruise-control PID
- motorcycle speed measurement
- brake logic
- clutch logic
- user-interface logic
- motorcycle ECU communication

Those functions belong to the ESP32 project unless explicitly moved later.

Do not add motorcycle-specific high-level logic to this firmware without an
explicit request.