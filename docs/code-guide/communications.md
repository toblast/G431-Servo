# Serial protocol, DMA and telemetry

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

- [Serial packet parser](#module-protocol)
- [UART DMA transport](#module-serial)
- [Inline wire conversions](#module-telemetry)

<a id="module-protocol"></a>

## Serial packet parser

**Files:** [Inc/protocol.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/protocol.h), [Src/protocol.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/protocol.c).

One foreground parser and shared packet buffers. Check framing/CRC, inspect registers and delegate guarded configuration/reset. No serial ARM or runtime motor-register writes. UART buffering lives in serial.c.

**Verification entry points:** protocol, integration. See [test instructions](../../tests/README.md).

<a id="fn-updatecrc"></a>

### `updateCRC`

`uint16_t updateCRC(uint16_t crc_accum, uint8_t *data_blk_ptr, uint16_t data_blk_size);`

[Source: protocol.c:19](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/protocol.c#L19) · External linkage.

Compute protocol CRC-16 over a byte span from an initial accumulator with its lookup table. Used for RX validation and TX construction.

- **Direct Core calls:** None.
- **Other direct calls / macros:** None.
- **Direct Core callers:** [packet_encapsulate](communications.md#fn-packet-encapsulate), [packet_handler](communications.md#fn-packet-handler).

<a id="fn-packet-encapsulate"></a>

### `packet_encapsulate`

`void packet_encapsulate(uint32_t payload_status_length);`

[Source: protocol.c:92](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/protocol.c#L92) · External linkage.

Complete shared reply buffer header/ID/length/CRC and set TX length. Caller supplied status payload; this function does not transmit.

- **Direct Core calls:** [updateCRC](communications.md#fn-updatecrc).
- **Other direct calls / macros:** `HIGH_BYTE`, `LOW_BYTE`.
- **Direct Core callers:** [instruction_handler](communications.md#fn-instruction-handler), [packet_handler](communications.md#fn-packet-handler).

<a id="fn-protocol-write-registers"></a>

### `protocol_write_registers`

`static bool protocol_write_registers(uint16_t address, uint16_t length, uint8_t const *data);`

[Source: protocol.c:127](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/protocol.c#L127) · Private/static.

Private write bridge: permit a one-byte LED update atomically; delegate configuration to Config. Protect other runtime fields. Returns bool mapped to existing serial access error on failure.

- **Direct Core calls:** [Config_ApplyRegisterWrite](configuration.md#fn-config-applyregisterwrite).
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** [instruction_handler](communications.md#fn-instruction-handler).

<a id="fn-instruction-handler"></a>

### `instruction_handler`

`void instruction_handler();`

[Source: protocol.c:140](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/protocol.c#L140) · External linkage.

Dispatch checked packets: ping, bounded read, guarded write, reset/reboot or instruction error. Reset/reboot acquire maintenance then wait 100 ms before reset; calibration rejects them silently.

- **Direct Core calls:** [Config_FactoryReset](configuration.md#fn-config-factoryreset), [Drive_BeginMaintenance](application.md#fn-drive-beginmaintenance), [HAL_Serial_Write](communications.md#fn-hal-serial-write), [packet_encapsulate](communications.md#fn-packet-encapsulate), [protocol_write_registers](communications.md#fn-protocol-write-registers).
- **Other direct calls / macros:** `HAL_Delay`, `HAL_NVIC_SystemReset`, `MAKE_SHORT`.
- **Direct Core callers:** [packet_handler](communications.md#fn-packet-handler).

<a id="fn-packet-handler"></a>

### `packet_handler`

`void packet_handler(char c);`

[Source: protocol.c:258](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/protocol.c#L258) · External linkage.

Consume one byte through header/ID/length/payload/CRC states. Bound payload, check CRC and dispatch this-ID/broadcast packets. CRC error increments shared diagnostic and queues an error reply; reset parser for next packet.

- **Direct Core calls:** [HAL_Serial_Write](communications.md#fn-hal-serial-write), [instruction_handler](communications.md#fn-instruction-handler), [packet_encapsulate](communications.md#fn-packet-encapsulate), [updateCRC](communications.md#fn-updatecrc).
- **Other direct calls / macros:** `MAKE_SHORT`.
- **Direct Core callers:** [service](application.md#fn-service).

<a id="module-serial"></a>

## UART DMA transport

**Files:** [Inc/serial.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/serial.h), [Src/serial.c](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c).

Persistent registered handlers, circular RX and bounded packet TX. Foreground produces packets; completion releases the DMA-owned slot. Selected main registers one full-duplex handler.

**Verification entry points:** serial, protocol, integration boundary. See [test instructions](../../tests/README.md).

<a id="fn-hal-uart-errorcallback"></a>

### `HAL_UART_ErrorCallback`

`void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);`

[Source: serial.c:21](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c#L21) · External linkage; HAL hook.

HAL hook finding its registered handler: discard/latch failed active TX DMA where applicable, reset RX position and restart RX DMA. Transport recovery, not a motor fault operation.

- **Direct Core calls:** [Serial_Tx_Fail](communications.md#fn-serial-tx-fail).
- **Other direct calls / macros:** `HAL_UART_Receive_DMA`, `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-hal-serial-init"></a>

### `HAL_Serial_Init`

`void HAL_Serial_Init(UART_HandleTypeDef * huart, HAL_Serial_Handler * hserial);`

[Source: serial.c:40](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c#L40) · External linkage.

Register persistent handles, clear queues/error and start circular RX DMA in full duplex. Call once for the configured handler, not repeatedly as runtime cleanup.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_UART_Receive_DMA`.
- **Direct Core callers:** [main](platform.md#fn-main).

<a id="fn-hal-serial-init-half-duplex"></a>

### `HAL_Serial_Init_Half_Duplex`

`void HAL_Serial_Init_Half_Duplex( UART_HandleTypeDef * huart, HAL_Serial_Handler * hserial, GPIO_TypeDef *GPIOx_TX, uint16_t GPIO_Pin_TX, GPIO_PinState PinState_TX, GPIO_TypeDef *GPIOx_RX, uint16_t GPIO_Pin_RX, GPIO_PinState PinState_RX );`

[Source: serial.c:68](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c#L68) · External linkage.

Retained alternative with GPIO direction control; register/reset buffers, start RX and select receive direction. Main uses full duplex.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_GPIO_WritePin`, `HAL_UART_Receive_DMA`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-hal-serial-available"></a>

### `HAL_Serial_Available`

`int HAL_Serial_Available(HAL_Serial_Handler * hserial);`

[Source: serial.c:108](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c#L108) · External linkage.

Compute unread bytes from RX DMA position and tail, or zero if unbound. Does not consume data.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `__HAL_DMA_GET_COUNTER`.
- **Direct Core callers:** [service](application.md#fn-service).

<a id="fn-hal-serial-getchar"></a>

### `HAL_Serial_GetChar`

`char HAL_Serial_GetChar(HAL_Serial_Handler * hserial);`

[Source: serial.c:120](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c#L120) · External linkage.

Consume one RX byte and wrap tail, or return zero if empty/unbound. Use Available to distinguish no data from byte zero.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `__HAL_DMA_GET_COUNTER`.
- **Direct Core callers:** [service](application.md#fn-service).

<a id="fn-hal-serial-read"></a>

### `HAL_Serial_Read`

`int HAL_Serial_Read(HAL_Serial_Handler * hserial, uint8_t * ptr, int len );`

[Source: serial.c:137](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c#L137) · External linkage.

Copy up to len available RX bytes and return count; invalid/unbound input returns zero. Retained bulk API, not App's current byte loop.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `__HAL_DMA_GET_COUNTER`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-serial-tx-direction"></a>

### `Serial_Tx_Direction`

`static void Serial_Tx_Direction(HAL_Serial_Handler *hserial, bool transmit);`

[Source: serial.c:155](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c#L155) · Private/static.

Private half-duplex TX/RX GPIO selection; full duplex returns without toggling.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `HAL_GPIO_WritePin`.
- **Direct Core callers:** [Serial_Tx_Fail](communications.md#fn-serial-tx-fail), [Serial_Tx_Start](communications.md#fn-serial-tx-start).

<a id="fn-serial-tx-fail"></a>

### `Serial_Tx_Fail`

`static void Serial_Tx_Fail(HAL_Serial_Handler *hserial, HAL_StatusTypeDef status);`

[Source: serial.c:166](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c#L166) · Private/static.

IRQ-protected latch/discard of failed queued TX, clear DMA ownership and restore receive direction. Does not retry old replies.

- **Direct Core calls:** [Serial_Tx_Direction](communications.md#fn-serial-tx-direction).
- **Other direct calls / macros:** None.
- **Direct Core callers:** [HAL_UART_ErrorCallback](communications.md#fn-hal-uart-errorcallback), [Serial_Tx_Start](communications.md#fn-serial-tx-start).

<a id="fn-serial-tx-start"></a>

### `Serial_Tx_Start`

`static HAL_StatusTypeDef Serial_Tx_Start(HAL_Serial_Handler *hserial);`

[Source: serial.c:174](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c#L174) · Private/static.

Under queue guard, select receive if empty or start DMA on the tail slot. Keep the slot owned until callback; start failure invokes cleanup. Return HAL status.

- **Direct Core calls:** [Serial_Tx_Direction](communications.md#fn-serial-tx-direction), [Serial_Tx_Fail](communications.md#fn-serial-tx-fail).
- **Other direct calls / macros:** `HAL_UART_Transmit_DMA`.
- **Direct Core callers:** [HAL_Serial_Write](communications.md#fn-hal-serial-write), [HAL_UART_TxCpltCallback](communications.md#fn-hal-uart-txcpltcallback).

<a id="fn-hal-serial-write"></a>

### `HAL_Serial_Write`

`int HAL_Serial_Write(HAL_Serial_Handler *hserial, uint8_t const *ptr, int len);`

[Source: serial.c:188](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c#L188) · External linkage.

Atomically copy 1..256 bytes into a free slot/start DMA if idle. Return queued count or zero on invalid/full/start failure. Acceptance does not prove later transmission.

- **Direct Core calls:** [Serial_Tx_Start](communications.md#fn-serial-tx-start).
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`, `memcpy`.
- **Direct Core callers:** [HAL_Serial_Print](communications.md#fn-hal-serial-print), [instruction_handler](communications.md#fn-instruction-handler), [packet_handler](communications.md#fn-packet-handler).

<a id="fn-hal-uart-txcpltcallback"></a>

### `HAL_UART_TxCpltCallback`

`void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);`

[Source: serial.c:208](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c#L208) · External linkage; HAL hook.

HAL completion hook releasing the matching DMA-owned slot, advancing tail and starting the next packet. Unrelated/unowned callbacks release nothing.

- **Direct Core calls:** [Serial_Tx_Start](communications.md#fn-serial-tx-start).
- **Other direct calls / macros:** `__disable_irq`, `__get_PRIMASK`, `__set_PRIMASK`.
- **Direct Core callers:** No direct Core caller; see module/hook context.

<a id="fn-hal-serial-print"></a>

### `HAL_Serial_Print`

`int HAL_Serial_Print(HAL_Serial_Handler *hserial, const char *fmt, ...);`

[Source: serial.c:224](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c#L224) · External linkage.

Bounded format into fixed local buffer, then queue a copy. Return queued length or zero on oversized/empty formatting or transport rejection. Does not use libc _write.

- **Direct Core calls:** [HAL_Serial_Write](communications.md#fn-hal-serial-write).
- **Other direct calls / macros:** `va_end`, `va_start`, `vsnprintf`.
- **Direct Core callers:** [App_Process](application.md#fn-app-process), [service](application.md#fn-service).

<a id="module-telemetry"></a>

## Inline wire conversions

**Files:** [Inc/telemetry.h](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/telemetry.h).

Header-only saturating float-to-wire helpers. Callers supply units/scales, valid address ranges and group publication guards. No hardware acquisition or new CAN format.

**Verification entry points:** telemetry, can, integration. See [test instructions](../../tests/README.md).

<a id="fn-telemetry-i16"></a>

### `telemetry_i16`

`static inline int16_t telemetry_i16(float value);`

[Source: telemetry.h:9](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/telemetry.h#L9) · Private/static.

Float-to-int16 diagnostic conversion: NaN to zero, high/low/infinity saturation, finite fraction truncation toward zero.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `isnan`.
- **Direct Core callers:** [CanServo_OnRxFifo0](application.md#fn-canservo-onrxfifo0), [telemetry_store_i16](communications.md#fn-telemetry-store-i16).

<a id="fn-telemetry-u8"></a>

### `telemetry_u8`

`static inline uint8_t telemetry_u8(float value);`

[Source: telemetry.h:17](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/telemetry.h#L17) · Private/static.

Float-to-byte conversion: NaN/nonpositive to zero, high values to 255, finite fraction truncation.

- **Direct Core calls:** None.
- **Other direct calls / macros:** `isnan`.
- **Direct Core callers:** [control_mirror_measurements](configuration.md#fn-control-mirror-measurements), [update_servo](application.md#fn-update-servo).

<a id="fn-telemetry-store-i16"></a>

### `telemetry_store_i16`

`static inline void telemetry_store_i16(uint8_t *registers, unsigned address, float value);`

[Source: telemetry.h:24](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/telemetry.h#L24) · Private/static.

Convert through telemetry_i16 and store two little-endian bytes. Caller guarantees valid range/publication guard.

- **Direct Core calls:** [telemetry_i16](communications.md#fn-telemetry-i16).
- **Other direct calls / macros:** None.
- **Direct Core callers:** [control_mirror_measurements](configuration.md#fn-control-mirror-measurements), [update_servo](application.md#fn-update-servo).
