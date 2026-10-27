#include "config_storage.h"
#include "eeprom.h"

#if CONFIG_IMAGE_SIZE % 8U != 0
#error "Legacy configuration image must contain complete flash doublewords"
#endif

bool ConfigStorage_IsEmpty(void) { return eeprom_empty(); }
bool ConfigStorage_Load(ConfigImage *image)
{
    return eeprom_restore(image->bytes, sizeof image->bytes) == HAL_OK;
}
bool ConfigStorage_Save(ConfigImage const *image)
{
    return eeprom_store(image->bytes, sizeof image->bytes) == HAL_OK;
}
