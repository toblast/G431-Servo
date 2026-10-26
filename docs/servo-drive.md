# B-G431B-ESC1 + AS5600 drive contract

For diagrams of the control loops, code modules and runtime behavior, see the
[visual architecture guide](architecture.md) or its [HTML edition](architecture.html).

This describes `01-Firmware/B-G431B-ESC1-v3` on `professional`, preserving the
`simplified` behavior. The [professional architecture](professional-architecture.md)
documents the supervisor, outer controller and CAN module. All angles and
velocities refer to the **motor shaft**. No gear ratio or throttle travel is
encoded in firmware. The ESP32 converts its desired actuator position into the
motor coordinate system.

## Sources and execution

Paths below are relative to the selected firmware directory.

| Area | Implementation and call path |
| --- | --- |
| Current acquisition and FOC | ADC1/ADC2 DMA → `HAL_ADC_ConvCpltCallback` → `API_FOC_It` → current protection → `API_FOC_Torque_Update`, in `Core/Src/foc.c` |
| PI controllers | `pi_process_antiwindup_clamp` in `Core/Src/pic.c`; declarations in `Core/Inc/pid.h` |
| Trigonometry | `Core/Inc/cordic.h`, called by the FOC loop; `Core/Src/cordic.c` is an empty upstream translation unit |
| PWM, coast and brake | Inline helpers in `Core/Inc/foc_utils.h`; TIM1 configuration in `Core/Src/main.c`; pin setup in `Core/Src/stm32g4xx_hal_msp.c` |
| AS5600 acquisition | Foreground `positionSensor_update` → magnet check and angle read in `Core/Src/as5600.c`; prediction and atomic sample publication in `Core/Src/position_sensor.c` |
| CAN RX and state | `HAL_FDCAN_RxFifo0Callback` forwards to `CanServo_OnRxFifo0` in `can_servo.c` for bounded FIFO handling, frame validation/decoding and reply encoding; drive permission/watchdogs remain in `drive.c` |
| Watchdogs | `Drive_Service` in `drive.c`, shared `API_FOC_Check_Watchdog` in `foc.c` (also used during calibration), and sample-age check in `positionSensor_getRadiansEstimation` |
| Position/velocity loop | Foreground schedule/acquisition in `main.c` → `ServoController_Update` in `servo_controller.c` → existing PID helper in `pic.c` → `Drive_Publish_Currents` → FOC |
| Configuration/fault flags | `control_table.c` decodes typed configuration and mirrors typed commands/status; fault helpers remain in `control_table.h`; serial maintenance permission is owned by `drive.c` |
| Calibration | `API_FOC_Calibrate` and serviced waits in `foc.c`; button edge in `main.c` |
| Fatal stop | `Core/Inc/emergency_stop.h`, called by `Error_Handler` and CPU fault handlers in `Core/Src/stm32g4xx_it.c` |

TIM1 uses 160 MHz, prescaler 0, center-aligned counting and ARR 1999:
160 MHz / (2 × 2000) = 40 kHz PWM. ADC update triggers continue while MOE is
clear. FOC runs after all three phase-current samples are collected on the
selected counter half-cycle. The foreground targets 250 us for encoder/servo
work and 10 ms for voltage, temperature, button and serial service. These are
configuration targets, not measured guarantees; I2C failures and flash writes
can delay foreground work.

FOC requires fresh ADC1 and ADC2 data in the same sampling half-cycle. A
duplicate contribution or an incomplete active pair latches an internal fault
and coasts. Incomplete IDLE startup pairs are discarded; missing complete pairs
also stop refreshing the FOC watchdog.

Encoder initialization writes and reads back CONF at `0x07`, preserving ZPOS.
It selects nominal mode and the 2x slow filter, with the sensor's power-saving
watchdog disabled so inactivity cannot select a slower acquisition mode.
Repeat motor alignment on the bench after upgrading from the earlier driver:
its erroneous CONF write overwrote ZPOS, so saved electrical alignment may
refer to a different encoder zero. No automatic calibration is performed.

