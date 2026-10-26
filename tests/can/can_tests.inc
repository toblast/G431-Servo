/* This HAL mock follows the bundled driver's DLC-to-byte conversion. It checks
 * the production receive allocation before copying, so a regression reports a
 * failure without deliberately corrupting the host test process. */
HAL_StatusTypeDef test_can_get_rx_message(FDCAN_HandleTypeDef *handle, uint32_t fifo,
                                        FDCAN_RxHeaderTypeDef *header, uint8_t *data,
                                        size_t capacity)
{
    static const uint8_t dlc_to_bytes[16] = {0,1,2,3,4,5,6,7,8,12,16,20,24,32,48,64};
    if (handle != expected_can_handle || fifo != FDCAN_RX_FIFO0) ++failures;
    ++rx_read_calls;
    /* Keep the baseline's unbounded loop finite so it reports a test failure. */
    if (rx_read_calls > 100) { pending_rx = 0; return HAL_ERROR; }
    if (fail_rx_read) return HAL_ERROR;
    if (!pending_rx) return HAL_ERROR;
    --pending_rx;
    if (refill_rx) ++pending_rx;
    *header = pending_header;
    size_t count = dlc_to_bytes[(header->DataLength >> 16U) & 15U];
    receive_capacity = capacity;
    if (capacity == (size_t)-1 || count > capacity) {
        receive_buffer_too_small = 1;
        count = capacity == (size_t)-1 ? 0 : capacity;
    }
    memcpy(data, pending_data, count);
    return HAL_OK;
}

static void reset_fixture(bool armed)
{
    memset(regs, 0, sizeof(regs));
    factory_reset_eeprom_regs();
    memset(pending_data, 0x5A, sizeof(pending_data));
    regs[REG_ID] = 1;
    regs[REG_MIN_POSITION_DEG_L] = LOW_BYTE(-180);
    regs[REG_MIN_POSITION_DEG_H] = HIGH_BYTE(-180);
    regs[REG_MAX_POSITION_DEG_L] = LOW_BYTE(180);
    regs[REG_MAX_POSITION_DEG_H] = HIGH_BYTE(180);
    mock_position_deg = 90;
    mock_tick = 300;
    mock_foc_tick = mock_tick;
    test_primask = 0;
    sensor_ready = true;
    mock_calibrating = false;
    Drive_Init();
    CanServo_Init(&hfdcan1);
    expected_can_handle = &hfdcan1;
    if (armed) { mock_tick = 17; Drive_Request_Arm(); }
    initial_command_ms = can_last_time;
    mock_tick = mock_foc_tick = 300;
    tx_count = disable_count = pending_rx = receive_buffer_too_small = 0;
    rx_read_calls = fail_rx_read = refill_rx = 0;
    fifo_fill_calls = 0;
    receive_capacity = 0;
    tx_status = HAL_OK;
    enable_count = current_updates = 0;
    pending_header = (FDCAN_RxHeaderTypeDef){
        .Identifier = 1,
        .IdType = FDCAN_STANDARD_ID,
        .RxFrameType = FDCAN_DATA_FRAME,
        .DataLength = FDCAN_DLC_BYTES_8,
        .FDFormat = FDCAN_CLASSIC_CAN,
        .BitRateSwitch = FDCAN_BRS_OFF,
    };
}

static void receive(void)
{
    pending_rx = 1;
    HAL_FDCAN_RxFifo0Callback(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE);
    CHECK(!pending_rx, "RX FIFO was consumed");
    CHECK(!receive_buffer_too_small, "receive allocation accommodates HAL DLC copy");
}

static void expect_rejected(const char *label)
{
    uint8_t previous[REG_MAX];
    memcpy(previous, regs, sizeof(previous));
    receive();
    CHECK(memcmp(previous, regs, sizeof(previous)) == 0, label);
    CHECK(can_armed, "rejected frame preserves armed state");
    CHECK(can_last_time == initial_command_ms, "rejected frame does not refresh watchdog");
    CHECK(tx_count == 0, "rejected frame does not receive acceptance feedback");
}

