/*
 * serial.c
 *
 *  Created on: 29 nov. 2015
 *      Author: Patrick
 */

#include "serial.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

static void Serial_Tx_Fail(HAL_Serial_Handler *hserial, HAL_StatusTypeDef status);

/// Store hserial to handle HAL_UART_TxCpltCallback
/// This table has to be filled in the application code
HAL_Serial_Handler * hserial_table[HAL_Serial_Handler_Count] = {0};
uint32_t serial_counter = 0;

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	for(uint32_t id=0;id!=serial_counter;++id)
	{
		if(hserial_table[id]->huart == huart)
		{
			uint32_t const primask = __get_PRIMASK();
			__disable_irq();
			if ((huart->ErrorCode & HAL_UART_ERROR_DMA) != 0 &&
			    hserial_table[id]->tx_dma && huart->gState == HAL_UART_STATE_READY)
				Serial_Tx_Fail(hserial_table[id], HAL_ERROR);
			hserial_table[id]->rx_tail_ptr = hserial_table[id]->rx_circular_buffer;
			HAL_UART_Receive_DMA(hserial_table[id]->huart, hserial_table[id]->rx_circular_buffer,SERIAL_RX_BUFFER_SIZE);
			__set_PRIMASK(primask);
		   break;
		}
	}
}

void HAL_Serial_Init(UART_HandleTypeDef * huart, HAL_Serial_Handler * hserial)
{
	// register this handle
	hserial_table[serial_counter++]=hserial;
    hserial->huart = huart;

    // reset tx fifo pool buffer
    hserial->tx_head_position = 0;
    hserial->tx_tail_position = 0;
	hserial->tx_dma = 0;
	hserial->tx_error = HAL_OK;

	// reset rx circular buffer
	hserial->rx_tail_ptr = hserial->rx_circular_buffer;

    // start rx DMA
	HAL_UART_Receive_DMA(hserial->huart, hserial->rx_circular_buffer,SERIAL_RX_BUFFER_SIZE);

	// half-duplex
	hserial->half_duplex = false;
	hserial->GPIOx_DIR_TX = 0;
	hserial->GPIO_Pin_DIR_TX = 0;
	hserial->PinState_DIR_TX = 0;
	hserial->GPIOx_DIR_RX = 0;
	hserial->GPIO_Pin_DIR_RX = 0;
	hserial->PinState_DIR_RX = 0;
}

void HAL_Serial_Init_Half_Duplex(
		UART_HandleTypeDef * huart,
		HAL_Serial_Handler * hserial,
		GPIO_TypeDef *GPIOx_TX,
		uint16_t GPIO_Pin_TX,
		GPIO_PinState PinState_TX,
		GPIO_TypeDef *GPIOx_RX,
		uint16_t GPIO_Pin_RX,
		GPIO_PinState PinState_RX
)
{
	// register this handle
	hserial_table[serial_counter++]=hserial;
    hserial->huart = huart;

    // reset tx fifo pool buffer
    hserial->tx_head_position = 0;
    hserial->tx_tail_position = 0;
	hserial->tx_dma = 0;
	hserial->tx_error = HAL_OK;

	// reset rx circular buffer
	hserial->rx_tail_ptr = hserial->rx_circular_buffer;

    // start rx DMA
	HAL_UART_Receive_DMA(hserial->huart, hserial->rx_circular_buffer,SERIAL_RX_BUFFER_SIZE);

	// half-duplex
	hserial->half_duplex = true;
	hserial->GPIOx_DIR_TX = GPIOx_TX;
	hserial->GPIO_Pin_DIR_TX = GPIO_Pin_TX;
	hserial->PinState_DIR_TX = PinState_TX;
	hserial->GPIOx_DIR_RX = GPIOx_RX;
	hserial->GPIO_Pin_DIR_RX = GPIO_Pin_RX;
	hserial->PinState_DIR_RX = PinState_RX;
    // TX = OFF
    HAL_GPIO_WritePin(hserial->GPIOx_DIR_TX,hserial->GPIO_Pin_DIR_TX,hserial->PinState_DIR_TX==GPIO_PIN_RESET?GPIO_PIN_SET:GPIO_PIN_RESET);
    HAL_GPIO_WritePin(hserial->GPIOx_DIR_RX,hserial->GPIO_Pin_DIR_RX,hserial->PinState_DIR_RX);
}

int HAL_Serial_Available(HAL_Serial_Handler * hserial)
{
    if(hserial->huart == 0)
        return 0;
    uint8_t const * head = hserial->rx_circular_buffer + (SERIAL_RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(hserial->huart->hdmarx)) % SERIAL_RX_BUFFER_SIZE;
    uint8_t const * tail = hserial->rx_tail_ptr;
    if( head>=tail )
        return head-tail;
    else
        return head-tail+SERIAL_RX_BUFFER_SIZE;
}

