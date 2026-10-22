#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "../../01-Firmware/B-G431B-ESC1-v3/Core/Src/serial.c"

uint32_t test_primask;
static HAL_Serial_Handler handler;
static DMA_HandleTypeDef rx_dma;
static UART_HandleTypeDef uart = { &rx_dma, HAL_UART_STATE_READY, 0 };
static GPIO_TypeDef tx_direction, rx_direction;
static uint8_t const *active_data;
static uint8_t active_snapshot[size_of_tx_buffer];
static unsigned active_length, starts, received;
static HAL_StatusTypeDef start_status;
#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "Serial transport line %d: %s\n", __LINE__, #condition); exit(1); } } while (0)

HAL_StatusTypeDef HAL_UART_Receive_DMA(UART_HandleTypeDef *handle, uint8_t *data, uint16_t size)
{
    CHECK(handle == &uart && data == handler.rx_circular_buffer && size == size_of_rx_circular_buffer);
    rx_dma.counter = size;
    ++received;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef *handle, uint8_t const *data, uint16_t size)
{
    CHECK(test_primask == 1 && handle == &uart && size > 0 && size <= size_of_tx_buffer);
    ++starts;
    if (start_status != HAL_OK) return start_status;
    CHECK(active_data == NULL);
    active_data = data;
    active_length = size;
    memcpy(active_snapshot, data, size);
    uart.gState = HAL_UART_STATE_BUSY_TX;
    return HAL_OK;
}
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state)
{
    (void)pin;
    CHECK(port == &tx_direction || port == &rx_direction);
    port->state = state;
}
static void reset_fixture(void)
{
    memset(&handler, 0, sizeof handler);
    serial_counter = 0;
    test_primask = 0;
    active_data = NULL;
    starts = received = 0;
    start_status = HAL_OK;
    uart.gState = HAL_UART_STATE_READY;
    uart.ErrorCode = 0;
    HAL_Serial_Init_Half_Duplex(&uart, &handler, &tx_direction, 1, GPIO_PIN_SET, &rx_direction, 1, GPIO_PIN_SET);
    CHECK(received == 1 && tx_direction.state == GPIO_PIN_RESET && rx_direction.state == GPIO_PIN_SET);
}
static uint8_t complete_transfer(void)
{
    CHECK(active_data != NULL && memcmp(active_data, active_snapshot, active_length) == 0);
    uint8_t const first = active_data[0];
    active_data = NULL;
    uart.gState = HAL_UART_STATE_READY;
    HAL_UART_TxCpltCallback(&uart);
    CHECK(test_primask == 0);
    return first;
}
int main(void)
{
    reset_fixture();
    for (unsigned i = 0; i < size_of_tx_pool - 1; ++i) {
        uint8_t value = (uint8_t)i;
        CHECK(HAL_Serial_Write(&handler, &value, 1) == 1);
    }
    uint8_t value = 99;
    CHECK(HAL_Serial_Write(&handler, &value, 1) == 0 && starts == 1 && active_data[0] == 0);
    for (unsigned i = 0; i < size_of_tx_pool - 1; ++i) CHECK(complete_transfer() == i);
    CHECK(!handler.tx_dma && handler.tx_head_position == handler.tx_tail_position);
    CHECK(tx_direction.state == GPIO_PIN_RESET && rx_direction.state == GPIO_PIN_SET);
    for (unsigned i = 0; i < 1000; ++i) {
        value = (uint8_t)i;
        CHECK(HAL_Serial_Write(&handler, &value, 1) == 1);
        CHECK(complete_transfer() == (uint8_t)i);
    }
    CHECK(HAL_Serial_Write(&handler, NULL, 1) == 0);
    CHECK(HAL_Serial_Write(&handler, &value, -1) == 0);
    CHECK(HAL_Serial_Write(&handler, &value, size_of_tx_buffer + 1) == 0);
    CHECK(HAL_Serial_Read(&handler, &value, -1) == 0);
    rx_dma.counter = 0; /* Circular DMA reload boundary is the start of the buffer. */
    CHECK(HAL_Serial_Available(&handler) == 0 && HAL_Serial_Read(&handler, &value, 1) == 0);
    CHECK(HAL_Serial_GetChar(&handler) == 0);
    rx_dma.counter = size_of_rx_circular_buffer - 1;
    handler.rx_circular_buffer[0] = 0x7e;
    CHECK(HAL_Serial_Available(&handler) == 1 && HAL_Serial_GetChar(&handler) == 0x7e);
    CHECK(HAL_Serial_Available(&handler) == 0);

    reset_fixture(); start_status = HAL_BUSY;
    CHECK(HAL_Serial_Write(&handler, &value, 1) == 0 && handler.tx_error == HAL_BUSY && !handler.tx_dma);
    CHECK(tx_direction.state == GPIO_PIN_RESET && handler.tx_head_position == handler.tx_tail_position);
    start_status = HAL_OK;
    CHECK(HAL_Serial_Write(&handler, &value, 1) == 1);
    CHECK(HAL_Serial_Write(&handler, &value, 1) == 1);
    start_status = HAL_ERROR; complete_transfer();
    CHECK(!handler.tx_dma && handler.tx_error == HAL_ERROR && handler.tx_head_position == handler.tx_tail_position);

    reset_fixture(); CHECK(HAL_Serial_Write(&handler, &value, 1) == 1);
    uart.ErrorCode = HAL_UART_ERROR_ORE;
    HAL_UART_ErrorCallback(&uart);
    CHECK(handler.tx_dma && active_data != NULL && handler.tx_error == HAL_OK);
    active_data = NULL; uart.gState = HAL_UART_STATE_READY; uart.ErrorCode = HAL_UART_ERROR_DMA;
    HAL_UART_ErrorCallback(&uart);
    CHECK(!handler.tx_dma && handler.tx_error == HAL_ERROR);
    CHECK(handler.tx_head_position == handler.tx_tail_position && tx_direction.state == GPIO_PIN_RESET);
    uart.ErrorCode = 0;
    CHECK(HAL_Serial_Write(&handler, &value, 1) == 1); complete_transfer();

    reset_fixture();
    CHECK(HAL_Serial_Print(&handler, "%d %u %x", INT_MIN, UINT_MAX, UINT_MAX) == 31);
    CHECK(memcmp(active_data, "-2147483648 4294967295 ffffffff", 31) == 0);
    complete_transfer();
    char text[size_of_tx_buffer + 2];
    memset(text, 'x', sizeof text); text[size_of_tx_buffer] = 0;
    CHECK(HAL_Serial_Print(&handler, "%s", text) == size_of_tx_buffer); complete_transfer();
    text[size_of_tx_buffer] = 'x'; text[size_of_tx_buffer + 1] = 0;
    CHECK(HAL_Serial_Print(&handler, "%s", text) == 0 && !handler.tx_dma);
    CHECK(HAL_Serial_Print(&handler, "") == 0);
    puts("Serial queue saturation, DMA ownership/failures and bounded formatting passed.");
    return 0;
}
