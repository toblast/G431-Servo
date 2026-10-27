#include "can_servo.h"
#include "drive.h"
#include "control_table.h"
#include "binary_tool.h"
#include "telemetry.h"
#include "foc.h"
#include "drive_mode.h"
#include <stddef.h>

static FDCAN_HandleTypeDef *can_handle;
static FDCAN_RxHeaderTypeDef rx_header;
// HAL copies the DLC-derived byte count before software validation, including
// up to 64 bytes for an FD frame that the command protocol will then reject.
static uint8_t rx_data[64];
static FDCAN_TxHeaderTypeDef tx_header;
static uint8_t tx_data[8];

void CanServo_Init(FDCAN_HandleTypeDef *hfdcan)
{
    tx_header = (FDCAN_TxHeaderTypeDef){
        .Identifier = 0,
        .IdType = FDCAN_STANDARD_ID,
        .TxFrameType = FDCAN_DATA_FRAME,
        .DataLength = FDCAN_DLC_BYTES_8,
        .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
        .BitRateSwitch = FDCAN_BRS_OFF,
        .FDFormat = FDCAN_CLASSIC_CAN,
        .TxEventFifoControl = FDCAN_NO_TX_EVENTS,
        .MessageMarker = 0
    };
    can_handle = hfdcan;
}

void CanServo_OnRxFifo0(FDCAN_HandleTypeDef *hfdcan, uint32_t events)
{
    if (can_handle == NULL || hfdcan != can_handle ||
        (events & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) == 0)
        return;

    // Bound interrupt work to messages already queued at entry.
    uint32_t remaining = HAL_FDCAN_GetRxFifoFillLevel(hfdcan, FDCAN_RX_FIFO0);
    while (remaining-- != 0) {
        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rx_header, rx_data) != HAL_OK)
            break;

        // The command protocol accepts one classic standard data-frame format.
        if (rx_header.Identifier != regs[REG_ID] ||
            rx_header.IdType != FDCAN_STANDARD_ID ||
            rx_header.RxFrameType != FDCAN_DATA_FRAME ||
            rx_header.FDFormat != FDCAN_CLASSIC_CAN ||
            rx_header.BitRateSwitch != FDCAN_BRS_OFF ||
            rx_header.DataLength != FDCAN_DLC_BYTES_8)
            continue;

        bool const disarm = (rx_data[0] == 0) && (rx_data[1] == 0) &&
            (rx_data[2] == 0) && (rx_data[3] == 0) &&
            (rx_data[4] == 0) && (rx_data[5] == 0) &&
            (rx_data[6] == 0) && (rx_data[7] == 0);
#if DRIVE_STANDALONE
        // Defensive guard even if a caller starts CAN in a custom build.
        // Local cruise is the only ARM/goal source; DISARM remains acceptable.
        if (!disarm) continue;
#endif
        if (API_FOC_Is_Calibrating() && !disarm) continue;
        if (disarm) {
            Drive_Disarm();
        } else if ((rx_data[0] == 0xFF) && (rx_data[1] == 0xFF) &&
                   (rx_data[2] == 0xFF) && (rx_data[3] == 0xFF) &&
                   (rx_data[4] == 0xFF) && (rx_data[5] == 0xFF) &&
                   (rx_data[6] == 0xFF) && (rx_data[7] == 0xFF)) {
            if (!Drive_RequestArm()) continue;
        } else {
            ServoCommand const command = {
                .position_tenths_deg = (int16_t)MAKE_SHORT(rx_data[0], rx_data[1]),
                .velocity_dps = (int16_t)MAKE_SHORT(rx_data[2], rx_data[3]),
                .torque_mA = (int16_t)MAKE_SHORT(rx_data[4], rx_data[5]),
                .position_kp_raw = rx_data[6],
                .position_kd_raw = rx_data[7]
            };
            if (!Drive_SubmitCommand(&command)) continue;
        }

        tx_header.Identifier = 0x10 + regs[REG_ID];
        tx_header.DataLength = FDCAN_DLC_BYTES_4;
        DriveStatus const status = Drive_GetStatus();
        int16_t const position = telemetry_i16(status.measured.position_deg * 10.0f);
        int16_t const torque = telemetry_i16(status.measured.torque_mA);
        tx_data[0] = LOW_BYTE(position);
        tx_data[1] = HIGH_BYTE(position);
        tx_data[2] = LOW_BYTE(torque);
        tx_data[3] = HIGH_BYTE(torque);
        HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, &tx_header, tx_data);
    }
}
