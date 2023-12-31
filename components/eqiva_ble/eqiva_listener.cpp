#include "eqiva_listener.h"
#include "esphome/core/log.h"
#include <cinttypes>

#ifdef USE_ESP32

namespace esphome {
namespace eqiva_ble {

static const char *const TAG = "eqiva_ble";

bool EqivaListener::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
    for (auto &it : device.get_manufacturer_datas()) {
        if (it.uuid == esp32_ble_tracker::ESPBTUUID::from_uint16(0x1A00)) {
            ESP_LOGI(TAG, "Found Eqiva device (MAC: %s) (UUID): %s  (Name): %s", device.address_str().c_str(), it.uuid.to_string().c_str(), device.get_name().c_str());
            return true;
        }
    }

    return false;
}

}  // namespace eqiva_ble
}  // namespace esphome

#endif