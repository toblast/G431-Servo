/*
 * serial.h
 *
 *  Created on: 29 nov. 2015
 *      Author: Patrick
 */

#ifndef HAL_SERIAL_H_
#define HAL_SERIAL_H_

#include "stm32g4xx_hal.h"
#include "stdbool.h"

/* HAL Public Data ------------------------------------------------------------------*/

#define SERIAL_TX_POOL_SIZE 32
#define SERIAL_TX_BUFFER_SIZE 256
#define SERIAL_RX_BUFFER_SIZE 2048


typedef struct
{
	uint8_t data[SERIAL_TX_BUFFER_SIZE];
	uint32_t length;
} tx_buffer;

typedef struct
{
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

#define HAL_Serial_Handler_Count 1

/* HAL Functions ------------------------------------------------------------------*/

void HAL_Serial_Init(UART_HandleTypeDef * huart, HAL_Serial_Handler * hserial);
void HAL_Serial_Init_Half_Duplex(
		UART_HandleTypeDef * huart,
		HAL_Serial_Handler * hserial,
		GPIO_TypeDef *GPIOx_TX,
		uint16_t GPIO_Pin_TX,
		GPIO_PinState PinState_TX,
		GPIO_TypeDef *GPIOx_RX,
		uint16_t GPIO_Pin_RX,
		GPIO_PinState PinState_RX
);
int HAL_Serial_Available(HAL_Serial_Handler * hserial);
char HAL_Serial_GetChar(HAL_Serial_Handler * hserial);
int HAL_Serial_Read(HAL_Serial_Handler * hserial, uint8_t * ptr, int len );
// Returns len when queued, or zero for invalid input, full queue or start failure.
int HAL_Serial_Write(HAL_Serial_Handler * hserial, uint8_t const * ptr, int len );
int HAL_Serial_Print(HAL_Serial_Handler * hserial,const char *fmt, ...);

/* HAL Functions ------------------------------------------------------------------*/


#endif /* HAL_SERIAL_H_ */
