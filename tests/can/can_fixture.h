#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "control_table.h"
#include "configuration.h"
#include "binary_tool.h"
#include "foc.h"
#include "calibration.h"
#include "drive.h"
#include "telemetry.h"
#include "can_servo.h"


FDCAN_HandleTypeDef hfdcan1;
static FDCAN_HandleTypeDef *expected_can_handle = &hfdcan1;
uint32_t test_primask;

static unsigned failures;
static unsigned assertions;
static unsigned tx_count;
static uint8_t last_reply[4];
static uint32_t last_reply_id;
static FDCAN_TxHeaderTypeDef last_reply_header;
static HAL_StatusTypeDef tx_status;
static unsigned calibration_calls;
static unsigned disable_count;
static unsigned enable_count, current_updates;
static float published_torque, published_flux;
static unsigned pending_rx;
static unsigned receive_buffer_too_small;
static unsigned rx_read_calls;
static unsigned fifo_fill_calls;
static size_t receive_capacity;
static unsigned fail_rx_read;
static unsigned refill_rx;
static uint32_t mock_tick;
static uint32_t mock_foc_tick;
static bool sensor_ready = true;
static bool mock_calibrating;
static uint32_t initial_command_ms;
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
bool Calibration_Start(void) { ++calibration_calls; return true; }
HAL_StatusTypeDef eeprom_store(uint8_t const *data, uint32_t size) { (void)data; (void)size; return HAL_OK; }
HAL_StatusTypeDef eeprom_restore(uint8_t *data, uint32_t size) { (void)data; (void)size; return HAL_OK; }
float positionSensor_getDegreesMultiturn(void) { return mock_position_deg; }
uint32_t HAL_GetTick(void) { return mock_tick; }
uint32_t API_FOC_Get_Timestamp_ms(void) {
    if (test_primask != 1) ++failures;
    return mock_foc_tick;
}
uint32_t HAL_FDCAN_GetRxFifoFillLevel(FDCAN_HandleTypeDef *handle, uint32_t fifo)
{
    if (handle != expected_can_handle || fifo != FDCAN_RX_FIFO0) ++failures;
    ++fifo_fill_calls;
    return pending_rx;
}
HAL_StatusTypeDef HAL_FDCAN_AddMessageToTxFifoQ(FDCAN_HandleTypeDef *handle,
                                              FDCAN_TxHeaderTypeDef *header,
                                              uint8_t *data)
{
    if (handle != expected_can_handle) ++failures;
    last_reply_id = header->Identifier;
    last_reply_header = *header;
    memcpy(last_reply, data, sizeof last_reply);
    ++tx_count;
    return tx_status;
}

#define CHECK(condition, label) do { \
    ++assertions; \
    if (!(condition)) { ++failures; printf("FAIL: %s (line %u)\n", label, (unsigned)__LINE__); } \
} while (0)

// Observations through the public supervisor API; tests never include drive.c.
#define can_armed (Drive_Get_Status().armed)
#define can_arm_generation (Drive_Get_Status().generation)
#define can_last_time (Drive_Get_Status().last_command_ms)
