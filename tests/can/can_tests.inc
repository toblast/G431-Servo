/* This HAL mock follows the bundled driver's DLC-to-byte conversion. It checks
 * the production receive allocation before copying, so a regression reports a
 * failure without deliberately corrupting the host test process. */
HAL_StatusTypeDef HAL_FDCAN_GetRxMessage(FDCAN_HandleTypeDef *handle, uint32_t fifo,
                                        FDCAN_RxHeaderTypeDef *header, uint8_t *data)
{
    static const uint8_t dlc_to_bytes[16] = {0,1,2,3,4,5,6,7,8,12,16,20,24,32,48,64};
    (void)handle;
    (void)fifo;
    ++rx_read_calls;
    /* Keep the baseline's unbounded loop finite so it reports a test failure. */
    if (rx_read_calls > 100) { pending_rx = 0; return HAL_ERROR; }
    if (fail_rx_read) return HAL_ERROR;
    if (!pending_rx) return HAL_ERROR;
    --pending_rx;
    if (refill_rx) ++pending_rx;
    *header = pending_header;
    size_t count = dlc_to_bytes[(header->DataLength >> 16U) & 15U];
    if (count > sizeof(RxData)) {
        receive_buffer_too_small = 1;
        count = sizeof(RxData);
    }
    memcpy(data, pending_data, count);
    return HAL_OK;
}

static void reset_fixture(bool armed)
{
    memset(regs, 0, sizeof(regs));
    memset(pending_data, 0x5A, sizeof(pending_data));
    regs[REG_ID] = 1;
    regs[REG_TORQUE_ENABLE] = armed ? 1 : 0;
    can_armed = armed;
    can_last_time = 17;
    mock_tick = 300;
    sensor_ready = true;
    tx_count = disable_count = pending_rx = receive_buffer_too_small = 0;
    rx_read_calls = fail_rx_read = refill_rx = 0;
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
    CHECK(can_last_time == 17, "rejected frame does not refresh watchdog");
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
    CHECK(can_last_time == 17, "disarmed command does not refresh watchdog");
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
    CHECK(can_last_time == 17, "rejected ARM does not start watchdog");

    reset_fixture(false);
    memset(pending_data, 0xFF, 8);
    regs[REG_HARDWARE_ERROR_STATUS] = 1U << HW_ERROR_BIT_POSITION_SENSOR_STATUS_ERROR;
    receive();
    CHECK(!can_armed && !regs[REG_TORQUE_ENABLE], "sensor fault prevents ARM");
    CHECK(can_last_time == 17, "faulted ARM does not start watchdog");
}

static void test_irq_bounds(void)
{
    reset_fixture(true);
    pending_rx = 1;
    fail_rx_read = 1;
    HAL_FDCAN_RxFifo0Callback(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE);
    CHECK(rx_read_calls == 1, "HAL receive failure terminates interrupt work");
    CHECK(can_last_time == 17, "HAL receive failure does not refresh watchdog");

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

int main(int argc, char **argv)
{
    const char *suite = argc > 1 ? argv[1] : "all";
    if (strcmp(suite, "all") == 0 || strcmp(suite, "strict") == 0) {
        test_frame_validation();
        test_disarmed_and_arm();
        test_full_command();
        test_arm_requires_healthy_encoder();
        test_irq_bounds();
    }
    printf("CAN callback: %u assertions, %u failures\n", assertions, failures);
    return failures ? 1 : 0;
}