char HAL_Serial_GetChar(HAL_Serial_Handler * hserial)
{
    if(hserial->huart == 0)
        return 0;
    uint8_t const * head = hserial->rx_circular_buffer + (SERIAL_RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(hserial->huart->hdmarx)) % SERIAL_RX_BUFFER_SIZE;
    uint8_t const * tail = hserial->rx_tail_ptr;
    if(head!=tail)
    {
        char c =  *hserial->rx_tail_ptr++;
        if(hserial->rx_tail_ptr>=hserial->rx_circular_buffer + SERIAL_RX_BUFFER_SIZE)
            hserial->rx_tail_ptr-=SERIAL_RX_BUFFER_SIZE;
        return c;
    }
    else
        return 0;
}

int HAL_Serial_Read(HAL_Serial_Handler * hserial, uint8_t * ptr, int len )
{
    if(hserial->huart == 0 || ptr == NULL || len <= 0)
        return 0;
    uint8_t const * head = hserial->rx_circular_buffer + (SERIAL_RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(hserial->huart->hdmarx)) % SERIAL_RX_BUFFER_SIZE;
    int counter = 0;
    while(counter!=len)
    {
        if(head==hserial->rx_tail_ptr)
            return counter;
        *ptr++=*hserial->rx_tail_ptr++;
        if(hserial->rx_tail_ptr>=hserial->rx_circular_buffer + SERIAL_RX_BUFFER_SIZE)
            hserial->rx_tail_ptr-=SERIAL_RX_BUFFER_SIZE;
        ++counter;
    }
    return counter;
}

static void Serial_Tx_Direction(HAL_Serial_Handler *hserial, bool transmit)
{
    if (!hserial->half_duplex) return;
    HAL_GPIO_WritePin(hserial->GPIOx_DIR_TX, hserial->GPIO_Pin_DIR_TX,
        transmit ? hserial->PinState_DIR_TX : (hserial->PinState_DIR_TX == GPIO_PIN_RESET ? GPIO_PIN_SET : GPIO_PIN_RESET));
    HAL_GPIO_WritePin(hserial->GPIOx_DIR_RX, hserial->GPIO_Pin_DIR_RX,
        transmit ? (hserial->PinState_DIR_RX == GPIO_PIN_RESET ? GPIO_PIN_SET : GPIO_PIN_RESET) : hserial->PinState_DIR_RX);
}

// Call with interrupts masked. A failed transfer discards pending replies so
// a later request cannot receive old responses from before the UART failure.
static void Serial_Tx_Fail(HAL_Serial_Handler *hserial, HAL_StatusTypeDef status)
{
    hserial->tx_error = status;
    hserial->tx_head_position = hserial->tx_tail_position;
    hserial->tx_dma = 0;
    Serial_Tx_Direction(hserial, false);
}

static HAL_StatusTypeDef Serial_Tx_Start(HAL_Serial_Handler *hserial)
{
    if (hserial->tx_head_position == hserial->tx_tail_position) {
        Serial_Tx_Direction(hserial, false);
        return HAL_OK;
    }
    Serial_Tx_Direction(hserial, true);
    tx_buffer const *slot = &hserial->tx_buffer_pool[hserial->tx_tail_position];
    HAL_StatusTypeDef const status = HAL_UART_Transmit_DMA(hserial->huart, slot->data, slot->length);
    if (status == HAL_OK) hserial->tx_dma = 1;
    else Serial_Tx_Fail(hserial, status);
    return status;
}

int HAL_Serial_Write(HAL_Serial_Handler *hserial, uint8_t const *ptr, int len)
{
    if (hserial == NULL || hserial->huart == NULL || ptr == NULL ||
        len <= 0 || len > SERIAL_TX_BUFFER_SIZE) return 0;
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    uint32_t const next = (hserial->tx_head_position + 1U) % SERIAL_TX_POOL_SIZE;
    if (next == hserial->tx_tail_position) {
        __set_PRIMASK(primask);
        return 0;
    }
    tx_buffer *slot = &hserial->tx_buffer_pool[hserial->tx_head_position];
    memcpy(slot->data, ptr, (size_t)len);
    slot->length = (uint32_t)len;
    hserial->tx_head_position = next;
    HAL_StatusTypeDef const status = hserial->tx_dma ? HAL_OK : Serial_Tx_Start(hserial);
    __set_PRIMASK(primask);
    return status == HAL_OK ? len : 0;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    for (uint32_t id = 0; id < serial_counter; ++id) {
        HAL_Serial_Handler *hserial = hserial_table[id];
        if (hserial->huart != huart || !hserial->tx_dma) continue;
        // Only the completion callback releases the buffer owned by DMA.
        hserial->tx_dma = 0;
        hserial->tx_tail_position = (hserial->tx_tail_position + 1U) % SERIAL_TX_POOL_SIZE;
        Serial_Tx_Start(hserial);
        break;
    }
    __set_PRIMASK(primask);
}

int HAL_Serial_Print(HAL_Serial_Handler *hserial, const char *fmt, ...)
{
    if (fmt == NULL) return 0;
    char buffer[SERIAL_TX_BUFFER_SIZE + 1];
    va_list arguments;
    va_start(arguments, fmt);
    int const length = vsnprintf(buffer, sizeof buffer, fmt, arguments);
    va_end(arguments);
    if (length <= 0 || length > SERIAL_TX_BUFFER_SIZE) return 0;
    return HAL_Serial_Write(hserial, (uint8_t const *)buffer, length);
}
