#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../../01-Firmware/B-G431B-ESC1-v3/Core/Src/protocol.c"

uint8_t regs[REG_MAX];
static bool maintenance_active;
HAL_Serial_Handler serial;
uint32_t test_primask;
void (*before_irq_disable)(void);
static bool armed, calibrating;
static unsigned failures, stores, disarms, resets;
static HAL_StatusTypeDef store_status = HAL_OK;
#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "Serial check failed at line %d: %s\n", __LINE__, #condition); ++failures; \
} } while (0)
bool Drive_Is_Disarmed(void) { CHECK(test_primask == 1); return !armed && !calibrating && !regs[REG_TORQUE_ENABLE]; }
void Drive_Disarm(void) { armed = false; regs[REG_TORQUE_ENABLE] = 0; ++disarms; }

bool Drive_Begin_Maintenance(bool disarm_first)
{
    if (!disarm_first) CHECK(test_primask == 1);
    if (maintenance_active || calibrating || (!disarm_first && armed)) return false;
    maintenance_active = true;
    if (disarm_first) Drive_Disarm();
    return true;
}
void Drive_End_Maintenance(void) { maintenance_active = false; }
bool API_FOC_Is_Calibrating(void) { return calibrating; }
HAL_StatusTypeDef store_eeprom_regs(void) { CHECK(!armed && !calibrating && test_primask == 0 && maintenance_active); ++stores; return store_status; }
HAL_StatusTypeDef factory_reset_eeprom_regs(void) { CHECK(!armed && !regs[REG_TORQUE_ENABLE] && maintenance_active); ++stores; return store_status; }
void HAL_Delay(uint32_t time) { (void)time; CHECK(!armed && !regs[REG_TORQUE_ENABLE]); }
void HAL_NVIC_SystemReset(void) { CHECK(maintenance_active); maintenance_active = false; ++resets; }
int HAL_Serial_Write(HAL_Serial_Handler *handler, uint8_t const *data, int length) {
    (void)handler; (void)data; return length;
}
static void reset_fixture(void)
{
    memset(regs, 0, sizeof regs);
    memset(rx_packet_buffer, 0, sizeof rx_packet_buffer);
    armed = calibrating = maintenance_active = false;
    stores = disarms = resets = 0;
    test_primask = 0;
    before_irq_disable = NULL;
    packet_state = HEADER1;
    regs[REG_ID] = 1;
}
static void write_byte(uint16_t address, uint8_t value)
{
    rx_packet_buffer[PKT_INSTRUCTION] = INSTR_WRITE;
    rx_packet_buffer[PKT_LENGTH_L] = 6;
    rx_packet_buffer[PKT_LENGTH_H] = 0;
    rx_packet_buffer[PKT_PARAMETER0] = LOW_BYTE(address);
    rx_packet_buffer[PKT_PARAMETER1] = HIGH_BYTE(address);
    rx_packet_buffer[PKT_PARAMETER2] = value;
    instruction_handler();
}
static void can_arm_interrupt(void) { armed = true; regs[REG_TORQUE_ENABLE] = 1; }
int main(void)
{
    reset_fixture();
    write_byte(REG_TORQUE_ENABLE, 1);
    CHECK(!regs[REG_TORQUE_ENABLE] && tx_packet_buffer[PKT_ERROR] == ERROR_ACCESS_ERROR);
    write_byte(REG_HARDWARE_ERROR_STATUS, 0xFF);
    CHECK(regs[REG_HARDWARE_ERROR_STATUS] == 0 && tx_packet_buffer[PKT_ERROR] == ERROR_ACCESS_ERROR);
    write_byte(REG_GOAL_POSITION_DEG_L, 1);
    CHECK(regs[REG_GOAL_POSITION_DEG_L] == 0);
    write_byte(REG_MAX_CURRENT_MA_L, 42);
    CHECK(regs[REG_MAX_CURRENT_MA_L] == 42 && stores == 1);

    armed = true; regs[REG_TORQUE_ENABLE] = 1;
    write_byte(REG_MAX_CURRENT_MA_L, 99);
    CHECK(regs[REG_MAX_CURRENT_MA_L] == 42 && stores == 1);
    write_byte(REG_LED, 1);
    CHECK(regs[REG_LED] == 1);
    armed = false; regs[REG_TORQUE_ENABLE] = 0; calibrating = true;
    write_byte(REG_MAX_CURRENT_MA_L, 99);
    CHECK(regs[REG_MAX_CURRENT_MA_L] == 42);

    reset_fixture();
    before_irq_disable = can_arm_interrupt;
    write_byte(REG_MAX_CURRENT_MA_L, 99);
    CHECK(stores == 0 && regs[REG_MAX_CURRENT_MA_L] == 0);
    CHECK(test_primask == 0);

    reset_fixture();
    uint8_t values[2] = {1, 1};
    CHECK(!protocol_write_registers(REG_TORQUE_ENABLE - 1, 2, values));
    CHECK(!protocol_write_registers(REG_MAX - 1, 2, values));
    CHECK(!protocol_write_registers(REG_LED, 2, values));
    CHECK(!protocol_write_registers(0, 0, values));
    rx_packet_buffer[PKT_INSTRUCTION] = 0x83;
    instruction_handler();
    CHECK(tx_packet_buffer[PKT_ERROR] == ERROR_INSTRUCTION_ERROR && !regs[REG_TORQUE_ENABLE]);

    rx_packet_buffer[PKT_INSTRUCTION] = INSTR_READ;
    rx_packet_buffer[PKT_LENGTH_L] = 7;
    rx_packet_buffer[PKT_PARAMETER0] = REG_MAX - 1;
    rx_packet_buffer[PKT_PARAMETER2] = 2;
    instruction_handler();
    CHECK(tx_packet_buffer[PKT_ERROR] == ERROR_ACCESS_ERROR);

    reset_fixture();
    const uint8_t bad_header[] = {0xFF,0xFF,0xFD,0,1,2,0};
    for (unsigned i = 0; i < sizeof bad_header; ++i) packet_handler((char)bad_header[i]);
    CHECK(packet_state == HEADER1);
    for (unsigned i = 0; i < 1200; ++i) packet_handler(0);
    CHECK(packet_state == HEADER1 && regs[REG_TORQUE_ENABLE] == 0);

    armed = true; regs[REG_TORQUE_ENABLE] = 1;
    rx_packet_buffer[PKT_INSTRUCTION] = INSTR_REBOOT;
    instruction_handler();
    CHECK(disarms == 1 && resets == 1);
    armed = true; regs[REG_TORQUE_ENABLE] = 1;
    rx_packet_buffer[PKT_INSTRUCTION] = INSTR_FACTORY_RESET;
    instruction_handler();
    CHECK(disarms == 2 && resets == 2 && stores == 1);
    store_status = HAL_ERROR;
    write_byte(REG_MAX_CURRENT_MA_L, 1);
    CHECK(tx_packet_buffer[PKT_ERROR] != ERROR_NONE && !maintenance_active);
    rx_packet_buffer[PKT_INSTRUCTION] = INSTR_FACTORY_RESET;
    instruction_handler();
    CHECK(tx_packet_buffer[PKT_ERROR] == ERROR_RESULT_FAIL && resets == 2 && !maintenance_active);
    puts(failures ? "Serial protocol regressions failed." : "Serial protocol regressions passed.");
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
