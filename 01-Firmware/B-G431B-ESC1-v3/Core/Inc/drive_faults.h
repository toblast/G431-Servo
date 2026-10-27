#ifndef INC_DRIVE_FAULTS_H_
#define INC_DRIVE_FAULTS_H_

#include <stdint.h>

typedef uint8_t DriveFaultFlags;
typedef enum {
    DRIVE_FAULT_VOLTAGE = 1U << 0,
    DRIVE_FAULT_ENCODER_READ = 1U << 1,
    DRIVE_FAULT_ENCODER_STALE = 1U << 2,
    DRIVE_FAULT_INTERNAL = 1U << 3,
    DRIVE_FAULT_FOC_TIMEOUT = 1U << 4,
    DRIVE_FAULT_OVERCURRENT = 1U << 5,
    DRIVE_FAULT_OVERHEATING = 1U << 6,
    DRIVE_FAULT_CONFIGURATION = 1U << 7
} DriveFault;

// Live flags, with unchanged serial bit assignments and clearing policies.
// Set/clear atomically preserve unrelated bits and restore the caller's IRQ mask.
// Callable from foreground or IRQ; no cached copy of the register-backed flags.
DriveFaultFlags hardware_fault_status(void) __attribute__((section(".ccmram")));
void hardware_fault_set(DriveFaultFlags flags) __attribute__((section(".ccmram")));
void hardware_fault_clear(DriveFaultFlags flags) __attribute__((section(".ccmram")));

#endif
