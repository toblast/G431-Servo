#include "configuration.h"
#include "config_storage.h"
#include "control_table.h"
#include "drive.h"
#include <stddef.h>

bool Config_Save(void)
{
    ConfigImage image;
    control_capture_config(&image);
    if (ConfigStorage_Save(&image)) return true;
    hardware_fault_set(DRIVE_FAULT_INTERNAL);
    return false;
}

bool Config_Load(void)
{
    if (ConfigStorage_IsEmpty()) {
        control_reset_config_defaults();
        if (!Config_Save()) return false;
    }
    ConfigImage image;
    if (!ConfigStorage_Load(&image)) {
        hardware_fault_set(DRIVE_FAULT_INTERNAL);
        return false;
    }
    control_apply_config(&image);
    return true;
}

ConfigResult Config_ApplyRegisterWrite(uint16_t address, uint16_t length, uint8_t const *data)
{
    if (data == NULL || length == 0 || address >= CONFIG_IMAGE_SIZE || length > CONFIG_IMAGE_SIZE - address)
        return CONFIG_ACCESS_ERROR;
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    bool const allowed = Drive_BeginMaintenance(false);
    if (allowed) control_apply_config_bytes(address, length, data);
    __set_PRIMASK(primask);
    if (!allowed) return CONFIG_BUSY;
    bool const saved = Config_Save();
    Drive_EndMaintenance();
    return saved ? CONFIG_OK : CONFIG_STORAGE_ERROR;
}

ConfigResult Config_FactoryReset(void)
{
    if (!Drive_BeginMaintenance(true)) return CONFIG_BUSY;
    control_reset_config_defaults();
    if (Config_Save()) return CONFIG_OK;
    Drive_EndMaintenance();
    return CONFIG_STORAGE_ERROR;
}
