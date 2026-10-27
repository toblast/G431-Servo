# Simplified firmware pseudocode

Describes the selected `01-Firmware/B-G431B-ESC1-v3` firmware at `8629fd6`.
This is the historical `simplified` baseline; its foreground/controller/CAN
ownership and blocking calibration predate the refactors. Current module paths,
scheduling and calibration states are in [professional-architecture.md](professional-architecture.md),
with contracts in [firmware-conventions.md](firmware-conventions.md).
This is an explanation of the saved implementation, not replacement firmware.
Peripheral register setup, byte packing and detailed equations are condensed.
All position and velocity values refer to the motor shaft.

The foreground runs continuously. ADC interrupts perform fast motor control;
CAN interrupts receive commands. These interrupts can interrupt foreground work.
The rates below are configuration targets, not measured timing guarantees.

## 1. Startup and foreground loop

```text
ON POWER-UP OR RESET:
    armed = false
    FOC mode = IDLE
    motor outputs = OFF / COAST

    initialize MCU, clocks, GPIO, DMA, UART, timers, ADCs, amplifiers,
               CORDIC math accelerator, CAN peripheral and I2C
    start microsecond timer
    if a required peripheral initialization/start fails:
        FATAL_STOP()

    if configuration flash is empty:
        save factory settings; FATAL_STOP() if this fails
    load saved configuration
    clear runtime registers, commands and faults
    if configuration is invalid:
        set CONFIGURATION fault

    initialize serial communication
    configure AS5600 and verify configuration by reading it back
    read initial encoder position; initialize velocity to zero
    if encoder initialization fails:
        latch ENCODER fault; keep servo disabled
    UPDATE_ENCODER()

    configure CORDIC and start PWM timer, amplifiers and ADC/DMA acquisition
    keep motor outputs off; sampling continues while disarmed
    update voltage and temperature protection
    start CAN reception only after control dependencies are initialized
    require button release before accepting a calibration press

    FOREVER:
        if any drive fault exists:
            DISARM()
        CHECK_WATCHDOGS()

        if 250 microseconds of scheduled time have elapsed:   // target 4 kHz
            UPDATE_ENCODER()
            RUN_POSITION_CONTROLLER()
            update telemetry and diagnostic registers
            saturate numbers before storing them in integer registers

        if 10 milliseconds of scheduled time have elapsed:   // target 100 Hz
            validate configuration; set/clear CONFIGURATION fault
            update voltage and temperature protection
            update status LED

            detect a new calibration-button press
            if new press and servo is already disarmed:
                RUN_CALIBRATION()     // temporarily occupies foreground
                print result over serial

            process only the serial bytes available at the start of service
            decode complete packets and HANDLE_SERIAL_REQUEST()

        update performance counters
```

## 2. CAN receive interrupt

```text
ON CAN RECEIVE INTERRUPT:
    process at most the messages already queued when this interrupt started

    for each message:
        if message cannot be read:
            stop processing this interrupt
        if wrong ID, extended ID, remote frame, CAN FD, BRS, or length != 8:
            ignore message without refreshing watchdog
            continue

        if calibration is active and payload is not eight zero bytes:
            ignore message
            continue

        if payload is eight zero bytes:                       // DISARM
            DISARM()

        else if payload is eight FF bytes:                   // ARM
            if configuration is invalid or maintenance is active:
                set CONFIGURATION fault; reject message
                continue
            if encoder is not initialized or any drive fault exists:
                reject message
                continue

            if not already armed:
                if measured position is nonfinite or outside configured bounds:
                    reject message
                    continue
                clear commands, gains and feed-forward
                position target = measured position
                increment arm generation
                enable servo command permission
                armed = true
            // A repeated ARM preserves the current command.

        else if armed:
            atomically store position, velocity, Iq feed-forward, Kp and Kd
        else:
            reject message
            continue

        if still armed:
            last accepted command time = now
        attempt CAN reply containing latest stored position and measured Iq
        // Reply has no ARM confirmation or fault/state flags; TX may be full.
```

