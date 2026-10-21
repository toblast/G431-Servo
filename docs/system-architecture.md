# Kawasaki Ninja 300 Cruise-Control System Architecture

For an illustrated overview of the system and the current firmware modules,
start with the [visual architecture guide](architecture.md)
([offline HTML edition](architecture.html)). This document records the broader
project decisions and open questions.

Firmware status for branch `simplified`: the selected B-G431B-ESC1 + AS5600
implementation and its bench-only validation are described in
[servo-drive.md](servo-drive.md). The classified audit, completed patches and
remaining priorities are in [simplified-audit.md](simplified-audit.md).

## 1. Project goal

The project aims to build an electronic cruise-control system for a 2015
Kawasaki Ninja 300.

The motorcycle does not use electronic ride-by-wire throttle control for the
main throttle mechanism.

A separate electric actuator will therefore mechanically move the throttle
pulley near the throttle body.

The actuator is intended to maintain a commanded throttle opening while the
high-level controller regulates motorcycle speed.

---

# 2. System overview

The system is divided into two control levels.

## High-level controller

ESP32-WROOM-32D.

Responsibilities:

- read motorcycle speed
- read brake input
- read clutch input
- read cruise ON/OFF button
- maintain cruise-control state
- calculate desired motorcycle speed
- calculate desired throttle opening
- send servo commands over CAN
- receive servo telemetry and faults

## Local servo controller

ST B-G431B-ESC1.

Responsibilities:

- read motor rotor angle
- execute FOC
- regulate motor current / torque
- regulate actuator position
- monitor motor-control faults
- communicate with ESP32 over CAN

This separation intentionally resembles an industrial servo architecture:

    PLC / motion controller -> servo drive -> motor

For this project:

    ESP32 -> B-G431B-ESC1 -> GB5215

---

# 3. Motor

Selected motor:

    SteadyWin GB5215

Motor type:

    BLDC gimbal / low-Kv motor

Reasons for selection:

- high torque at low speed
- suitable for holding position
- no internal gearbox
- good back-drive characteristics compared with high-ratio gearmotors
- relatively inexpensive
- compatible with external absolute encoder
- well suited to FOC

Approximate reference data currently being used:

    Nominal voltage:       24 V
    Nominal torque:        ~0.76 Nm
    Peak/stall torque:     ~1.15 Nm
    Torque constant:       ~0.68 Nm/A

These values must be verified against the exact purchased motor variant.

---

# 4. Mechanical transmission

Direct drive was considered.

Estimated throttle-body torque is currently uncertain.

Published examples of throttle actuators suggest that peak/breakaway torque can
potentially reach approximately 1 to 1.5 Nm.

This value has NOT yet been measured on the Kawasaki Ninja 300.

Because the GB5215 nominal torque is below this worst-case range, an external
gear reduction is currently expected.

Current concept:

    spur gears
    parallel shafts
    external 3D-printed reduction

Initial target ratio:

    approximately 3:1

This ratio is provisional.

The final ratio must be selected after measuring actual throttle-pulley torque.

The earlier bevel-gear concept has been abandoned because the selected gimbal
motor is relatively wide and flat, making a same-plane spur-gear arrangement
more convenient.

---

# 5. Motor position sensor

Selected encoder:

    AS5600

Type:

    magnetic absolute encoder

Resolution:

    12 bit
    4096 positions/revolution
    approximately 0.088 degrees/count

Interface:

    I2C

The AS5600 is connected directly to the STM32 servo drive.

The ESP32 does not need direct access to the encoder.

The encoder is used locally for:

- rotor mechanical angle
- electrical-angle calculation
- servo position feedback

The selected firmware acquires AS5600 angle and magnet status over I2C in the
foreground, targeting 4 kHz. The 40 kHz FOC loop uses angle prediction from the
last complete sample and velocity, retaining a 200 us filter-delay compensation.
Invalid reads preserve the last valid sample and latch a sensor fault; samples
older than 1200 us inhibit outputs. Actual timing/filter delay requires bench
measurement. AS5048A/PWM support has been removed from the selected variant.

