#ifndef INC_TELEMETRY_H_
#define INC_TELEMETRY_H_

#include <stdint.h>
#include <math.h>

// Keep the existing wire types; never convert a nonfinite or out-of-range
// float directly to an integer. NaN has no numeric representation and maps to 0.
static inline int16_t telemetry_i16(float value)
{
    if (isnan(value)) return 0;
    if (value >= INT16_MAX) return INT16_MAX;
    if (value <= INT16_MIN) return INT16_MIN;
    return (int16_t)value;
}

static inline uint8_t telemetry_u8(float value)
{
    if (isnan(value) || value <= 0.0f) return 0;
    if (value >= UINT8_MAX) return UINT8_MAX;
    return (uint8_t)value;
}

static inline void telemetry_store_i16(uint8_t *registers, unsigned address, float value)
{
    uint16_t const wire_value = (uint16_t)telemetry_i16(value);
    registers[address] = (uint8_t)wire_value;
    registers[address + 1U] = (uint8_t)(wire_value >> 8);
}

#endif