## 3. Encoder acquisition

```text
UPDATE_ENCODER():
    if initialization previously failed:
        return
    read magnet status, then angle over I2C
    if either read fails or magnet status is invalid:
        latch ENCODER fault
        preserve the complete previous sample
        return

    measure elapsed time since previous valid sample
    if elapsed time is zero:
        return
    detect crossings between 0 and 360 degrees; update accumulated turn count
    calculate unwrapped position and velocity from angle change / elapsed time
    atomically publish position, velocity and sample timestamp together
```

## 4. Position controller: approximately 4 kHz

```text
RUN_POSITION_CONTROLLER():
    atomically snapshot command fields and arm generation
    if snapshot is disabled or faulted:
        reset local setpoints
        disable FOC if still required after rechecking shared state
        return

    if this is a new arm generation:
        reset outer-controller history

    target = clamp(command.position, configured position bounds)
    error = target - measured position
    derivative = 0.1 * (error - previous_error) + 0.9 * previous_derivative

    Iq_target = phase_direction * clamp(
        2 * command.Kp * error
        + 200 * command.Kd * derivative
        + command.Iq_feedforward,
        -configured_current_limit, +configured_current_limit)
    Id_target = 0
    // The velocity field is decoded/clamped, but its separate gain is zero.
    // No outer position integral is used. Derivative is per sample, not /dt.

    atomically recheck armed state, faults, calibration and arm generation
    if still permitted and generation is unchanged:
        if newly armed:
            reset current-controller targets/history and select TORQUE_CONTROL
        publish Iq_target and Id_target together
    else:
        discard this calculation
```

## 5. ADC/DMA interrupts and FOC: approximately 40 kHz

```text
ON ADC DMA COMPLETION:
    identify converter and PWM half-cycle
    if duplicate contribution, or an active sampling half-cycle ended incomplete:
        latch INTERNAL fault; disable FOC and coast; discard partial pair
        return

    if this is the offset-measurement half-cycle:
        discard any incomplete IDLE startup pair
        update current-sensor offset estimates
        return

    convert this converter's samples into phase currents
    ADC1 also collects bus voltage, temperature and potentiometer samples
    wait until ADC1 and ADC2 both supplied data in this sampling half-cycle

    if motor control or calibration is active:
        if any phase current is nonfinite or abs(current) > 1500 mA:
            latch INTERNAL or OVERCURRENT fault; disable FOC and coast

    RUN_FOC()             // also runs in IDLE for measurement/diagnostics

RUN_FOC():
    refresh FOC timestamp

    if FOC mode == FLUX_CONTROL:                              // calibration
        electrical_angle = commanded alignment angle
        Vd = commanded alignment voltage
        Vq = 0
        report Id = Iq = 0 in this mode
    else:
        if initialized encoder sample is older than 1200 microseconds:
            set STALE_ENCODER fault
        estimate rotor angle using last valid position, velocity and sample age
        electrical_angle = rotor_angle * pole_pairs * phase_direction + offsets

    calculate sine/cosine using CORDIC with bounded waiting
    if calculation fails:
        latch INTERNAL fault; disable FOC and coast; return

    if FOC mode != FLUX_CONTROL:
        measured_Id, measured_Iq = Clarke/Park transform(phase currents, angle)
        if FOC mode == IDLE:
            coast; return
        Vd = current_PI(Id_target - measured_Id)
        Vq = current_PI(Iq_target - measured_Iq)
        limit voltage-vector magnitude according to bus voltage

    if any drive fault exists:
        coast; return
    if TORQUE_CONTROL and measured speed exceeds configured speed limit:
        reset current PI history
        write equal phase duties; preserve existing output-enable state
        // Electrical braking if outputs are already enabled; no latched fault.
        return

    inverse Park/Clarke transform and calculate three PWM duties
    reject invalid arithmetic and clamp duty cycles
    write PWM preloads
    after a natural PWM update has latched safe duties, allow motor outputs
```

