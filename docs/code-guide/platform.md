# Startup, generated hardware setup and interrupts

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

- [Hardware composition/startup](#module-main)
- [Fatal output primitive](#module-emergency-stop)
- [HAL low-level hooks](#module-stm32g4xx-hal-msp)
- [Interrupt adapters](#module-stm32g4xx-it)
- [HAL build-time options](#module-stm32g4xx-hal-conf)
- [CMSIS system support](#module-system-stm32g4xx)
- [Minimal libc environment](#module-syscalls)
- [Libc heap support](#module-sysmem)

<a id="module-main"></a>

## Hardware composition/startup

**Files:** [Inc/main.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/main.h), [Src/main.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c).

Owns peripheral handles, checked startup, CAN hardware setup and App_Process call site. Most MX functions configure hardware; CAN callback only forwards. main.h contains board-pin definitions.

**Verification entry points:** startup, can, fatal. See [test instructions](../../tests/README.md).

<a id="fn-fdcan-config"></a>

### `FDCAN_Config`

`static void FDCAN_Config(void);`

[Source: main.c:112](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L112) · Private/static.

Startup filter/transceiver/termination setup, checked CAN start and RX notifications. CanServo binding precedes it; no parsing/ARM policy here.

- **Direct Core calls:** [Error_Handler](platform.md#fn-error-handler).
- **Other direct calls / macros:** `HAL_FDCAN_ActivateNotification`, `HAL_FDCAN_ConfigFilter`, `HAL_FDCAN_ConfigGlobalFilter`, `HAL_FDCAN_Start`, `HAL_GPIO_WritePin`.
- **Direct Core callers:** [main](platform.md#fn-main).

<a id="fn-hal-fdcan-rxfifo0callback"></a>

### `HAL_FDCAN_RxFifo0Callback`

`void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs);`

[Source: main.c:157](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L157) · External linkage; HAL hook.

HAL RX hook forwarding handle/events to CanServo; no policy in this adapter.

- **Direct Core calls:** [CanServo_OnRxFifo0](application.md#fn-canservo-onrxfifo0).
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-main"></a>

### `main`

`int main(void);`

[Source: main.c:168](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L168) · External linkage.

Configure MCU/peripherals, timer, config/runtime/Drive, UART/encoder and FOC dependencies; bind/start CAN and initialize App, then App_Process forever. Checked startup failures are fatal.

- **Direct Core calls:** [API_FOC_Init](motor.md#fn-api-foc-init), [API_FOC_Service_Update](motor.md#fn-api-foc-service-update), [App_Init](application.md#fn-app-init), [App_Process](application.md#fn-app-process), [CanServo_Init](application.md#fn-canservo-init), [Config_IsValid](configuration.md#fn-config-isvalid), [Config_Load](configuration.md#fn-config-load), [Drive_Init](application.md#fn-drive-init), [Error_Handler](platform.md#fn-error-handler), [FDCAN_Config](platform.md#fn-fdcan-config), [HAL_Serial_Init](communications.md#fn-hal-serial-init), [MX_ADC1_Init](platform.md#fn-mx-adc1-init), [MX_ADC2_Init](platform.md#fn-mx-adc2-init), [MX_CORDIC_Init](platform.md#fn-mx-cordic-init), [MX_DMA_Init](platform.md#fn-mx-dma-init), [MX_FDCAN1_Init](platform.md#fn-mx-fdcan1-init), [MX_GPIO_Init](platform.md#fn-mx-gpio-init), [MX_I2C1_Init](platform.md#fn-mx-i2c1-init), [MX_OPAMP1_Init](platform.md#fn-mx-opamp1-init), [MX_OPAMP2_Init](platform.md#fn-mx-opamp2-init), [MX_OPAMP3_Init](platform.md#fn-mx-opamp3-init), [MX_TIM1_Init](platform.md#fn-mx-tim1-init), [MX_TIM6_Init](platform.md#fn-mx-tim6-init), [MX_USART2_UART_Init](platform.md#fn-mx-usart2-uart-init), [SystemClock_Config](platform.md#fn-systemclock-config), [control_reset_runtime](configuration.md#fn-control-reset-runtime), [hardware_fault_set](configuration.md#fn-hardware-fault-set), [positionSensor_init](motor.md#fn-positionsensor-init), [positionSensor_update](motor.md#fn-positionsensor-update).
- **Other direct calls / macros:** `HAL_Init`, `HAL_TIM_Base_Start`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-systemclock-config"></a>

### `SystemClock_Config`

`void SystemClock_Config(void);`

[Source: main.c:244](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L244) · External linkage.

Configure HSE/PLL, regulator and CPU/AHB/APB tree for selected 160 MHz setup. HAL oscillator/clock failures are fatal; distinct from early SystemInit.

- **Direct Core calls:** [Error_Handler](platform.md#fn-error-handler).
- **Other direct calls / macros:** `HAL_PWREx_ControlVoltageScaling`, `HAL_RCC_ClockConfig`, `HAL_RCC_OscConfig`.
- **Direct Core callers:** [main](platform.md#fn-main).

<a id="fn-mx-adc1-init"></a>

### `MX_ADC1_Init`

`static void MX_ADC1_Init(void);`

[Source: main.c:290](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L290) · Private/static.

Generated ADC1 scan/trigger/channel configuration for phase-current/auxiliary inputs. Does not itself start acquisition or run FOC.

- **Direct Core calls:** [Error_Handler](platform.md#fn-error-handler).
- **Other direct calls / macros:** `HAL_ADCEx_MultiModeConfigChannel`, `HAL_ADC_ConfigChannel`, `HAL_ADC_Init`.
- **Direct Core callers:** [main](platform.md#fn-main).

<a id="fn-mx-adc2-init"></a>

### `MX_ADC2_Init`

`static void MX_ADC2_Init(void);`

[Source: main.c:393](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L393) · Private/static.

Generated ADC2 scan/trigger setup for other phase currents; FOC initialization later starts DMA.

- **Direct Core calls:** [Error_Handler](platform.md#fn-error-handler).
- **Other direct calls / macros:** `HAL_ADC_ConfigChannel`, `HAL_ADC_Init`.
- **Direct Core callers:** [main](platform.md#fn-main).

<a id="fn-mx-cordic-init"></a>

### `MX_CORDIC_Init`

`static void MX_CORDIC_Init(void);`

[Source: main.c:469](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L469) · Private/static.

Initialize HAL accelerator handle/peripheral; processor initialization later selects cosine/sine parameters.

- **Direct Core calls:** [Error_Handler](platform.md#fn-error-handler).
- **Other direct calls / macros:** `HAL_CORDIC_Init`.
- **Direct Core callers:** [main](platform.md#fn-main).

<a id="fn-mx-fdcan1-init"></a>

### `MX_FDCAN1_Init`

`static void MX_FDCAN1_Init(void);`

[Source: main.c:495](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L495) · Private/static.

Generated CAN timing/FIFO/handle setup. Filters, transceiver pins, start and notifications happen later.

- **Direct Core calls:** [Error_Handler](platform.md#fn-error-handler).
- **Other direct calls / macros:** `HAL_FDCAN_Init`.
- **Direct Core callers:** [main](platform.md#fn-main).

<a id="fn-mx-i2c1-init"></a>

### `MX_I2C1_Init`

`static void MX_I2C1_Init(void);`

[Source: main.c:538](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L538) · Private/static.

Configure I2C1 timing/filters/fast-mode-plus for the encoder bus; no encoder read/calibration.

- **Direct Core calls:** [Error_Handler](platform.md#fn-error-handler).
- **Other direct calls / macros:** `HAL_I2CEx_ConfigAnalogFilter`, `HAL_I2CEx_ConfigDigitalFilter`, `HAL_I2C_Init`, `__HAL_SYSCFG_FASTMODEPLUS_ENABLE`.
- **Direct Core callers:** [main](platform.md#fn-main).

<a id="fn-mx-opamp1-init"></a>

### `MX_OPAMP1_Init`

`static void MX_OPAMP1_Init(void);`

[Source: main.c:590](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L590) · Private/static.

Configure first current-sense op-amp; FOC initialization later starts it. Failure is fatal.

- **Direct Core calls:** [Error_Handler](platform.md#fn-error-handler).
- **Other direct calls / macros:** `HAL_OPAMP_Init`.
- **Direct Core callers:** [main](platform.md#fn-main).

<a id="fn-mx-opamp2-init"></a>

### `MX_OPAMP2_Init`

`static void MX_OPAMP2_Init(void);`

[Source: main.c:624](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L624) · Private/static.

Configure second current-sense op-amp; FOC initialization later starts it. Failure is fatal.

- **Direct Core calls:** [Error_Handler](platform.md#fn-error-handler).
- **Other direct calls / macros:** `HAL_OPAMP_Init`.
- **Direct Core callers:** [main](platform.md#fn-main).

<a id="fn-mx-opamp3-init"></a>

### `MX_OPAMP3_Init`

`static void MX_OPAMP3_Init(void);`

[Source: main.c:658](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L658) · Private/static.

Configure third current-sense op-amp; FOC initialization later starts it. Failure is fatal.

- **Direct Core calls:** [Error_Handler](platform.md#fn-error-handler).
- **Other direct calls / macros:** `HAL_OPAMP_Init`.
- **Direct Core callers:** [main](platform.md#fn-main).

<a id="fn-mx-tim1-init"></a>

### `MX_TIM1_Init`

`static void MX_TIM1_Init(void);`

[Source: main.c:692](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L692) · Private/static.

Configure center-aligned PWM, preloads, idle levels, ADC trigger, break/dead-time and pins. Prescaler 0/period 1999 configure 40 kHz; not an ARM operation.

- **Direct Core calls:** [Error_Handler](platform.md#fn-error-handler), [HAL_TIM_MspPostInit](platform.md#fn-hal-tim-msppostinit).
- **Other direct calls / macros:** `HAL_TIMEx_ConfigBreakDeadTime`, `HAL_TIMEx_MasterConfigSynchronization`, `HAL_TIM_Base_Init`, `HAL_TIM_ConfigClockSource`, `HAL_TIM_PWM_ConfigChannel`, `HAL_TIM_PWM_Init`.
- **Direct Core callers:** [main](platform.md#fn-main).

<a id="fn-mx-tim6-init"></a>

### `MX_TIM6_Init`

`static void MX_TIM6_Init(void);`

[Source: main.c:782](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L782) · Private/static.

Configure free-running 16-bit 1 MHz timer (prescaler 159/period 65535 at selected clock). Main starts it before config/encoder use.

- **Direct Core calls:** [Error_Handler](platform.md#fn-error-handler).
- **Other direct calls / macros:** `HAL_TIMEx_MasterConfigSynchronization`, `HAL_TIM_Base_Init`.
- **Direct Core callers:** [main](platform.md#fn-main).

<a id="fn-mx-usart2-uart-init"></a>

### `MX_USART2_UART_Init`

`static void MX_USART2_UART_Init(void);`

[Source: main.c:820](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L820) · Private/static.

Configure USART2 at 1 Mbit/s and HAL FIFO settings. HAL_Serial_Init separately binds transport/starts RX DMA.

- **Direct Core calls:** [Error_Handler](platform.md#fn-error-handler).
- **Other direct calls / macros:** `HAL_UARTEx_DisableFifoMode`, `HAL_UARTEx_SetRxFifoThreshold`, `HAL_UARTEx_SetTxFifoThreshold`, `HAL_UART_Init`.
- **Direct Core callers:** [main](platform.md#fn-main).

<a id="fn-mx-dma-init"></a>

### `MX_DMA_Init`

`static void MX_DMA_Init(void);`

[Source: main.c:866](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L866) · Private/static.

Enable DMA/DMAMUX clocks and DMA1 channel IRQs/priorities. Peripheral MSP hooks configure/link DMA handles.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_NVIC_EnableIRQ`, `HAL_NVIC_SetPriority`, `__HAL_RCC_DMA1_CLK_ENABLE`, `__HAL_RCC_DMAMUX1_CLK_ENABLE`.
- **Direct Core callers:** [main](platform.md#fn-main).

<a id="fn-mx-gpio-init"></a>

### `MX_GPIO_Init`

`static void MX_GPIO_Init(void);`

[Source: main.c:894](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L894) · Private/static.

Enable GPIO clocks and initial modes/levels for board/button/status/CAN pins. App and FDCAN_Config later drive behavior.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_GPIO_Init`, `HAL_GPIO_WritePin`, `__HAL_RCC_GPIOA_CLK_ENABLE`, `__HAL_RCC_GPIOB_CLK_ENABLE`, `__HAL_RCC_GPIOC_CLK_ENABLE`, `__HAL_RCC_GPIOF_CLK_ENABLE`.
- **Direct Core callers:** [main](platform.md#fn-main).

<a id="fn-error-handler"></a>

### `Error_Handler`

`void Error_Handler(void);`

[Source: main.c:934](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L934) · External linkage; vector/fatal entry.

Use handle-independent emergency output disable, then halt forever. No timer, initialized handle or scheduler required.

- **Direct Core calls:** [Servo_Emergency_Stop](platform.md#fn-servo-emergency-stop).
- **Other direct calls / macros:** None.
- **Direct Core callers:** [API_FOC_Init](motor.md#fn-api-foc-init), [FDCAN_Config](platform.md#fn-fdcan-config), [HAL_ADC_MspInit](platform.md#fn-hal-adc-mspinit), [HAL_FDCAN_MspInit](platform.md#fn-hal-fdcan-mspinit), [HAL_I2C_MspInit](platform.md#fn-hal-i2c-mspinit), [HAL_UART_MspInit](platform.md#fn-hal-uart-mspinit), [MX_ADC1_Init](platform.md#fn-mx-adc1-init), [MX_ADC2_Init](platform.md#fn-mx-adc2-init), [MX_CORDIC_Init](platform.md#fn-mx-cordic-init), [MX_FDCAN1_Init](platform.md#fn-mx-fdcan1-init), [MX_I2C1_Init](platform.md#fn-mx-i2c1-init), [MX_OPAMP1_Init](platform.md#fn-mx-opamp1-init), [MX_OPAMP2_Init](platform.md#fn-mx-opamp2-init), [MX_OPAMP3_Init](platform.md#fn-mx-opamp3-init), [MX_TIM1_Init](platform.md#fn-mx-tim1-init), [MX_TIM6_Init](platform.md#fn-mx-tim6-init), [MX_USART2_UART_Init](platform.md#fn-mx-usart2-uart-init), [SystemClock_Config](platform.md#fn-systemclock-config), [main](platform.md#fn-main).

<a id="fn-assert-failed"></a>

### `assert_failed`

`void assert_failed(uint8_t *file, uint32_t line);`

[Source: main.c:953](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/main.c#L953) · External linkage; conditional on USE_FULL_ASSERT.

Optional USE_FULL_ASSERT hook with empty user body. Current HAL config disables full assertions; not a motor fault handler.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="module-emergency-stop"></a>

## Fatal output primitive

**Files:** [Inc/emergency_stop.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/emergency_stop.h).

Header-only direct TIM1 action for Error_Handler and fatal exceptions. Requires no initialized handle/tick. Distinct from normal recoverable Drive_Disarm.

**Verification entry points:** fatal. See [test instructions](../../tests/README.md).

<a id="fn-servo-emergency-stop"></a>

### `Servo_Emergency_Stop`

`static inline void Servo_Emergency_Stop(void);`

[Source: emergency_stop.h:7](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/emergency_stop.h#L7) · Private/static.

Fatal inline primitive disabling IRQs, clearing TIM1 MOE directly and issuing a synchronization barrier. Fatal callers then halt.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `CLEAR_BIT`, `__DSB`, `__disable_irq`.
- **Direct Core callers:** [BusFault_Handler](platform.md#fn-busfault-handler), [Error_Handler](platform.md#fn-error-handler), [HardFault_Handler](platform.md#fn-hardfault-handler), [MemManage_Handler](platform.md#fn-memmanage-handler), [NMI_Handler](platform.md#fn-nmi-handler), [UsageFault_Handler](platform.md#fn-usagefault-handler).

<a id="module-stm32g4xx-hal-msp"></a>

## HAL low-level hooks

**Files:** [Src/stm32g4xx_hal_msp.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c).

MCU support package: HAL initializers invoke pin/clock/DMA/priority hooks. Here MSP is the support-package name, not the CPU stack pointer. Deinit hooks are retained outside ordinary startup/control.

**Verification entry points:** startup; hardware configuration needs bench checks. See [test instructions](../../tests/README.md).

<a id="fn-hal-mspinit"></a>

### `HAL_MspInit`

`void HAL_MspInit(void);`

[Source: stm32g4xx_hal_msp.c:73](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L73) · External linkage; HAL hook.

Global HAL initialization hook: enable SYSCFG/PWR clocks and disable UCPD dead-battery pull-ups. HAL_Init invokes it during startup.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_PWREx_DisableUCPDDeadBattery`, `__HAL_RCC_PWR_CLK_ENABLE`, `__HAL_RCC_SYSCFG_CLK_ENABLE`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-hal-adc-mspinit"></a>

### `HAL_ADC_MspInit`

`void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc);`

[Source: stm32g4xx_hal_msp.c:101](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L101) · External linkage; HAL hook.

HAL low-level initialization hook configuring ADC12 clock, analog pins, linked DMA streams and ADC interrupts. Called through HAL peripheral initialization, not the foreground schedule.

- **Direct Core calls:** [Error_Handler](platform.md#fn-error-handler).
- **Other direct calls / macros:** `HAL_DMA_Init`, `HAL_GPIO_Init`, `HAL_NVIC_EnableIRQ`, `HAL_NVIC_SetPriority`, `HAL_RCCEx_PeriphCLKConfig`, `__HAL_LINKDMA`, `__HAL_RCC_ADC12_CLK_ENABLE`, `__HAL_RCC_GPIOA_CLK_ENABLE`, `__HAL_RCC_GPIOB_CLK_ENABLE`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-hal-adc-mspdeinit"></a>

### `HAL_ADC_MspDeInit`

`void HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc);`

[Source: stm32g4xx_hal_msp.c:223](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L223) · External linkage; HAL hook.

HAL teardown hook releasing the clocks/pins/DMA/IRQs present in its body. Retained for deinitialization; not used by normal selected startup/control.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_DMA_DeInit`, `HAL_GPIO_DeInit`, `__HAL_RCC_ADC12_CLK_DISABLE`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-hal-cordic-mspinit"></a>

### `HAL_CORDIC_MspInit`

`void HAL_CORDIC_MspInit(CORDIC_HandleTypeDef* hcordic);`

[Source: stm32g4xx_hal_msp.c:297](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L297) · External linkage; HAL hook.

HAL low-level initialization hook configuring accelerator clock and interrupt. Called through HAL peripheral initialization, not the foreground schedule.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_NVIC_EnableIRQ`, `HAL_NVIC_SetPriority`, `__HAL_RCC_CORDIC_CLK_ENABLE`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-hal-cordic-mspdeinit"></a>

### `HAL_CORDIC_MspDeInit`

`void HAL_CORDIC_MspDeInit(CORDIC_HandleTypeDef* hcordic);`

[Source: stm32g4xx_hal_msp.c:322](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L322) · External linkage; HAL hook.

HAL teardown hook releasing the clocks/pins/DMA/IRQs present in its body. Retained for deinitialization; not used by normal selected startup/control.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_NVIC_DisableIRQ`, `__HAL_RCC_CORDIC_CLK_DISABLE`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-hal-fdcan-mspinit"></a>

### `HAL_FDCAN_MspInit`

`void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef* hfdcan);`

[Source: stm32g4xx_hal_msp.c:347](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L347) · External linkage; HAL hook.

HAL low-level initialization hook configuring CAN clock source, alternate-function pins and interrupt. Called through HAL peripheral initialization, not the foreground schedule.

- **Direct Core calls:** [Error_Handler](platform.md#fn-error-handler).
- **Other direct calls / macros:** `HAL_GPIO_Init`, `HAL_NVIC_EnableIRQ`, `HAL_NVIC_SetPriority`, `HAL_RCCEx_PeriphCLKConfig`, `__HAL_RCC_FDCAN_CLK_ENABLE`, `__HAL_RCC_GPIOA_CLK_ENABLE`, `__HAL_RCC_GPIOB_CLK_ENABLE`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-hal-fdcan-mspdeinit"></a>

### `HAL_FDCAN_MspDeInit`

`void HAL_FDCAN_MspDeInit(FDCAN_HandleTypeDef* hfdcan);`

[Source: stm32g4xx_hal_msp.c:405](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L405) · External linkage; HAL hook.

HAL teardown hook releasing the clocks/pins/DMA/IRQs present in its body. Retained for deinitialization; not used by normal selected startup/control.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_GPIO_DeInit`, `HAL_NVIC_DisableIRQ`, `__HAL_RCC_FDCAN_CLK_DISABLE`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-hal-i2c-mspinit"></a>

### `HAL_I2C_MspInit`

`void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c);`

[Source: stm32g4xx_hal_msp.c:438](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L438) · External linkage; HAL hook.

HAL low-level initialization hook configuring I2C clock source, SDA/SCL pins and fast-mode-plus. Called through HAL peripheral initialization, not the foreground schedule.

- **Direct Core calls:** [Error_Handler](platform.md#fn-error-handler).
- **Other direct calls / macros:** `HAL_GPIO_Init`, `HAL_RCCEx_PeriphCLKConfig`, `__HAL_RCC_GPIOB_CLK_ENABLE`, `__HAL_RCC_I2C1_CLK_ENABLE`, `__HAL_SYSCFG_FASTMODEPLUS_ENABLE`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-hal-i2c-mspdeinit"></a>

### `HAL_I2C_MspDeInit`

`void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c);`

[Source: stm32g4xx_hal_msp.c:488](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L488) · External linkage; HAL hook.

HAL teardown hook releasing the clocks/pins/DMA/IRQs present in its body. Retained for deinitialization; not used by normal selected startup/control.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_GPIO_DeInit`, `__HAL_RCC_I2C1_CLK_DISABLE`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-hal-opamp-mspinit"></a>

### `HAL_OPAMP_MspInit`

`void HAL_OPAMP_MspInit(OPAMP_HandleTypeDef* hopamp);`

[Source: stm32g4xx_hal_msp.c:519](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L519) · External linkage; HAL hook.

HAL low-level initialization hook configuring analog pins for the chosen current-sense op-amp. Called through HAL peripheral initialization, not the foreground schedule.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_GPIO_Init`, `__HAL_RCC_GPIOA_CLK_ENABLE`, `__HAL_RCC_GPIOB_CLK_ENABLE`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-hal-opamp-mspdeinit"></a>

### `HAL_OPAMP_MspDeInit`

`void HAL_OPAMP_MspDeInit(OPAMP_HandleTypeDef* hopamp);`

[Source: stm32g4xx_hal_msp.c:591](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L591) · External linkage; HAL hook.

HAL teardown hook releasing the clocks/pins/DMA/IRQs present in its body. Retained for deinitialization; not used by normal selected startup/control.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_GPIO_DeInit`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-hal-tim-base-mspinit"></a>

### `HAL_TIM_Base_MspInit`

`void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim_base);`

[Source: stm32g4xx_hal_msp.c:650](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L650) · External linkage; HAL hook.

HAL low-level initialization hook configuring TIM1/TIM6 clocks. Called through HAL peripheral initialization, not the foreground schedule.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `__HAL_RCC_TIM1_CLK_ENABLE`, `__HAL_RCC_TIM6_CLK_ENABLE`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-hal-tim-msppostinit"></a>

### `HAL_TIM_MspPostInit`

`void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim);`

[Source: stm32g4xx_hal_msp.c:679](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L679) · External linkage; HAL hook.

Configure TIM1 PWM alternate-function GPIOs after timer setup. Explicitly called from MX_TIM1_Init; does not enable motor torque.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_GPIO_Init`, `__HAL_RCC_GPIOA_CLK_ENABLE`, `__HAL_RCC_GPIOB_CLK_ENABLE`, `__HAL_RCC_GPIOC_CLK_ENABLE`.
- **Direct Core callers:** [MX_TIM1_Init](platform.md#fn-mx-tim1-init).

<a id="fn-hal-tim-base-mspdeinit"></a>

### `HAL_TIM_Base_MspDeInit`

`void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* htim_base);`

[Source: stm32g4xx_hal_msp.c:732](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L732) · External linkage; HAL hook.

HAL teardown hook releasing the clocks/pins/DMA/IRQs present in its body. Retained for deinitialization; not used by normal selected startup/control.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `__HAL_RCC_TIM1_CLK_DISABLE`, `__HAL_RCC_TIM6_CLK_DISABLE`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-hal-uart-mspinit"></a>

### `HAL_UART_MspInit`

`void HAL_UART_MspInit(UART_HandleTypeDef* huart);`

[Source: stm32g4xx_hal_msp.c:767](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L767) · External linkage; HAL hook.

HAL low-level initialization hook configuring USART2 clock/pins, linked TX/RX DMA and interrupt. Called through HAL peripheral initialization, not the foreground schedule.

- **Direct Core calls:** [Error_Handler](platform.md#fn-error-handler).
- **Other direct calls / macros:** `HAL_DMA_Init`, `HAL_GPIO_Init`, `HAL_NVIC_EnableIRQ`, `HAL_NVIC_SetPriority`, `HAL_RCCEx_PeriphCLKConfig`, `__HAL_LINKDMA`, `__HAL_RCC_GPIOB_CLK_ENABLE`, `__HAL_RCC_USART2_CLK_ENABLE`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-hal-uart-mspdeinit"></a>

### `HAL_UART_MspDeInit`

`void HAL_UART_MspDeInit(UART_HandleTypeDef* huart);`

[Source: stm32g4xx_hal_msp.c:852](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_hal_msp.c#L852) · External linkage; HAL hook.

HAL teardown hook releasing the clocks/pins/DMA/IRQs present in its body. Retained for deinitialization; not used by normal selected startup/control.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_DMA_DeInit`, `HAL_GPIO_DeInit`, `HAL_NVIC_DisableIRQ`, `__HAL_RCC_USART2_CLK_DISABLE`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="module-stm32g4xx-it"></a>

## Interrupt adapters

**Files:** [Inc/stm32g4xx_it.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/stm32g4xx_it.h), [Src/stm32g4xx_it.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c).

Names referenced by startup vectors. Peripheral handlers dispatch HAL, which invokes user callbacks. Fatal handlers stop/halt; empty CPU service hooks implement no RTOS.

**Verification entry points:** fatal, foc, serial, can. See [test instructions](../../tests/README.md).

<a id="fn-nmi-handler"></a>

### `NMI_Handler`

`void NMI_Handler(void);`

[Source: stm32g4xx_it.c:78](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L78) · External linkage; vector/fatal entry.

CPU exception vector requesting handle-independent emergency output disable, then halting forever. No HAL tick, UART or initialized motor handle is needed.

- **Direct Core calls:** [Servo_Emergency_Stop](platform.md#fn-servo-emergency-stop).
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-hardfault-handler"></a>

### `HardFault_Handler`

`void HardFault_Handler(void);`

[Source: stm32g4xx_it.c:93](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L93) · External linkage; vector/fatal entry.

CPU exception vector requesting handle-independent emergency output disable, then halting forever. No HAL tick, UART or initialized motor handle is needed.

- **Direct Core calls:** [Servo_Emergency_Stop](platform.md#fn-servo-emergency-stop).
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-memmanage-handler"></a>

### `MemManage_Handler`

`void MemManage_Handler(void);`

[Source: stm32g4xx_it.c:109](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L109) · External linkage; vector/fatal entry.

CPU exception vector requesting handle-independent emergency output disable, then halting forever. No HAL tick, UART or initialized motor handle is needed.

- **Direct Core calls:** [Servo_Emergency_Stop](platform.md#fn-servo-emergency-stop).
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-busfault-handler"></a>

### `BusFault_Handler`

`void BusFault_Handler(void);`

[Source: stm32g4xx_it.c:124](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L124) · External linkage; vector/fatal entry.

CPU exception vector requesting handle-independent emergency output disable, then halting forever. No HAL tick, UART or initialized motor handle is needed.

- **Direct Core calls:** [Servo_Emergency_Stop](platform.md#fn-servo-emergency-stop).
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-usagefault-handler"></a>

### `UsageFault_Handler`

`void UsageFault_Handler(void);`

[Source: stm32g4xx_it.c:139](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L139) · External linkage; vector/fatal entry.

CPU exception vector requesting handle-independent emergency output disable, then halting forever. No HAL tick, UART or initialized motor handle is needed.

- **Direct Core calls:** [Servo_Emergency_Stop](platform.md#fn-servo-emergency-stop).
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-svc-handler"></a>

### `SVC_Handler`

`void SVC_Handler(void);`

[Source: stm32g4xx_it.c:154](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L154) · External linkage; vector/fatal entry.

Generated CPU service/exception hook with empty body. No RTOS or control job is implemented here.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-debugmon-handler"></a>

### `DebugMon_Handler`

`void DebugMon_Handler(void);`

[Source: stm32g4xx_it.c:167](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L167) · External linkage; vector/fatal entry.

Generated CPU service/exception hook with empty body. No RTOS or control job is implemented here.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-pendsv-handler"></a>

### `PendSV_Handler`

`void PendSV_Handler(void);`

[Source: stm32g4xx_it.c:180](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L180) · External linkage; vector/fatal entry.

Generated CPU service/exception hook with empty body. No RTOS or control job is implemented here.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-systick-handler"></a>

### `SysTick_Handler`

`void SysTick_Handler(void);`

[Source: stm32g4xx_it.c:193](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L193) · External linkage; vector/fatal entry.

System tick vector incrementing HAL milliseconds for watchdogs/calibration. Separate from the TIM6 microsecond counter.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_IncTick`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-dma1-channel1-irqhandler"></a>

### `DMA1_Channel1_IRQHandler`

`void DMA1_Channel1_IRQHandler(void);`

[Source: stm32g4xx_it.c:214](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L214) · External linkage; vector/fatal entry.

Hardware IRQ vector dispatching UART RX DMA into HAL with the selected handle(s). HAL then invokes applicable callbacks; those are indirect callback edges, not literal calls in this body.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_DMA_IRQHandler`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-dma1-channel2-irqhandler"></a>

### `DMA1_Channel2_IRQHandler`

`void DMA1_Channel2_IRQHandler(void);`

[Source: stm32g4xx_it.c:228](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L228) · External linkage; vector/fatal entry.

Hardware IRQ vector dispatching UART TX DMA into HAL with the selected handle(s). HAL then invokes applicable callbacks; those are indirect callback edges, not literal calls in this body.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_DMA_IRQHandler`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-dma1-channel3-irqhandler"></a>

### `DMA1_Channel3_IRQHandler`

`void DMA1_Channel3_IRQHandler(void);`

[Source: stm32g4xx_it.c:242](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L242) · External linkage; vector/fatal entry.

Hardware IRQ vector dispatching ADC1 DMA into HAL with the selected handle(s). HAL then invokes applicable callbacks; those are indirect callback edges, not literal calls in this body.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_DMA_IRQHandler`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-dma1-channel4-irqhandler"></a>

### `DMA1_Channel4_IRQHandler`

`void DMA1_Channel4_IRQHandler(void);`

[Source: stm32g4xx_it.c:256](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L256) · External linkage; vector/fatal entry.

Hardware IRQ vector dispatching ADC2 DMA into HAL with the selected handle(s). HAL then invokes applicable callbacks; those are indirect callback edges, not literal calls in this body.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_DMA_IRQHandler`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-adc1-2-irqhandler"></a>

### `ADC1_2_IRQHandler`

`void ADC1_2_IRQHandler(void);`

[Source: stm32g4xx_it.c:270](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L270) · External linkage; vector/fatal entry.

Hardware IRQ vector dispatching ADC1/ADC2 interrupts/errors into HAL with the selected handle(s). HAL then invokes applicable callbacks; those are indirect callback edges, not literal calls in this body.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_ADC_IRQHandler`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-fdcan1-it0-irqhandler"></a>

### `FDCAN1_IT0_IRQHandler`

`void FDCAN1_IT0_IRQHandler(void);`

[Source: stm32g4xx_it.c:285](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L285) · External linkage; vector/fatal entry.

Hardware IRQ vector dispatching CAN FIFO events into HAL with the selected handle(s). HAL then invokes applicable callbacks; those are indirect callback edges, not literal calls in this body.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_FDCAN_IRQHandler`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-usart2-irqhandler"></a>

### `USART2_IRQHandler`

`void USART2_IRQHandler(void);`

[Source: stm32g4xx_it.c:301](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L301) · External linkage; vector/fatal entry.

Hardware IRQ vector dispatching UART errors/events into HAL with the selected handle(s). HAL then invokes applicable callbacks; those are indirect callback edges, not literal calls in this body.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_UART_IRQHandler`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-cordic-irqhandler"></a>

### `CORDIC_IRQHandler`

`void CORDIC_IRQHandler(void);`

[Source: stm32g4xx_it.c:315](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/stm32g4xx_it.c#L315) · External linkage; vector/fatal entry.

Hardware IRQ vector dispatching CORDIC events into HAL with the selected handle(s). HAL then invokes applicable callbacks; those are indirect callback edges, not literal calls in this body.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_CORDIC_IRQHandler`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="module-stm32g4xx-hal-conf"></a>

## HAL build-time options

**Files:** [Inc/stm32g4xx_hal_conf.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/stm32g4xx_hal_conf.h).

Header-only component/oscillator/tick/assert options. USE_FULL_ASSERT is disabled in the saved build. No function definitions; inspect source before changing global HAL assumptions.

**Verification entry points:** ARM build, startup. See [test instructions](../../tests/README.md).

No C function definitions. See the [type catalogue](types.md) for data types and the source for macros/options.

<a id="module-system-stm32g4xx"></a>

## CMSIS system support

**Files:** [Src/system_stm32g4xx.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/system_stm32g4xx.c).

Defines SystemCoreClock and prescaler tables; early reset support is distinct from main clock-tree setup. Used with startup assembly and vendor device headers.

**Verification entry points:** startup, ARM build. See [test instructions](../../tests/README.md).

<a id="fn-systeminit"></a>

### `SystemInit`

`void SystemInit(void);`

[Source: system_stm32g4xx.c:180](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/system_stm32g4xx.c#L180) · External linkage.

Early CMSIS reset hook enabling configured FPU access/vector-table location. Application PLL/peripheral setup occurs later.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-systemcoreclockupdate"></a>

### `SystemCoreClockUpdate`

`void SystemCoreClockUpdate(void);`

[Source: system_stm32g4xx.c:229](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/system_stm32g4xx.c#L229) · External linkage.

Recompute SystemCoreClock from RCC registers/prescalers. Updates software frequency view, not hardware clock settings; no direct Core caller.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="module-syscalls"></a>

## Minimal libc environment

**Files:** [Src/syscalls.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c).

Generated newlib stubs, not an OS or the DMA serial transport. Several intentionally fail. Weak character hooks are separate from HAL_Serial_Print. Unreferenced functions can be discarded by the linker.

**Verification entry points:** ARM build; not a control-path module. See [test instructions](../../tests/README.md).

<a id="fn-initialise-monitor-handles"></a>

### `initialise_monitor_handles`

`void initialise_monitor_handles();`

[Source: syscalls.c:45](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L45) · External linkage.

Empty generated debugger/semihosting compatibility hook. Not needed to start DMA serial transport.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn--getpid"></a>

### `_getpid`

`int _getpid(void);`

[Source: syscalls.c:49](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L49) · External linkage.

Bare-metal newlib stub returning 1; no operating-system process model.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn--kill"></a>

### `_kill`

`int _kill(int pid, int sig);`

[Source: syscalls.c:54](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L54) · External linkage.

Unsupported process/signal stub: errno=EINVAL and -1. Does not disarm/reset hardware.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [_exit](platform.md#fn--exit).

<a id="fn--exit"></a>

### `_exit`

`void _exit (int status);`

[Source: syscalls.c:60](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L60) · External linkage.

Invoke unsupported _kill then loop forever. Not the firmware's checked emergency-stop path.

- **Direct Core calls:** [_kill](platform.md#fn--kill).
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn--read"></a>

### `_read`

`int _read(int file, char *ptr, int len);`

[Source: syscalls.c:66](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L66) · External linkage.

Weak newlib byte-read forwarding to external weak __io_getchar. App uses DMA RX instead.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `__io_getchar`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn--write"></a>

### `_write`

`int _write(int file, char *ptr, int len);`

[Source: syscalls.c:78](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L78) · External linkage.

Weak newlib output forwarding to external weak __io_putchar. Does not automatically connect printf to HAL serial.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `__io_putchar`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn--close"></a>

### `_close`

`int _close(int file);`

[Source: syscalls.c:89](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L89) · External linkage.

Unsupported file-close stub returning -1; no filesystem here.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn--fstat"></a>

### `_fstat`

`int _fstat(int file, struct stat *st);`

[Source: syscalls.c:95](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L95) · External linkage.

Descriptor query stub marking a character device and returning zero.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn--isatty"></a>

### `_isatty`

`int _isatty(int file);`

[Source: syscalls.c:101](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L101) · External linkage.

Terminal-query stub returning 1; does not configure UART.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn--lseek"></a>

### `_lseek`

`int _lseek(int file, int ptr, int dir);`

[Source: syscalls.c:106](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L106) · External linkage.

Seek stub returning zero, without real seekable-file behavior.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn--open"></a>

### `_open`

`int _open(char *path, int flags, ...);`

[Source: syscalls.c:111](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L111) · External linkage.

Unsupported file-open stub returning -1.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn--wait"></a>

### `_wait`

`int _wait(int *status);`

[Source: syscalls.c:117](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L117) · External linkage.

Unsupported process-wait stub: errno=ECHILD and -1.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn--unlink"></a>

### `_unlink`

`int _unlink(char *name);`

[Source: syscalls.c:123](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L123) · External linkage.

Unsupported filesystem delete: errno=ENOENT and -1. Does not erase configuration.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn--times"></a>

### `_times`

`int _times(struct tms *buf);`

[Source: syscalls.c:129](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L129) · External linkage.

Unsupported process-time stub returning -1, unrelated to HAL/control timing.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn--stat"></a>

### `_stat`

`int _stat(char *file, struct stat *st);`

[Source: syscalls.c:134](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L134) · External linkage.

Filename-stat stub marking a character device and returning zero; no filesystem inspection.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn--link"></a>

### `_link`

`int _link(char *old, char *new);`

[Source: syscalls.c:140](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L140) · External linkage.

Unsupported file-link stub: errno=EMLINK and -1.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn--fork"></a>

### `_fork`

`int _fork(void);`

[Source: syscalls.c:146](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L146) · External linkage.

Unsupported process creation: errno=EAGAIN and -1. Creates no task/thread.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn--execve"></a>

### `_execve`

`int _execve(char *name, char **argv, char **env);`

[Source: syscalls.c:152](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/syscalls.c#L152) · External linkage.

Unsupported program execution: errno=ENOMEM and -1. Loads no firmware.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="module-sysmem"></a>

## Libc heap support

**Files:** [Src/sysmem.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/sysmem.c).

Implements _sbrk with linker heap/stack boundaries and private watermark. Supports libc; real-time control paths allocate no dynamic objects.

**Verification entry points:** ARM build; not a control-path module. See [test instructions](../../tests/README.md).

<a id="fn--sbrk"></a>

### `_sbrk`

`void *_sbrk(ptrdiff_t incr);`

[Source: sysmem.c:54](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/sysmem.c#L54) · External linkage.

Extend newlib heap watermark from linker _end, refusing growth into reserved stack with ENOMEM/(void*)-1. Supports libc; real-time control does not allocate.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** No direct Core caller; see module/hook context.


## Reset assembly and linker layout

[startup_stm32g431xx.s](../../01-Firmware/B-G431B-ESC1-v3/startup_stm32g431xx.s)
provides the vector table and two assembly function entries:

| Entry | Purpose | Calls |
| --- | --- | --- |
| Reset_Handler | Set stack pointer, copy initialized data and CCMRAM code from flash, clear BSS, enter C startup/application. | SystemInit, C-library __libc_init_array, main; loops if main returns. |
| Default_Handler | Infinite loop for an unoverridden weak vector. | None; this is not the explicit emergency-stop handler. Named fault handlers override their vectors. |

Labels such as CopyDataInit are branches within reset, not separate C APIs.
[STM32G431CBUx_FLASH.ld](../../01-Firmware/B-G431B-ESC1-v3/STM32G431CBUx_FLASH.ld)
places flash/RAM, stack/heap and .ccmram code and defines symbols used by reset
and _sbrk. The [Makefile](../../01-Firmware/B-G431B-ESC1-v3/Makefile) selects the
actual source list, CPU flags, linker script, optimization and libraries.

## External dependencies

The reference stops at these boundaries rather than duplicating thousands of
vendor/library routines. Their direct names still appear in each function entry.

| Name family | Meaning and where to continue |
| --- | --- |
| HAL functions | ST Hardware Abstraction Layer for init, DMA/I2C/CAN/UART I/O, IRQ dispatch, tick and flash. [Sources](../../01-Firmware/B-G431B-ESC1-v3/Drivers/STM32G4xx_HAL_Driver/Src), [headers](../../01-Firmware/B-G431B-ESC1-v3/Drivers/STM32G4xx_HAL_Driver/Inc). |
| LL_CORDIC functions | Register-level accelerator access, in [the LL header](../../01-Firmware/B-G431B-ESC1-v3/Drivers/STM32G4xx_HAL_Driver/Inc/stm32g4xx_ll_cordic.h). |
| __HAL macros, SET_BIT, CLEAR_BIT | Register/clock/compare/DMA-link expansions, not independent jobs or tasks. |
| __get_PRIMASK, __disable_irq, __set_PRIMASK | CMSIS interrupt-mask intrinsics. Restoring saved state preserves nesting. [CMSIS headers](../../01-Firmware/B-G431B-ESC1-v3/Drivers/CMSIS/Include). |
| __DMB, __DSB | CMSIS memory/synchronization barriers. Not locks or substitutes for a coherent group publication guard. |
| memcpy, memset, vsnprintf, va_start, va_end | C-library memory/formatting operations and variadic macros. Formatting is not transmission. |
| fabsf, sqrtf, logf, fminf, fmaxf, isfinite, isnan | C math functions/macros operating on values, not peripheral state. |
| LOW_BYTE, HIGH_BYTE, MAKE_SHORT | Project packing macros in [binary_tool.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/binary_tool.h). Byte order does not establish physical units. |
| DEGREES_TO_RADIANS, RADIANS_TO_DEGREES | Project unit-conversion macros in [math_tool.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/math_tool.h). |
| assert_param / assert_failed | Conditional HAL diagnostics; full assertions are disabled in this saved build. |
| __io_getchar / __io_putchar | External weak newlib character hooks. They are separate from the selected DMA serial transport. |

Callback control flows in reverse: your startup configures/starts hardware,
hardware interrupts, HAL dispatches, then HAL invokes a user hook. Do not add a
direct call merely because the function catalogue reports no direct Core caller.