static void test_frame_validation(void)
{
    for (uint32_t dlc = 0; dlc != 16; ++dlc) {
        if (dlc == 8) continue;
        reset_fixture(true);
        pending_header.DataLength = dlc << 16U;
        expect_rejected("unsupported DLC does not change command registers");
    }
    reset_fixture(true);
    pending_header.Identifier = 2;
    expect_rejected("wrong identifier rejected");
    reset_fixture(true);
    pending_header.IdType = FDCAN_EXTENDED_ID;
    expect_rejected("extended identifier rejected");
    reset_fixture(true);
    pending_header.RxFrameType = FDCAN_REMOTE_FRAME;
    expect_rejected("remote frame rejected");
    reset_fixture(true);
    pending_header.FDFormat = FDCAN_FD_CAN;
    expect_rejected("FD frame rejected");
    reset_fixture(true);
    pending_header.BitRateSwitch = FDCAN_BRS_ON;
    expect_rejected("bit rate switching rejected");
}

static void test_disarmed_and_arm(void)
{
    reset_fixture(false);
    receive();
    CHECK(!can_armed && !regs[REG_TORQUE_ENABLE], "ordinary command cannot arm");
    CHECK(can_last_time == initial_command_ms, "disarmed command does not refresh watchdog");
    CHECK(regs[REG_GOAL_POSITION_DEG_L] == 0, "disarmed command is ignored");

    reset_fixture(false);
    memset(pending_data, 0xFF, 8);
    receive();
    CHECK(can_armed && regs[REG_TORQUE_ENABLE], "explicit ARM enables command acceptance");
    CHECK(can_last_time == mock_tick, "accepted ARM starts watchdog");
    CHECK(regs[REG_GOAL_POS_KP] == 0 && regs[REG_GOAL_POS_KD] == 0,
          "ARM starts with zero position gains");
    CHECK(regs[REG_GOAL_TORQUE_CURRENT_MA_L] == 0 &&
          regs[REG_GOAL_TORQUE_CURRENT_MA_H] == 0, "ARM starts with zero feedforward");
}

static void test_full_command(void)
{
    static const uint8_t command[8] = {0x34,0x12,0xFE,0xFF,0x78,0x56,3,4};
    reset_fixture(true);
    memcpy(pending_data, command, sizeof(command));
    receive();
    CHECK(regs[REG_GOAL_POSITION_DEG_L] == command[0] &&
          regs[REG_GOAL_POSITION_DEG_H] == command[1], "position decoded little endian");
    CHECK(regs[REG_GOAL_VELOCITY_DPS_L] == command[2] &&
          regs[REG_GOAL_VELOCITY_DPS_H] == command[3], "velocity decoded little endian");
    CHECK(regs[REG_GOAL_TORQUE_CURRENT_MA_L] == command[4] &&
          regs[REG_GOAL_TORQUE_CURRENT_MA_H] == command[5], "feedforward decoded little endian");
    CHECK(regs[REG_GOAL_POS_KP] == 3 && regs[REG_GOAL_POS_KD] == 4, "gains decoded");
    CHECK(can_last_time == mock_tick, "accepted full command refreshes watchdog");
    CHECK(can_armed && regs[REG_TORQUE_ENABLE], "full command preserves active state");
}

static void test_arm_requires_healthy_encoder(void)
{
    reset_fixture(false);
    memset(pending_data, 0xFF, 8);
    sensor_ready = false;
    receive();
    CHECK(!can_armed && !regs[REG_TORQUE_ENABLE], "uninitialized encoder cannot arm");
    CHECK(can_last_time == initial_command_ms, "rejected ARM does not start watchdog");

    reset_fixture(false);
    memset(pending_data, 0xFF, 8);
    regs[REG_HARDWARE_ERROR_STATUS] = 1U << HW_ERROR_BIT_POSITION_SENSOR_STATUS_ERROR;
    receive();
    CHECK(!can_armed && !regs[REG_TORQUE_ENABLE], "sensor fault prevents ARM");
    CHECK(can_last_time == initial_command_ms, "faulted ARM does not start watchdog");

    reset_fixture(false);
    memset(pending_data, 0xFF, 8);
    mock_calibrating = true;
    receive();
    CHECK(!can_armed && !regs[REG_TORQUE_ENABLE], "calibration rejects ARM until cleanup ends");
    memset(pending_data, 0, 8);
    receive();
    CHECK(disable_count == 1, "DISARM remains accepted during calibration");

    reset_fixture(false);
    memset(pending_data, 0xFF, 8);
    regs[REG_ENCODER_BITS] = 0;
    receive();
    CHECK(!can_armed && !regs[REG_TORQUE_ENABLE], "invalid configuration prevents ARM");
    CHECK(hardware_fault_status() & (1U << HW_ERROR_BIT_CONFIGURATION), "configuration fault is recorded");
}