---

# 6. Motor drive

Selected hardware:

    ST B-G431B-ESC1

Processor:

    STM32G431

The board contains:

- three-phase power stage
- current sensing
- PWM generation hardware
- STM32G4 motor-control MCU
- programming/debug interface
- CAN-capable MCU hardware

The board is inexpensive compared with commercial servo drives.

The original ST ecosystem supports motor control through X-CUBE-MCSDK.

For this project, the current plan is to adapt the community G431-Servo
firmware rather than implement the complete servo stack from scratch.

---

# 7. Firmware base

Community repository:

    G431-Servo

Target firmware variant:

    B-G431B-ESC1-v3

The repository contains multiple experiments and firmware variants.

Do not assume every directory is part of the final firmware.

The project should be simplified around the exact variant required for the
B-G431B-ESC1 + AS5600 combination.

---

# 8. Existing motor-control architecture

The firmware currently performs approximately:

    configured PWM/FOC rate: 40 kHz
    position/encoder loop target: 4 kHz
    voltage/temperature service target: 100 Hz

TIM1 uses 160 MHz, center-aligned counting, prescaler 0 and ARR 1999:
160 MHz / (2 * 2000) = 40 kHz. FOC is called after the three selected phase
current samples arrive. Achieved frequencies and worst-case execution time
still require bench measurement.

The FOC implementation includes:

- phase-current acquisition
- Clarke transformation
- Park transformation
- Id/Iq current regulation
- inverse transform
- PWM generation

Current control uses PI regulation with anti-windup clamping in `Core/Src/pic.c`.

The intention is to preserve this implementation unless specific defects are
found.

---

# 9. Servo control philosophy

The desired servo model is approximately:

    Iq_command =
        Kp * position_error
      + Kd * velocity_error
      + Iq_feedforward

