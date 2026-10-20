#ifndef CAN_TEST_HAL_H
#define CAN_TEST_HAL_H

#include <stdint.h>

typedef enum { HAL_OK = 0, HAL_ERROR = 1 } HAL_StatusTypeDef;
typedef struct { unsigned unused; } FDCAN_HandleTypeDef;
typedef struct {
    uint32_t Identifier, IdType, RxFrameType, DataLength;
    uint32_t ErrorStateIndicator, BitRateSwitch, FDFormat;
    uint32_t RxTimestamp, FilterIndex, IsFilterMatchingFrame;
} FDCAN_RxHeaderTypeDef;
typedef struct {
    uint32_t Identifier, IdType, TxFrameType, DataLength;
    uint32_t ErrorStateIndicator, BitRateSwitch, FDFormat;
    uint32_t TxEventFifoControl, MessageMarker;
} FDCAN_TxHeaderTypeDef;

#define FDCAN_STANDARD_ID 0x00000000U
#define FDCAN_EXTENDED_ID 0x40000000U
#define FDCAN_DATA_FRAME 0x00000000U
#define FDCAN_REMOTE_FRAME 0x20000000U
#define FDCAN_CLASSIC_CAN 0x00000000U
#define FDCAN_FD_CAN 0x00200000U
#define FDCAN_BRS_OFF 0x00000000U
#define FDCAN_BRS_ON 0x00100000U
#define FDCAN_DLC_BYTES_4 0x00040000U
#define FDCAN_DLC_BYTES_8 0x00080000U
#define FDCAN_IT_RX_FIFO0_NEW_MESSAGE 1U
#define FDCAN_RX_FIFO0 0U

uint32_t HAL_GetTick(void);
uint32_t HAL_FDCAN_GetRxFifoFillLevel(FDCAN_HandleTypeDef *, uint32_t);
HAL_StatusTypeDef HAL_FDCAN_GetRxMessage(FDCAN_HandleTypeDef *, uint32_t,
                                       FDCAN_RxHeaderTypeDef *, uint8_t *);
HAL_StatusTypeDef HAL_FDCAN_AddMessageToTxFifoQ(FDCAN_HandleTypeDef *,
                                             FDCAN_TxHeaderTypeDef *, uint8_t *);

/* Single-threaded host equivalents for short Cortex-M critical sections. */
extern uint32_t test_primask;
static inline uint32_t __get_PRIMASK(void) { return test_primask; }
static inline void __disable_irq(void) { test_primask = 1; }
static inline void __set_PRIMASK(uint32_t mask) { test_primask = mask; }
static inline void __enable_irq(void) { }

#endif