static void test_irq_bounds(void)
{
    reset_fixture(true);
    pending_rx = 1;
    fail_rx_read = 1;
    HAL_FDCAN_RxFifo0Callback(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE);
    CHECK(rx_read_calls == 1, "HAL receive failure terminates interrupt work");
    CHECK(can_last_time == initial_command_ms, "HAL receive failure does not refresh watchdog");

    reset_fixture(true);
    pending_rx = 2;
    refill_rx = 1;
    HAL_FDCAN_RxFifo0Callback(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE);
    CHECK(rx_read_calls == 2, "interrupt drains only FIFO entries present at entry");
    CHECK(pending_rx == 2, "new arrivals remain for a subsequent interrupt");

    reset_fixture(true);
    pending_rx = 1;
    HAL_FDCAN_RxFifo0Callback(&hfdcan1, 0);
    CHECK(!rx_read_calls && pending_rx == 1, "unrelated IRQ flags do not consume FIFO");

    reset_fixture(true);
    pending_rx = 1;
    FDCAN_HandleTypeDef other_handle = {0};
    HAL_FDCAN_RxFifo0Callback(&other_handle, FDCAN_IT_RX_FIFO0_NEW_MESSAGE);
    CHECK(!rx_read_calls && pending_rx == 1, "unrelated controller does not consume FIFO");
}

static void test_disarm_and_rearm(void)
{
    reset_fixture(false);
    memset(pending_data, 0xFF, 8);
    receive();
    CHECK(can_arm_generation == 1, "ARM identifies a new enable transition");
    CHECK((int16_t)MAKE_SHORT(regs[REG_GOAL_POSITION_DEG_L], regs[REG_GOAL_POSITION_DEG_H]) == 900,
          "ARM holds measured position");

    memset(pending_data, 1, 8);
    receive();
    uint8_t command[REG_PRESENT_POSITION_DEG_L - REG_GOAL_POSITION_DEG_L];
    memcpy(command, &regs[REG_GOAL_POSITION_DEG_L], sizeof command);
    memset(pending_data, 0xFF, 8);
    receive();
    CHECK(can_arm_generation == 1, "duplicate ARM is idempotent");
    CHECK(memcmp(command, &regs[REG_GOAL_POSITION_DEG_L], sizeof command) == 0,
          "duplicate ARM preserves accepted goals");

    memset(pending_data, 0, 8);
    receive();
    CHECK(!can_armed && !regs[REG_TORQUE_ENABLE], "zero DISARM disables command acceptance");
    CHECK(disable_count == 1, "DISARM disables FOC immediately");
    for (unsigned i = REG_GOAL_POSITION_DEG_L; i < REG_PRESENT_POSITION_DEG_L; ++i)
        CHECK(regs[i] == 0, "DISARM clears every command field");

    memset(pending_data, 0x5A, 8);
    receive();
    CHECK(!can_armed && !regs[REG_TORQUE_ENABLE], "ordinary command cannot rearm");
    memset(pending_data, 0xFF, 8);
    receive();
    CHECK(can_armed && can_arm_generation == 2, "explicit rearm creates a fresh transition");
}

