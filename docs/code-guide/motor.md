# Encoder, motor control and math

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

- [Motor sample owner](#module-position-sensor)
- [AS5600 register driver](#module-as5600)
- [Fast current-control engine](#module-foc)
- [Inline PWM/coast helpers](#module-foc-utils)
- [Inline hardware trigonometry](#module-cordic)
- [PI/PID helpers](#module-pid)
- [Numeric helpers](#module-math-tool)

<a id="module-position-sensor"></a>

## Motor sample owner

**Files:** [Inc/position_sensor.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/position_sensor.h), [Src/position_sensor.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/position_sensor.c).

Static sensor storage and foreground I2C acquisition; atomic sample publication is read by ADC/FOC. Unwrap counts, compute velocities, predict angle and report faults. Readiness is initialization success, not continuing health.

**Verification entry points:** sensor, calibration, integration. See [test instructions](../../tests/README.md).

<a id="fn-positionsensor-isready"></a>

### `positionSensor_isReady`

`bool positionSensor_isReady(void);`

[Source: position_sensor.c:46](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/position_sensor.c#L46) · External linkage.

Report successful initialization, not absence of later acquisition faults. Readiness remains true after a bad read.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [API_FOC_Calibration_Begin](motor.md#fn-api-foc-calibration-begin), [API_FOC_Calibration_SetFlux](motor.md#fn-api-foc-calibration-setflux), [Calibration_Update](application.md#fn-calibration-update), [Drive_RequestArm](application.md#fn-drive-requestarm), [complete](application.md#fn-complete).

<a id="fn-positionsensor-init"></a>

### `positionSensor_init`

`bool positionSensor_init(void);`

[Source: position_sensor.c:51](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/position_sensor.c#L51) · External linkage.

Reset static state, bind I2C1, configure/check AS5600 and obtain the initial angle/timestamp. Return true when ready; failure latches encoder-read fault. No allocation.

- **Direct Core calls:** [AS5600_GetAngle](motor.md#fn-as5600-getangle), [AS5600_Init](motor.md#fn-as5600-init), [hardware_fault_set](configuration.md#fn-hardware-fault-set).
- **Other direct calls / macros:** `RADIANS_TO_DEGREES`, `__HAL_TIM_GET_COUNTER`, `memset`.
- **Direct Core callers:** [main](platform.md#fn-main).

<a id="fn-positionsensor-getradiansestimation"></a>

### `positionSensor_getRadiansEstimation`

`float positionSensor_getRadiansEstimation(uint16_t time_us);`

[Source: position_sensor.c:74](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/position_sensor.c#L74) · External linkage.

Predict from angle/velocity plus retained 200 us latency compensation. Before init return zero. Samples older than 1200 us fault stale and return zero; fresh samples clear stale only after torque permission is off.

- **Direct Core calls:** [control_torque_requested](configuration.md#fn-control-torque-requested), [hardware_fault_clear](configuration.md#fn-hardware-fault-clear), [hardware_fault_set](configuration.md#fn-hardware-fault-set).
- **Other direct calls / macros:** None.
- **Direct Core callers:** [API_FOC_Torque_Update](motor.md#fn-api-foc-torque-update).

<a id="fn-positionsensor-update"></a>

### `positionSensor_update`

`void positionSensor_update(void);`

[Source: position_sensor.c:90](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/position_sensor.c#L90) · External linkage.

Foreground magnet/angle reads, unwrap turns and compute velocity from elapsed us. Failed reads fault/increment diagnostics/preserve the whole sample. Zero elapsed skips publication; successful complete sample publication masks IRQs.

- **Direct Core calls:** [AS5600_CheckMagnet](motor.md#fn-as5600-checkmagnet), [AS5600_GetAngle](motor.md#fn-as5600-getangle), [control_record_sensor_error](configuration.md#fn-control-record-sensor-error), [hardware_fault_set](configuration.md#fn-hardware-fault-set).
- **Other direct calls / macros:** `RADIANS_TO_DEGREES`, `__DMB`, `__HAL_TIM_GET_COUNTER`, `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`, `fabsf`.
- **Direct Core callers:** [main](platform.md#fn-main), [update_servo](application.md#fn-update-servo).

<a id="fn-positionsensor-getradiansmultiturn"></a>

### `positionSensor_getRadiansMultiturn`

`float positionSensor_getRadiansMultiturn(void);`

[Source: position_sensor.c:147](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/position_sensor.c#L147) · External linkage.

Return last unwrapped motor angle in radians, or zero before init. No new acquisition/acknowledgement.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [Calibration_Update](application.md#fn-calibration-update).

<a id="fn-positionsensor-getdegreesmultiturn"></a>

### `positionSensor_getDegreesMultiturn`

`float positionSensor_getDegreesMultiturn(void);`

[Source: position_sensor.c:152](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/position_sensor.c#L152) · External linkage.

Return last unwrapped motor angle in degrees, or zero before init. Separate getters do not create a group snapshot.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [Drive_RequestArm](application.md#fn-drive-requestarm), [update_servo](application.md#fn-update-servo).

<a id="fn-positionsensor-getvelocitydps"></a>

### `positionSensor_getVelocityDps`

`float positionSensor_getVelocityDps(void);`

[Source: position_sensor.c:157](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/position_sensor.c#L157) · External linkage.

Return last calculated motor velocity in degrees/s, or zero before init. Getter performs no differentiation.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [API_FOC_Torque_Update](motor.md#fn-api-foc-torque-update), [update_servo](application.md#fn-update-servo).

<a id="module-as5600"></a>

## AS5600 register driver

**Files:** [Inc/as5600.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/as5600.h), [Src/as5600.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c).

Synchronous startup/foreground I2C only. Private unions encode hardware registers. Normal path uses Init, CheckMagnet and GetAngle. Retained register utilities are included in this reference.

**Verification entry points:** sensor, integration. See [test instructions](../../tests/README.md).

<a id="fn-as5600-init"></a>

### `AS5600_Init`

`HAL_StatusTypeDef AS5600_Init(AS5600_TypeDef *as5600);`

[Source: as5600.c:182](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L182) · External linkage.

Set shifted address, write acquisition/filter CONF, verify readback and check magnet health. Return HAL status. Preserve stored zero/start position.

- **Direct Core calls:** [AS5600_CheckMagnet](motor.md#fn-as5600-checkmagnet), [AS5600_GetConfig](motor.md#fn-as5600-getconfig), [AS5600_SetConfig](motor.md#fn-as5600-setconfig).
- **Other direct calls / macros:** None.
- **Direct Core callers:** [positionSensor_init](motor.md#fn-positionsensor-init).

<a id="fn-as5600-checkmagnet"></a>

### `AS5600_CheckMagnet`

`HAL_StatusTypeDef AS5600_CheckMagnet(AS5600_TypeDef *const a);`

[Source: as5600.c:266](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L266) · External linkage.

Read status and require magnet detection without strength-error flags; HAL_ERROR on read/status failure.

- **Direct Core calls:** [AS5600_GetMagnetStatus](motor.md#fn-as5600-getmagnetstatus).
- **Other direct calls / macros:** None.
- **Direct Core callers:** [AS5600_Init](motor.md#fn-as5600-init), [positionSensor_update](motor.md#fn-positionsensor-update).

<a id="fn-as5600-getrawangle"></a>

### `AS5600_GetRawAngle`

`HAL_StatusTypeDef AS5600_GetRawAngle(AS5600_TypeDef *const a, uint16_t *const angle);`

[Source: as5600.c:277](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L277) · External linkage.

Read raw angle into a local register value and publish 12-bit counts only on success; failure preserves caller output.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_I2C_Mem_Read`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-as5600-getangle"></a>

### `AS5600_GetAngle`

`HAL_StatusTypeDef AS5600_GetAngle(AS5600_TypeDef *const a, uint16_t *const angle);`

[Source: as5600.c:292](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L292) · External linkage.

Read normal angle register and publish 12-bit counts only after success. Used by positionSensor; output is not radians.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_I2C_Mem_Read`.
- **Direct Core callers:** [positionSensor_init](motor.md#fn-positionsensor-init), [positionSensor_update](motor.md#fn-positionsensor-update).

<a id="fn-as5600-getmagnetstatus"></a>

### `AS5600_GetMagnetStatus`

`HAL_StatusTypeDef AS5600_GetMagnetStatus(AS5600_TypeDef *const a, status_reg_t *stat);`

[Source: as5600.c:307](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L307) · External linkage.

Internal status-byte I2C read into status_reg_t. Ignore failed output, which HAL may have touched.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_I2C_Mem_Read`.
- **Direct Core callers:** [AS5600_CheckMagnet](motor.md#fn-as5600-checkmagnet).

<a id="fn-as5600-getagcsetting"></a>

### `AS5600_GetAGCSetting`

`HAL_StatusTypeDef AS5600_GetAGCSetting(AS5600_TypeDef *const a, uint8_t *const agc);`

[Source: as5600.c:319](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L319) · External linkage.

Read sensor automatic-gain-control byte and return HAL status. Retained diagnostic API; ignore failed output.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_I2C_Mem_Read`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-as5600-getcordicmagnitude"></a>

### `AS5600_GetCORDICMagnitude`

`HAL_StatusTypeDef AS5600_GetCORDICMagnitude(AS5600_TypeDef *const a, uint16_t *const mag);`

[Source: as5600.c:331](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L331) · External linkage.

Read sensor's 12-bit magnetic-magnitude diagnostic. Unrelated to STM32 CORDIC trigonometry.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_I2C_Mem_Read`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-as5600-setmaxangle"></a>

### `AS5600_SetMaxAngle`

`HAL_StatusTypeDef AS5600_SetMaxAngle(AS5600_TypeDef *const a, const uint16_t angle);`

[Source: as5600.c:347](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L347) · External linkage.

Write raw-count MANG maximum angle. Retained sensor utility, not used for actuator travel limits.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_I2C_Mem_Write`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-as5600-getmaxangle"></a>

### `AS5600_GetMaxAngle`

`HAL_StatusTypeDef AS5600_GetMaxAngle(AS5600_TypeDef *const a, uint16_t *angle);`

[Source: as5600.c:364](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L364) · External linkage.

Read/assemble raw-count MANG and publish on success; retained inspection utility.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_I2C_Mem_Read`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-as5600-setstopposition"></a>

### `AS5600_SetStopPosition`

`HAL_StatusTypeDef AS5600_SetStopPosition(AS5600_TypeDef *const a, const uint16_t pos);`

[Source: as5600.c:382](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L382) · External linkage.

Write raw-count MPOS stop position. Not a DriveConfig travel-limit setter.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_I2C_Mem_Write`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-as5600-getstopposition"></a>

### `AS5600_GetStopPosition`

`HAL_StatusTypeDef AS5600_GetStopPosition(AS5600_TypeDef *const a, uint16_t *pos);`

[Source: as5600.c:399](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L399) · External linkage.

Read/assemble raw-count MPOS and publish on success.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_I2C_Mem_Read`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-as5600-setstartposition"></a>

### `AS5600_SetStartPosition`

`HAL_StatusTypeDef AS5600_SetStartPosition(AS5600_TypeDef *const a, const uint16_t pos);`

[Source: as5600.c:416](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L416) · External linkage.

Write raw-count ZPOS, changing sensor zero. Retained utility intentionally not called by selected startup.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_I2C_Mem_Write`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-as5600-getstartposition"></a>

### `AS5600_GetStartPosition`

`HAL_StatusTypeDef AS5600_GetStartPosition(AS5600_TypeDef *const a, uint16_t *pos);`

[Source: as5600.c:433](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L433) · External linkage.

Read/assemble raw-count ZPOS/start position and publish on success.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_I2C_Mem_Read`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-as5600-setconfig"></a>

### `AS5600_SetConfig`

`HAL_StatusTypeDef AS5600_SetConfig(AS5600_TypeDef *const a, conf_reg_t conf);`

[Source: as5600.c:449](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L449) · External linkage.

Internal CONF register encode/write; configures sensor options, not firmware DriveConfig or flash.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_I2C_Mem_Write`.
- **Direct Core callers:** [AS5600_Init](motor.md#fn-as5600-init).

<a id="fn-as5600-getconfig"></a>

### `AS5600_GetConfig`

`HAL_StatusTypeDef AS5600_GetConfig(AS5600_TypeDef *const a, conf_reg_t *conf);`

[Source: as5600.c:461](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L461) · External linkage.

Internal CONF readback for verification; caller checks status before using bitfields.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_I2C_Mem_Read`.
- **Direct Core callers:** [AS5600_Init](motor.md#fn-as5600-init).

<a id="module-foc"></a>

## Fast current-control engine

**Files:** [Inc/foc.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/foc.h), [Src/foc.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c).

ADC path owns phase acquisition/offsets, PI contexts, FOC modes, targets and observations. Foreground analog service applies bus/temperature protection. Drive owns application permission; Calibration owns the trajectory.

**Verification entry points:** foc, startup, cordic, calibration, integration boundary. See [test instructions](../../tests/README.md).

<a id="fn-hal-adc-convcpltcallback"></a>

### `HAL_ADC_ConvCpltCallback`

`void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc);`

[Source: foc.c:52](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L52) · External linkage; HAL hook.

HAL completion hook forwarding the ADC handle to API_FOC_It, which filters converters and pairs samples.

- **Direct Core calls:** [API_FOC_It](motor.md#fn-api-foc-it).
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-hal-adc-errorcallback"></a>

### `HAL_ADC_ErrorCallback`

`void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc);`

[Source: foc.c:57](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L57) · External linkage; HAL hook.

For ADC1/ADC2, latch internal fault and immediately disable/coast. Unrelated ADC handles are ignored.

- **Direct Core calls:** [API_FOC_Torque_Disable](motor.md#fn-api-foc-torque-disable), [hardware_fault_set](configuration.md#fn-hardware-fault-set).
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-api-foc-torque-enable"></a>

### `API_FOC_Torque_Enable`

`void API_FOC_Torque_Enable();`

[Source: foc.c:114](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L114) · External linkage.

Atomically reset targets, measurements and PI histories, then enter torque mode only with live permission, valid settings and no fault. Not an independent ARM API.

- **Direct Core calls:** [Config_IsValid](configuration.md#fn-config-isvalid), [control_torque_requested](configuration.md#fn-control-torque-requested), [hardware_fault_status](configuration.md#fn-hardware-fault-status), [pid_reset](motor.md#fn-pid-reset).
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [Drive_PublishCurrents](application.md#fn-drive-publishcurrents).

<a id="fn-api-foc-torque-disable"></a>

### `API_FOC_Torque_Disable`

`void API_FOC_Torque_Disable();`

[Source: foc.c:136](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L136) · External linkage.

Mark active calibration cancelled, enter IDLE, zero current targets and coast atomically. Supervisor arming is separately cleared by Drive_Disarm.

- **Direct Core calls:** [LL_FOC_coast](motor.md#fn-ll-foc-coast).
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [API_FOC_Check_Watchdog](motor.md#fn-api-foc-check-watchdog), [API_FOC_Init](motor.md#fn-api-foc-init), [API_FOC_It](motor.md#fn-api-foc-it), [API_FOC_Torque_Update](motor.md#fn-api-foc-torque-update), [Drive_DisableIfDisarmed](application.md#fn-drive-disableifdisarmed), [Drive_Disarm](application.md#fn-drive-disarm), [FOC_Check_Current_Protection](motor.md#fn-foc-check-current-protection), [HAL_ADC_ErrorCallback](motor.md#fn-hal-adc-errorcallback).

<a id="fn-api-foc-is-calibrating"></a>

### `API_FOC_Is_Calibrating`

`bool API_FOC_Is_Calibrating(void);`

[Source: foc.c:149](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L149) · External linkage.

Return the active exclusion flag, including settling, cleanup and saving rather than motor motion alone.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [CanServo_OnRxFifo0](application.md#fn-canservo-onrxfifo0), [Drive_BeginMaintenance](application.md#fn-drive-beginmaintenance), [Drive_GetStatus](application.md#fn-drive-getstatus), [Drive_IsDisarmed](application.md#fn-drive-isdisarmed), [Drive_PublishCurrents](application.md#fn-drive-publishcurrents), [Drive_RequestArm](application.md#fn-drive-requestarm), [Drive_SubmitCommand](application.md#fn-drive-submitcommand), [update_servo](application.md#fn-update-servo).

<a id="fn-api-foc-check-watchdog"></a>

### `API_FOC_Check_Watchdog`

`bool API_FOC_Check_Watchdog(void);`

[Source: foc.c:154](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L154) · External linkage.

True if FOC timestamp age is at most 10 ms. Otherwise latch FOC timeout and disable/coast immediately. Uses unsigned time under an IRQ guard.

- **Direct Core calls:** [API_FOC_Get_Timestamp_ms](motor.md#fn-api-foc-get-timestamp-ms), [API_FOC_Torque_Disable](motor.md#fn-api-foc-torque-disable), [hardware_fault_set](configuration.md#fn-hardware-fault-set).
- **Other direct calls / macros:** `HAL_GetTick`, `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [Calibration_Update](application.md#fn-calibration-update), [Drive_Service](application.md#fn-drive-service).

<a id="fn-api-foc-set-torque-flux-currents-ma"></a>

### `API_FOC_Set_Torque_Flux_Currents_mA`

`void API_FOC_Set_Torque_Flux_Currents_mA(float Iq_mA, float Id_mA);`

[Source: foc.c:169](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L169) · External linkage.

Atomically accept Iq/Id only in torque mode with live permission and no fault. Returns void; normal callers use the surrounding Drive publication guard.

- **Direct Core calls:** [control_torque_requested](configuration.md#fn-control-torque-requested), [hardware_fault_status](configuration.md#fn-hardware-fault-status).
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [Drive_PublishCurrents](application.md#fn-drive-publishcurrents).

<a id="fn-api-foc-init"></a>

### `API_FOC_Init`

`void API_FOC_Init();`

[Source: foc.c:185](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L185) · External linkage.

Initialize CORDIC, safe PWM compares/coast, op-amps, timer and ADC DMA. Checked start failures call Error_Handler. Config/encoder/timebase must exist first.

- **Direct Core calls:** [API_CORDIC_Processor_Init](motor.md#fn-api-cordic-processor-init), [API_FOC_Torque_Disable](motor.md#fn-api-foc-torque-disable), [Error_Handler](platform.md#fn-error-handler), [LL_FOC_coast](motor.md#fn-ll-foc-coast).
- **Other direct calls / macros:** `HAL_ADC_Start_DMA`, `HAL_OPAMP_Start`, `HAL_TIM_Base_Start`, `SET_BIT`, `__HAL_TIM_SET_COMPARE`.
- **Direct Core callers:** [main](platform.md#fn-main).

<a id="fn-ll-foc-update-temperature"></a>

### `LL_FOC_Update_Temperature`

`void LL_FOC_Update_Temperature();`

[Source: foc.c:216](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L216) · External linkage.

Apply the existing thermistor calculation to ADC observations, yielding Celsius. Set overheating above the configured limit and clear below limit minus 12 degrees.

- **Direct Core calls:** [control_read_protection_config](configuration.md#fn-control-read-protection-config), [hardware_fault_clear](configuration.md#fn-hardware-fault-clear), [hardware_fault_set](configuration.md#fn-hardware-fault-set), [hardware_fault_status](configuration.md#fn-hardware-fault-status).
- **Other direct calls / macros:** `logf`.
- **Direct Core callers:** [API_FOC_Service_Update](motor.md#fn-api-foc-service-update).

<a id="fn-ll-foc-update-voltage"></a>

### `LL_FOC_Update_Voltage`

`void LL_FOC_Update_Voltage();`

[Source: foc.c:259](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L259) · External linkage.

Filter/scale bus ADC observations to volts and set/clear bus fault against configured limits.

- **Direct Core calls:** [control_read_protection_config](configuration.md#fn-control-read-protection-config), [hardware_fault_clear](configuration.md#fn-hardware-fault-clear), [hardware_fault_set](configuration.md#fn-hardware-fault-set).
- **Other direct calls / macros:** None.
- **Direct Core callers:** [API_FOC_Service_Update](motor.md#fn-api-foc-service-update).

<a id="fn-api-foc-calibration-idle"></a>

### `API_FOC_Calibration_Idle`

`void API_FOC_Calibration_Idle(void);`

[Source: foc.c:287](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L287) · External linkage.

Enter IDLE, clear calibration angle/voltage targets and coast atomically, retaining the active guard for cleanup/save.

- **Direct Core calls:** [LL_FOC_coast](motor.md#fn-ll-foc-coast).
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [API_FOC_Calibration_Begin](motor.md#fn-api-foc-calibration-begin), [API_FOC_Calibration_End](motor.md#fn-api-foc-calibration-end), [Calibration_Update](application.md#fn-calibration-update), [complete](application.md#fn-complete).

<a id="fn-api-foc-calibration-begin"></a>

### `API_FOC_Calibration_Begin`

`bool API_FOC_Calibration_Begin(void);`

[Source: foc.c:298](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L298) · External linkage.

Atomically require no active calibration/torque/fault, ready encoder and valid config. Set active, clear cancellation and coast on success; return acceptance.

- **Direct Core calls:** [API_FOC_Calibration_Idle](motor.md#fn-api-foc-calibration-idle), [Config_IsValid](configuration.md#fn-config-isvalid), [control_torque_requested](configuration.md#fn-control-torque-requested), [hardware_fault_status](configuration.md#fn-hardware-fault-status), [positionSensor_isReady](motor.md#fn-positionsensor-isready).
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [Calibration_Start](application.md#fn-calibration-start).

<a id="fn-api-foc-calibration-setflux"></a>

### `API_FOC_Calibration_SetFlux`

`bool API_FOC_Calibration_SetFlux(float angle_rad, float voltage_V);`

[Source: foc.c:313](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L313) · External linkage.

Publish open-loop angle/voltage only while active, uncancelled, fault-free and sensor-ready. True means accepted; cancellation cannot reenable via this primitive.

- **Direct Core calls:** [hardware_fault_status](configuration.md#fn-hardware-fault-status), [positionSensor_isReady](motor.md#fn-positionsensor-isready).
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [excite](application.md#fn-excite).

<a id="fn-api-foc-calibration-cancelled"></a>

### `API_FOC_Calibration_Cancelled`

`bool API_FOC_Calibration_Cancelled(void);`

[Source: foc.c:328](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L328) · External linkage.

Read the active job's cancellation latch. The next accepted start clears it.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [Calibration_Update](application.md#fn-calibration-update), [complete](application.md#fn-complete).

<a id="fn-api-foc-calibration-end"></a>

### `API_FOC_Calibration_End`

`void API_FOC_Calibration_End(void);`

[Source: foc.c:330](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L330) · External linkage.

Coast and release the active guard atomically after job cleanup/result publication/save. ARM may be accepted afterwards.

- **Direct Core calls:** [API_FOC_Calibration_Idle](motor.md#fn-api-foc-calibration-idle).
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [complete](application.md#fn-complete).

<a id="fn-api-foc-service-update"></a>

### `API_FOC_Service_Update`

`void API_FOC_Service_Update();`

[Source: foc.c:339](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L339) · External linkage.

Foreground analog service: update temperature and bus voltage. App schedules 10 ms; startup also calls it before CAN acceptance.

- **Direct Core calls:** [LL_FOC_Update_Temperature](motor.md#fn-ll-foc-update-temperature), [LL_FOC_Update_Voltage](motor.md#fn-ll-foc-update-voltage).
- **Other direct calls / macros:** None.
- **Direct Core callers:** [main](platform.md#fn-main), [service](application.md#fn-service).

<a id="fn-api-foc-torque-update"></a>

### `API_FOC_Torque_Update`

`void API_FOC_Torque_Update();`

[Source: foc.c:351](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L351) · External linkage.

Timestamp one fast iteration, read motor settings and select IDLE/current/calibration behavior. Transform currents, run CORDIC/current PI/voltage limiting and coast/brake/set PWM. Handle CORDIC errors and retain armed overspeed braking. Update averaged timing/counter diagnostics.

- **Direct Core calls:** [API_CORDIC_Processor_Update](motor.md#fn-api-cordic-processor-update), [API_FOC_Torque_Disable](motor.md#fn-api-foc-torque-disable), [LL_FOC_brake](motor.md#fn-ll-foc-brake), [LL_FOC_coast](motor.md#fn-ll-foc-coast), [LL_FOC_set_phase_voltage](motor.md#fn-ll-foc-set-phase-voltage), [control_read_motor_config](configuration.md#fn-control-read-motor-config), [hardware_fault_set](configuration.md#fn-hardware-fault-set), [normalize_angle](motor.md#fn-normalize-angle), [pi_process_antiwindup_clamp](motor.md#fn-pi-process-antiwindup-clamp), [pid_reset](motor.md#fn-pid-reset), [positionSensor_getRadiansEstimation](motor.md#fn-positionsensor-getradiansestimation), [positionSensor_getVelocityDps](motor.md#fn-positionsensor-getvelocitydps).
- **Other direct calls / macros:** `DEGREES_TO_RADIANS`, `HAL_GetTick`, `__HAL_TIM_GET_COUNTER`, `fabsf`, `sqrtf`.
- **Direct Core callers:** [API_FOC_It](motor.md#fn-api-foc-it).

<a id="fn-api-foc-get-timestamp-ms"></a>

### `API_FOC_Get_Timestamp_ms`

`uint32_t API_FOC_Get_Timestamp_ms();`

[Source: foc.c:517](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L517) · External linkage.

Return wrapping HAL ms captured at entry to the last torque update; used by the heartbeat watchdog.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [API_FOC_Check_Watchdog](motor.md#fn-api-foc-check-watchdog).

<a id="fn-api-foc-get-timestamp-us"></a>

### `API_FOC_Get_Timestamp_us`

`uint16_t API_FOC_Get_Timestamp_us();`

[Source: foc.c:522](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L522) · External linkage.

Return wrapping 16-bit timer us captured at entry to the last torque update.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-api-foc-get-present-torque-current"></a>

### `API_FOC_Get_Present_Torque_Current`

`float API_FOC_Get_Present_Torque_Current();`

[Source: foc.c:527](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L527) · External linkage.

Return last transformed Iq in mA, not shaft torque in Nm. Mask ADCs for coherent multi-field capture.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [update_servo](application.md#fn-update-servo).

<a id="fn-api-foc-get-present-flux-current"></a>

### `API_FOC_Get_Present_Flux_Current`

`float API_FOC_Get_Present_Flux_Current();`

[Source: foc.c:532](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L532) · External linkage.

Return last transformed Id in mA. Open-loop calibration reports transformed Id/Iq as zero rather than estimating them from that excitation angle.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [update_servo](application.md#fn-update-servo).

<a id="fn-api-foc-get-present-voltage"></a>

### `API_FOC_Get_Present_Voltage`

`float API_FOC_Get_Present_Voltage();`

[Source: foc.c:537](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L537) · External linkage.

Return filtered bus voltage in V, last updated by foreground analog service.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [update_servo](application.md#fn-update-servo).

<a id="fn-api-foc-get-present-temp"></a>

### `API_FOC_Get_Present_Temp`

`float API_FOC_Get_Present_Temp();`

[Source: foc.c:542](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L542) · External linkage.

Return calculated controller temperature in Celsius; not an independent motor-temperature measurement.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [update_servo](application.md#fn-update-servo).

<a id="fn-api-foc-get-processing-time"></a>

### `API_FOC_Get_Processing_Time`

`float API_FOC_Get_Processing_Time();`

[Source: foc.c:547](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L547) · External linkage.

Return smoothed torque-update duration in us, not worst-case execution time.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [update_servo](application.md#fn-update-servo).

<a id="fn-api-foc-get-processing-frequency"></a>

### `API_FOC_Get_Processing_Frequency`

`float API_FOC_Get_Processing_Frequency();`

[Source: foc.c:552](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L552) · External linkage.

Return completed torque updates divided by HAL uptime in Hz, or zero at zero uptime. Lifetime average, not per-period measurement.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_GetTick`.
- **Direct Core callers:** [update_servo](application.md#fn-update-servo).

<a id="fn-foc-check-current-protection"></a>

### `FOC_Check_Current_Protection`

`static inline void FOC_Check_Current_Protection(void);`

[Source: foc.c:558](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L558) · Private/static.

Private active-state check: nonfinite phase current faults internal; absolute current above 1500 mA faults overcurrent. Both disable/coast. IDLE skips this active-current trip.

- **Direct Core calls:** [API_FOC_Torque_Disable](motor.md#fn-api-foc-torque-disable), [hardware_fault_set](configuration.md#fn-hardware-fault-set).
- **Other direct calls / macros:** `fabsf`, `isfinite`.
- **Direct Core callers:** [API_FOC_It](motor.md#fn-api-foc-it).

<a id="fn-api-foc-it"></a>

### `API_FOC_It`

`void API_FOC_It(ADC_HandleTypeDef *hadc);`

[Source: foc.c:578](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L578) · External linkage.

Pair ADC1/ADC2 DMA samples in the selected TIM1 half-cycle, update phase/auxiliary observations and offsets on the other half-cycle. Duplicate/incomplete active pairs fault/coast. A complete pair runs protection then torque update.

- **Direct Core calls:** [API_FOC_Torque_Disable](motor.md#fn-api-foc-torque-disable), [API_FOC_Torque_Update](motor.md#fn-api-foc-torque-update), [FOC_Check_Current_Protection](motor.md#fn-foc-check-current-protection), [hardware_fault_set](configuration.md#fn-hardware-fault-set).
- **Other direct calls / macros:** `__HAL_TIM_IS_TIM_COUNTING_DOWN`.
- **Direct Core callers:** [HAL_ADC_ConvCpltCallback](motor.md#fn-hal-adc-convcpltcallback).

<a id="module-foc-utils"></a>

## Inline PWM/coast helpers

**Files:** [Inc/foc_utils.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/foc_utils.h), [Src/foc_utils.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc_utils.c).

Implementation is in foc_utils.h; foc_utils.c is an empty compatibility unit. Operate TIM1 compares/MOE with fault precedence. phase_voltage_pending is output-enable history, not ARM permission.

**Verification entry points:** foc, fatal. See [test instructions](../../tests/README.md).

<a id="fn-ll-foc-coast"></a>

### `LL_FOC_coast`

`static inline void LL_FOC_coast(void);`

[Source: foc_utils.h:33](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/foc_utils.h#L33) · Private/static.

Inline clear of pending-enable history and TIM1 MOE. Keep timer/channels running for ADC triggers. Physical gate/back-drive behavior remains a bench check.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `__HAL_TIM_MOE_DISABLE_UNCONDITIONALLY`.
- **Direct Core callers:** [API_FOC_Calibration_Idle](motor.md#fn-api-foc-calibration-idle), [API_FOC_Init](motor.md#fn-api-foc-init), [API_FOC_Torque_Disable](motor.md#fn-api-foc-torque-disable), [API_FOC_Torque_Update](motor.md#fn-api-foc-torque-update), [LL_FOC_brake](motor.md#fn-ll-foc-brake), [LL_FOC_set_phase_voltage](motor.md#fn-ll-foc-set-phase-voltage).

<a id="fn-ll-foc-set-phase-voltage"></a>

### `LL_FOC_set_phase_voltage`

`static inline void LL_FOC_set_phase_voltage( float Vd, float Vq, float cosine_theta, float sine_theta, float present_voltage_V );`

[Source: foc_utils.h:40](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/foc_utils.h#L40) · Private/static.

Inline inverse Park/Clarke and CSVPWM. Reject faults/nonfinite values, clamp duties to 6..94%, write preloads and wait for a later call/natural PWM cycle before enabling MOE. Not application permission policy.

- **Direct Core calls:** [LL_FOC_coast](motor.md#fn-ll-foc-coast), [fconstrain](motor.md#fn-fconstrain), [hardware_fault_set](configuration.md#fn-hardware-fault-set), [hardware_fault_status](configuration.md#fn-hardware-fault-status).
- **Other direct calls / macros:** `__HAL_TIM_GET_AUTORELOAD`, `__HAL_TIM_MOE_ENABLE`, `__HAL_TIM_SET_COMPARE`, `fmaxf`, `fminf`, `isfinite`.
- **Direct Core callers:** [API_FOC_Torque_Update](motor.md#fn-api-foc-torque-update).

<a id="fn-ll-foc-brake"></a>

### `LL_FOC_brake`

`static inline void LL_FOC_brake(void);`

[Source: foc_utils.h:103](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/foc_utils.h#L103) · Private/static.

Retained armed-overspeed helper: coast if faulted, otherwise write equal 50% preloads. Does not enable already-disabled outputs; not the normal disabled policy.

- **Direct Core calls:** [LL_FOC_coast](motor.md#fn-ll-foc-coast), [hardware_fault_status](configuration.md#fn-hardware-fault-status).
- **Other direct calls / macros:** `__HAL_TIM_GET_AUTORELOAD`, `__HAL_TIM_SET_COMPARE`.
- **Direct Core callers:** [API_FOC_Torque_Update](motor.md#fn-api-foc-torque-update).

<a id="module-cordic"></a>

## Inline hardware trigonometry

**Files:** [Inc/cordic.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/cordic.h), [Src/cordic.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/cordic.c).

Implementation is in cordic.h; cordic.c is an empty compatibility unit. Bounded polling prevents an indefinite accelerator wait. Borrow the global handle initialized in main.

**Verification entry points:** cordic, startup. See [test instructions](../../tests/README.md).

<a id="fn-radianstoq31"></a>

### `RadiansToQ31`

`inline int32_t RadiansToQ31(float x);`

[Source: cordic.h:27](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/cordic.h#L27) · External linkage.

Wrap/convert radians to the signed Q31 angle representation CORDIC expects. Intended behind the checked update API, not a validator for arbitrary/nonfinite input.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [API_CORDIC_Processor_Update](motor.md#fn-api-cordic-processor-update).

<a id="fn-q31tofloat"></a>

### `Q31ToFloat`

`inline float Q31ToFloat(int32_t x);`

[Source: cordic.h:42](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/cordic.h#L42) · External linkage.

Convert a unitless signed Q1.31 sine/cosine result by dividing by 2^31. Not an angle conversion.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [API_CORDIC_Processor_Update](motor.md#fn-api-cordic-processor-update).

<a id="fn-api-cordic-processor-init"></a>

### `API_CORDIC_Processor_Init`

`HAL_StatusTypeDef API_CORDIC_Processor_Init();`

[Source: cordic.h:47](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/cordic.h#L47) · External linkage.

Configure one 32-bit angle write and cosine/sine result reads at existing precision; return HAL configuration status.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_CORDIC_Configure`.
- **Direct Core callers:** [API_FOC_Init](motor.md#fn-api-foc-init).

<a id="fn-api-cordic-processor-update"></a>

### `API_CORDIC_Processor_Update`

`inline HAL_StatusTypeDef API_CORDIC_Processor_Update(float theta_rad, float * c, float * s);`

[Source: cordic.h:63](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/cordic.h#L63) · External linkage.

Require finite angle in 0..2*pi and ready hardware. Write Q31, poll at most 64 times and copy cosine/sine on HAL_OK. Timeout marks handle error and returns HAL_TIMEOUT without publishing outputs.

- **Direct Core calls:** [Q31ToFloat](motor.md#fn-q31tofloat), [RadiansToQ31](motor.md#fn-radianstoq31).
- **Other direct calls / macros:** `LL_CORDIC_IsActiveFlag_RRDY`, `LL_CORDIC_ReadData`, `LL_CORDIC_WriteData`, `isfinite`.
- **Direct Core callers:** [API_FOC_Torque_Update](motor.md#fn-api-foc-torque-update).

<a id="module-pid"></a>

## PI/PID helpers

**Files:** [Inc/pid.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/pid.h), [Src/pic.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/pic.c).

Implementation is historically named pic.c; declarations are in pid.h. Callers own separate pid_context_t histories. Current PI and outer PID-plus-feed-forward are active; other variants are retained.

**Verification entry points:** servo, foc, cordic. See [test instructions](../../tests/README.md).

<a id="fn-pid-reset"></a>

### `pid_reset`

`void pid_reset( pid_context_t * ctx );`

[Source: pic.c:13](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/pic.c#L13) · External linkage.

Clear prior error, integral and filtered derivative in the supplied context. Used on new ARM/inner-loop transitions.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [API_FOC_Torque_Enable](motor.md#fn-api-foc-torque-enable), [API_FOC_Torque_Update](motor.md#fn-api-foc-torque-update), [ServoController_Init](application.md#fn-servocontroller-init), [ServoController_Update](application.md#fn-servocontroller-update).

<a id="fn-pi-process-antiwindup-clamp"></a>

### `pi_process_antiwindup_clamp`

`float pi_process_antiwindup_clamp( pid_context_t * ctx, float error, float kp, float ki, float output_limit );`

[Source: pic.c:20](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/pic.c#L20) · External linkage.

Compute P plus prior I, clamp output, conditionally integrate unless saturating further with the error. Gains are per invocation; no elapsed-time normalization.

- **Direct Core calls:** [fconstrain](motor.md#fn-fconstrain).
- **Other direct calls / macros:** None.
- **Direct Core callers:** [API_FOC_Torque_Update](motor.md#fn-api-foc-torque-update).

<a id="fn-pid-process-antiwindup-clamp-with-ff"></a>

### `pid_process_antiwindup_clamp_with_ff`

`float pid_process_antiwindup_clamp_with_ff( pid_context_t * ctx, float error, float kp, float ki, float kd, float output_limit, float alpha_derivative, float feed_forward );`

[Source: pic.c:47](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/pic.c#L47) · External linkage.

Filter per-sample error difference, compute P+prior I+D+feed-forward, clamp and conditionally integrate. Outer control passes Ki=0; this generic helper supports integration.

- **Direct Core calls:** [fconstrain](motor.md#fn-fconstrain).
- **Other direct calls / macros:** None.
- **Direct Core callers:** [ServoController_Update](application.md#fn-servocontroller-update).

<a id="fn-pid-process-antiwindup-clamp"></a>

### `pid_process_antiwindup_clamp`

`float pid_process_antiwindup_clamp( pid_context_t * ctx, float error, float kp, float ki, float kd, float output_limit, float alpha_derivative );`

[Source: pic.c:82](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/pic.c#L82) · External linkage.

Retained PID without feed-forward: filtered difference, saturated output and conditional integration. No direct active Core caller.

- **Direct Core calls:** [fconstrain](motor.md#fn-fconstrain).
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-pid-process-antiwindup-back-calculation"></a>

### `pid_process_antiwindup_back_calculation`

`float pid_process_antiwindup_back_calculation( pid_context_t * ctx, float error, float kp, float ki, float kd, float output_limit, float kt, float alpha_derivative );`

[Source: pic.c:116](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/pic.c#L116) · External linkage.

Retained PID using saturation error times kt as integral feedback. Not selected by the active control path.

- **Direct Core calls:** [fconstrain](motor.md#fn-fconstrain).
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="module-math-tool"></a>

## Numeric helpers

**Files:** [Inc/math_tool.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/math_tool.h), [Src/math_tool.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/math_tool.c).

Bounds, mappings, remainder and angle wrapping with documented input assumptions. Retained utilities are not hidden active control stages. Angle/unit conversion constants are macros.

**Verification entry points:** servo, calibration, foc, cordic. See [test instructions](../../tests/README.md).

<a id="fn-constrain"></a>

### `constrain`

`int32_t constrain(int32_t x, int32_t min, int32_t max);`

[Source: math_tool.c:11](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/math_tool.c#L11) · External linkage.

Retained integer clamp to ordered bounds. Not declared in math_tool.h and has no direct Core caller.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-fconstrain"></a>

### `fconstrain`

`float fconstrain(float x, float min, float max);`

[Source: math_tool.c:18](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/math_tool.c#L18) · External linkage.

Float clamp to ordered bounds. Not a nonfinite validator; relevant raw-input callers check arithmetic separately.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [LL_FOC_set_phase_voltage](motor.md#fn-ll-foc-set-phase-voltage), [ServoController_Update](application.md#fn-servocontroller-update), [pi_process_antiwindup_clamp](motor.md#fn-pi-process-antiwindup-clamp), [pid_process_antiwindup_back_calculation](motor.md#fn-pid-process-antiwindup-back-calculation), [pid_process_antiwindup_clamp](motor.md#fn-pid-process-antiwindup-clamp), [pid_process_antiwindup_clamp_with_ff](motor.md#fn-pid-process-antiwindup-clamp-with-ff).

<a id="fn-fconstrain-both"></a>

### `fconstrain_both`

`float fconstrain_both(float x, float abs);`

[Source: math_tool.c:25](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/math_tool.c#L25) · External linkage.

Retained symmetric float clamp to [-abs,+abs]. No direct Core caller/public-header declaration.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-mfmod"></a>

### `mfmod`

`float mfmod(float x,float y);`

[Source: math_tool.c:32](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/math_tool.c#L32) · External linkage.

Truncation-based remainder using x/y and an integer cast. Has input-range assumptions; not a general safe fmodf replacement.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [normalize_angle](motor.md#fn-normalize-angle).

<a id="fn-map"></a>

### `map`

`uint32_t map(uint32_t x, uint32_t in_min, uint32_t in_max, uint32_t out_min, uint32_t out_max);`

[Source: math_tool.c:38](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/math_tool.c#L38) · External linkage.

Retained saturated integer linear mapping. Assumes ordered/nondegenerate ranges; integer arithmetic can overflow for arbitrary inputs.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-fmap"></a>

### `fmap`

`float fmap(float x, float in_min, float in_max, float out_min, float out_max);`

[Source: math_tool.c:45](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/math_tool.c#L45) · External linkage.

Retained saturated float linear mapping for ordered/nondegenerate ranges; no direct Core caller.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-normalize-angle"></a>

### `normalize_angle`

`float normalize_angle(float angle_rad);`

[Source: math_tool.c:52](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/math_tool.c#L52) · External linkage.

Wrap a valid radian angle into one revolution via mfmod and negative-remainder adjustment. Not an arbitrary-input fault validator.

- **Direct Core calls:** [mfmod](motor.md#fn-mfmod).
- **Other direct calls / macros:** None.
- **Direct Core callers:** [API_FOC_Torque_Update](motor.md#fn-api-foc-torque-update), [Calibration_Update](application.md#fn-calibration-update).

<a id="fn-difference-angle"></a>

### `difference_angle`

`float difference_angle(float a_rad, float b_rad);`

[Source: math_tool.c:58](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/math_tool.c#L58) · External linkage.

Adjust a-b by at most one revolution into (-pi,pi] under its single-wrap input assumption. Not a general multi-turn difference reducer.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.
