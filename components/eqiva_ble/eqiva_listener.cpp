#include "eqiva_listener.h"
#include "esphome/core/log.h"
#include <cinttypes>

#ifdef USE_ESP32

namespace esphome {
namespace eqiva_ble {

static const char *const TAG = "eqiva_ble";

bool EqivaListener::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  for (auto &it : device.get_manufacturer_datas()) {
    if (it.uuid == esp32_ble_tracker::ESPBTUUID::from_raw("58e06900-15d8-11e6-b737-0002a5d5c51b")) {
      if (it.data.size() < 4)
        continue;

      uint32_t sn = it.data[0];
      sn |= ((uint32_t) it.data[1] << 8);
      sn |= ((uint32_t) it.data[2] << 16);
      sn |= ((uint32_t) it.data[3] << 24);

      ESP_LOGD(TAG, "Found Eqiva device Serial:%" PRIu32 " (MAC: %s)", sn, device.address_str().c_str());
      return true;
    }
  }

  return false;
}

}  // namespace eqiva_ble
}  // namespace esphome

#endif