## State transitions

Power-up/reset → DISARMED. Configuration, serial and encoder initialization
precede ADC callbacks; CAN reception starts last. No startup calibration or
automatic motor enabling is present.

An accepted ARM sets a zero-gain, zero-feed-forward hold target at the measured
position. The foreground enables the current controller on the next eligible
servo iteration. ARM requires valid configuration, a successfully initialized
encoder, no fault, no calibration/maintenance, and a finite initial position
within configured limits. Repeated ARM preserves the current command.

DISARM clears arming, torque enable and command fields, then disables FOC
immediately. Ordinary frames while disarmed cannot enable the motor. CAN
timeout, FOC timeout and local faults also lead to DISARMED. A new ARM is
required after a recoverable condition clears; a new arm generation prevents
an older foreground calculation from being applied after DISARM/rearm.

Manual calibration is a distinct bench operation requested by a new button
press while fault-free and disarmed. It deliberately energizes the motor with
an open-loop alignment voltage. Encoder and analog service continue; CAN ARM
and servo commands are rejected during calibration, while DISARM cancels it.
Calibration does not require periodic CAN traffic. It ends in coast, never in
the armed servo state. Results printed over serial: 0 success, 1 no motion,
2 pole-pair mismatch, 3 rejected/cancelled/fault, 4 storage failure. Failed
calibration restores the previous RAM alignment settings.
An armed button press is ignored and must be released before a later disarmed
press can request calibration. FOC age is checked throughout calibration;
loss of callbacks for more than 10 ms cancels it, coasts, and latches bit 4.

## CAN protocol

Classic CAN, 1 Mbit/s, standard 11-bit **data** frames, DLC = 8, BRS off.
Command ID = configured `REG_ID` (default 1); replies use `0x10 + REG_ID`.
Wrong IDs, remote/extended/FD frames and every other DLC are rejected. The
board's switchable termination is enabled by the current build; verify the
physical two-end termination before connecting the ESP32/SN65HVD230.

| Eight payload bytes | Meaning |
| --- | --- |
| `FF FF FF FF FF FF FF FF` | ARM; rejected if its prerequisites fail |
| `00 00 00 00 00 00 00 00` | DISARM; this is a reserved frame, not a zero-torque servo command |
| Any other payload | Servo command, accepted only while armed |

All 16-bit values are signed two's-complement, little-endian.

| Byte offset | Field | Unit |
| --- | --- | --- |
| 0–1 | Position target | 0.1 motor degree |
| 2–3 | Velocity target | motor degrees/s; see control law below |
| 4–5 | Iq feed-forward | mA |
| 6 | Position Kp byte | Internal multiplier = byte × 2 |
| 7 | Derivative Kd byte | Internal multiplier = byte × 200 |

Position/velocity commands are clamped to configured limits; the total torque
current demand is clamped to the configured current limit. There are no short
formats, no flux-current field and no independent velocity-gain field. A full
command is syntactically valid for every non-reserved 8-byte payload; values
outside configured operating limits are clamped, not treated as malformed.

Only accepted ARM/servo frames refresh the command timestamp. DISARM needs no
refresh. Expiry uses unsigned elapsed milliseconds and trips when elapsed
time is **greater than 1000 ms**, plus foreground scheduling latency. This is a
provisional bench timeout, not a vehicle-level timing requirement. Wrong DLC,
wrong ID and rejected ARM do not extend it. An accepted duplicate ARM does.

Replies remain four bytes: signed position in 0.1 degree followed by signed
measured Iq in mA, little-endian. Accepted DISARM also requests a reply. Replies
are asynchronous snapshots, can be dropped when TX is full, and contain no
state/fault flags or explicit acceptance code. Do not use them as proof that
the motor is armed. Fault telemetry expansion remains deferred.
Numeric telemetry is saturated to its existing signed-16-bit or unsigned-byte
range before conversion. NaN maps to zero; infinities map to the corresponding
numeric limit. Saturation does not prove sample validity or replace fault flags.