static void test_watchdog_rollover_and_boundaries(void)
{
    reset_fixture(true);
    mock_tick = mock_foc_tick = 1017;
    Drive_Service();
    CHECK(can_armed, "CAN remains armed at the exact timeout boundary");
    mock_tick = mock_foc_tick = 1018;
    Drive_Service();
    CHECK(!can_armed && disable_count == 1, "CAN expiry disables immediately");

    reset_fixture(true);
    mock_tick = UINT32_MAX - 200; Drive_Request_Arm();
    mock_tick = mock_foc_tick = UINT32_MAX - 100;
    Drive_Service();
    CHECK(can_armed, "CAN does not expire prematurely before rollover");
    mock_tick = UINT32_MAX - 500; Drive_Request_Arm();
    mock_tick = mock_foc_tick = 100;
    Drive_Service();
    CHECK(can_armed, "CAN elapsed time remains valid after rollover");
    mock_tick = mock_foc_tick = 600;
    Drive_Service();
    CHECK(!can_armed, "CAN expires across rollover");

    reset_fixture(true);
    mock_tick = 4; Drive_Request_Arm();
    mock_foc_tick = UINT32_MAX - 5;
    Drive_Service();
    CHECK(can_armed, "FOC remains healthy at ten milliseconds across rollover");
    mock_tick = 5;
    Drive_Service();
    CHECK(!can_armed, "FOC timeout disables across rollover");
    CHECK(regs[REG_HARDWARE_ERROR_STATUS] & (1U << HW_ERROR_BIT_FOC_TIMEOUT),
          "FOC timeout records a fault");
    CHECK(test_primask == 0, "watchdog snapshot restores IRQ state");
}

static void test_command_snapshot_and_stale_publication(void)
{
    reset_fixture(true);
    ServoCommand const goal = { .position_tenths_deg = 0x1234, .torque_mA = -2 };
    Drive_Submit_Command(&goal);
    DriveCommandSnapshot const command = Drive_Get_Command();
    CHECK(command.enabled && command.goal.position_tenths_deg == 0x1234 && command.goal.torque_mA == -2,
          "foreground takes a signed coherent command snapshot");
    regs[REG_GOAL_POSITION_DEG_L] = regs[REG_GOAL_POSITION_DEG_H] = 0;
    CHECK(command.goal.position_tenths_deg == 0x1234, "later RX does not alter snapshot");
    CHECK(Drive_Get_Command().goal.position_tenths_deg == 0x1234, "register mirrors do not own the live command");
    CHECK(Drive_Publish_Currents(&command, true, 123, -45), "current snapshot can publish");
    CHECK(enable_count == 1 && current_updates == 1 && published_torque == 123 && published_flux == -45,
          "new ARM initializes FOC and publishes the complete current pair");

    Drive_Disarm();
    CHECK(!Drive_Publish_Currents(&command, true, 999, 999), "DISARM rejects an in-flight calculation");
    memset(pending_data, 0xFF, 8);
    receive();
    CHECK(!Drive_Publish_Currents(&command, true, 999, 999), "rearm rejects the preceding ARM generation");
    CHECK(current_updates == 1, "stale computations never publish");
    DriveCommandSnapshot const rearmed = Drive_Get_Command();
    CHECK(Drive_Publish_Currents(&rearmed, true, 12, 0), "fresh rearm generation publishes");
    CHECK(enable_count == 2 && current_updates == 2, "rearm resets FOC exactly once");
}

