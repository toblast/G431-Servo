#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "control_table.h"
#include "binary_tool.h"

uint8_t regs[REG_MAX];
FDCAN_HandleTypeDef hfdcan1;
uint32_t test_primask;

static unsigned failures;
static unsigned assertions;
static unsigned tx_count;
static unsigned disable_count;
static unsigned enable_count, current_updates;
static float published_torque, published_flux;
static unsigned pending_rx;
static unsigned receive_buffer_too_small;
static unsigned rx_read_calls;
static unsigned fail_rx_read;
static unsigned refill_rx;
static uint32_t mock_tick;
static uint32_t mock_foc_tick;
static bool sensor_ready = true;
static bool mock_calibrating;
static float mock_position_deg = 90.0f;
static FDCAN_RxHeaderTypeDef pending_header;
static uint8_t pending_data[64];

void API_FOC_Torque_Disable(void) { ++disable_count; }
void API_FOC_Torque_Enable(void) { ++enable_count; }
void API_FOC_Set_Torque_Flux_Currents_mA(float torque, float flux) {
    if (test_primask != 1) ++failures;
    ++current_updates;
    published_torque = torque;
    published_flux = flux;
}
int positionSensor_isReady(void) { return sensor_ready; }
bool API_FOC_Is_Calibrating(void) { return mock_calibrating; }
float positionSensor_getDegreeMultiturn(void) { return mock_position_deg; }
uint32_t HAL_GetTick(void) { return mock_tick; }
uint32_t API_FOC_Get_Timestamp_ms(void) {
    if (test_primask != 1) ++failures;
    return mock_foc_tick;
}
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
