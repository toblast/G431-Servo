#include "drive_faults.h"
#include "control_table.h"

// The existing diagnostic byte remains authoritative; packing belongs here.
DriveFaultFlags hardware_fault_status(void)
{
    return *(volatile uint8_t *)&regs[REG_HARDWARE_ERROR_STATUS];
}

void hardware_fault_set(DriveFaultFlags flags)
{
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    *(volatile uint8_t *)&regs[REG_HARDWARE_ERROR_STATUS] = hardware_fault_status() | flags;
    __set_PRIMASK(primask);
}

void hardware_fault_clear(DriveFaultFlags flags)
{
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    *(volatile uint8_t *)&regs[REG_HARDWARE_ERROR_STATUS] = hardware_fault_status() & (uint8_t)~flags;
    __set_PRIMASK(primask);
}