static void test_typed_views_and_supervisor_policy(void)
{
    reset_fixture(false);
    DriveStatus status = Drive_Get_Status();
    CHECK(status.state == DRIVE_DISARMED && !status.armed, "supervisor starts disarmed");
    DriveConfig const config = control_read_config();
    CHECK(config.min_position_deg == -180 && config.max_position_deg == 180 && config.max_current_mA == 500,
          "configuration adapter preserves signed units and bench defaults");

    DriveMeasurements measurements = {
        .position_deg = 123.45f, .velocity_dps = -20.75f,
        .torque_mA = -500, .flux_mA = 123, .voltage_V = 24, .temperature_C = 30, .moving = true
    };
    Drive_Publish_Measurements(&measurements);
    measurements.position_deg = 0;
    status = Drive_Get_Status();
    CHECK(fabsf(status.measured.position_deg - 123.45f) < 0.001f && status.measured.torque_mA == -500,
          "status owns a copy of typed measurements");
    CHECK((int16_t)MAKE_SHORT(regs[REG_PRESENT_POSITION_DEG_L], regs[REG_PRESENT_POSITION_DEG_H]) == 1234,
          "typed status retains legacy telemetry scaling");
    memset(pending_data, 0, 8);
    receive();
    CHECK(last_reply_id == 0x11 && (int16_t)MAKE_SHORT(last_reply[0], last_reply[1]) == 1234 &&
          (int16_t)MAKE_SHORT(last_reply[2], last_reply[3]) == -500,
          "typed status produces the unchanged four-byte CAN reply");

    CHECK(Drive_Begin_Maintenance(false), "disarmed maintenance entry succeeds");
    CHECK(Drive_Get_Status().state == DRIVE_MAINTENANCE && Drive_Is_Maintenance(), "maintenance has one owner");
    CHECK(!Drive_Request_Arm(), "maintenance blocks ARM");
    CHECK(hardware_fault_status() == (1U << HW_ERROR_BIT_CONFIGURATION), "rejected maintenance ARM retains fault semantics");
    Drive_End_Maintenance();
    hardware_fault_clear(1U << HW_ERROR_BIT_CONFIGURATION);
    CHECK(Drive_Request_Arm() && Drive_Get_Status().state == DRIVE_ARMED, "ARM succeeds after maintenance ends and fault clears");
    CHECK(!Drive_Begin_Maintenance(false) && can_armed, "ordinary configuration cannot disarm an active servo");
    calibration_calls = 0;
    CHECK(Drive_Run_Calibration() == 3 && calibration_calls == 0 && can_armed,
          "armed calibration request is rejected without touching FOC");
    CHECK(Drive_Begin_Maintenance(true) && !can_armed, "reset maintenance disarms before persistence");
    CHECK(Drive_Run_Calibration() == 3 && calibration_calls == 0, "maintenance rejects calibration");
    Drive_End_Maintenance();
    CHECK(Drive_Run_Calibration() == 0 && calibration_calls == 1, "disarmed calibration forwards through the supervisor");
    mock_calibrating = true;
    CHECK(Drive_Get_Status().state == DRIVE_CALIBRATING && !Drive_Is_Disarmed(), "FOC job state is visible through the supervisor");
    CHECK(!Drive_Begin_Maintenance(false), "calibration excludes configuration maintenance");
    mock_calibrating = false;
    CHECK(Drive_Request_Arm(), "arm before asynchronous fault");
    hardware_fault_set(1U << HW_ERROR_BIT_OVERLOAD);
    CHECK(Drive_Get_Status().state == DRIVE_FAULT && !Drive_Get_Command().enabled, "fault immediately inhibits command publication");
    Drive_Service();
    CHECK(!can_armed && !regs[REG_TORQUE_ENABLE], "foreground service revokes permission after a fault");
    hardware_fault_clear(0xff);
    CHECK(Drive_Get_Status().state == DRIVE_DISARMED, "fault clear does not automatically rearm");
    test_primask = 1;
    Drive_Disarm();
    CHECK(test_primask == 1, "supervisor preserves an existing critical section");
    test_primask = 0;
}

static void test_uninitialized_transport(void)
{
    // The module starts unbound, before any fixture calls CanServo_Init.
    pending_rx = 1;
    HAL_FDCAN_RxFifo0Callback(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE);
    HAL_FDCAN_RxFifo0Callback(NULL, FDCAN_IT_RX_FIFO0_NEW_MESSAGE);
    CHECK(pending_rx == 1 && fifo_fill_calls == 0 && rx_read_calls == 0,
          "uninitialized transport ignores callbacks before touching HAL");
    CHECK(tx_count == 0 && disable_count == 0, "uninitialized transport cannot change motor state or reply");
}

