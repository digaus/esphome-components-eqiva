#pragma once

#include "esphome/components/esp32_ble_client/ble_client_base.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/core/component.h"
#include "esphome/core/automation.h"

#include <queue>
#include "eQ3_constants.h"
#include "eQ3_message.h"
#include "eQ3_util.h"

#ifdef USE_ESP32


namespace esphome {
namespace eqiva_key_ble {


using namespace esp32_ble_client;

class EqivaKeyBle;

class EqivaKeyBle : public BLEClientBase {
    bool sendMessage(eQ3Message::Message *msg);
    void sendFragment();
    void sendNonce();
    void init();
    void finishPair();

    std::queue<eQ3Message::MessageFragment> sendQueue;
    BLECharacteristic *write;
    BLECharacteristic *read;
    bool sendingNonce;
    bool sending;
    public:
        std::string card_key;
        ClientState clientState;
        void pair(std::string card);
        void sendCommand(CommandType command);
        void set_user_id(int user_id) {
            clientState.user_id = user_id;
        }
        void set_user_key(std::string user_key) {
            if (user_key.length() > 0) {
                clientState.user_key = hexstring_to_string(user_key);
            }
        }
        void dump_config() override;
        bool gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                                esp_ble_gattc_cb_param_t *param) override;
                                
};




template<typename... Ts>
class EqivaPair : public Action<Ts...>, public Parented<EqivaKeyBle> {
    TEMPLATABLE_VALUE(std::string, card_key)
    public:
        void play(Ts... x) override { 
            auto card_key = this->card_key_.value(x...);
            if (card_key.length() > 0) {
                for(char &c : card_key)
                    c = tolower(c);
                this->parent_->pair(card_key.substr(14,32));
            }
        }
};

template<typename... Ts>
class EqivaLock : public Action<Ts...>, public Parented<EqivaKeyBle> {
 public:
  void play(Ts... x) override { this->parent_->sendCommand(LOCK); }
};

template<typename... Ts>
class EqivaUnlock : public Action<Ts...>, public Parented<EqivaKeyBle> {
 public:
  void play(Ts... x) override { this->parent_->sendCommand(UNLOCK); }
};

template<typename... Ts>
class EqivaOpen : public Action<Ts...>, public Parented<EqivaKeyBle> {
 public:
  void play(Ts... x) override { this->parent_->sendCommand(OPEN); }
};

template<typename... Ts>
class EqivaStatus : public Action<Ts...>, public Parented<EqivaKeyBle> {
 public:
  void play(Ts... x) override { this->parent_->sendCommand(REQUEST_STATUS); }
};



}  // namespace eqiva_key_ble
}  // namespace esphome

#endif