## Actual outer control law

The existing controller is retained:

```text
e[k] = clamped_position_target - measured_multiturn_position
d[k] = 0.1 * (e[k] - e[k-1]) + 0.9 * d[k-1]
velocity_error_filtered = 0.24 * (clamped_velocity_target - measured_velocity)
                        + 0.76 * previous_velocity_error_filtered
Iq = phase_direction * clamp(2*Kp_byte*e[k] + 200*Kd_byte*d[k]
                             + (VEL_KP_register/10)*velocity_error_filtered
                             + Iq_feedforward, -current_limit, current_limit)
```

The position integral gain is zero. The derivative is a filtered **difference
per sample**, without division by elapsed time. Its gain is therefore tied to
the nominal outer-loop rate; Kd is not directly a physical mA/(degree/s) gain.
There is derivative response to a changing position target.

`VEL_KP_register` is reset to zero at startup/ARM and cannot be written through
the simplified serial/CAN interfaces. Consequently the velocity-target field
currently has **no control contribution**. It is retained for the existing
8-byte layout. A future change to direct velocity-error damping needs separate
tuning and protocol review. No outer integral has been added. Current-loop PI
gains remain per-sample values with existing scaling and anti-windup.

## Limits and configuration

| Setting | Current bench policy |
| --- | --- |
| Iq command limit | Default 500 mA; valid range 1–1000 mA |
| Measured phase-current trip | Absolute current >1500 mA on any phase while active/calibrating |
| Rotor speed limit | Default and maximum configurable 360 degrees/s |
| Position limits | Inherited defaults −800 to +800 degrees; configure actual travel before loading the mechanism |
| Encoder resolution | Exactly 12 bits |
| Pole pairs | Nonzero; inherited default 14 must be checked against the actual motor |
| Voltage | Default 10–28 V, valid ordered positive limits with upper bound 28 V |
| Temperature | Default/max 70 °C; clear overheating below limit −12 °C |

Limits are provisional bench limits, not a measured GB5215/throttle envelope.
Position limits constrain command targets and initial ARM position; they are
not a continuous measured-travel interlock. Nonzero feed-forward with zero
position gains can move beyond them. A travel-violation response and recovery
policy still need to be selected before relying on firmware travel protection.
The 1 A command cap and 1.5 A measured trip are different protections. Software
phase-current protection relies on working acquisition/CPU execution and does
not replace an independently validated hardware current cutoff. TIM1 break
inputs are disabled in this configuration.

Serial permits EEPROM-region writes only while disarmed and not calibrating,
and the LED register at runtime. Torque enable, command and fault registers
are protected; SYNC_WRITE/SYNC_READ have been removed. Configuration writes,
factory reset and requested reboot inhibit ARM. Flash errors set the internal
fault and are reported as a failed operation; they do not guarantee rollback
of a partially written flash page. Reset always starts disarmed.
Serial TX holds at most 31 packets, each up to 256 bytes, including the active
DMA packet. Full queues and invalid lengths are rejected without overwriting
pending data. Start/runtime DMA failures discard pending replies, release the
half-duplex direction and latch `HAL_Serial_Handler.tx_error` until Init.
Formatted diagnostics use a fixed buffer and reject oversized output.

Existing EEPROM is not silently migrated. Settings such as 14 encoder bits,
26 A or 16000 degrees/s now prevent ARM. Read/repair configuration while
disarmed, verify it and calibrate explicitly. Flash persistence still uses one
page without transactional power-loss recovery or a configuration CRC.

## Fault behavior

All detected local faults inhibit output generation and are observed by the
foreground to disarm. Fault flags are updated with atomic byte read/modify/write.
The register is `REG_HARDWARE_ERROR_STATUS` at `0xB1`, readable over serial.

