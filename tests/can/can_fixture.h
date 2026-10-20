#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "control_table.h"
#include "binary_tool.h"

uint8_t regs[REG_MAX];
FDCAN_HandleTypeDef hfdcan1;

static unsigned failures;
static unsigned assertions;
static unsigned tx_count;
static unsigned disable_count;
static unsigned pending_rx;
static unsigned receive_buffer_too_small;
static unsigned rx_read_calls;
static unsigned fail_rx_read;
static unsigned refill_rx;
static uint32_t mock_tick;
static bool sensor_ready = true;
static float mock_position_deg = 90.0f;
static FDCAN_RxHeaderTypeDef pending_header;
static uint8_t pending_data[64];

void API_FOC_Torque_Disable(void) { ++disable_count; }
int positionSensor_isReady(void) { return sensor_ready; }
float positionSensor_getDegreeMultiturn(void) { return mock_position_deg; }
uint32_t HAL_GetTick(void) { return mock_tick; }
uint32_t HAL_FDCAN_GetRxFifoFillLevel(FDCAN_HandleTypeDef *handle, uint32_t fifo)
{
    (void)handle;
    (void)fifo;
    return pending_rx;
}
HAL_StatusTypeDef HAL_FDCAN_AddMessageToTxFifoQ(FDCAN_HandleTypeDef *handle,
                                              FDCAN_TxHeaderTypeDef *header,
                                              uint8_t *data)
{
    (void)handle;
    (void)header;
    (void)data;
    ++tx_count;
    return HAL_OK;
}

#define CHECK(condition, label) do { \
    ++assertions; \
    if (!(condition)) { ++failures; printf("FAIL: %s (line %u)\n", label, (unsigned)__LINE__); } \
} while (0)
