# Scoped audit and patch plan — simplified

Scope: B-G431B-ESC1 + AS5600 acquisition, FOC/current integration, CAN RX,
ARM/DISARM, watchdogs, disabled outputs and outer position/velocity control.
`AGENTS.md` and `docs/system-architecture.md` were read completely before changes.
The subsequent instruction to implement the points and commit each one replaced
the original read-only instruction.

## Repository and selected variant

- Active branch: `simplified`.
- Source baseline for the patch series: `3f283ea`.
- At resumption, HEAD was `cf8b53f`; tracked files were clean, with the user's
  `AGENTS.md` and `docs/system-architecture.md` untracked. The renamed architecture
  file was present at its requested `.md` path.
- Selected firmware: `01-Firmware/B-G431B-ESC1-v3`. Its main selected AS5600/I2C
  before simplification; the unused AS5048A/PWM alternative has now been removed
  from this variant. Other firmware experiments were not edited or audited.

The [source/call-path map](servo-drive.md#sources-and-execution) identifies all
requested implementation areas. `pic.c`, despite its name, implements PI/PID;
the important PWM helpers live in `foc_utils.h`, not the empty `.c` file.

## Confirmed bugs

The locations below refer to the final sources, where the fixes can be reviewed.
Paths are relative to `01-Firmware/B-G431B-ESC1-v3/`. Function names and branch
anchors identify the relevant code even after line numbers change. Original
behavior is available at baseline `3f283ea` or the named patch's parent.

| Priority / finding | File, function and relevant code location | Runtime consequence before correction | Smallest reasonable fix / patch |
| --- | --- | --- | --- |
| P0 Startup callback race | `Core/Src/main.c:375`, `main`, USER CODE BEGIN 2; `Core/Src/foc.c:170`, `API_FOC_Init`, first ADC start | ADC callbacks could run before configuration/encoder state and CORDIC configuration existed, including dereferencing an uninitialized sensor pointer | Initialize dependencies before ADC; start CAN last. `61d2254` |
| P1 Failed AS5600 read published | `Core/Src/as5600.c:271`, `AS5600_GetRawAngle`/`AS5600_GetAngle`; `Core/Src/position_sensor.c:89`, `positionSensor_update`, failed-status branch | Failed/partial I2C transactions could replace the rotor sample and corrupt angle/velocity used for commutation | Write caller output only on success; return before publishing any sample on failure. `1af6f67` |
| P1 Ignored encoder initialization failures | `Core/Src/position_sensor.c:50`, `positionSensor_init`; `Core/Src/main.c:273`, CAN ARM prerequisites | Missing encoder could be marked initialized and ARM accepted with invalid feedback | Propagate initialization/read failure, latch status fault and gate ARM on readiness/faults. `a293a29` |
| P1 CAN short payload indexes | `Core/Src/main.c:273`, `HAL_FDCAN_RxFifo0Callback`, format validation; removed `payload_length==3/5` branches | Three-byte frame used `RxData[4]`; five-byte frame used `RxData[5]`. These were reads beyond the valid payload, often stale bytes in the allocated buffer, not necessarily outside the C array | Remove abbreviated formats; accept exactly one validated 8-byte format. `5e6b28d` |
| P1 Missing DISARM semantics | `Core/Src/main.c:207`, `Servo_Disarm`; callback all-zero branch | The documented zero DISARM was treated as an ordinary command, leaving arming/torque enable active | Reserve and recognize eight zero bytes before ordinary decoding; clear state/goals and disable immediately. `5a8e6c3` |
| P1 Invalid frames refreshed watchdog | `Core/Src/main.c:273`, callback timestamp update after accepted branches | Matching-ID traffic with invalid payload could keep the drive armed indefinitely | Refresh only after a validated accepted ARM/servo command. `5e6b28d` |
| P1 Watchdog snapshot race / rollover | `Core/Src/main.c:219`, `Servo_Check_Watchdogs`, coherent now/last snapshot | An interrupt could publish a timestamp newer than the sampled `now`; subtraction or absolute deadline handling could cause false expiry, especially around wrap | Snapshot with IRQs masked briefly; use unsigned elapsed time. `7da6e54` |
| P1 Encoder equality/wrap/timing errors | `Core/Src/position_sensor.c:73`, `positionSensor_getRadiansEstimation`; `positionSensor_update`, delta_time_us | Equal timestamps could be interpreted as approximately 65 ms old; wrap lost one tick; inconsistent read timing distorted velocity | Unsigned 16-bit subtraction, skip zero interval, timestamp completed reads consistently. `d9d53a3` |
| P1 Unseeded initial velocity | `Core/Src/position_sensor.c:50`, `positionSensor_init`, initial histories | First valid nonzero angle produced a fictitious large movement and speed, potentially engaging overspeed behavior | Seed current/previous angles from the first valid sample and velocity to zero. `d9d53a3` |
| P1 Partially published encoder state | `Core/Src/position_sensor.c:89`, `positionSensor_update`, `*sensor = next` | ADC interrupt could observe new angle with old timestamp/velocity and use an inconsistent predicted angle | Prepare locally, then publish the complete sample in a short critical section. `3858ebe` |
| P1 Fault flags lost or cleared too early | `Core/Inc/control_table.h`, `hardware_fault_set/clear`; sensor estimator timeout-clear branch | Competing byte read/modify/write operations could lose another fault; a fresh sample could erase a timeout before foreground disarm | Atomic fault updates; clear stale-sample flag only while torque is disabled. `36eb1f4` |
| P1 FOC enable/disable publication race | `Core/Src/foc.c:114`, `API_FOC_Torque_Enable`, state publication; `API_FOC_Torque_Disable`; current pair setter | ISR could see active state before PI/targets were reset or see a torn pair of setpoints; blocking delay in disable was unsuitable in an ISR path | Initialize before publishing active state; atomic transitions/current pair; immediate nonblocking disable. `e154f96` |
| P1 Mixed or stale foreground commands | `Core/Src/main.c:238`, `Servo_Get_Command`; `Servo_Publish_Currents` generation check | Foreground could mix fields from different CAN commands or apply an old calculation after DISARM/rearm; first post-ARM command could be overwritten | Snapshot fields atomically, track ARM generation and recheck at publication; initialize ARM goals in RX. `c052c67`, `5a8e6c3` |
| P1 Calibration starved acquisition and could resume after cancellation | `Core/Src/foc.c:325`, `API_FOC_Calibrate`; `FOC_Calibration_Wait`/`Enter_Flux`; button edge in `main` | Long blocking waits left encoder samples stale; subsequent trajectory steps could restore flux after DISARM; held button retriggered calibration | Service encoder/analog work during waits, latch cancellation, guard flux entry, always coast/restore on failure, trigger on button edge. `254c8cf` |
| P1 Held button requested motion after power-up | `Core/Src/main.c`, `main`, initial calibration_button_was_down and button edge | An initially pressed/stuck button was treated as a new calibration request, allowing alignment motion at power-up | Initialize the edge interlock as pressed; require a release followed by a new press. Covered by the startup regression. `b5436b4` |
| P1 Serial bypass and malformed packet handling | `Core/Src/protocol.c`, `protocol_write_registers`, `instruction_handler`, packet-length parser | Serial runtime/SYNC writes bypassed CAN arming and limits; too-short payload length could underflow parser indexing | Permit disarmed configuration and LED only; remove unused SYNC handlers; validate packet/read/write bounds; disarm before reset. `fd223b5` |
| P1 Fatal handlers left outputs running | `Core/Src/main.c`, `Error_Handler`; `Core/Src/stm32g4xx_it.c`, NMI/HardFault/MemManage/BusFault/UsageFault handlers | Timer PWM could continue during a permanent CPU fault; zero CCRs in HardFault were not a high-impedance stop | Direct unconditional MOE clear before halt, without handles/ticks. `7396a10` |
| P1 CORDIC errors ignored | `Core/Inc/cordic.h`, `API_CORDIC_Processor_Update` and `RadiansToQ31`; `Core/Src/foc.c:434`, `API_FOC_Torque_Update`, all three CORDIC calls | Failed calculations could continue with default sine/cosine; the single-calculation HAL path read RDATA without a readiness timeout. Positive Q31 boundary at pi also exceeded int32 range | Poll ready with a fixed bound before reading, propagate failure and disable; map pi to the negative Q31 boundary. `41be46c` |
| P1 Fault overridden by overspeed branch | `Core/Inc/foc_utils.h`, `LL_FOC_brake`, fault guard | A newly detected sensor fault followed by the overspeed branch could leave outputs enabled for braking instead of coasting | Give any fault precedence in the brake helper. `c079bb0` |
| P2 Persistence reported as success | `Core/Src/control_table.c:139`, `store_eeprom_regs`; `foc.c`, calibration persistence branch; `protocol.c`, WRITE/FACTORY_RESET | Calibration/write could report success despite flash failure, with RAM and saved alignment differing | Return status, latch internal fault, return calibration result 4 and restore previous RAM alignment; report failed serial operation. `5ece1a6` |

No concrete defect was established in the existing Clarke/Park/CSVPWM or PI
equations themselves; their control math was retained. Integration guards,
state transitions and output enabling were changed where the above defects or
the explicit project requirements justified them.

## Robustness issues

| Finding | Relevant location | Consequence / resolution |
| --- | --- | --- |
| HAL receive copy precedes validation | `main.c`, RX buffer and callback; bundled `HAL_FDCAN_GetRxMessage` | An FD DLC can make HAL copy up to 64 bytes before the application rejects it. Reserve 64 bytes, reject nonclassic/incorrect DLC; bound ISR work to queued-at-entry messages and stop on read failure. Fixed in `5e6b28d` |
| Runtime magnet quality was unchecked | `as5600.c`, `AS5600_CheckMagnet`; `position_sensor.c`, update | A successful I2C angle read alone did not establish usable magnetic feedback. Check MD/ML/MH before angle acquisition and latch failures. Fixed in `7f903a6` |
| Unvalidated operating settings | `control_table.c`, `control_configuration_valid`; CAN ARM and service | Old/invalid encoder bits, reversed limits or large current/speed settings could arm. Validate explicitly; use provisional 500 mA default, 1 A cap, 360 degrees/s cap. Fixed in `ddfe2da`; actual motor pole pairs and travel still require measurement |
| Command limiting was not measured overcurrent protection | `foc.c`, `FOC_Check_Current_Protection` before torque update | Fault currents could exceed the commanded limit, particularly during voltage-driven calibration. Added per-phase >1.5 A software trip and nonfinite-current fault in `cf8b53f`; independent hardware cutoff remains unverified |
| Peripheral failures ignored | `foc.c`, `API_FOC_Init` and `HAL_ADC_ErrorCallback` | OPAMP/ADC/CORDIC start or ADC/DMA errors could silently invalidate current control. Check starts and latch/disable on ADC runtime errors. Fixed in `7396a10` |
| Unchecked encoder timebase start | `main.c`, startup `HAL_TIM_Base_Start(&htim6)` | A stopped microsecond counter prevents acquisition scheduling and can hide stale samples during manual calibration. Check its start status and halt with outputs off on failure; covered by the startup regression. `f8dfaed` |
| ARM conversion and initial limit mismatch | `main.c`, callback new-ARM branch | Multiturn position outside the signed wire range could be converted out of range; a hold outside soft limits could jump to a clamp boundary. Fixed by rejecting nonfinite/out-of-limit initial position before conversion in `9510497` |
| ARM during maintenance | `protocol.c`, configuration writes/reset; `control_table.c`, configuration interlock | CAN could rearm during flash operation or the serial reset delay after disarming. Inhibit ARM throughout maintenance; release on completion/failure, retain through reset. Fixed in `5ece1a6` |
| Foreground and software watchdog timing | `main.c`, foreground scheduler/watchdog; calibration serviced waits | Foreground stalls delay CAN timeout evaluation. Total MCU lockup is not covered by an independent watchdog. Fixed-bounds parsing and immediate ISR fault gating reduce exposure; an independent watchdog/timing budget remains a separate bench-driven task |
| Persistence integrity and telemetry range | `eeprom.c`, one-page store; `main.c`, telemetry conversion block | Power loss can leave partially programmed configuration; no CRC proves integrity. Extreme out-of-range multiturn/velocity telemetry is not saturated before integer conversion. Deferred targeted hardening; these are not claimed solved by parameter validation |

## Documentation mismatches

| Finding | Resolution |
| --- | --- |
| README said 16 kHz and P-only current regulation | Source config is 160 MHz / 2 / 2000 = 40 kHz PWM and current PI. Updated README/architecture; achieved frequencies need measurement |
| README described DISARM, multiple short formats and serial full control | DISARM did not exist as described; short formats and serial torque access were removed. Updated the protocol and compatibility notes |
| Intended Kd × velocity error differed from actual outer controller | Existing implementation differentiates position error per sample; full CAN has no velocity gain and its velocity field contributes zero under the simplified interface. Documented the actual equation; no unrequested controller redesign |
| AS5600 configuration default said 14 encoder bits | Selected AS5600 acquisition is fixed 12-bit. Corrected default and validation in `ddfe2da` |

## Simplification opportunities

- **Implemented:** remove short CAN formats and unused serial SYNC handlers
  (`5e6b28d`, `fd223b5`). One control path now owns arming.
- **Implemented:** retain only AS5600 in the selected variant; remove AS5048A
  driver, type dispatch, TIM4/PB6 capture setup and unused sensor allocation
  factory (`f0c79ea`). Static sensor storage was introduced in `a293a29`.
- **Deferred:** remove further unused register fields, PID helpers and empty
  translation units only when configuration/GUI compatibility is deliberately
  revised. No broad upstream cleanup was performed.

## Design preferences

- **Implemented project requirement:** disabled output behavior is coast,
  replacing upstream IDLE/fault braking (`faf9641`). This is a deliberate state
  policy change, not proof that braking was universally a bug. MOE-off, idle-low
  settings, continued ADC triggering and preload-before-enable were traced.
- **Retained:** existing armed overspeed brake, without a latched overspeed
  fault. Converting it to fault/coast requires an explicit policy decision and
  bench review; detected faults already override braking.
- **Retained:** 1000 ms CAN timeout, 1200 us sample-age threshold, existing outer
  derivative and no position integral. Final tuning and timeout values depend
  on measured timing/load, not an assumed gear ratio.
- **Deferred as requested:** expand CAN state/fault/voltage/temperature telemetry
  after the command contract has been exercised on the bench.

## Validation and prioritized remaining plan

Software validation: all nine native regression suites pass, including 240 CAN
assertions. They exercise saved production code with mocked peripherals. Both
default `-Og` and separate `-O2` ARM builds pass with Arm GNU 14.3.rel1. The
inherited CCMRAM RWX LOAD-segment linker warning remains. No firmware was
flashed and no physical motor result is claimed. Commands and suite scope are
in [tests/README.md](../tests/README.md).

The patch order already completed was: startup dependencies → valid sensor
samples → strict CAN → readiness/timing/coherence → state/watchdog races → coast
→ calibration/serial/configuration/current protection → internal/persistence
faults → AS5600-only simplification → documentation. Each logical implementation
point has its own commit; table entries above link fixes to commit IDs.

1. **P0 Bench gate levels and release behavior:** confirm power-up, reset,
   DISARM, every fault and coast-to-enable behavior with the physical board.
   Measure independent current cutoff behavior before increasing energy.
2. **P1 Timing and calibration:** verify 40 kHz FOC duration, CORDIC polling
   margin, I2C/4 kHz sample budget and encoder loss. Measure phase-current scale,
   actual pole pairs, direction and manual calibration/cancellation.
3. **P1 Fault and command campaign:** execute all abnormal conditions in the
   [bench checklist](servo-drive.md#bench-validation), including CAN loss,
   wrong/malformed frames, one-controller resets and low-current stall.
4. **P2 Targeted follow-up patches:** saturate telemetry conversion; make saved
   configuration integrity/power-loss behavior explicit; consider a hardware
   watchdog after the measured worst-case timing budget is known. One logical
   fix and test/build per commit.
5. **P2 Decide, then patch controller/protocol changes:** determine whether to
   retain or replace the inactive velocity field, derivative-per-sample damping
   and armed overspeed brake. Add minimal state/fault telemetry after command
   behavior is stable. Do not add an outer integral by default.
6. **P3 Mechanical integration:** measure throttle load/travel and select final
   gearing/current limits; leave speed, brake/clutch and cruise logic on ESP32.