| Bit | Fault | Clearing policy |
| --- | --- | --- |
| 0 | Bus below/above configured limits | Live voltage service clears in range; filtered reading needs startup settling |
| 1 | AS5600 initialization, I2C read or magnet-status failure | Latched until MCU reset |
| 2 | Encoder sample older than 1200 us | A fresh sample clears only after torque enable is off |
| 3 | Internal fault: ADC error or incoherent acquisition pair, CORDIC failure, invalid output arithmetic or persistence failure | Latched until MCU reset |
| 4 | FOC timestamp older than 10 ms | Latched until MCU reset |
| 5 | Measured phase overcurrent | Latched until MCU reset |
| 6 | Over-temperature | Live service clears after 12 °C hysteresis |
| 7 | Invalid configuration / attempted ARM during configuration update | Clears on the 100 Hz service once configuration is valid |

CAN timeout disarms without its own fault bit. ADC error/overcurrent and CORDIC
failure disable FOC immediately; output helpers coast whenever they observe a
fault. Encoder read errors cannot publish an invalid sample. Voltage and
temperature detection run at service rate; no automatic rearm follows clearing.

Fatal CPU exceptions and initialization failures disable interrupts, clear TIM1
MOE directly and halt until reset; they do not rely on a handle, HAL tick or
telemetry. A complete MCU lockup that does not reach these handlers is outside
the software watchdog guarantee: no independent MCU watchdog is configured.

## Disabled outputs and overspeed

Coast clears TIM1 MOE while leaving its counter and ADC trigger running. With
OSSI enabled, OIS/OISN low, active-high channels and automatic output disabled,
both high- and low-side gate-driver inputs are low while disarmed. The L6387
[truth table](https://www.st.com/resource/en/datasheet/l6387.pdf) specifies both
outputs low for this input pair. Physical gate levels and back-drive resistance
must still be measured; high impedance does not remove motor cogging, gearing
friction or passive diode effects.

On enable, CCR preloads are written with MOE still off. A subsequent natural
PWM update must latch them before output enable. Faults always take precedence.
The existing **armed overspeed** response is retained: equal 50% phase duties
electrically brake if outputs are already active. This is not the disabled
state and is not a latched overspeed fault. Changing it to a fault/coast policy
is a separate design decision.

## Bench validation

The following checks are **pending**, with no hardware result claimed:

1. Power up with/without the motor and encoder; measure all six gate outputs.
   Confirm no automatic torque, no startup calibration and no MOE pulse.
2. Confirm AS5600 address/status, angle, direction, pole pairs and current-sense
   scale/offset. Measure I2C transaction time and achieved 4 kHz acquisition
   with the additional magnet-status transaction.
3. Measure worst-case FOC duration at 40 kHz, CORDIC polling margin, sample age,
   preload latching and coast-to-enable transition at both build optimizations.
4. At a current-limited bench supply, run manual calibration with a free rotor.
   Test DISARM cancellation, lost encoder and persistence failure reporting.
5. Test ARM/DISARM, duplicate ARM, all DLCs, wrong IDs, remote/FD frames,
   malformed/continuous traffic, missing CAN traffic and bus-off behavior.
6. Disconnect encoder/magnet while running. Confirm coast and the required
   reset before rearm; check sample-age failure without I2C corruption.
7. Exercise voltage and temperature faults, ADC errors and low-energy current
   trips. Mechanically block the motor only at a low supply/current limit.
8. Restart either controller independently and cycle power. Confirm the STM32
   always starts disarmed and the ESP32 does not automatically rearm after faults.
9. Verify disabled back-driving, manual throttle movement, armed overspeed
   behavior and the complete mechanical release path before adding a load.
10. Measure throttle travel, load and required holding current, then select the
    final gearing and tune gains. Define the final watchdog/telemetry contract
    before any vehicle integration.