The retained firmware implementation differs from this idealized expression:
Kd multiplies a filtered position-error difference per sample, without dividing
by elapsed time. The separate velocity-error term has a gain that is zero and
not commandable through the simplified interfaces, so the CAN velocity field
currently has no control contribution. See the exact equation and scaling in
[servo-drive.md](servo-drive.md#actual-outer-control-law). Changing the controller
requires a separate tuning/protocol decision; no position integral was added.

For normal cruise-control position commands:

    velocity_reference ~= 0

Initially:

    position integral term = disabled

This is intentional.

The throttle return spring creates a mostly repeatable load versus throttle
angle.

The ESP32 may therefore calibrate a feed-forward table:

    throttle_angle -> required holding torque/current

Example concept:

    0 deg  -> 0.10 A
    10 deg -> 0.18 A
    20 deg -> 0.25 A
    30 deg -> 0.34 A
    ...

These values are illustrative only.

The actual table must be measured experimentally.

The feed-forward current compensates for most of the spring load.

The PD position controller then corrects:

- friction
- hysteresis
- disturbances
- modeling error

---

# 10. High-level controller

Selected MCU:

    ESP32-WROOM-32D

The ESP32 was preferred over the Raspberry Pi Pico primarily because the ESP32
contains an integrated CAN-compatible controller.

Espressif names this peripheral:

    TWAI

The ESP32 does NOT include the CAN physical-layer transceiver.

---

# 11. CAN physical layer

Selected external transceiver:

    SN65HVD230

Operating voltage:

    3.3 V

Connections:

    ESP32 TWAI_TX -> SN65HVD230 TXD
    ESP32 TWAI_RX <- SN65HVD230 RXD

    SN65HVD230 CANH -> CANH on servo drive
    SN65HVD230 CANL -> CANL on servo drive

The prototype SN65HVD230 module includes a selectable 120-ohm termination
resistor.

For a two-node CAN bus:

    ESP32/transceiver <------> STM32 servo drive

there should normally be a 120-ohm termination at each physical end.

With both terminations connected and power removed, resistance measured between
CANH and CANL should be approximately:

    60 ohms

The actual termination present on the B-G431B-ESC1 side must be verified.

---

# 12. CAN command philosophy

The simplified variant accepts one explicit 8-byte Classic CAN data frame
format at 1 Mbit/s with standard 11-bit IDs. All short command formats are
removed. Motor-shaft position, velocity, Iq feed-forward, Kp and Kd are encoded
in the full frame; the current velocity-field limitation is described above.

Eight `FF` bytes mean ARM; eight zero bytes mean DISARM. Ordinary commands
cannot enable a disarmed motor. Startup and reset always select DISARMED.
ARM requires valid configuration/encoder feedback and no fault, calibration
or configuration maintenance. DISARM immediately clears commands and disables
the outputs. Repeated ARM is idempotent.

The exact byte layout, limits and four-byte position/Iq reply are specified in
[servo-drive.md](servo-drive.md#can-protocol). State/fault telemetry expansion is
still deferred. Malformed frame types, IDs or DLCs are rejected.

---

# 13. CAN watchdog

If valid control messages stop arriving, the drive must transition to a defined
disabled state.

The watchdog timer must only be refreshed by a completely valid accepted
command.

The watchdog must NOT be refreshed merely because:

- the CAN ID matched
- a frame arrived
- the payload length was invalid
- the command payload was malformed

The current bench timeout is 1000 ms (trip when elapsed time exceeds it),
evaluated by foreground work with wrap-safe timestamp snapshots. A final
vehicle-level timeout is not fixed. FOC age is checked against 10 ms and
encoder sample age against 1200 us. Foreground stalls and a total MCU lockup
remain outside a guaranteed timeout bound; no independent MCU watchdog is
configured.

---

# 14. Disabled motor behavior

Back-driving is an important mechanical requirement. The rider should be able
to move the throttle mechanism manually with minimal motor resistance.

DISARMED, FOC IDLE and detected-fault output paths now select coast: TIM1 MOE is
cleared, with both complementary gate inputs configured idle-low. The counter
and ADC trigger remain running. Safe preloads are latched before re-enabling
outputs. Fatal exception paths also clear MOE before halting.

The existing armed overspeed response still uses equal 50% phase duties for
electrical braking. Detected faults override that brake path. Converting
overspeed to a latched coast fault remains a separate design decision.

Physical gate levels, passive motor resistance and the complete mechanical
release/back-drive behavior must be tested on the bench.

---

# 15. Motorcycle speed measurement

Current concept:

    read the front ABS tone ring

Motorcycle:

    2015 Kawasaki Ninja 300 ABS

Front OEM ABS sensor reference previously identified:

    Kawasaki 21176-0750

The exact electrical sensor type has not yet been conclusively confirmed.

Possible types include:

- passive variable-reluctance sensor
- active two-wire ABS sensor

A separate sensor may be mounted to read the same ABS ring.

The ABS system itself should not be electrically disturbed.

An alternative is to use a dedicated gear-tooth Hall sensor.

Final sensor choice is still open.

---

# 16. Brake and clutch inputs

The ESP32 will monitor:

- brake activation
- clutch activation

These motorcycle signals must not be connected directly to ESP32 GPIO if they
operate at motorcycle electrical-system voltage.

They will require suitable conditioning/protection.

Exact interface circuitry has not yet been selected.

---

# 17. Cruise-control user interface

Current requirement:

    one ON/OFF button

The user currently does not want a multi-button SET/RESUME interface.

Provisional behavior:

First press:

    enable cruise and store current speed

Second press:

    disable cruise

Brake or clutch activation:

    cancel cruise

Final state-machine details remain to be implemented on the ESP32.

---

# 18. ESP32 software environment

Initial preference:

    MicroPython

Reason:

- faster development
- easier high-level logic
- FOC is not running on the ESP32
- hard real-time motor control remains on the STM32

If MicroPython CAN/TWAI support becomes limiting, the project can migrate to:

    ESP-IDF / C / C++

without changing the overall system architecture.

---

# 19. Vehicle power architecture

Motorcycle electrical system:

    nominal 12 V
    approximately 12 to 14.5 V while operating

The B-G431B-ESC1 electronics can operate from this voltage range.

The GB5215 is nominally a 24 V motor.

Because the actuator operates mostly at very low speed, it may be possible to
run the motor directly from the motorcycle voltage.

This must be tested experimentally.

If required, a DC-DC boost converter can generate:

    24 V

for the motor drive.

A possible converter size is approximately:

    60 to 100 W

The converter should not be purchased until direct 12-14 V operation has been
tested.

---

# 20. Logic power

A separate buck converter is expected for logic electronics:

    motorcycle 12-14 V
          |
          v
       5 V buck
          |
          v
     ESP32 DevKit

The SN65HVD230 can be powered from the ESP32 3.3 V rail.

The AS5600 should preferably be powered locally from the STM32 drive side.

---

# 21. Ignition-controlled power

The complete cruise-control system will be switched by an automotive relay.

Architecture:

    battery
      |
    fuse
      |
    relay contacts
      |
      +---- motor drive
      |
      +---- logic DC/DC

Relay coil control:

    motorcycle ignition-switched 12 V

The ignition circuit should only energize the relay coil.

High-current system power should come from the battery through the fused relay
contacts.

---

# 22. Firmware audit and implementation status

The suspected issues were checked against the selected variant. The complete
classification, locations, consequences, minimal fixes and commits are recorded
in [simplified-audit.md](simplified-audit.md).

- CAN short-frame indexing, invalid-frame watchdog refresh and missing DISARM
  semantics were confirmed and corrected.
- Invalid AS5600 reads/initialization, initial velocity, timer wrap/equality,
  sample publication and fault-flag races were corrected.
- Startup ordering, FOC state/command publication, calibration cancellation,
  serial bypass, CORDIC/ADC error handling and fatal output shutdown were fixed.
- Disabled coast is an intentional project policy change, with hardware bench
  verification pending.
- Conservative configuration checks and measured phase-overcurrent protection
  were added. Default current is 500 mA, maximum command limit 1 A, measured
  phase trip 1.5 A; maximum rotor-speed setting is 360 degrees/s. These are
  provisional bench limits, not a validated throttle-load envelope.
- The encoder default is now 12 bits. Existing incompatible EEPROM settings
  prevent ARM; actual motor pole pairs, alignment and travel must be configured.
- The four-byte CAN reply still has no state/fault flags; expansion is deferred.

Software regression and ARM builds have passed. No motor flashing or bench
validation is claimed. Persistence integrity, extreme telemetry conversion,
independent watchdog protection and final control-policy tuning remain scoped
follow-ups.

---

# 23. Bench-testing sequence

The motor-control system must be validated on the bench before any motorcycle
installation.

Suggested progression:

1. power STM32 drive without motor enabled
2. verify CAN communication
3. verify AS5600 readings
4. verify motor electrical calibration
5. enable motor with very low current limit
6. command small position changes
7. verify direction
8. verify current readings
9. verify position holding
10. verify DISARM
11. verify coast/back-drive behavior
12. stop CAN traffic and verify watchdog
13. disconnect AS5600 and verify fault
14. test malformed CAN commands
15. gradually increase current limits
16. connect mechanical throttle load
17. measure actual required throttle torque/current
18. determine final gear ratio
19. characterize throttle feed-forward table

---

# 24. Items intentionally still open

The following decisions have NOT yet been finalized:

- exact throttle torque
- final gear ratio
- final motor supply voltage
- whether a 12->24 V boost converter is needed
- final ABS/speed sensor
- exact brake/clutch interface circuit
- any future revision to the documented 8-byte command format
- final watchdog timeout
- final telemetry frame
- final ESP32 cruise-control PID
- final throttle-position feed-forward map

Do not treat these as resolved unless the project documentation is updated.

---

# 25. Design priority

Current priorities are:

1. low cost
2. simple architecture
3. local servo intelligence in STM32
4. CAN interface to ESP32
5. easy debugging
6. minimal unnecessary firmware complexity
7. good back-drive behavior
8. predictable failure states

The current design is a prototype and must be developed incrementally.
