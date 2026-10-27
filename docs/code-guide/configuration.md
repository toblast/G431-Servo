# Configuration, register adapters and faults

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

- [Configuration service](#module-configuration)
- [Pure configuration validation](#module-config-validation)
- [Fixed image adapter](#module-config-storage)
- [Register/image codec](#module-control-table)
- [Motor configuration views](#module-motor-config)
- [Fault-bit adapter](#module-drive-faults)
- [Flash backend](#module-eeprom)
- [Byte packing macros](#module-binary-tool)

<a id="module-configuration"></a>

## Configuration service

**Files:** [Inc/configuration.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/configuration.h), [Src/configuration.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/configuration.c).

Owns load/write/reset/save policy and failure reporting. Synchronous I/O runs outside atomic snapshot guards under caller exclusion. Framing lives in protocol; encoding/backend live below this service.

**Verification entry points:** configuration, protocol, integration. See [test instructions](../../tests/README.md).

<a id="fn-config-save"></a>

### `Config_Save`

`bool Config_Save(void);`

[Source: configuration.c:7](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/configuration.c#L7) · External linkage.

Capture 128 configuration bytes atomically, then save with IRQs enabled under caller-owned exclusion. False latches internal fault; no flash/RAM rollback guarantee.

- **Direct Core calls:** [ConfigStorage_Save](configuration.md#fn-configstorage-save), [control_capture_config](configuration.md#fn-control-capture-config), [hardware_fault_set](configuration.md#fn-hardware-fault-set).
- **Other direct calls / macros:** None.
- **Direct Core callers:** [Calibration_Update](application.md#fn-calibration-update), [Config_ApplyRegisterWrite](configuration.md#fn-config-applyregisterwrite), [Config_FactoryReset](configuration.md#fn-config-factoryreset), [Config_Load](configuration.md#fn-config-load).

<a id="fn-config-load"></a>

### `Config_Load`

`bool Config_Load(void);`

[Source: configuration.c:16](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/configuration.c#L16) · External linkage.

Startup: provision defaults on an erased page, load an image and apply only after successful I/O. False latches internal fault and makes main halt before ADC/CAN. Invalid stored values remain available for repair.

- **Direct Core calls:** [ConfigStorage_IsEmpty](configuration.md#fn-configstorage-isempty), [ConfigStorage_Load](configuration.md#fn-configstorage-load), [Config_Save](configuration.md#fn-config-save), [control_apply_config](configuration.md#fn-control-apply-config), [control_reset_config_defaults](configuration.md#fn-control-reset-config-defaults), [hardware_fault_set](configuration.md#fn-hardware-fault-set).
- **Other direct calls / macros:** None.
- **Direct Core callers:** [main](platform.md#fn-main).

<a id="fn-config-applyregisterwrite"></a>

### `Config_ApplyRegisterWrite`

`ConfigResult Config_ApplyRegisterWrite(uint16_t address, uint16_t length, uint8_t const *data);`

[Source: configuration.c:31](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/configuration.c#L31) · External linkage.

Check pointer/range, acquire maintenance, copy bytes atomically, save and release. Return access/busy without application, OK after save, or storage error retaining applied RAM. Partial repairs may temporarily be invalid.

- **Direct Core calls:** [Config_Save](configuration.md#fn-config-save), [Drive_BeginMaintenance](application.md#fn-drive-beginmaintenance), [Drive_EndMaintenance](application.md#fn-drive-endmaintenance), [control_apply_config_bytes](configuration.md#fn-control-apply-config-bytes).
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [protocol_write_registers](communications.md#fn-protocol-write-registers).

<a id="fn-config-factoryreset"></a>

### `Config_FactoryReset`

`ConfigResult Config_FactoryReset(void);`

[Source: configuration.c:46](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/configuration.c#L46) · External linkage.

Acquire forced-disarm maintenance, apply/persist defaults. Success retains the guard through caller reset; storage failure releases it. Busy entry changes nothing.

- **Direct Core calls:** [Config_Save](configuration.md#fn-config-save), [Drive_BeginMaintenance](application.md#fn-drive-beginmaintenance), [Drive_EndMaintenance](application.md#fn-drive-endmaintenance), [control_reset_config_defaults](configuration.md#fn-control-reset-config-defaults).
- **Other direct calls / macros:** None.
- **Direct Core callers:** [instruction_handler](communications.md#fn-instruction-handler).

<a id="module-config-validation"></a>

## Pure configuration validation

**Files:** [Src/config_validation.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/config_validation.c).

Implements APIs declared in configuration.h. Separate numeric validity from ARM/maintenance. Invalid stored values remain available for staged disarmed repair.

**Verification entry points:** configuration, can, integration. See [test instructions](../../tests/README.md).

<a id="fn-config-validate"></a>

### `Config_Validate`

`bool Config_Validate(DriveConfig const *config);`

[Source: config_validation.c:5](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/config_validation.c#L5) · External linkage.

Pure predicate, false for NULL: validate travel/bus ordering, bench limits, 12-bit resolution, poles/direction and nonnegative/nonzero current-PI gain pairs. Does not save or arm.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [Config_IsValid](configuration.md#fn-config-isvalid).

<a id="fn-config-isvalid"></a>

### `Config_IsValid`

`bool Config_IsValid(void);`

[Source: config_validation.c:21](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/config_validation.c#L21) · External linkage.

Capture one coherent live DriveConfig and validate it. Does not repair invalid settings or fold maintenance state into pure validation.

- **Direct Core calls:** [Config_Validate](configuration.md#fn-config-validate), [control_read_config](configuration.md#fn-control-read-config).
- **Other direct calls / macros:** None.
- **Direct Core callers:** [API_FOC_Calibration_Begin](motor.md#fn-api-foc-calibration-begin), [API_FOC_Torque_Enable](motor.md#fn-api-foc-torque-enable), [Drive_RequestArm](application.md#fn-drive-requestarm), [main](platform.md#fn-main), [service](application.md#fn-service).

<a id="module-config-storage"></a>

## Fixed image adapter

**Files:** [Inc/config_storage.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/config_storage.h), [Src/config_storage.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/config_storage.c).

ConfigImage contains exactly 128 bytes. Converts backend status to bool without validation/register mutation/drive policy. Caller owns exclusion and fault handling.

**Verification entry points:** configuration, protocol, integration. See [test instructions](../../tests/README.md).

<a id="fn-configstorage-isempty"></a>

### `ConfigStorage_IsEmpty`

`bool ConfigStorage_IsEmpty(void);`

[Source: config_storage.c:8](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/config_storage.c#L8) · External linkage.

Return backend erased-marker test, not full-page integrity/CRC or parameter validity.

- **Direct Core calls:** [eeprom_empty](configuration.md#fn-eeprom-empty).
- **Other direct calls / macros:** None.
- **Direct Core callers:** [Config_Load](configuration.md#fn-config-load).

<a id="fn-configstorage-load"></a>

### `ConfigStorage_Load`

`bool ConfigStorage_Load(ConfigImage *image);`

[Source: config_storage.c:9](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/config_storage.c#L9) · External linkage.

Read exactly one ConfigImage through EEPROM and return whether HAL_OK. Ignore failed output; no validation/register mutation/drive policy here.

- **Direct Core calls:** [eeprom_restore](configuration.md#fn-eeprom-restore).
- **Other direct calls / macros:** None.
- **Direct Core callers:** [Config_Load](configuration.md#fn-config-load).

<a id="fn-configstorage-save"></a>

### `ConfigStorage_Save`

`bool ConfigStorage_Save(ConfigImage const *image);`

[Source: config_storage.c:13](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/config_storage.c#L13) · External linkage.

Write exactly one ConfigImage and return whether HAL_OK. Caller owns exclusion; the adapter adds no transactions or fault reporting.

- **Direct Core calls:** [eeprom_store](configuration.md#fn-eeprom-store).
- **Other direct calls / macros:** None.
- **Direct Core callers:** [Config_Save](configuration.md#fn-config-save).

<a id="module-control-table"></a>

## Register/image codec

**Files:** [Inc/control_table.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/control_table.h), [Src/control_table.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/control_table.c).

Owns regs, defaults and coherent typed/image codecs. Runtime command/status registers are mirrors. Bare mirror/byte-copy functions require a caller guard; image operations restore IRQ state themselves.

**Verification entry points:** configuration, can, telemetry, integration. See [test instructions](../../tests/README.md).

<a id="fn-control-read-config"></a>

### `control_read_config`

`DriveConfig control_read_config(void);`

[Source: control_table.c:21](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/control_table.c#L21) · External linkage.

Atomically decode legacy bytes into a copied DriveConfig with existing signedness/scales. Restore IRQ state; no second persistent cache.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `MAKE_SHORT`, `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [Config_IsValid](configuration.md#fn-config-isvalid), [Drive_RequestArm](application.md#fn-drive-requestarm), [update_servo](application.md#fn-update-servo).

<a id="fn-control-mirror-command"></a>

### `control_mirror_command`

`void control_mirror_command(bool enabled, ServoCommand const *command);`

[Source: control_table.c:46](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/control_table.c#L46) · External linkage.

Encode supervisor permission/goals into runtime registers, clearing the goal region first. Caller holds IRQs. Mirrors do not own the accepted command.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HIGH_BYTE`, `LOW_BYTE`, `memset`.
- **Direct Core callers:** [Drive_Disarm](application.md#fn-drive-disarm), [Drive_Init](application.md#fn-drive-init), [Drive_RequestArm](application.md#fn-drive-requestarm), [Drive_SubmitCommand](application.md#fn-drive-submitcommand).

<a id="fn-control-mirror-measurements"></a>

### `control_mirror_measurements`

`void control_mirror_measurements(DriveMeasurements const *measurements);`

[Source: control_table.c:63](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/control_table.c#L63) · External linkage.

Encode measurements with existing scales and saturating helpers. Caller holds the group-publication guard; position uses tenths of a degree.

- **Direct Core calls:** [telemetry_store_i16](communications.md#fn-telemetry-store-i16), [telemetry_u8](communications.md#fn-telemetry-u8).
- **Other direct calls / macros:** None.
- **Direct Core callers:** [Drive_PublishMeasurements](application.md#fn-drive-publishmeasurements).

<a id="fn-control-reset-config-defaults"></a>

### `control_reset_config_defaults`

`void control_reset_config_defaults(void);`

[Source: control_table.c:74](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/control_table.c#L74) · External linkage.

Write inherited defaults into configuration only under an IRQ guard. No I/O/runtime reset. The pole default is not a verified motor specification.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HIGH_BYTE`, `LOW_BYTE`, `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [Config_FactoryReset](configuration.md#fn-config-factoryreset), [Config_Load](configuration.md#fn-config-load).

<a id="fn-control-capture-config"></a>

### `control_capture_config`

`void control_capture_config(ConfigImage *image);`

[Source: control_table.c:168](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/control_table.c#L168) · External linkage.

Atomically copy bytes 0x00..0x7f into ConfigImage, excluding every runtime register.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`, `memcpy`.
- **Direct Core callers:** [Config_Save](configuration.md#fn-config-save).

<a id="fn-control-apply-config"></a>

### `control_apply_config`

`void control_apply_config(ConfigImage const *image);`

[Source: control_table.c:176](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/control_table.c#L176) · External linkage.

Atomically apply ConfigImage to configuration only. Startup/maintenance exclusion belongs to caller; no validation or persistence here.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`, `memcpy`.
- **Direct Core callers:** [Config_Load](configuration.md#fn-config-load).

<a id="fn-control-apply-config-bytes"></a>

### `control_apply_config_bytes`

`void control_apply_config_bytes(uint16_t address, uint16_t length, uint8_t const *data);`

[Source: control_table.c:184](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/control_table.c#L184) · External linkage.

Service-only raw byte copy: caller already checked configuration range and holds IRQs. This primitive adds no checks or permission policy.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `memcpy`.
- **Direct Core callers:** [Config_ApplyRegisterWrite](configuration.md#fn-config-applyregisterwrite).

<a id="fn-control-reset-runtime"></a>

### `control_reset_runtime`

`void control_reset_runtime(void);`

[Source: control_table.c:189](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/control_table.c#L189) · External linkage.

Startup-only clear from torque-enable through runtime registers. Not a running-drive shutdown API; use Drive_Disarm.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `memset`.
- **Direct Core callers:** [main](platform.md#fn-main).

<a id="module-motor-config"></a>

## Motor configuration views

**Files:** [Inc/motor_config.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/motor_config.h), [Src/motor_config.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/motor_config.c).

Narrow coherent motor/protection/alignment reads, nonzero torque permission and shared wrapping sensor-error diagnostic. Fast readers use CCMRAM; no separate settings cache or persistence.

**Verification entry points:** configuration, sensor, foc, cordic. See [test instructions](../../tests/README.md).

<a id="fn-control-read-motor-config"></a>

### `control_read_motor_config`

`MotorControlConfig control_read_motor_config(void);`

[Source: motor_config.c:5](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/motor_config.c#L5) · External linkage.

Read coherent motor settings: signed alignment/gains, unsigned trim, poles/direction and speed limit. CCMRAM reader preserves raw scales for FOC conversion.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `MAKE_SHORT`, `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [API_FOC_Torque_Update](motor.md#fn-api-foc-torque-update), [Calibration_Update](application.md#fn-calibration-update).

<a id="fn-control-read-protection-config"></a>

### `control_read_protection_config`

`MotorProtectionConfig control_read_protection_config(void);`

[Source: motor_config.c:24](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/motor_config.c#L24) · External linkage.

Read coherent voltage/temperature limits for foreground protections; does not acquire readings or clear faults.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [LL_FOC_Update_Temperature](motor.md#fn-ll-foc-update-temperature), [LL_FOC_Update_Voltage](motor.md#fn-ll-foc-update-voltage).

<a id="fn-control-read-motor-alignment"></a>

### `control_read_motor_alignment`

`MotorAlignment control_read_motor_alignment(void);`

[Source: motor_config.c:37](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/motor_config.c#L37) · External linkage.

Read the complete signed electrical offset/direction atomically for calibration backup/inspection.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `MAKE_SHORT`, `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [Calibration_Start](application.md#fn-calibration-start).

<a id="fn-control-write-motor-alignment"></a>

### `control_write_motor_alignment`

`void control_write_motor_alignment(MotorAlignment const *alignment);`

[Source: motor_config.c:49](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/motor_config.c#L49) · External linkage.

Atomically publish both alignment fields from a borrowed value. Caller owns calibration/configuration exclusion; no persistence occurs.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HIGH_BYTE`, `LOW_BYTE`, `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [Calibration_Update](application.md#fn-calibration-update), [complete](application.md#fn-complete).

<a id="fn-control-torque-requested"></a>

### `control_torque_requested`

`bool control_torque_requested(void);`

[Source: motor_config.c:59](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/motor_config.c#L59) · External linkage.

Read live torque-enable byte with nonzero semantics. Low-level FOC permission view, not a replacement for supervisor ARM/generation checks.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [API_FOC_Calibration_Begin](motor.md#fn-api-foc-calibration-begin), [API_FOC_Set_Torque_Flux_Currents_mA](motor.md#fn-api-foc-set-torque-flux-currents-ma), [API_FOC_Torque_Enable](motor.md#fn-api-foc-torque-enable), [positionSensor_getRadiansEstimation](motor.md#fn-positionsensor-getradiansestimation).

<a id="fn-control-record-sensor-error"></a>

### `control_record_sensor_error`

`void control_record_sensor_error(void);`

[Source: motor_config.c:64](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/motor_config.c#L64) · External linkage.

Increment the wrapping shared diagnostic byte. Its legacy name refers to protocol CRC; it is not a sensor-only counter.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [positionSensor_update](motor.md#fn-positionsensor-update).

<a id="module-drive-faults"></a>

## Fault-bit adapter

**Files:** [Inc/drive_faults.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/drive_faults.h), [Src/drive_faults.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive_faults.c).

Named masks and live diagnostic access. Atomic OR/clear retains other faults and IRQ state. Callers select shutdown and recovery policy; clearing a bit never rearms.

**Verification entry points:** configuration, can, foc, integration. See [test instructions](../../tests/README.md).

<a id="fn-hardware-fault-status"></a>

### `hardware_fault_status`

`DriveFaultFlags hardware_fault_status(void);`

[Source: drive_faults.c:5](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive_faults.c#L5) · External linkage.

Read live eight-bit flags without acknowledgement/clearing. Multiple faults can coexist.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [API_FOC_Calibration_Begin](motor.md#fn-api-foc-calibration-begin), [API_FOC_Calibration_SetFlux](motor.md#fn-api-foc-calibration-setflux), [API_FOC_Set_Torque_Flux_Currents_mA](motor.md#fn-api-foc-set-torque-flux-currents-ma), [API_FOC_Torque_Enable](motor.md#fn-api-foc-torque-enable), [Calibration_Update](application.md#fn-calibration-update), [Drive_DisableIfDisarmed](application.md#fn-drive-disableifdisarmed), [Drive_GetCommand](application.md#fn-drive-getcommand), [Drive_GetStatus](application.md#fn-drive-getstatus), [Drive_PublishCurrents](application.md#fn-drive-publishcurrents), [Drive_RequestArm](application.md#fn-drive-requestarm), [Drive_Service](application.md#fn-drive-service), [LL_FOC_Update_Temperature](motor.md#fn-ll-foc-update-temperature), [LL_FOC_brake](motor.md#fn-ll-foc-brake), [LL_FOC_set_phase_voltage](motor.md#fn-ll-foc-set-phase-voltage), [complete](application.md#fn-complete), [hardware_fault_clear](configuration.md#fn-hardware-fault-clear), [hardware_fault_set](configuration.md#fn-hardware-fault-set), [service](application.md#fn-service).

<a id="fn-hardware-fault-set"></a>

### `hardware_fault_set`

`void hardware_fault_set(DriveFaultFlags flags);`

[Source: drive_faults.c:10](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive_faults.c#L10) · External linkage.

Atomically OR bits, retaining unrelated faults and restoring IRQ mask. Immediate coast and foreground disarm are separate actions by callers.

- **Direct Core calls:** [hardware_fault_status](configuration.md#fn-hardware-fault-status).
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [API_FOC_Check_Watchdog](motor.md#fn-api-foc-check-watchdog), [API_FOC_It](motor.md#fn-api-foc-it), [API_FOC_Torque_Update](motor.md#fn-api-foc-torque-update), [Config_Load](configuration.md#fn-config-load), [Config_Save](configuration.md#fn-config-save), [Drive_RequestArm](application.md#fn-drive-requestarm), [FOC_Check_Current_Protection](motor.md#fn-foc-check-current-protection), [HAL_ADC_ErrorCallback](motor.md#fn-hal-adc-errorcallback), [LL_FOC_Update_Temperature](motor.md#fn-ll-foc-update-temperature), [LL_FOC_Update_Voltage](motor.md#fn-ll-foc-update-voltage), [LL_FOC_set_phase_voltage](motor.md#fn-ll-foc-set-phase-voltage), [main](platform.md#fn-main), [positionSensor_getRadiansEstimation](motor.md#fn-positionsensor-getradiansestimation), [positionSensor_init](motor.md#fn-positionsensor-init), [positionSensor_update](motor.md#fn-positionsensor-update), [service](application.md#fn-service).

<a id="fn-hardware-fault-clear"></a>

### `hardware_fault_clear`

`void hardware_fault_clear(DriveFaultFlags flags);`

[Source: drive_faults.c:18](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive_faults.c#L18) · External linkage.

Atomically clear only requested bits. Recovery policy belongs to caller; clearing a flag never constitutes ARM.

- **Direct Core calls:** [hardware_fault_status](configuration.md#fn-hardware-fault-status).
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [LL_FOC_Update_Temperature](motor.md#fn-ll-foc-update-temperature), [LL_FOC_Update_Voltage](motor.md#fn-ll-foc-update-voltage), [positionSensor_getRadiansEstimation](motor.md#fn-positionsensor-getradiansestimation), [service](application.md#fn-service).

<a id="module-eeprom"></a>

## Flash backend

**Files:** [Inc/eeprom.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/eeprom.h), [Src/eeprom.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/eeprom.c).

Existing page-63 read/erase/doubleword-program implementation. Synchronous flash storage, not an external EEPROM chip, CRC store or transaction system. Configuration service orchestrates it.

**Verification entry points:** configuration, protocol; physical flash needs bench checks. See [test instructions](../../tests/README.md).

<a id="fn-eeprom-base-address"></a>

### `eeprom_base_address`

`uint8_t const * eeprom_base_address();`

[Source: eeprom.c:14](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/eeprom.c#L14) · External linkage.

Return the constant memory-mapped configuration address 0x0801f800, page 63. No allocation.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-eeprom-empty"></a>

### `eeprom_empty`

`bool eeprom_empty();`

[Source: eeprom.c:19](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/eeprom.c#L19) · External linkage.

Test whether the first 32-bit word is erased (0xffffffff). No full-page, integrity or format check.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [ConfigStorage_IsEmpty](configuration.md#fn-configstorage-isempty).

<a id="fn-eeprom-restore"></a>

### `eeprom_restore`

`HAL_StatusTypeDef eeprom_restore(uint8_t * regs, uint32_t size);`

[Source: eeprom.c:24](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/eeprom.c#L24) · External linkage.

Copy size bytes directly from flash into the output and return HAL_OK. Current callers request 128 bytes; no CRC verification.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `memcpy`.
- **Direct Core callers:** [ConfigStorage_Load](configuration.md#fn-configstorage-load).

<a id="fn-eeprom-store"></a>

### `eeprom_store`

`HAL_StatusTypeDef eeprom_store(uint8_t const * regs, uint32_t size);`

[Source: eeprom.c:30](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/eeprom.c#L30) · External linkage.

Unlock flash, erase page 63, program 64-bit doublewords and relock on completion/error. Size must be divisible by eight. Return HAL status; partial flash after failure/power loss is possible.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_FLASHEx_Erase`, `HAL_FLASH_Lock`, `HAL_FLASH_Program`, `HAL_FLASH_Unlock`, `memcpy`.
- **Direct Core callers:** [ConfigStorage_Save](configuration.md#fn-configstorage-save).

<a id="module-binary-tool"></a>

## Byte packing macros

**Files:** [Inc/binary_tool.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/binary_tool.h).

Header-only LOW_BYTE, HIGH_BYTE and MAKE_SHORT. MAKE_SHORT assembles a little-endian unsigned 16-bit value; signed interpretation is an explicit consuming cast. No function definitions.

**Verification entry points:** can, protocol, configuration, telemetry. See [test instructions](../../tests/README.md).

No C function definitions. See the [type catalogue](types.md) for data types and the source for macros/options.