## 6. Watchdogs, disabling and faults

```text
CHECK_WATCHDOGS():
    take coherent, wrap-safe time snapshots
    if armed and accepted-command age > 1000 milliseconds:
        DISARM()
    if FOC timestamp age > 10 milliseconds:
        latch FOC_TIMEOUT fault
        DISARM()

DISARM():
    atomically:
        armed = false
        clear torque permission and command fields
        if calibration is active: request cancellation
        FOC mode = IDLE
        clear current targets
        turn off motor outputs immediately using COAST
    // Leave timer and ADC acquisition running.

ON ADC ERROR OR CURRENT/CORDIC FAILURE:
    latch corresponding fault and disable FOC immediately
    foreground subsequently clears normal arming through DISARM()

ON VOLTAGE OR TEMPERATURE SERVICE:
    convert/filter ADC readings and compare with configured limits
    set fault if outside allowed range
    voltage fault clears when voltage returns within bounds
    overheating clears after cooling 12 degrees below its configured limit
    // Clearing a fault never automatically arms the servo.

FATAL_STOP():
    disable interrupts
    directly clear PWM output enable
    halt until reset
```

## 7. Manual bench calibration

```text
RUN_CALIBRATION():
    atomically reject if already calibrating, torque-enabled, faulted,
                         encoder not ready, or configuration invalid
    mark calibration active; reject CAN commands except DISARM
    save previous direction/alignment settings
    coast initially

    apply a low open-loop alignment voltage
    sweep one electrical revolution forward, then backward
    use measured movement to determine direction and check pole-pair setting
    if no movement or pole-pair mismatch:
        finish with failure
    align rotor to a known electrical angle
    calculate electrical offset
    coast and save alignment to flash
    report storage failure if saving fails

    DURING EVERY WAIT:
        continue encoder acquisition and voltage/temperature service
        check FOC age against 10 ms
        abort on fault, lost readiness or CAN DISARM cancellation
        // ADC/CAN interrupts continue; normal foreground/serial service waits.

    ON EXIT AFTER CALIBRATION STARTED:
        coast and clear calibration-active flag
        restore previous RAM alignment if unsuccessful
        leave normal servo disarmed and return result to the caller
```

## 8. Serial configuration and persistence

```text
HANDLE_SERIAL_REQUEST():
    assemble packet; validate length, CRC, destination and register bounds
    PING / READ: prepare diagnostic reply
    WRITE:
        allow LED writes
        allow configuration writes only while disarmed and not calibrating
        reject serial writes to torque permission, commands or fault registers
        inhibit CAN ARM while copying configuration and saving it to flash
        latch INTERNAL fault on persistence failure and report failed operation
    FACTORY RESET / REBOOT:
        inhibit ARM and DISARM before performing the request
    queue bounded reply, or reject if TX queue is full

ON UART TRANSMIT COMPLETION:
    release completed DMA buffer and start next queued packet
    on DMA failure: record serial TX error and discard pending replies
```

The STM32 performs local servo control. The ESP32 supplies targets and owns
cruise logic, vehicle-speed measurement, brake/clutch inputs and user interface.
Position bounds clamp targets and check initial ARM position; they are not a
continuous measured-travel interlock. Flash storage is a single page without
configuration CRC or transactional power-loss recovery. No independent MCU
watchdog is configured.

For exact command encoding, fault clearing and limits, see
[the drive contract](servo-drive.md). Main implementation:
[main.c](../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c),
[foc.c](../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c),
[position_sensor.c](../01-Firmware/B-G431B-ESC1-v3/Core/Src/position_sensor.c),
[protocol.c](../01-Firmware/B-G431B-ESC1-v3/Core/Src/protocol.c).
