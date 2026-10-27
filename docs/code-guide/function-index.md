# Function and file index

[Begin with the reading guide](README.md). This catalogue covers **223 C function
definitions** across selected Core source/headers: public/private functions,
inline helpers, generated hooks and retained utilities. Assembly entry points
are documented in [platform support](platform.md#reset-assembly-and-linker-layout).

## Find a function

| Function | Module | Definition |
| --- | --- | --- |
| [_close](platform.md#fn--close) | Minimal libc environment | [syscalls.c:89](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L89) |
| [_execve](platform.md#fn--execve) | Minimal libc environment | [syscalls.c:152](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L152) |
| [_exit](platform.md#fn--exit) | Minimal libc environment | [syscalls.c:60](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L60) |
| [_fork](platform.md#fn--fork) | Minimal libc environment | [syscalls.c:146](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L146) |
| [_fstat](platform.md#fn--fstat) | Minimal libc environment | [syscalls.c:95](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L95) |
| [_getpid](platform.md#fn--getpid) | Minimal libc environment | [syscalls.c:49](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L49) |
| [_isatty](platform.md#fn--isatty) | Minimal libc environment | [syscalls.c:101](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L101) |
| [_kill](platform.md#fn--kill) | Minimal libc environment | [syscalls.c:54](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L54) |
| [_link](platform.md#fn--link) | Minimal libc environment | [syscalls.c:140](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L140) |
| [_lseek](platform.md#fn--lseek) | Minimal libc environment | [syscalls.c:106](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L106) |
| [_open](platform.md#fn--open) | Minimal libc environment | [syscalls.c:111](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L111) |
| [_read](platform.md#fn--read) | Minimal libc environment | [syscalls.c:66](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L66) |
| [_sbrk](platform.md#fn--sbrk) | Libc heap support | [sysmem.c:54](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/sysmem.c#L54) |
| [_stat](platform.md#fn--stat) | Minimal libc environment | [syscalls.c:134](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L134) |
| [_times](platform.md#fn--times) | Minimal libc environment | [syscalls.c:129](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L129) |
| [_unlink](platform.md#fn--unlink) | Minimal libc environment | [syscalls.c:123](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L123) |
| [_wait](platform.md#fn--wait) | Minimal libc environment | [syscalls.c:117](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L117) |
| [_write](platform.md#fn--write) | Minimal libc environment | [syscalls.c:78](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L78) |
| [ADC1_2_IRQHandler](platform.md#fn-adc1-2-irqhandler) | Interrupt adapters | [stm32g4xx_it.c:270](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L270) |
| [API_CORDIC_Processor_Init](motor.md#fn-api-cordic-processor-init) | Inline hardware trigonometry | [cordic.h:47](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/cordic.h#L47) |
| [API_CORDIC_Processor_Update](motor.md#fn-api-cordic-processor-update) | Inline hardware trigonometry | [cordic.h:63](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/cordic.h#L63) |
| [API_FOC_Calibration_Begin](motor.md#fn-api-foc-calibration-begin) | Fast current-control engine | [foc.c:298](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L298) |
| [API_FOC_Calibration_Cancelled](motor.md#fn-api-foc-calibration-cancelled) | Fast current-control engine | [foc.c:328](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L328) |
| [API_FOC_Calibration_End](motor.md#fn-api-foc-calibration-end) | Fast current-control engine | [foc.c:330](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L330) |
| [API_FOC_Calibration_Idle](motor.md#fn-api-foc-calibration-idle) | Fast current-control engine | [foc.c:287](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L287) |
| [API_FOC_Calibration_SetFlux](motor.md#fn-api-foc-calibration-setflux) | Fast current-control engine | [foc.c:313](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L313) |
| [API_FOC_Check_Watchdog](motor.md#fn-api-foc-check-watchdog) | Fast current-control engine | [foc.c:154](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L154) |
| [API_FOC_Get_Present_Flux_Current](motor.md#fn-api-foc-get-present-flux-current) | Fast current-control engine | [foc.c:532](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L532) |
| [API_FOC_Get_Present_Temp](motor.md#fn-api-foc-get-present-temp) | Fast current-control engine | [foc.c:542](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L542) |
| [API_FOC_Get_Present_Torque_Current](motor.md#fn-api-foc-get-present-torque-current) | Fast current-control engine | [foc.c:527](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L527) |
| [API_FOC_Get_Present_Voltage](motor.md#fn-api-foc-get-present-voltage) | Fast current-control engine | [foc.c:537](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L537) |
| [API_FOC_Get_Processing_Frequency](motor.md#fn-api-foc-get-processing-frequency) | Fast current-control engine | [foc.c:552](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L552) |
| [API_FOC_Get_Processing_Time](motor.md#fn-api-foc-get-processing-time) | Fast current-control engine | [foc.c:547](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L547) |
| [API_FOC_Get_Timestamp_ms](motor.md#fn-api-foc-get-timestamp-ms) | Fast current-control engine | [foc.c:517](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L517) |
| [API_FOC_Get_Timestamp_us](motor.md#fn-api-foc-get-timestamp-us) | Fast current-control engine | [foc.c:522](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L522) |
| [API_FOC_Init](motor.md#fn-api-foc-init) | Fast current-control engine | [foc.c:185](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L185) |
| [API_FOC_Is_Calibrating](motor.md#fn-api-foc-is-calibrating) | Fast current-control engine | [foc.c:149](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L149) |
| [API_FOC_It](motor.md#fn-api-foc-it) | Fast current-control engine | [foc.c:578](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L578) |
| [API_FOC_Service_Update](motor.md#fn-api-foc-service-update) | Fast current-control engine | [foc.c:339](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L339) |
| [API_FOC_Set_Torque_Flux_Currents_mA](motor.md#fn-api-foc-set-torque-flux-currents-ma) | Fast current-control engine | [foc.c:169](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L169) |
| [API_FOC_Torque_Disable](motor.md#fn-api-foc-torque-disable) | Fast current-control engine | [foc.c:136](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L136) |
| [API_FOC_Torque_Enable](motor.md#fn-api-foc-torque-enable) | Fast current-control engine | [foc.c:114](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L114) |
| [API_FOC_Torque_Update](motor.md#fn-api-foc-torque-update) | Fast current-control engine | [foc.c:351](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L351) |
| [App_Init](application.md#fn-app-init) | Foreground application | [app.c:25](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/app.c#L25) |
| [App_Process](application.md#fn-app-process) | Foreground application | [app.c:108](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/app.c#L108) |
| [AS5600_CheckMagnet](motor.md#fn-as5600-checkmagnet) | AS5600 register driver | [as5600.c:266](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L266) |
| [AS5600_GetAGCSetting](motor.md#fn-as5600-getagcsetting) | AS5600 register driver | [as5600.c:319](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L319) |
| [AS5600_GetAngle](motor.md#fn-as5600-getangle) | AS5600 register driver | [as5600.c:292](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L292) |
| [AS5600_GetConfig](motor.md#fn-as5600-getconfig) | AS5600 register driver | [as5600.c:461](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L461) |
| [AS5600_GetCORDICMagnitude](motor.md#fn-as5600-getcordicmagnitude) | AS5600 register driver | [as5600.c:331](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L331) |
| [AS5600_GetMagnetStatus](motor.md#fn-as5600-getmagnetstatus) | AS5600 register driver | [as5600.c:307](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L307) |
| [AS5600_GetMaxAngle](motor.md#fn-as5600-getmaxangle) | AS5600 register driver | [as5600.c:364](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L364) |
| [AS5600_GetRawAngle](motor.md#fn-as5600-getrawangle) | AS5600 register driver | [as5600.c:277](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L277) |
| [AS5600_GetStartPosition](motor.md#fn-as5600-getstartposition) | AS5600 register driver | [as5600.c:433](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L433) |
| [AS5600_GetStopPosition](motor.md#fn-as5600-getstopposition) | AS5600 register driver | [as5600.c:399](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L399) |
| [AS5600_Init](motor.md#fn-as5600-init) | AS5600 register driver | [as5600.c:182](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L182) |
| [AS5600_SetConfig](motor.md#fn-as5600-setconfig) | AS5600 register driver | [as5600.c:449](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L449) |
| [AS5600_SetMaxAngle](motor.md#fn-as5600-setmaxangle) | AS5600 register driver | [as5600.c:347](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L347) |
| [AS5600_SetStartPosition](motor.md#fn-as5600-setstartposition) | AS5600 register driver | [as5600.c:416](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L416) |
| [AS5600_SetStopPosition](motor.md#fn-as5600-setstopposition) | AS5600 register driver | [as5600.c:382](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L382) |
| [assert_failed](platform.md#fn-assert-failed) | Hardware composition/startup | [main.c:953](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L953) |
| [BusFault_Handler](platform.md#fn-busfault-handler) | Interrupt adapters | [stm32g4xx_it.c:124](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L124) |
| [Calibration_Init](application.md#fn-calibration-init) | Foreground calibration job | [calibration.c:35](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/calibration.c#L35) |
| [Calibration_Start](application.md#fn-calibration-start) | Foreground calibration job | [calibration.c:45](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/calibration.c#L45) |
| [Calibration_TakeResult](application.md#fn-calibration-takeresult) | Foreground calibration job | [calibration.c:175](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/calibration.c#L175) |
| [Calibration_Update](application.md#fn-calibration-update) | Foreground calibration job | [calibration.c:83](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/calibration.c#L83) |
| [CanServo_Init](application.md#fn-canservo-init) | CAN transport | [can_servo.c:17](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/can_servo.c#L17) |
| [CanServo_OnRxFifo0](application.md#fn-canservo-onrxfifo0) | CAN transport | [can_servo.c:33](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/can_servo.c#L33) |
| [complete](application.md#fn-complete) | Foreground calibration job | [calibration.c:55](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/calibration.c#L55) |
| [Config_ApplyRegisterWrite](configuration.md#fn-config-applyregisterwrite) | Configuration service | [configuration.c:31](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/configuration.c#L31) |
| [Config_FactoryReset](configuration.md#fn-config-factoryreset) | Configuration service | [configuration.c:46](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/configuration.c#L46) |
| [Config_IsValid](configuration.md#fn-config-isvalid) | Pure configuration validation | [config_validation.c:21](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/config_validation.c#L21) |
| [Config_Load](configuration.md#fn-config-load) | Configuration service | [configuration.c:16](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/configuration.c#L16) |
| [Config_Save](configuration.md#fn-config-save) | Configuration service | [configuration.c:7](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/configuration.c#L7) |
| [Config_Validate](configuration.md#fn-config-validate) | Pure configuration validation | [config_validation.c:5](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/config_validation.c#L5) |
| [ConfigStorage_IsEmpty](configuration.md#fn-configstorage-isempty) | Fixed image adapter | [config_storage.c:8](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/config_storage.c#L8) |
| [ConfigStorage_Load](configuration.md#fn-configstorage-load) | Fixed image adapter | [config_storage.c:9](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/config_storage.c#L9) |
| [ConfigStorage_Save](configuration.md#fn-configstorage-save) | Fixed image adapter | [config_storage.c:13](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/config_storage.c#L13) |
| [constrain](motor.md#fn-constrain) | Numeric helpers | [math_tool.c:11](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/math_tool.c#L11) |
| [control_apply_config](configuration.md#fn-control-apply-config) | Register/image codec | [control_table.c:176](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/control_table.c#L176) |
| [control_apply_config_bytes](configuration.md#fn-control-apply-config-bytes) | Register/image codec | [control_table.c:184](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/control_table.c#L184) |
| [control_capture_config](configuration.md#fn-control-capture-config) | Register/image codec | [control_table.c:168](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/control_table.c#L168) |
| [control_mirror_command](configuration.md#fn-control-mirror-command) | Register/image codec | [control_table.c:46](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/control_table.c#L46) |
| [control_mirror_measurements](configuration.md#fn-control-mirror-measurements) | Register/image codec | [control_table.c:63](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/control_table.c#L63) |
| [control_read_config](configuration.md#fn-control-read-config) | Register/image codec | [control_table.c:21](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/control_table.c#L21) |
| [control_read_motor_alignment](configuration.md#fn-control-read-motor-alignment) | Motor configuration views | [motor_config.c:37](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/motor_config.c#L37) |
| [control_read_motor_config](configuration.md#fn-control-read-motor-config) | Motor configuration views | [motor_config.c:5](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/motor_config.c#L5) |
| [control_read_protection_config](configuration.md#fn-control-read-protection-config) | Motor configuration views | [motor_config.c:24](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/motor_config.c#L24) |
| [control_record_sensor_error](configuration.md#fn-control-record-sensor-error) | Motor configuration views | [motor_config.c:64](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/motor_config.c#L64) |
| [control_reset_config_defaults](configuration.md#fn-control-reset-config-defaults) | Register/image codec | [control_table.c:74](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/control_table.c#L74) |
| [control_reset_runtime](configuration.md#fn-control-reset-runtime) | Register/image codec | [control_table.c:189](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/control_table.c#L189) |
| [control_torque_requested](configuration.md#fn-control-torque-requested) | Motor configuration views | [motor_config.c:59](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/motor_config.c#L59) |
| [control_write_motor_alignment](configuration.md#fn-control-write-motor-alignment) | Motor configuration views | [motor_config.c:49](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/motor_config.c#L49) |
| [CORDIC_IRQHandler](platform.md#fn-cordic-irqhandler) | Interrupt adapters | [stm32g4xx_it.c:315](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L315) |
| [DebugMon_Handler](platform.md#fn-debugmon-handler) | Interrupt adapters | [stm32g4xx_it.c:167](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L167) |
| [difference_angle](motor.md#fn-difference-angle) | Numeric helpers | [math_tool.c:58](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/math_tool.c#L58) |
| [DMA1_Channel1_IRQHandler](platform.md#fn-dma1-channel1-irqhandler) | Interrupt adapters | [stm32g4xx_it.c:214](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L214) |
| [DMA1_Channel2_IRQHandler](platform.md#fn-dma1-channel2-irqhandler) | Interrupt adapters | [stm32g4xx_it.c:228](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L228) |
| [DMA1_Channel3_IRQHandler](platform.md#fn-dma1-channel3-irqhandler) | Interrupt adapters | [stm32g4xx_it.c:242](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L242) |
| [DMA1_Channel4_IRQHandler](platform.md#fn-dma1-channel4-irqhandler) | Interrupt adapters | [stm32g4xx_it.c:256](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L256) |
| [Drive_BeginMaintenance](application.md#fn-drive-beginmaintenance) | Drive supervisor | [drive.c:146](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L146) |
| [Drive_DisableIfDisarmed](application.md#fn-drive-disableifdisarmed) | Drive supervisor | [drive.c:126](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L126) |
| [Drive_Disarm](application.md#fn-drive-disarm) | Drive supervisor | [drive.c:43](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L43) |
| [Drive_EndMaintenance](application.md#fn-drive-endmaintenance) | Drive supervisor | [drive.c:160](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L160) |
| [Drive_GetCommand](application.md#fn-drive-getcommand) | Drive supervisor | [drive.c:97](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L97) |
| [Drive_GetStatus](application.md#fn-drive-getstatus) | Drive supervisor | [drive.c:185](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L185) |
| [Drive_Init](application.md#fn-drive-init) | Drive supervisor | [drive.c:19](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L19) |
| [Drive_IsDisarmed](application.md#fn-drive-isdisarmed) | Drive supervisor | [drive.c:25](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L25) |
| [Drive_IsMaintenance](application.md#fn-drive-ismaintenance) | Drive supervisor | [drive.c:34](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L34) |
| [Drive_PublishCurrents](application.md#fn-drive-publishcurrents) | Drive supervisor | [drive.c:110](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L110) |
| [Drive_PublishMeasurements](application.md#fn-drive-publishmeasurements) | Drive supervisor | [drive.c:176](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L176) |
| [Drive_RequestArm](application.md#fn-drive-requestarm) | Drive supervisor | [drive.c:54](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L54) |
| [Drive_Service](application.md#fn-drive-service) | Drive supervisor | [drive.c:134](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L134) |
| [Drive_StartCalibration](application.md#fn-drive-startcalibration) | Drive supervisor | [drive.c:168](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L168) |
| [Drive_SubmitCommand](application.md#fn-drive-submitcommand) | Drive supervisor | [drive.c:83](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c#L83) |
| [eeprom_base_address](configuration.md#fn-eeprom-base-address) | Flash backend | [eeprom.c:14](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/eeprom.c#L14) |
| [eeprom_empty](configuration.md#fn-eeprom-empty) | Flash backend | [eeprom.c:19](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/eeprom.c#L19) |
| [eeprom_restore](configuration.md#fn-eeprom-restore) | Flash backend | [eeprom.c:24](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/eeprom.c#L24) |
| [eeprom_store](configuration.md#fn-eeprom-store) | Flash backend | [eeprom.c:30](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/eeprom.c#L30) |
| [enter_phase](application.md#fn-enter-phase) | Foreground calibration job | [calibration.c:37](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/calibration.c#L37) |
| [Error_Handler](platform.md#fn-error-handler) | Hardware composition/startup | [main.c:934](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L934) |
| [excite](application.md#fn-excite) | Foreground calibration job | [calibration.c:74](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/calibration.c#L74) |
| [fconstrain](motor.md#fn-fconstrain) | Numeric helpers | [math_tool.c:18](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/math_tool.c#L18) |
| [fconstrain_both](motor.md#fn-fconstrain-both) | Numeric helpers | [math_tool.c:25](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/math_tool.c#L25) |
| [FDCAN1_IT0_IRQHandler](platform.md#fn-fdcan1-it0-irqhandler) | Interrupt adapters | [stm32g4xx_it.c:285](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L285) |
| [FDCAN_Config](platform.md#fn-fdcan-config) | Hardware composition/startup | [main.c:112](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L112) |
| [fmap](motor.md#fn-fmap) | Numeric helpers | [math_tool.c:45](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/math_tool.c#L45) |
| [FOC_Check_Current_Protection](motor.md#fn-foc-check-current-protection) | Fast current-control engine | [foc.c:558](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L558) |
| [HAL_ADC_ConvCpltCallback](motor.md#fn-hal-adc-convcpltcallback) | Fast current-control engine | [foc.c:52](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L52) |
| [HAL_ADC_ErrorCallback](motor.md#fn-hal-adc-errorcallback) | Fast current-control engine | [foc.c:57](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L57) |
| [HAL_ADC_MspDeInit](platform.md#fn-hal-adc-mspdeinit) | HAL low-level hooks | [stm32g4xx_hal_msp.c:223](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L223) |
| [HAL_ADC_MspInit](platform.md#fn-hal-adc-mspinit) | HAL low-level hooks | [stm32g4xx_hal_msp.c:101](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L101) |
| [HAL_CORDIC_MspDeInit](platform.md#fn-hal-cordic-mspdeinit) | HAL low-level hooks | [stm32g4xx_hal_msp.c:322](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L322) |
| [HAL_CORDIC_MspInit](platform.md#fn-hal-cordic-mspinit) | HAL low-level hooks | [stm32g4xx_hal_msp.c:297](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L297) |
| [HAL_FDCAN_MspDeInit](platform.md#fn-hal-fdcan-mspdeinit) | HAL low-level hooks | [stm32g4xx_hal_msp.c:405](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L405) |
| [HAL_FDCAN_MspInit](platform.md#fn-hal-fdcan-mspinit) | HAL low-level hooks | [stm32g4xx_hal_msp.c:347](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L347) |
| [HAL_FDCAN_RxFifo0Callback](platform.md#fn-hal-fdcan-rxfifo0callback) | Hardware composition/startup | [main.c:157](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L157) |
| [HAL_I2C_MspDeInit](platform.md#fn-hal-i2c-mspdeinit) | HAL low-level hooks | [stm32g4xx_hal_msp.c:488](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L488) |
| [HAL_I2C_MspInit](platform.md#fn-hal-i2c-mspinit) | HAL low-level hooks | [stm32g4xx_hal_msp.c:438](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L438) |
| [HAL_MspInit](platform.md#fn-hal-mspinit) | HAL low-level hooks | [stm32g4xx_hal_msp.c:73](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L73) |
| [HAL_OPAMP_MspDeInit](platform.md#fn-hal-opamp-mspdeinit) | HAL low-level hooks | [stm32g4xx_hal_msp.c:591](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L591) |
| [HAL_OPAMP_MspInit](platform.md#fn-hal-opamp-mspinit) | HAL low-level hooks | [stm32g4xx_hal_msp.c:519](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L519) |
| [HAL_Serial_Available](communications.md#fn-hal-serial-available) | UART DMA transport | [serial.c:108](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c#L108) |
| [HAL_Serial_GetChar](communications.md#fn-hal-serial-getchar) | UART DMA transport | [serial.c:120](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c#L120) |
| [HAL_Serial_Init](communications.md#fn-hal-serial-init) | UART DMA transport | [serial.c:40](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c#L40) |
| [HAL_Serial_Init_Half_Duplex](communications.md#fn-hal-serial-init-half-duplex) | UART DMA transport | [serial.c:68](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c#L68) |
| [HAL_Serial_Print](communications.md#fn-hal-serial-print) | UART DMA transport | [serial.c:224](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c#L224) |
| [HAL_Serial_Read](communications.md#fn-hal-serial-read) | UART DMA transport | [serial.c:137](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c#L137) |
| [HAL_Serial_Write](communications.md#fn-hal-serial-write) | UART DMA transport | [serial.c:188](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c#L188) |
| [HAL_TIM_Base_MspDeInit](platform.md#fn-hal-tim-base-mspdeinit) | HAL low-level hooks | [stm32g4xx_hal_msp.c:732](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L732) |
| [HAL_TIM_Base_MspInit](platform.md#fn-hal-tim-base-mspinit) | HAL low-level hooks | [stm32g4xx_hal_msp.c:650](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L650) |
| [HAL_TIM_MspPostInit](platform.md#fn-hal-tim-msppostinit) | HAL low-level hooks | [stm32g4xx_hal_msp.c:679](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L679) |
| [HAL_UART_ErrorCallback](communications.md#fn-hal-uart-errorcallback) | UART DMA transport | [serial.c:21](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c#L21) |
| [HAL_UART_MspDeInit](platform.md#fn-hal-uart-mspdeinit) | HAL low-level hooks | [stm32g4xx_hal_msp.c:852](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L852) |
| [HAL_UART_MspInit](platform.md#fn-hal-uart-mspinit) | HAL low-level hooks | [stm32g4xx_hal_msp.c:767](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L767) |
| [HAL_UART_TxCpltCallback](communications.md#fn-hal-uart-txcpltcallback) | UART DMA transport | [serial.c:208](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c#L208) |
| [HardFault_Handler](platform.md#fn-hardfault-handler) | Interrupt adapters | [stm32g4xx_it.c:93](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L93) |
| [hardware_fault_clear](configuration.md#fn-hardware-fault-clear) | Fault-bit adapter | [drive_faults.c:18](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive_faults.c#L18) |
| [hardware_fault_set](configuration.md#fn-hardware-fault-set) | Fault-bit adapter | [drive_faults.c:10](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive_faults.c#L10) |
| [hardware_fault_status](configuration.md#fn-hardware-fault-status) | Fault-bit adapter | [drive_faults.c:5](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive_faults.c#L5) |
| [initialise_monitor_handles](platform.md#fn-initialise-monitor-handles) | Minimal libc environment | [syscalls.c:45](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L45) |
| [instruction_handler](communications.md#fn-instruction-handler) | Serial packet parser | [protocol.c:140](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/protocol.c#L140) |
| [LL_FOC_brake](motor.md#fn-ll-foc-brake) | Inline PWM/coast helpers | [foc_utils.h:103](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/foc_utils.h#L103) |
| [LL_FOC_coast](motor.md#fn-ll-foc-coast) | Inline PWM/coast helpers | [foc_utils.h:33](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/foc_utils.h#L33) |
| [LL_FOC_set_phase_voltage](motor.md#fn-ll-foc-set-phase-voltage) | Inline PWM/coast helpers | [foc_utils.h:40](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/foc_utils.h#L40) |
| [LL_FOC_Update_Temperature](motor.md#fn-ll-foc-update-temperature) | Fast current-control engine | [foc.c:216](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L216) |
| [LL_FOC_Update_Voltage](motor.md#fn-ll-foc-update-voltage) | Fast current-control engine | [foc.c:259](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c#L259) |
| [main](platform.md#fn-main) | Hardware composition/startup | [main.c:168](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L168) |
| [map](motor.md#fn-map) | Numeric helpers | [math_tool.c:38](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/math_tool.c#L38) |
| [MemManage_Handler](platform.md#fn-memmanage-handler) | Interrupt adapters | [stm32g4xx_it.c:109](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L109) |
| [mfmod](motor.md#fn-mfmod) | Numeric helpers | [math_tool.c:32](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/math_tool.c#L32) |
| [MX_ADC1_Init](platform.md#fn-mx-adc1-init) | Hardware composition/startup | [main.c:290](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L290) |
| [MX_ADC2_Init](platform.md#fn-mx-adc2-init) | Hardware composition/startup | [main.c:393](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L393) |
| [MX_CORDIC_Init](platform.md#fn-mx-cordic-init) | Hardware composition/startup | [main.c:469](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L469) |
| [MX_DMA_Init](platform.md#fn-mx-dma-init) | Hardware composition/startup | [main.c:866](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L866) |
| [MX_FDCAN1_Init](platform.md#fn-mx-fdcan1-init) | Hardware composition/startup | [main.c:495](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L495) |
| [MX_GPIO_Init](platform.md#fn-mx-gpio-init) | Hardware composition/startup | [main.c:894](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L894) |
| [MX_I2C1_Init](platform.md#fn-mx-i2c1-init) | Hardware composition/startup | [main.c:538](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L538) |
| [MX_OPAMP1_Init](platform.md#fn-mx-opamp1-init) | Hardware composition/startup | [main.c:590](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L590) |
| [MX_OPAMP2_Init](platform.md#fn-mx-opamp2-init) | Hardware composition/startup | [main.c:624](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L624) |
| [MX_OPAMP3_Init](platform.md#fn-mx-opamp3-init) | Hardware composition/startup | [main.c:658](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L658) |
| [MX_TIM1_Init](platform.md#fn-mx-tim1-init) | Hardware composition/startup | [main.c:692](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L692) |
| [MX_TIM6_Init](platform.md#fn-mx-tim6-init) | Hardware composition/startup | [main.c:782](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L782) |
| [MX_USART2_UART_Init](platform.md#fn-mx-usart2-uart-init) | Hardware composition/startup | [main.c:820](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L820) |
| [NMI_Handler](platform.md#fn-nmi-handler) | Interrupt adapters | [stm32g4xx_it.c:78](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L78) |
| [normalize_angle](motor.md#fn-normalize-angle) | Numeric helpers | [math_tool.c:52](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/math_tool.c#L52) |
| [packet_encapsulate](communications.md#fn-packet-encapsulate) | Serial packet parser | [protocol.c:92](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/protocol.c#L92) |
| [packet_handler](communications.md#fn-packet-handler) | Serial packet parser | [protocol.c:258](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/protocol.c#L258) |
| [PendSV_Handler](platform.md#fn-pendsv-handler) | Interrupt adapters | [stm32g4xx_it.c:180](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L180) |
| [pi_process_antiwindup_clamp](motor.md#fn-pi-process-antiwindup-clamp) | PI/PID helpers | [pic.c:20](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/pic.c#L20) |
| [pid_process_antiwindup_back_calculation](motor.md#fn-pid-process-antiwindup-back-calculation) | PI/PID helpers | [pic.c:116](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/pic.c#L116) |
| [pid_process_antiwindup_clamp](motor.md#fn-pid-process-antiwindup-clamp) | PI/PID helpers | [pic.c:82](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/pic.c#L82) |
| [pid_process_antiwindup_clamp_with_ff](motor.md#fn-pid-process-antiwindup-clamp-with-ff) | PI/PID helpers | [pic.c:47](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/pic.c#L47) |
| [pid_reset](motor.md#fn-pid-reset) | PI/PID helpers | [pic.c:13](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/pic.c#L13) |
| [positionSensor_getDegreesMultiturn](motor.md#fn-positionsensor-getdegreesmultiturn) | Motor sample owner | [position_sensor.c:152](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/position_sensor.c#L152) |
| [positionSensor_getRadiansEstimation](motor.md#fn-positionsensor-getradiansestimation) | Motor sample owner | [position_sensor.c:74](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/position_sensor.c#L74) |
| [positionSensor_getRadiansMultiturn](motor.md#fn-positionsensor-getradiansmultiturn) | Motor sample owner | [position_sensor.c:147](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/position_sensor.c#L147) |
| [positionSensor_getVelocityDps](motor.md#fn-positionsensor-getvelocitydps) | Motor sample owner | [position_sensor.c:157](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/position_sensor.c#L157) |
| [positionSensor_init](motor.md#fn-positionsensor-init) | Motor sample owner | [position_sensor.c:51](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/position_sensor.c#L51) |
| [positionSensor_isReady](motor.md#fn-positionsensor-isready) | Motor sample owner | [position_sensor.c:46](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/position_sensor.c#L46) |
| [positionSensor_update](motor.md#fn-positionsensor-update) | Motor sample owner | [position_sensor.c:90](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/position_sensor.c#L90) |
| [protocol_write_registers](communications.md#fn-protocol-write-registers) | Serial packet parser | [protocol.c:127](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/protocol.c#L127) |
| [Q31ToFloat](motor.md#fn-q31tofloat) | Inline hardware trigonometry | [cordic.h:42](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/cordic.h#L42) |
| [RadiansToQ31](motor.md#fn-radianstoq31) | Inline hardware trigonometry | [cordic.h:27](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/cordic.h#L27) |
| [Serial_Tx_Direction](communications.md#fn-serial-tx-direction) | UART DMA transport | [serial.c:155](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c#L155) |
| [Serial_Tx_Fail](communications.md#fn-serial-tx-fail) | UART DMA transport | [serial.c:166](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c#L166) |
| [Serial_Tx_Start](communications.md#fn-serial-tx-start) | UART DMA transport | [serial.c:174](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c#L174) |
| [service](application.md#fn-service) | Foreground application | [app.c:81](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/app.c#L81) |
| [Servo_Emergency_Stop](platform.md#fn-servo-emergency-stop) | Fatal output primitive | [emergency_stop.h:7](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/emergency_stop.h#L7) |
| [ServoController_Init](application.md#fn-servocontroller-init) | Outer servo controller | [servo_controller.c:18](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/servo_controller.c#L18) |
| [ServoController_Update](application.md#fn-servocontroller-update) | Outer servo controller | [servo_controller.c:24](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/servo_controller.c#L24) |
| [SVC_Handler](platform.md#fn-svc-handler) | Interrupt adapters | [stm32g4xx_it.c:154](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L154) |
| [SystemClock_Config](platform.md#fn-systemclock-config) | Hardware composition/startup | [main.c:244](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L244) |
| [SystemCoreClockUpdate](platform.md#fn-systemcoreclockupdate) | CMSIS system support | [system_stm32g4xx.c:229](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/system_stm32g4xx.c#L229) |
| [SystemInit](platform.md#fn-systeminit) | CMSIS system support | [system_stm32g4xx.c:180](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/system_stm32g4xx.c#L180) |
| [SysTick_Handler](platform.md#fn-systick-handler) | Interrupt adapters | [stm32g4xx_it.c:193](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L193) |
| [telemetry_i16](communications.md#fn-telemetry-i16) | Inline wire conversions | [telemetry.h:9](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/telemetry.h#L9) |
| [telemetry_store_i16](communications.md#fn-telemetry-store-i16) | Inline wire conversions | [telemetry.h:24](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/telemetry.h#L24) |
| [telemetry_u8](communications.md#fn-telemetry-u8) | Inline wire conversions | [telemetry.h:17](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/telemetry.h#L17) |
| [update_servo](application.md#fn-update-servo) | Foreground application | [app.c:38](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/app.c#L38) |
| [updateCRC](communications.md#fn-updatecrc) | Serial packet parser | [protocol.c:19](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/protocol.c#L19) |
| [UsageFault_Handler](platform.md#fn-usagefault-handler) | Interrupt adapters | [stm32g4xx_it.c:139](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L139) |
| [USART2_IRQHandler](platform.md#fn-usart2-irqhandler) | Interrupt adapters | [stm32g4xx_it.c:301](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L301) |

## Find a source or header file

Every selected Core C/header file is mapped below. Empty compatibility C files
can have implementations in inline headers.

| File | Module guide |
| --- | --- |
| [Inc/app.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/app.h) | [Foreground application](application.md#module-app) |
| [Inc/as5600.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/as5600.h) | [AS5600 register driver](motor.md#module-as5600) |
| [Inc/binary_tool.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/binary_tool.h) | [Byte packing macros](configuration.md#module-binary-tool) |
| [Inc/calibration.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/calibration.h) | [Foreground calibration job](application.md#module-calibration) |
| [Inc/can_servo.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/can_servo.h) | [CAN transport](application.md#module-can-servo) |
| [Inc/config_storage.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/config_storage.h) | [Fixed image adapter](configuration.md#module-config-storage) |
| [Inc/configuration.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/configuration.h) | [Configuration service](configuration.md#module-configuration) |
| [Inc/control_table.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/control_table.h) | [Register/image codec](configuration.md#module-control-table) |
| [Inc/cordic.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/cordic.h) | [Inline hardware trigonometry](motor.md#module-cordic) |
| [Inc/drive.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/drive.h) | [Drive supervisor](application.md#module-drive) |
| [Inc/drive_faults.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/drive_faults.h) | [Fault-bit adapter](configuration.md#module-drive-faults) |
| [Inc/drive_types.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/drive_types.h) | [Application data vocabulary](application.md#module-drive-types) |
| [Inc/eeprom.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/eeprom.h) | [Flash backend](configuration.md#module-eeprom) |
| [Inc/emergency_stop.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/emergency_stop.h) | [Fatal output primitive](platform.md#module-emergency-stop) |
| [Inc/foc.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/foc.h) | [Fast current-control engine](motor.md#module-foc) |
| [Inc/foc_utils.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/foc_utils.h) | [Inline PWM/coast helpers](motor.md#module-foc-utils) |
| [Inc/main.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/main.h) | [Hardware composition/startup](platform.md#module-main) |
| [Inc/math_tool.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/math_tool.h) | [Numeric helpers](motor.md#module-math-tool) |
| [Inc/motor_config.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/motor_config.h) | [Motor configuration views](configuration.md#module-motor-config) |
| [Inc/pid.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/pid.h) | [PI/PID helpers](motor.md#module-pid) |
| [Inc/position_sensor.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/position_sensor.h) | [Motor sample owner](motor.md#module-position-sensor) |
| [Inc/protocol.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/protocol.h) | [Serial packet parser](communications.md#module-protocol) |
| [Inc/serial.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/serial.h) | [UART DMA transport](communications.md#module-serial) |
| [Inc/servo_controller.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/servo_controller.h) | [Outer servo controller](application.md#module-servo-controller) |
| [Inc/stm32g4xx_hal_conf.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/stm32g4xx_hal_conf.h) | [HAL build-time options](platform.md#module-stm32g4xx-hal-conf) |
| [Inc/stm32g4xx_it.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/stm32g4xx_it.h) | [Interrupt adapters](platform.md#module-stm32g4xx-it) |
| [Inc/telemetry.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/telemetry.h) | [Inline wire conversions](communications.md#module-telemetry) |
| [Src/app.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/app.c) | [Foreground application](application.md#module-app) |
| [Src/as5600.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c) | [AS5600 register driver](motor.md#module-as5600) |
| [Src/calibration.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/calibration.c) | [Foreground calibration job](application.md#module-calibration) |
| [Src/can_servo.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/can_servo.c) | [CAN transport](application.md#module-can-servo) |
| [Src/config_storage.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/config_storage.c) | [Fixed image adapter](configuration.md#module-config-storage) |
| [Src/config_validation.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/config_validation.c) | [Pure configuration validation](configuration.md#module-config-validation) |
| [Src/configuration.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/configuration.c) | [Configuration service](configuration.md#module-configuration) |
| [Src/control_table.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/control_table.c) | [Register/image codec](configuration.md#module-control-table) |
| [Src/cordic.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/cordic.c) | [Inline hardware trigonometry](motor.md#module-cordic) |
| [Src/drive.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive.c) | [Drive supervisor](application.md#module-drive) |
| [Src/drive_faults.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/drive_faults.c) | [Fault-bit adapter](configuration.md#module-drive-faults) |
| [Src/eeprom.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/eeprom.c) | [Flash backend](configuration.md#module-eeprom) |
| [Src/foc.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc.c) | [Fast current-control engine](motor.md#module-foc) |
| [Src/foc_utils.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/foc_utils.c) | [Inline PWM/coast helpers](motor.md#module-foc-utils) |
| [Src/main.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c) | [Hardware composition/startup](platform.md#module-main) |
| [Src/math_tool.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/math_tool.c) | [Numeric helpers](motor.md#module-math-tool) |
| [Src/motor_config.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/motor_config.c) | [Motor configuration views](configuration.md#module-motor-config) |
| [Src/pic.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/pic.c) | [PI/PID helpers](motor.md#module-pid) |
| [Src/position_sensor.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/position_sensor.c) | [Motor sample owner](motor.md#module-position-sensor) |
| [Src/protocol.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/protocol.c) | [Serial packet parser](communications.md#module-protocol) |
| [Src/serial.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c) | [UART DMA transport](communications.md#module-serial) |
| [Src/servo_controller.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/servo_controller.c) | [Outer servo controller](application.md#module-servo-controller) |
| [Src/stm32g4xx_hal_msp.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c) | [HAL low-level hooks](platform.md#module-stm32g4xx-hal-msp) |
| [Src/stm32g4xx_it.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c) | [Interrupt adapters](platform.md#module-stm32g4xx-it) |
| [Src/syscalls.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c) | [Minimal libc environment](platform.md#module-syscalls) |
| [Src/sysmem.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/sysmem.c) | [Libc heap support](platform.md#module-sysmem) |
| [Src/system_stm32g4xx.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/system_stm32g4xx.c) | [CMSIS system support](platform.md#module-system-stm32g4xx) |
