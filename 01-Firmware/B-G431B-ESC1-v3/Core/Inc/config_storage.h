#ifndef INC_CONFIG_STORAGE_H_
#define INC_CONFIG_STORAGE_H_

#include <stdbool.h>
#include <stdint.h>

// Existing EEPROM region: bytes 0x00..0x7f, with no header or format migration.
#define CONFIG_IMAGE_SIZE 128U
typedef struct { uint8_t bytes[CONFIG_IMAGE_SIZE]; } ConfigImage;

// Foreground/startup I/O only. No validation, live-register mutation or drive
// policy here. The configuration service handles errors and fault reporting.
bool ConfigStorage_IsEmpty(void);
bool ConfigStorage_Load(ConfigImage *image);
bool ConfigStorage_Save(ConfigImage const *image);

#endif
