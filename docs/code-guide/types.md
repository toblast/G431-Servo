# Types, state and units

[Reading guide](README.md) · [Functions](function-index.md)

This catalogue covers all **31 named typedefs** in selected Core source/headers.
Public value types are copied numeric models; driver-local unions and private
parser/sample types are implementation details. Definitions below are copied
from the saved source, not a new binary format. Comments describe field meaning.

## Application values

Start with DriveConfig, ServoCommand, DriveCommandSnapshot, DriveMeasurements,
DriveStatus and ServoSetpoints. Their C memory layouts can contain padding;
CAN/serial codecs explicitly pack fields, and flash persists ConfigImage bytes.

## State and result enums

DriveState, FOC numeric modes, CalibrationPhase and e_packet_state describe
different layers. Do not compare their integer values as if they were one state
machine. Bool success is true; low-level HAL_OK is zero.

## Find a type

| Type | Meaning |
| --- | --- |
| [AS5600_TypeDef](#type-as5600-typedef) | Borrowed I2C device handle |
| [CalibrationResult](#type-calibrationresult) | Asynchronous completion code |
| [ConfigImage](#type-configimage) | Persistent byte image |
| [ConfigResult](#type-configresult) | Synchronous configuration outcome |
| [DriveFault](#type-drivefault) | Fault bit names |
| [DriveFaultFlags](#type-drivefaultflags) | Eight-bit fault combination |
| [DriveConfig](#type-driveconfig) | Application settings snapshot |
| [ServoCommand](#type-servocommand) | Accepted numeric servo goal |
| [DriveCommandSnapshot](#type-drivecommandsnapshot) | Permission plus goal copy |
| [DriveState](#type-drivestate) | Derived application operating state |
| [DriveMeasurements](#type-drivemeasurements) | Published observed values |
| [DriveStatus](#type-drivestatus) | Application status snapshot |
| [MotorAlignment](#type-motoralignment) | Calibration result/backup |
| [MotorControlConfig](#type-motorcontrolconfig) | FOC settings view |
| [MotorProtectionConfig](#type-motorprotectionconfig) | Analog protection limits |
| [pid_context_t](#type-pid-context-t) | Per-controller calculation history |
| [tx_buffer](#type-tx-buffer) | One UART transmit slot |
| [HAL_Serial_Handler](#type-hal-serial-handler) | Project-defined UART transport state |
| [ServoSetpoints](#type-servosetpoints) | Calculated diagnostic outputs |
| [conf_reg_t](#type-conf-reg-t) | Private AS5600 CONF view |
| [mang_reg_t](#type-mang-reg-t) | Private AS5600 maximum-angle view |
| [mpos_reg_t](#type-mpos-reg-t) | Private AS5600 stop-position view |
| [zpos_reg_t](#type-zpos-reg-t) | Private AS5600 zero/start-position view |
| [zmco_reg_t](#type-zmco-reg-t) | Retained sensor diagnostic view |
| [angle_reg_t](#type-angle-reg-t) | Private AS5600 angle view |
| [status_reg_t](#type-status-reg-t) | Private magnet-health view |
| [agc_reg_t](#type-agc-reg-t) | Retained sensor AGC view |
| [mag_reg_t](#type-mag-reg-t) | Private sensor magnitude view |
| [CalibrationPhase](#type-calibrationphase) | Private motion-job progress |
| [positionSensor_t](#type-positionsensor-t) | Private complete sample/history |
| [e_packet_state](#type-e-packet-state) | Private serial byte-parser stage |

<a id="type-as5600-typedef"></a>

## `AS5600_TypeDef` — Borrowed I2C device handle

[Definition: as5600.h:17](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/as5600.h#L17)

i2cHandle points to initialized I2C1; i2cAddr is the HAL-format shifted slave address set to 0x6c by Init. Static storage lives in position_sensor; no heap allocation. It identifies the sensor, not a sample or configuration image.

```c
typedef struct {
    I2C_HandleTypeDef *i2cHandle;
    uint8_t i2cAddr;
} AS5600_TypeDef;
```

<a id="type-calibrationresult"></a>

## `CalibrationResult` — Asynchronous completion code

[Definition: calibration.h:6](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/calibration.h#L6)

SUCCESS=0, NO_MOTION=1, POLE_MISMATCH=2, CANCELLED=3, STORAGE_ERROR=4. Start's bool reports acceptance separately. App consumes/prints one completion; failed starts create no pending completion. Failures restore RAM alignment, not necessarily an already written flash page.

```c
typedef enum {
    CALIBRATION_SUCCESS = 0,
    CALIBRATION_NO_MOTION = 1,
    CALIBRATION_POLE_MISMATCH = 2,
    CALIBRATION_CANCELLED = 3,
    CALIBRATION_STORAGE_ERROR = 4
} CalibrationResult;
```

<a id="type-configimage"></a>

## `ConfigImage` — Persistent byte image

[Definition: config_storage.h:9](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/config_storage.h#L9)

bytes is exactly CONFIG_IMAGE_SIZE=128 bytes covering register addresses 0x00..0x7f. Contains only configuration/default/reserved bytes, never torque/goal/runtime telemetry. Its layout is the compatibility storage format; it is not memcpy of DriveConfig and contains no CRC/version transaction header.

```c
typedef struct { uint8_t bytes[CONFIG_IMAGE_SIZE]; } ConfigImage;
```

<a id="type-configresult"></a>

## `ConfigResult` — Synchronous configuration outcome

[Definition: configuration.h:6](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/configuration.h#L6)

CONFIG_OK=0 means applied/saved; CONFIG_ACCESS_ERROR=1 invalid pointer/range; CONFIG_BUSY=2 failed maintenance entry; CONFIG_STORAGE_ERROR=3 failed persistence. Access/busy apply nothing; storage failure retains applied RAM and latches internal fault. Successful factory reset retains maintenance until caller reset.

```c
typedef enum {
    CONFIG_OK,
    CONFIG_ACCESS_ERROR,
    CONFIG_BUSY,
    CONFIG_STORAGE_ERROR
} ConfigResult;
```

<a id="type-drivefault"></a>

## `DriveFault` — Fault bit names

[Definition: drive_faults.h:7](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/drive_faults.h#L7)

Masks, not sequential exclusive states: voltage bit 0, encoder read/status bit 1, encoder stale bit 2, internal bit 3, FOC timeout bit 4, phase overcurrent bit 5, overheating bit 6, invalid configuration bit 7. Several can coexist. Clearing policies differ; consult the drive contract rather than clearing every fault to retry.

```c
typedef enum {
    DRIVE_FAULT_VOLTAGE = 1U << 0,
    DRIVE_FAULT_ENCODER_READ = 1U << 1,
    DRIVE_FAULT_ENCODER_STALE = 1U << 2,
    DRIVE_FAULT_INTERNAL = 1U << 3,
    DRIVE_FAULT_FOC_TIMEOUT = 1U << 4,
    DRIVE_FAULT_OVERCURRENT = 1U << 5,
    DRIVE_FAULT_OVERHEATING = 1U << 6,
    DRIVE_FAULT_CONFIGURATION = 1U << 7
} DriveFault;
```

<a id="type-drivefaultflags"></a>

## `DriveFaultFlags` — Eight-bit fault combination

[Definition: drive_faults.h:6](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/drive_faults.h#L6)

Alias of uint8_t. Zero means no bits set; bitwise OR combines DriveFault masks. hardware_fault_set/clear preserve unrelated bits under an IRQ guard. The live register byte remains authoritative.

```c
typedef uint8_t DriveFaultFlags;
```

<a id="type-driveconfig"></a>

## `DriveConfig` — Application settings snapshot

[Definition: drive_types.h:10](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/drive_types.h#L10)

Configuration bytes decoded into numeric limits/settings, not a persisted struct. min_position_deg/max_position_deg bound commanded/initial ARM travel; max_velocity_dps is positive motor speed limit; max_current_mA clamps demanded current. encoder_bits must be 12; motor_pole_pairs and invert_phase_motor define electrical conversion. low_voltage_V/high_voltage_V/temperature_limit_C govern analog faults; moving_threshold_dps is telemetry classification. Four current-gain raw fields use Kp/100000 and Ki/100000000. Drive/Config validate, FOC consumes narrower views.

```c
typedef struct {
    int16_t min_position_deg, max_position_deg, max_velocity_dps;
    uint16_t max_current_mA;
    uint8_t encoder_bits, motor_pole_pairs, invert_phase_motor;
    uint8_t low_voltage_V, high_voltage_V, temperature_limit_C;
    uint8_t moving_threshold_dps;
    int16_t flux_current_kp_raw, flux_current_ki_raw;
    int16_t torque_current_kp_raw, torque_current_ki_raw;
} DriveConfig;
```

<a id="type-servocommand"></a>

## `ServoCommand` — Accepted numeric servo goal

[Definition: drive_types.h:22](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/drive_types.h#L22)

position_tenths_deg uses 0.1 motor degree; velocity_dps is degrees/s; torque_mA and flux_mA are Iq feed-forward/Id demand. position_kp_raw is multiplied by 2, position_kd_raw by 200 against filtered per-sample error difference, velocity_kp_raw divided by 10. CAN carries position, velocity, Iq and two position gains; it supplies zero flux/velocity gain. No packed-memory transmission or physical torque-in-Nm field.

```c
typedef struct {
    int16_t position_tenths_deg, velocity_dps, torque_mA, flux_mA;
    // Controller multipliers: Kp = raw * 2, Kd = raw * 200 per-sample derivative,
    // velocity Kp = raw / 10. The CAN format supplies zero velocity Kp.
    uint8_t position_kp_raw, position_kd_raw, velocity_kp_raw;
} ServoCommand;
```

<a id="type-drivecommandsnapshot"></a>

## `DriveCommandSnapshot` — Permission plus goal copy

[Definition: drive_types.h:29](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/drive_types.h#L29)

enabled combines supervisor ARM with current fault inhibition; generation identifies the accepted ARM lifetime; goal is a value copy. Calculation can outlive the permission that produced it. Drive_PublishCurrents rechecks the live generation/fault/ARM guard. Modifying a snapshot never changes the owned command.

```c
typedef struct {
    // A copied value, not a live reference. Publication must recheck generation.
    bool enabled;
    uint32_t generation;
    ServoCommand goal;
} DriveCommandSnapshot;
```

<a id="type-drivestate"></a>

## `DriveState` — Derived application operating state

[Definition: drive_types.h:36](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/drive_types.h#L36)

DISARMED means no active normal ARM; ARMED means permission, not measured PWM enable; CALIBRATING means the guarded job including saving/cleanup; MAINTENANCE excludes ARM for configuration/reset; FAULT takes precedence over all. This enum is not the FOC mode and is not a newly defined CAN status byte.

```c
typedef enum {
    DRIVE_DISARMED,
    DRIVE_ARMED,
    DRIVE_CALIBRATING,
    DRIVE_MAINTENANCE,
    DRIVE_FAULT
} DriveState;
```

<a id="type-drivemeasurements"></a>

## `DriveMeasurements` — Published observed values

[Definition: drive_types.h:44](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/drive_types.h#L44)

position_deg/velocity_dps describe motor shaft; torque_mA/flux_mA are observed Iq/Id, voltage_V bus voltage, temperature_C controller temperature. moving compares absolute velocity with the configured threshold. These are observations, not demands or proof of enabled outputs. App captures a coherent group and Drive owns/publishes the copy.

```c
typedef struct {
    float position_deg, velocity_dps;
    float torque_mA, flux_mA, voltage_V, temperature_C;
    bool moving;
} DriveMeasurements;
```

<a id="type-drivestatus"></a>

## `DriveStatus` — Application status snapshot

[Definition: drive_types.h:50](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/drive_types.h#L50)

state is the derived DriveState, faults the live bit mask, generation the current ARM identity, armed application permission, last_command_ms the wrapping accepted ARM/command tick, measured the last published group. Fault state takes precedence even before a foreground pass has revoked armed. Not serialized wholesale into CAN.

```c
typedef struct {
    DriveState state;
    DriveFaultFlags faults;
    uint32_t generation;
    bool armed; // Command permission, not proof that PWM outputs are enabled.
    uint32_t last_command_ms; // HAL tick at last accepted ARM/servo command; wraps.
    DriveMeasurements measured;
} DriveStatus;
```

<a id="type-motoralignment"></a>

## `MotorAlignment` — Calibration result/backup

[Definition: motor_config.h:12](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/motor_config.h#L12)

phase_offset_deg is signed electrical commutation offset in degrees; invert_phase_motor selects direction (0 normal, 1 reversed). Different from encoder's absolute rotor angle and external gearing. Calibration backs up/restores/publishes the pair atomically; persistence is a separate step.

```c
typedef struct {
    int16_t phase_offset_deg;
    uint8_t invert_phase_motor;
} MotorAlignment;
```

<a id="type-motorcontrolconfig"></a>

## `MotorControlConfig` — FOC settings view

[Definition: motor_config.h:19](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/motor_config.h#L19)

phase_offset_deg is signed, phase_trim_deg unsigned runtime manual trim; motor_pole_pairs and invert_phase_motor convert rotor to electrical angle. Four signed raw gains retain their register scale; max_velocity_dps governs the existing armed overspeed check. Snapshot is coherent and read afresh by FOC, not a second cache.

```c
typedef struct {
    int16_t phase_offset_deg;
    uint16_t phase_trim_deg;
    uint8_t motor_pole_pairs, invert_phase_motor;
    int16_t flux_current_kp_raw, flux_current_ki_raw;
    int16_t torque_current_kp_raw, torque_current_ki_raw;
    int16_t max_velocity_dps;
} MotorControlConfig;
```

<a id="type-motorprotectionconfig"></a>

## `MotorProtectionConfig` — Analog protection limits

[Definition: motor_config.h:28](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/motor_config.h#L28)

low_voltage_V/high_voltage_V and temperature_limit_C are copied bus/temperature thresholds. Reading them does not acquire hardware or clear faults. Foreground analog service owns those effects.

```c
typedef struct {
    uint8_t low_voltage_V, high_voltage_V, temperature_limit_C;
} MotorProtectionConfig;
```

<a id="type-pid-context-t"></a>

## `pid_context_t` — Per-controller calculation history

[Definition: pid.h:19](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/pid.h#L19)

err_last_one is prior error for the next difference, err_integral accumulated integral contribution, derivative_filtered the smoothed per-sample difference. Units depend on the controller. Separate instances belong to outer PD and Id/Iq PI. The outer call uses Ki=0 even though the helper supports integral action.

```c
typedef struct {
    float err_last_one;
    float err_integral;
    float derivative_filtered;

} pid_context_t;
```

<a id="type-tx-buffer"></a>

## `tx_buffer` — One UART transmit slot

[Definition: serial.h:21](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/serial.h#L21)

data stores at most SERIAL_TX_BUFFER_SIZE=256 bytes; length is valid-byte count. Fixed pool has 32 slots with one kept empty, so at most 31 packets including active DMA. Tail slot belongs to DMA until completion; producers must not overwrite it.

```c
typedef struct {
    uint8_t data[SERIAL_TX_BUFFER_SIZE];
    uint32_t length;
} tx_buffer;
```

<a id="type-hal-serial-handler"></a>

## `HAL_Serial_Handler` — Project-defined UART transport state

[Definition: serial.h:27](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/serial.h#L27)

Despite HAL prefix this struct is in serial.h. huart is borrowed; tx_buffer_pool plus head/tail store packets, tx_dma marks DMA ownership, tx_error latches a transport error. RX circular buffer is 2048 bytes; rx_tail_ptr marks consumed data, DMA position supplies head. half_duplex and paired GPIO/pin/polarity fields select direction. The selected startup registers one persistent handler.

```c
typedef struct {
    UART_HandleTypeDef * huart;

    tx_buffer tx_buffer_pool[SERIAL_TX_POOL_SIZE];
    volatile uint32_t tx_head_position;
    volatile uint32_t tx_tail_position; // Includes the slot currently owned by DMA.
    volatile uint32_t tx_dma;
    volatile HAL_StatusTypeDef tx_error; // Latched DMA-start/runtime error; reset by Init.

    uint8_t rx_circular_buffer[SERIAL_RX_BUFFER_SIZE];
    uint8_t const * rx_tail_ptr;

    // for half duplex serial, DIR TX and RX signals
    bool half_duplex;
    GPIO_TypeDef *GPIOx_DIR_TX;
    uint16_t GPIO_Pin_DIR_TX;
    GPIO_PinState PinState_DIR_TX;
    GPIO_TypeDef *GPIOx_DIR_RX;
    uint16_t GPIO_Pin_DIR_RX;
    GPIO_PinState PinState_DIR_RX;

} HAL_Serial_Handler;
```

<a id="type-servosetpoints"></a>

## `ServoSetpoints` — Calculated diagnostic outputs

[Definition: servo_controller.h:7](../../01-Firmware/B-G431B-ESC1-v3/Core/Inc/servo_controller.h#L7)

position_deg/velocity_dps are clamped targets; torque_mA/flux_mA calculated current demands. Returned values describe the calculation even if its later publication is refused. During calibration App publishes zero outer-loop diagnostic setpoints, while measured values remain live.

```c
typedef struct {
    float position_deg, velocity_dps;
    float torque_mA, flux_mA;
} ServoSetpoints;
```

<a id="type-conf-reg-t"></a>

## `conf_reg_t` — Private AS5600 CONF view

[Definition: as5600.c:70](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L70)

raw and bit share one register value. SF/FTH select filters, WD sensor watchdog option, PM power mode, HYST hysteresis, OUTS output mode, PWMF output PWM setting; notused bits are reserved. Init writes/checks the selected settings. This register view is not DriveConfig.

```c
typedef union {
    volatile uint16_t      raw;
    struct
    {
        volatile uint16_t      SF            :  2;
        volatile uint16_t      FTH           :  3;
        volatile uint16_t      WD            :  1;
        volatile uint16_t      notused       :  2;
        volatile uint16_t      PM            :  2;
        volatile uint16_t      HYST          :  2;
        volatile uint16_t      OUTS          :  2;
        volatile uint16_t      PWMF          :  2;

    } bit;
} conf_reg_t;
```

<a id="type-mang-reg-t"></a>

## `mang_reg_t` — Private AS5600 maximum-angle view

[Definition: as5600.c:87](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L87)

raw word and split mang high/low bit fields encode/decode MANG; notused is reserved. Counts are sensor register units, not a supervisor travel limit.

```c
typedef union {
    volatile uint16_t      raw;
    struct
    {
        volatile uint16_t      mang8_12      :   4;
        volatile uint16_t      notused       :   4;
        volatile uint16_t      mang0_7       :   8;
    } bit;
} mang_reg_t;
```

<a id="type-mpos-reg-t"></a>

## `mpos_reg_t` — Private AS5600 stop-position view

[Definition: as5600.c:98](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L98)

raw word and split mpos high/low fields encode/decode MPOS; reserved bits are not a new control feature.

```c
typedef union {
    volatile uint16_t      raw;
    struct
    {
        volatile uint16_t      mpos8_12      :   4;
        volatile uint16_t      notused       :   4;
        volatile uint16_t      mpos0_7       :   8;
    } bit;
} mpos_reg_t;
```

<a id="type-zpos-reg-t"></a>

## `zpos_reg_t` — Private AS5600 zero/start-position view

[Definition: as5600.c:109](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L109)

raw word and split zpos fields encode/decode ZPOS. Selected startup preserves this hardware zero; retained setter can change it.

```c
typedef union {
    volatile uint16_t      raw;
    struct
    {
        volatile uint16_t      zpos8_12      :   4;
        volatile uint16_t      notused       :   4;
        volatile uint16_t      zpos0_7       :   8;
    } bit;
} zpos_reg_t;
```

<a id="type-zmco-reg-t"></a>

## `zmco_reg_t` — Retained sensor diagnostic view

[Definition: as5600.c:120](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L120)

raw byte/zmco field for the sensor programming-count register and reserved bits. Defined for register support but no current driver function uses this type; it is not the firmware boot counter.

```c
typedef union {
    volatile uint8_t       raw;
    struct
    {
        volatile uint8_t       zmco          :   2;
        volatile uint8_t       notused       :   6;
    } bit;
} zmco_reg_t;
```

<a id="type-angle-reg-t"></a>

## `angle_reg_t` — Private AS5600 angle view

[Definition: as5600.c:131](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L131)

raw word and split angle high/low fields assemble the 12-bit angle. Driver getters use a local value so failed reads do not publish partially initialized angles.

```c
typedef union {
    volatile uint16_t      raw;
    struct
    {
        volatile uint16_t      angle8_12     :   4;
        volatile uint16_t      notused       :   4;
        volatile uint16_t      angle0_7      :   8;
    } bit;
} angle_reg_t;
```

<a id="type-status-reg-t"></a>

## `status_reg_t` — Private magnet-health view

[Definition: as5600.c:143](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L143)

raw byte and MagnetDetected/Agc_min_gain_overflow/Agc_max_gain_overflow flags plus reserved bits. CheckMagnet requires detection without error flags; ignore failed I2C output.

```c
typedef union {
    volatile uint8_t       raw;
    struct
    {
        volatile uint8_t       notused1              :   3;
        volatile uint8_t       Agc_min_gain_overflow :   1;
        volatile uint8_t       Agc_max_gain_overflow :   1;
        volatile uint8_t       MagnetDetected        :   1;
        volatile uint8_t       notused2              :   2;
    } bit;
} status_reg_t;
```

<a id="type-agc-reg-t"></a>

## `agc_reg_t` — Retained sensor AGC view

[Definition: as5600.c:156](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L156)

raw byte/agc field for sensor gain diagnostics. Type is retained but current GetAGCSetting reads directly into a uint8_t output.

```c
typedef union {
    volatile uint8_t       raw;
    struct
    {
        volatile uint8_t       agc      :   8;
    } bit;
} agc_reg_t;
```

<a id="type-mag-reg-t"></a>

## `mag_reg_t` — Private sensor magnitude view

[Definition: as5600.c:165](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/as5600.c#L165)

raw word and split magnitude fields assemble the magnetic-magnitude diagnostic. Not the STM32 hardware CORDIC configuration/result type.

```c
typedef union {
    volatile uint16_t      raw;
    struct
    {
        volatile uint16_t      magnitude8_12 :   4;
        volatile uint16_t      notused       :   4;
        volatile uint16_t      magnitude0_7  :   8;
    } bit;
} mag_reg_t;
```

<a id="type-calibrationphase"></a>

## `CalibrationPhase` — Private motion-job progress

[Definition: calibration.c:10](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/calibration.c#L10)

CAL_IDLE is no job; CAL_COAST/CAL_SETTLE prepare excitation; forward/reverse states advance all positions and settle; CAPTURE states wait for a subsequent normal acquisition; ALIGN settles angle zero; SAVE persists while coasting. Internal scheduling state, not a user command or DriveState value.

```c
typedef enum {
    CAL_IDLE,
    CAL_COAST,
    CAL_SETTLE,
    CAL_FORWARD,
    CAL_FORWARD_SETTLE,
    CAL_CAPTURE_FORWARD,
    CAL_REVERSE,
    CAL_REVERSE_SETTLE,
    CAL_CAPTURE_REVERSE,
    CAL_ALIGN,
    CAL_CAPTURE_ALIGNMENT,
    CAL_SAVE
} CalibrationPhase;
```

<a id="type-positionsensor-t"></a>

## `positionSensor_t` — Private complete sample/history

[Definition: position_sensor.c:19](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/position_sensor.c#L19)

initialized records successful startup; as5600Handle points to static device storage. last_sample_us wraps at 16 bits; last_angle_data is the previous 12-bit count. velocity_dps/velocity_radps are rates; angle_rad/angle_deg current unwrapped position; angle_prev_rad/deg hold values used on the next update; full_rotation_offset_rad accumulates turn crossings. A successful update builds a local next value and publishes it atomically.

```c
typedef struct {

    bool initialized;

    // AS5600 handle
    AS5600_TypeDef *as5600Handle;

    // common sensor values
    uint16_t last_sample_us;
    uint16_t last_angle_data;
    float velocity_dps;
    float velocity_radps;

    float angle_rad;
    float angle_deg;

    float angle_prev_rad;
    float angle_prev_deg;

    float full_rotation_offset_rad;

} positionSensor_t;
```

<a id="type-e-packet-state"></a>

## `e_packet_state` — Private serial byte-parser stage

[Definition: protocol.c:252](../../01-Firmware/B-G431B-ESC1-v3/Core/Src/protocol.c#L252)

HEADER1/2/3 and RESERVED collect the prefix; ID and LENGTH1/2 establish destination/size; PAYLOAD gathers instruction/parameters; CRC1/2 finish/check integrity. Global packet_state belongs to the foreground parser. Not a motor operating state.

```c
typedef enum {
    HEADER1,HEADER2,HEADER3,RESERVED,ID,LENGTH1,LENGTH2,PAYLOAD,CRC1,CRC2
} e_packet_state;
```

## Private owners without named typedefs

| Owner | Stored state and why it exists |
| --- | --- |
| App's anonymous static struct | Borrowed handles, servo/service deadlines, loop counters and button-edge history. One foreground owner. |
| Drive's anonymous static struct | armed/maintenance, last accepted command tick, generation, accepted goal and measured snapshot. Authoritative permission/command owner. |
| ServoController's anonymous static struct | PD context, filtered velocity error, applied generation and whether currents were applied. Separates control history from arming policy. |
| Calibration's anonymous static struct | phase/timestamp/step, captured angles/poles, original/working alignment and one pending result. Not shared as a wire struct. |
| FOC static variables | IDLE=0, TORQUE_CONTROL=1, FLUX_CONTROL=10; current/voltage/angle targets, PI contexts, ADC pair mask, phase offsets/observations and timing. Different from DriveState. |
| CAN and protocol buffers | Private CAN headers/data; serial framing uses global packet buffers/state owned by protocol. These buffers do not own normal servo permission. |
| PWM helper | phase_voltage_pending delays output enable until safe preloads have had a natural PWM cycle. Not an ARM flag. |

## Vendor and C types you will encounter

| Type/family | Meaning in this project |
| --- | --- |
| bool | true/false. A query/start returning true need not mean the motor is energized or the asynchronous job finished. |
| uint8_t / int16_t / uint16_t / uint32_t | Fixed-width unsigned/signed integers. Register bytes use uint8_t; signed targets/gains use explicit conversion. Unsigned time counters wrap. |
| float | Single-precision measurements/controller arithmetic. Physical units come from the name/API, not the C type. |
| HAL_StatusTypeDef | HAL_OK=zero means success; error/busy/timeout are failures or other states. Compare explicitly with HAL_OK. |
| TIM_HandleTypeDef | Borrowed timer identity/configuration. htim1 is PWM/ADC trigger; htim6 is 1 MHz foreground/sample timing. |
| ADC_HandleTypeDef / DMA_HandleTypeDef | ADC conversion and DMA stream identities/configuration. DMA writes sample buffers before HAL completion callbacks. |
| I2C_HandleTypeDef | Encoder bus handle hi2c1. Not an AS5600 sample. |
| FDCAN_HandleTypeDef / FDCAN_RxHeaderTypeDef / FDCAN_TxHeaderTypeDef | CAN peripheral and frame metadata; payload bytes are separate. IdType, frame type and DLC must be checked before command acceptance. |
| UART_HandleTypeDef | UART peripheral handle huart2, distinct from project HAL_Serial_Handler buffering/ownership. |
| CORDIC_HandleTypeDef / CORDIC_ConfigTypeDef | Accelerator runtime state/configuration. These are not the AS5600 mag_reg_t register view. |
| OPAMP_HandleTypeDef | Current-sense op-amp identity/configuration, initialized and started before active FOC. |
| GPIO_TypeDef / GPIO_PinState | Port-register type and high/low value. Board mapping lives in main.h; GPIO pin masks are macros. |
| RCC_*, GPIO_InitTypeDef, ADC_ChannelConfTypeDef, TIM_*, DMA config records | Temporary HAL initialization records in generated startup code. Start from the relevant MX function rather than treating them as application state. |
| FLASH_EraseInitTypeDef | Page-erasure request built by eeprom_store. Not a persistent configuration record. |
| pid_context_t pointers / const pointers | Caller-owned objects passed by address; const protects writes through that pointer, not concurrent state changes. |
| ptrdiff_t, size_t, struct stat, struct tms | C-library memory/count/syscall types in support code, not motion-control data. |

The vendor definitions live in the selected [HAL headers](../../01-Firmware/B-G431B-ESC1-v3/Drivers/STM32G4xx_HAL_Driver/Inc)
and [CMSIS headers](../../01-Firmware/B-G431B-ESC1-v3/Drivers/CMSIS/Include).

## Unit dictionary

All application position/speed values refer to the motor shaft, not the throttle
pulley. No final external gear ratio is encoded.

| Name/unit | Interpretation |
| --- | --- |
| deg / rad | Motor angle in degrees/radians. Electrical angle additionally applies pole pairs, direction, alignment and trim. |
| position_tenths_deg | Signed integer tenths of a motor degree; 123 means 12.3 degrees. |
| dps / radps | Degrees/second or radians/second; sensor velocity is a rate, not another angle. |
| mA / V / C | Milliamps, bus volts, degrees Celsius. Iq/Id are current components, not direct torque/flux sensor units. |
| us / ms | Microseconds from TIM6 / milliseconds from HAL tick. They are different counters. |
| raw | Encoded gain/setting before the documented scale conversion. Never infer physical gain directly from the stored integer. |
| Q31 | Signed fixed-point format used at the CORDIC boundary; result conversion and angle conversion differ. |
| register address / raw count | Byte offset in regs or AS5600 count. Neither is automatically a physical angle or pointer. |
