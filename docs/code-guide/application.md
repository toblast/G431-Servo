# Application, supervisor and command handling

Start with the [reading guide](README.md), then use the [function index](function-index.md)
or [type catalogue](types.md) as needed.

Direct calls below come from saved Core function bodies, excluding comments.
They cover conditional branches, not execution order/frequency. HAL/CMSIS,
compiler, C-library calls and macros are separated from Core definitions.
HAL-invoked callbacks are indirect edges; no **direct Core caller** does not mean
a callback is unused. Retained utilities and the disabled assertion hook are
included even if the linker discards them. Source snapshot: **f9c5c00**.
See [callback paths](README.md#interrupts-and-callbacks) and
[external dependencies](platform.md#external-dependencies).

## Module navigation

- [Foreground application](#module-app)
- [Application data vocabulary](#module-drive-types)
- [Drive supervisor](#module-drive)
- [Outer servo controller](#module-servo-controller)
- [CAN transport](#module-can-servo)
- [Foreground calibration job](#module-calibration)

<a id="module-app"></a>

## Foreground application

**Files:** [Inc/app.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/app.h), [Src/app.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/app.c).

Owns timer/serial bindings, deadlines, button history and diagnostic counters. Runs only in the single foreground loop. Read App_Process first, then its two private jobs.

**Verification entry points:** app, telemetry, integration. See [test instructions](../../tests/README.md).

<a id="fn-app-init"></a>

### `App_Init`

`void App_Init(TIM_HandleTypeDef *timer, HAL_Serial_Handler *serial);`

[Source: app.c:25](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/app.c#L25) · External linkage.

Bind borrowed timer/serial handles, reset controller/calibration/scheduler history and start deadlines at the current counter. A held startup button is interlocked. Does not erase a CAN command accepted during startup.

- **Direct Core calls:** [Calibration_Init](application.md#fn-calibration-init), [ServoController_Init](application.md#fn-servocontroller-init).
- **Other direct calls / macros:** `__HAL_TIM_GET_COUNTER`.
- **Direct Core callers:** [main](platform.md#fn-main).

<a id="fn-update-servo"></a>

### `update_servo`

`static void update_servo(void);`

[Source: app.c:38](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/app.c#L38) · Private/static.

Private 250 us job: acquire encoder, copy command/config, calculate currents unless calibration owns excitation, capture/publish measurements and encode diagnostics. Outer-controller diagnostic setpoints are zero during calibration.

- **Direct Core calls:** [API_FOC_Get_Present_Flux_Current](motor.md#fn-api-foc-get-present-flux-current), [API_FOC_Get_Present_Temp](motor.md#fn-api-foc-get-present-temp), [API_FOC_Get_Present_Torque_Current](motor.md#fn-api-foc-get-present-torque-current), [API_FOC_Get_Present_Voltage](motor.md#fn-api-foc-get-present-voltage), [API_FOC_Get_Processing_Frequency](motor.md#fn-api-foc-get-processing-frequency), [API_FOC_Get_Processing_Time](motor.md#fn-api-foc-get-processing-time), [API_FOC_Is_Calibrating](motor.md#fn-api-foc-is-calibrating), [Drive_GetCommand](application.md#fn-drive-getcommand), [Drive_PublishMeasurements](application.md#fn-drive-publishmeasurements), [ServoController_Update](application.md#fn-servocontroller-update), [control_read_config](configuration.md#fn-control-read-config), [positionSensor_getDegreesMultiturn](motor.md#fn-positionsensor-getdegreesmultiturn), [positionSensor_getVelocityDps](motor.md#fn-positionsensor-getvelocitydps), [positionSensor_update](motor.md#fn-positionsensor-update), [telemetry_store_i16](communications.md#fn-telemetry-store-i16), [telemetry_u8](communications.md#fn-telemetry-u8).
- **Other direct calls / macros:** `HAL_GetTick`, `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`, `fabsf`.
- **Direct Core callers:** [App_Process](application.md#fn-app-process).

<a id="fn-service"></a>

### `service`

`static void service(void);`

[Source: app.c:81](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/app.c#L81) · Private/static.

Private 10000 us job: validate configuration, disarm if invalid, service analog protections, update LED/button, request calibration on a new allowed press and process at most the serial bytes present at entry.

- **Direct Core calls:** [API_FOC_Service_Update](motor.md#fn-api-foc-service-update), [Config_IsValid](configuration.md#fn-config-isvalid), [Drive_Disarm](application.md#fn-drive-disarm), [Drive_IsDisarmed](application.md#fn-drive-isdisarmed), [Drive_StartCalibration](application.md#fn-drive-startcalibration), [HAL_Serial_Available](communications.md#fn-hal-serial-available), [HAL_Serial_GetChar](communications.md#fn-hal-serial-getchar), [HAL_Serial_Print](communications.md#fn-hal-serial-print), [hardware_fault_clear](configuration.md#fn-hardware-fault-clear), [hardware_fault_set](configuration.md#fn-hardware-fault-set), [hardware_fault_status](configuration.md#fn-hardware-fault-status), [packet_handler](communications.md#fn-packet-handler).
- **Other direct calls / macros:** `HAL_GPIO_ReadPin`, `HAL_GPIO_WritePin`.
- **Direct Core callers:** [App_Process](application.md#fn-app-process).

<a id="fn-app-process"></a>

### `App_Process`

`void App_Process(void);`

[Source: app.c:108](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/app.c#L108) · External linkage.

One foreground pass: watchdogs first, one timer sample, at most one servo/service job, then one calibration transition and completion print. Advance deadlines by fixed periods to preserve scheduling phase.

- **Direct Core calls:** [Calibration_TakeResult](application.md#fn-calibration-takeresult), [Calibration_Update](application.md#fn-calibration-update), [Drive_Service](application.md#fn-drive-service), [HAL_Serial_Print](communications.md#fn-hal-serial-print), [service](application.md#fn-service), [update_servo](application.md#fn-update-servo).
- **Other direct calls / macros:** `__HAL_TIM_GET_COUNTER`.
- **Direct Core callers:** [main](platform.md#fn-main).

<a id="module-drive-types"></a>

## Application data vocabulary

**Files:** [Inc/drive_types.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/drive_types.h).

Header-only copied value types. Start with ServoCommand, DriveCommandSnapshot and DriveStatus in the type catalogue. Numeric structs are not packed CAN/flash formats.

**Verification entry points:** can, servo, configuration. See [test instructions](../../tests/README.md).

No C function definitions. See the [type catalogue](types.md) for data types and the source for macros/options.

<a id="module-drive"></a>

## Drive supervisor

**Files:** [Inc/drive.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/drive.h), [Src/drive.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c).

Owns ARM, accepted command, generation, watchdog time, maintenance and measured status. Foreground/CAN IRQ share guarded operations; maintenance/calibration entry is foreground-only. A copied command requires live revalidation before publication.

**Verification entry points:** can, servo, integration. See [test instructions](../../tests/README.md).

<a id="fn-drive-init"></a>

### `Drive_Init`

`void Drive_Init(void);`

[Source: drive.c:19](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L19) · External linkage.

Startup-only: zero supervisor permission, command, generation, maintenance and measurements, and mirror a disarmed command. Does not start PWM.

- **Direct Core calls:** [control_mirror_command](configuration.md#fn-control-mirror-command).
- **Other direct calls / macros:** `memset`.
- **Direct Core callers:** [main](platform.md#fn-main).

<a id="fn-drive-isdisarmed"></a>

### `Drive_IsDisarmed`

`bool Drive_IsDisarmed(void);`

[Source: drive.c:25](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L25) · External linkage.

True when application ARM and calibration are absent. May still be true with faults or maintenance; this is not a complete permission check for another operation.

- **Direct Core calls:** [API_FOC_Is_Calibrating](motor.md#fn-api-foc-is-calibrating).
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [Drive_StartCalibration](application.md#fn-drive-startcalibration), [service](application.md#fn-service).

<a id="fn-drive-ismaintenance"></a>

### `Drive_IsMaintenance`

`bool Drive_IsMaintenance(void);`

[Source: drive.c:34](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L34) · External linkage.

Return the coherently read maintenance flag, which excludes ARM during configuration/reset work.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [Drive_StartCalibration](application.md#fn-drive-startcalibration).

<a id="fn-drive-disarm"></a>

### `Drive_Disarm`

`void Drive_Disarm(void);`

[Source: drive.c:43](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L43) · External linkage.

Atomically revoke ARM, zero accepted command/mirrors and immediately disable FOC. FOC disable also cancels active calibration. Restore the caller IRQ mask.

- **Direct Core calls:** [API_FOC_Torque_Disable](motor.md#fn-api-foc-torque-disable), [control_mirror_command](configuration.md#fn-control-mirror-command).
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`, `memset`.
- **Direct Core callers:** [CanServo_OnRxFifo0](application.md#fn-canservo-onrxfifo0), [Drive_BeginMaintenance](application.md#fn-drive-beginmaintenance), [Drive_Service](application.md#fn-drive-service), [service](application.md#fn-service).

<a id="fn-drive-requestarm"></a>

### `Drive_RequestArm`

`bool Drive_RequestArm(void);`

[Source: drive.c:54](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L54) · External linkage.

Return acceptance after calibration/maintenance/configuration/encoder/fault checks. New ARM validates measured travel, initializes zero-gain/feed-forward goals and increments generation; duplicate ARM retains goals. Accepted ARM refreshes the watchdog tick.

- **Direct Core calls:** [API_FOC_Is_Calibrating](motor.md#fn-api-foc-is-calibrating), [Config_IsValid](configuration.md#fn-config-isvalid), [control_mirror_command](configuration.md#fn-control-mirror-command), [control_read_config](configuration.md#fn-control-read-config), [hardware_fault_set](configuration.md#fn-hardware-fault-set), [hardware_fault_status](configuration.md#fn-hardware-fault-status), [positionSensor_getDegreesMultiturn](motor.md#fn-positionsensor-getdegreesmultiturn), [positionSensor_isReady](motor.md#fn-positionsensor-isready).
- **Other direct calls / macros:** `HAL_GetTick`, `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`, `isfinite`, `memset`.
- **Direct Core callers:** [CanServo_OnRxFifo0](application.md#fn-canservo-onrxfifo0).

<a id="fn-drive-submitcommand"></a>

### `Drive_SubmitCommand`

`bool Drive_SubmitCommand(ServoCommand const *command);`

[Source: drive.c:83](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L83) · External linkage.

Copy/mirror a command only while armed and noncalibrating; true also refreshes its acceptance tick. Rejection changes neither. Faults separately inhibit publication and cause foreground disarm.

- **Direct Core calls:** [API_FOC_Is_Calibrating](motor.md#fn-api-foc-is-calibrating), [control_mirror_command](configuration.md#fn-control-mirror-command).
- **Other direct calls / macros:** `HAL_GetTick`, `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [CanServo_OnRxFifo0](application.md#fn-canservo-onrxfifo0).

<a id="fn-drive-getcommand"></a>

### `Drive_GetCommand`

`DriveCommandSnapshot Drive_GetCommand(void);`

[Source: drive.c:97](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L97) · External linkage.

Return a coherent value copy of enabled permission, generation and goal. Enabled includes a live fault check; the copy can become stale before it is used.

- **Direct Core calls:** [hardware_fault_status](configuration.md#fn-hardware-fault-status).
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [update_servo](application.md#fn-update-servo).

<a id="fn-drive-publishcurrents"></a>

### `Drive_PublishCurrents`

`bool Drive_PublishCurrents(DriveCommandSnapshot const *command, bool new_arm, float torque_mA, float flux_mA);`

[Source: drive.c:110](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L110) · External linkage.

Return whether currents passed live ARM/fault/calibration/generation checks. Initialize FOC for a new accepted arm, then publish Iq/Id. A stale calculation changes no currents.

- **Direct Core calls:** [API_FOC_Is_Calibrating](motor.md#fn-api-foc-is-calibrating), [API_FOC_Set_Torque_Flux_Currents_mA](motor.md#fn-api-foc-set-torque-flux-currents-ma), [API_FOC_Torque_Enable](motor.md#fn-api-foc-torque-enable), [hardware_fault_status](configuration.md#fn-hardware-fault-status).
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [ServoController_Update](application.md#fn-servocontroller-update).

<a id="fn-drive-disableifdisarmed"></a>

### `Drive_DisableIfDisarmed`

`void Drive_DisableIfDisarmed(void);`

[Source: drive.c:126](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L126) · External linkage.

Controller cleanup rechecks live permission/faults before disabling, protecting a more recent ARM. App bypasses controller cleanup while calibration owns excitation.

- **Direct Core calls:** [API_FOC_Torque_Disable](motor.md#fn-api-foc-torque-disable), [hardware_fault_status](configuration.md#fn-hardware-fault-status).
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [ServoController_Update](application.md#fn-servocontroller-update).

<a id="fn-drive-service"></a>

### `Drive_Service`

`void Drive_Service(void);`

[Source: drive.c:134](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L134) · External linkage.

Foreground fault/watchdog service: disarm on faults, command age greater than 1000 ms while armed, or FOC heartbeat failure. Uses unsigned elapsed milliseconds.

- **Direct Core calls:** [API_FOC_Check_Watchdog](motor.md#fn-api-foc-check-watchdog), [Drive_Disarm](application.md#fn-drive-disarm), [hardware_fault_status](configuration.md#fn-hardware-fault-status).
- **Other direct calls / macros:** `HAL_GetTick`, `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [App_Process](application.md#fn-app-process).

<a id="fn-drive-beginmaintenance"></a>

### `Drive_BeginMaintenance`

`bool Drive_BeginMaintenance(bool disarm_first);`

[Source: drive.c:146](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L146) · External linkage.

Return acceptance of nonnested maintenance. Reject calibration/nested entry and normal writes while armed; disarm_first=true atomically disarms on successful entry.

- **Direct Core calls:** [API_FOC_Is_Calibrating](motor.md#fn-api-foc-is-calibrating), [Drive_Disarm](application.md#fn-drive-disarm).
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [Config_ApplyRegisterWrite](configuration.md#fn-config-applyregisterwrite), [Config_FactoryReset](configuration.md#fn-config-factoryreset), [instruction_handler](communications.md#fn-instruction-handler).

<a id="fn-drive-endmaintenance"></a>

### `Drive_EndMaintenance`

`void Drive_EndMaintenance(void);`

[Source: drive.c:160](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L160) · External linkage.

Release the owner's maintenance guard after work/failure cleanup. Successful reset/reboot retains the guard until reset instead.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [Config_ApplyRegisterWrite](configuration.md#fn-config-applyregisterwrite), [Config_FactoryReset](configuration.md#fn-config-factoryreset).

<a id="fn-drive-startcalibration"></a>

### `Drive_StartCalibration`

`bool Drive_StartCalibration(void);`

[Source: drive.c:168](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L168) · External linkage.

Require disarmed, nonmaintenance operation and start the foreground job. True is acceptance, not completion; FOC repeats atomic entry checks.

- **Direct Core calls:** [Calibration_Start](application.md#fn-calibration-start), [Drive_IsDisarmed](application.md#fn-drive-isdisarmed), [Drive_IsMaintenance](application.md#fn-drive-ismaintenance).
- **Other direct calls / macros:** None.
- **Direct Core callers:** [service](application.md#fn-service).

<a id="fn-drive-publishmeasurements"></a>

### `Drive_PublishMeasurements`

`void Drive_PublishMeasurements(DriveMeasurements const *measurements);`

[Source: drive.c:176](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L176) · External linkage.

Copy a complete measurement group into owned status and compatibility registers atomically. App masks ADCs while collecting that group.

- **Direct Core calls:** [control_mirror_measurements](configuration.md#fn-control-mirror-measurements).
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [update_servo](application.md#fn-update-servo).

<a id="fn-drive-getstatus"></a>

### `Drive_GetStatus`

`DriveStatus Drive_GetStatus(void);`

[Source: drive.c:185](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L185) · External linkage.

Return coherent status with state precedence fault, calibration, maintenance, armed, disarmed. armed records permission, not proof of enabled PWM outputs.

- **Direct Core calls:** [API_FOC_Is_Calibrating](motor.md#fn-api-foc-is-calibrating), [hardware_fault_status](configuration.md#fn-hardware-fault-status).
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [CanServo_OnRxFifo0](application.md#fn-canservo-onrxfifo0).

<a id="module-servo-controller"></a>

## Outer servo controller

**Files:** [Inc/servo_controller.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/servo_controller.h), [Src/servo_controller.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/servo_controller.c).

Foreground PD/filter and applied-generation history. Receives samples/typed inputs, returns diagnostics and delegates permission/publication to Drive. No acquisition, scheduling or register decoding.

**Verification entry points:** servo, integration. See [test instructions](../../tests/README.md).

<a id="fn-servocontroller-init"></a>

### `ServoController_Init`

`void ServoController_Init(void);`

[Source: servo_controller.c:18](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/servo_controller.c#L18) · External linkage.

Clear PD history, filtered velocity error, last generation and current-publication history. Does not arm the drive.

- **Direct Core calls:** [pid_reset](motor.md#fn-pid-reset).
- **Other direct calls / macros:** `memset`.
- **Direct Core callers:** [App_Init](application.md#fn-app-init).

<a id="fn-servocontroller-update"></a>

### `ServoController_Update`

`ServoSetpoints ServoController_Update(DriveCommandSnapshot const *command, DriveConfig const *config, float position_deg, float velocity_dps);`

[Source: servo_controller.c:24](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/servo_controller.c#L24) · External linkage.

Clamp targets, scale gains, update per-sample filters, add feed-forward, clamp/direction-correct Iq and publish through Drive. Reset history on rearm/rejection; disabled input yields zero diagnostics. Returns calculations even if publication is rejected.

- **Direct Core calls:** [Drive_DisableIfDisarmed](application.md#fn-drive-disableifdisarmed), [Drive_PublishCurrents](application.md#fn-drive-publishcurrents), [fconstrain](motor.md#fn-fconstrain), [pid_process_antiwindup_clamp_with_ff](motor.md#fn-pid-process-antiwindup-clamp-with-ff), [pid_reset](motor.md#fn-pid-reset).
- **Other direct calls / macros:** None.
- **Direct Core callers:** [update_servo](application.md#fn-update-servo).

<a id="module-can-servo"></a>

## CAN transport

**Files:** [Inc/can_servo.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/can_servo.h), [Src/can_servo.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/can_servo.c).

IRQ context after startup binding. Owns RX/TX metadata/buffers; delegates numeric commands to Drive. RX storage is 64 bytes for HAL copy safety even though only classic DLC8 commands are accepted.

**Verification entry points:** can, integration. See [test instructions](../../tests/README.md).

<a id="fn-canservo-init"></a>

### `CanServo_Init`

`void CanServo_Init(FDCAN_HandleTypeDef *hfdcan);`

[Source: can_servo.c:17](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/can_servo.c#L17) · External linkage.

Bind a borrowed FDCAN handle and initialize TX metadata before CAN start. NULL leaves transport unbound. Does not configure peripherals or arming.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [main](platform.md#fn-main).

<a id="fn-canservo-onrxfifo0"></a>

### `CanServo_OnRxFifo0`

`void CanServo_OnRxFifo0(FDCAN_HandleTypeDef *hfdcan, uint32_t events);`

[Source: can_servo.c:33](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/can_servo.c#L33) · External linkage.

Bound FIFO0 processing to entry occupancy, validate frame metadata, dispatch reserved ARM/DISARM or full command and queue accepted-command feedback. During calibration accept only DISARM. Failed TX does not roll back acceptance.

- **Direct Core calls:** [API_FOC_Is_Calibrating](motor.md#fn-api-foc-is-calibrating), [Drive_Disarm](application.md#fn-drive-disarm), [Drive_GetStatus](application.md#fn-drive-getstatus), [Drive_RequestArm](application.md#fn-drive-requestarm), [Drive_SubmitCommand](application.md#fn-drive-submitcommand), [telemetry_i16](communications.md#fn-telemetry-i16).
- **Other direct calls / macros:** `HAL_FDCAN_AddMessageToTxFifoQ`, `HAL_FDCAN_GetRxFifoFillLevel`, `HAL_FDCAN_GetRxMessage`, `HIGH_BYTE`, `LOW_BYTE`, `MAKE_SHORT`.
- **Direct Core callers:** [HAL_FDCAN_RxFifo0Callback](platform.md#fn-hal-fdcan-rxfifo0callback).

<a id="module-calibration"></a>

## Foreground calibration job

**Files:** [Inc/calibration.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/calibration.h), [Src/calibration.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/calibration.c).

Owns one job, backup/temporary alignment and one unread result. Normal acquisition supplies samples. FOC owns active/cancel guards. Motion advances without busy waits; final saving is synchronous while coasting.

**Verification entry points:** calibration, app, integration. See [test instructions](../../tests/README.md).

<a id="fn-calibration-init"></a>

### `Calibration_Init`

`void Calibration_Init(void);`

[Source: calibration.c:35](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/calibration.c#L35) · External linkage.

Startup-only reset of one private job and completion slot; no automatic motion. Reinitializing an active job violates its caller contract.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `memset`.
- **Direct Core callers:** [App_Init](application.md#fn-app-init).

<a id="fn-enter-phase"></a>

### `enter_phase`

`static void enter_phase(CalibrationPhase phase);`

[Source: calibration.c:37](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/calibration.c#L37) · Private/static.

Private transition helper: assign phase and timestamp after target publication. Foreground delays extend a dwell instead of compressing later steps.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_GetTick`.
- **Direct Core callers:** [Calibration_Start](application.md#fn-calibration-start), [Calibration_Update](application.md#fn-calibration-update), [excite](application.md#fn-excite).

<a id="fn-calibration-start"></a>

### `Calibration_Start`

`bool Calibration_Start(void);`

[Source: calibration.c:45](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/calibration.c#L45) · External linkage.

Reject active/unread jobs, acquire FOC guard, save original alignment, prepare zero alignment and begin the 200 ms coast phase. True means accepted; rejection creates no completion.

- **Direct Core calls:** [API_FOC_Calibration_Begin](motor.md#fn-api-foc-calibration-begin), [control_read_motor_alignment](configuration.md#fn-control-read-motor-alignment), [enter_phase](application.md#fn-enter-phase).
- **Other direct calls / macros:** None.
- **Direct Core callers:** [Drive_StartCalibration](application.md#fn-drive-startcalibration).

<a id="fn-complete"></a>

### `complete`

`static void complete(CalibrationResult result);`

[Source: calibration.c:55](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/calibration.c#L55) · Private/static.

Private cleanup: coast, resolve late cancellation/fault/readiness loss, roll back RAM alignment on failure, publish one result and release the active guard last. Storage failure retains result 4.

- **Direct Core calls:** [API_FOC_Calibration_Cancelled](motor.md#fn-api-foc-calibration-cancelled), [API_FOC_Calibration_End](motor.md#fn-api-foc-calibration-end), [API_FOC_Calibration_Idle](motor.md#fn-api-foc-calibration-idle), [control_write_motor_alignment](configuration.md#fn-control-write-motor-alignment), [hardware_fault_status](configuration.md#fn-hardware-fault-status), [positionSensor_isReady](motor.md#fn-positionsensor-isready).
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [Calibration_Update](application.md#fn-calibration-update), [excite](application.md#fn-excite).

<a id="fn-excite"></a>

### `excite`

`static void excite(float angle_rad, CalibrationPhase phase);`

[Source: calibration.c:74](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/calibration.c#L74) · Private/static.

Private helper publishing guarded 3 V excitation at an electrical angle, then entering its phase. Rejection completes as cancelled, preventing post-DISARM reenergizing.

- **Direct Core calls:** [API_FOC_Calibration_SetFlux](motor.md#fn-api-foc-calibration-setflux), [complete](application.md#fn-complete), [enter_phase](application.md#fn-enter-phase).
- **Other direct calls / macros:** None.
- **Direct Core callers:** [Calibration_Update](application.md#fn-calibration-update).

<a id="fn-calibration-update"></a>

### `Calibration_Update`

`void Calibration_Update(bool acquisition_serviced);`

[Source: calibration.c:83](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/calibration.c#L83) · External linkage.

Advance at most one state after heartbeat/cancel/fault/readiness checks. Perform timed sweeps, fresh endpoint captures, motion/pole/direction checks and offset calculation, then coast/save. Final flash I/O remains synchronous.

- **Direct Core calls:** [API_FOC_Calibration_Cancelled](motor.md#fn-api-foc-calibration-cancelled), [API_FOC_Calibration_Idle](motor.md#fn-api-foc-calibration-idle), [API_FOC_Check_Watchdog](motor.md#fn-api-foc-check-watchdog), [Config_Save](configuration.md#fn-config-save), [complete](application.md#fn-complete), [control_read_motor_config](configuration.md#fn-control-read-motor-config), [control_write_motor_alignment](configuration.md#fn-control-write-motor-alignment), [enter_phase](application.md#fn-enter-phase), [excite](application.md#fn-excite), [hardware_fault_status](configuration.md#fn-hardware-fault-status), [normalize_angle](motor.md#fn-normalize-angle), [positionSensor_getRadiansMultiturn](motor.md#fn-positionsensor-getradiansmultiturn), [positionSensor_isReady](motor.md#fn-positionsensor-isready).
- **Other direct calls / macros:** `HAL_GetTick`, `RADIANS_TO_DEGREES`, `fabsf`, `isfinite`.
- **Direct Core callers:** [App_Process](application.md#fn-app-process).

<a id="fn-calibration-takeresult"></a>

### `Calibration_TakeResult`

`bool Calibration_TakeResult(CalibrationResult *result);`

[Source: calibration.c:175](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/calibration.c#L175) · External linkage.

True copies/consumes one result. No pending result or NULL returns false without changing output/pending state; unread completion blocks another start.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [App_Process](application.md#fn-app-process).