static void test_transport_binding_and_feedback(void)
{
    reset_fixture(true);
    FDCAN_HandleTypeDef other = {0};
    CanServo_Init(&other);
    expected_can_handle = &other;
    pending_rx = 1;
    HAL_FDCAN_RxFifo0Callback(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE);
    CHECK(pending_rx == 1 && fifo_fill_calls == 0 && rx_read_calls == 0,
          "transport uses its supplied handle rather than a global controller");
    HAL_FDCAN_RxFifo0Callback(&other, FDCAN_IT_RX_FIFO0_NEW_MESSAGE);
    CHECK(pending_rx == 0 && tx_count == 1 && receive_capacity == 64,
          "bound controller receives through private storage sized for all HAL DLCs");
    CHECK(last_reply_header.IdType == FDCAN_STANDARD_ID &&
          last_reply_header.TxFrameType == FDCAN_DATA_FRAME &&
          last_reply_header.DataLength == FDCAN_DLC_BYTES_4,
          "reply retains standard four-byte data framing");
    CHECK(last_reply_header.FDFormat == FDCAN_CLASSIC_CAN &&
          last_reply_header.BitRateSwitch == FDCAN_BRS_OFF &&
          last_reply_header.ErrorStateIndicator == FDCAN_ESI_ACTIVE &&
          last_reply_header.TxEventFifoControl == FDCAN_NO_TX_EVENTS &&
          last_reply_header.MessageMarker == 0,
          "reply retains classic CAN metadata initialized by the transport");

    reset_fixture(false);
    regs[REG_ID] = 7;
    pending_header.Identifier = 7;
    DriveMeasurements measurements = { .position_deg = -12.3f, .torque_mA = 456 };
    Drive_Publish_Measurements(&measurements);
    memset(pending_data, 0, 8);
    receive();
    CHECK(last_reply_id == 0x17 && last_reply[0] == 0x85 && last_reply[1] == 0xff &&
          last_reply[2] == 0xc8 && last_reply[3] == 0x01,
          "feedback retains configured ID, signed scaling and little-endian byte order");
    measurements.position_deg = INFINITY;
    measurements.torque_mA = -INFINITY;
    Drive_Publish_Measurements(&measurements);
    receive();
    CHECK(last_reply[0] == 0xff && last_reply[1] == 0x7f &&
          last_reply[2] == 0x00 && last_reply[3] == 0x80,
          "feedback saturates both signed fields before encoding");
    measurements.position_deg = measurements.torque_mA = NAN;
    Drive_Publish_Measurements(&measurements);
    receive();
    CHECK(last_reply[0] == 0 && last_reply[1] == 0 && last_reply[2] == 0 && last_reply[3] == 0,
          "feedback maps nonfinite NaN measurements to zero");

    reset_fixture(false);
    tx_status = HAL_ERROR;
    memset(pending_data, 0xff, 8);
    receive();
    CHECK(can_armed && can_last_time == mock_tick && tx_count == 1,
          "reply failure does not roll back an already accepted ARM");
    pending_rx = 2;
    memset(pending_data, 1, 8);
    HAL_FDCAN_RxFifo0Callback(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE);
    CHECK(pending_rx == 0 && rx_read_calls == 3 && tx_count == 3,
          "reply failure neither retries nor interrupts bounded receive draining");
    memset(pending_data, 0, 8);
    receive();
    CHECK(!can_armed && disable_count == 1 && tx_count == 4,
          "DISARM remains immediate when the reply cannot be queued");
}

int main(int argc, char **argv)
{
    test_uninitialized_transport();
    const float rejected_positions[] = { NAN, INFINITY, -INFINITY, 181, -181, 40000 };
    for (unsigned i = 0; i < sizeof rejected_positions / sizeof *rejected_positions; ++i) {
        reset_fixture(false);
        mock_position_deg = rejected_positions[i];
        memset(pending_data, 0xff, 8);
        receive();
        CHECK(!can_armed && !regs[REG_TORQUE_ENABLE], "ARM rejects invalid or out-of-limit initial position");
        CHECK(can_last_time == initial_command_ms && can_arm_generation == 0, "rejected ARM has no enable or watchdog effect");
    }
    const char *suite = argc > 1 ? argv[1] : "all";
    if (strcmp(suite, "all") == 0 || strcmp(suite, "strict") == 0) {
        test_frame_validation();
        test_disarmed_and_arm();
        test_full_command();
        test_arm_requires_healthy_encoder();
        test_irq_bounds();
        test_disarm_and_rearm();
        test_watchdog_rollover_and_boundaries();
        test_command_snapshot_and_stale_publication();
        test_typed_views_and_supervisor_policy();
        test_transport_binding_and_feedback();
    }
    printf("CAN callback: %u assertions, %u failures\n", assertions, failures);
    return failures ? 1 : 0;
}
