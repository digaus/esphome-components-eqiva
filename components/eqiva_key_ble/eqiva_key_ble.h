#pragma once

#include "esphome/components/esp32_ble_client/ble_client_base.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/automation.h"

#include <queue>
#include "eQ3_constants.h"
#include "eQ3_message.h"
#include "eQ3_util.h"
#include <esp_log.h>

#ifdef USE_ESP32


namespace esphome {
namespace eqiva_key_ble {


using namespace esp32_ble_client;

class EqivaKeyBle;

class EqivaKeyBle : public BLEClientBase {
    bool sendMessage(eQ3Message::Message *msg, bool nonce);
    void sendFragment();
    void sendNonce();
    void init();
    void finishPair();

    std::queue<eQ3Message::MessageFragment> sendQueue;
    BLECharacteristic *write;
    BLECharacteristic *read;
    bool sendingNonce;
    unsigned long sending;
    eQ3Message::Message *currentMsg;
    bool requestPair;

    unsigned long getTime() {
        time_t now;
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        time(&now);
        return now;
    }
    std::string getClientState() {
        std::string client_state;
        switch(this->state_) {
            case espbt::ClientState::INIT: {
                client_state = "INIT";
                break;
            }
            case espbt::ClientState::DISCONNECTING: {
                client_state = "DISCONNECTING";
                break;
            }
            case espbt::ClientState::IDLE: {
                client_state = "IDLE";
                break;
            }
            case espbt::ClientState::DISCOVERED: {
                client_state = "DISCOVERED";
                break;
            }
            case espbt::ClientState::CONNECTING: {
                client_state = "CONNECTING";
                break;
            }
            case espbt::ClientState::CONNECTED: {
                client_state = "CONNECTED";
                break;
            }
            case espbt::ClientState::ESTABLISHED: {
                client_state = "ESTABLISHED";
                break;
            }
        }
        return client_state;
    }
    public:
        ClientState clientState;
        void startPair();
        void applySettings();
        void sendCommand(CommandType command);
        void set_user_id(int user_id) {
            clientState.user_id = user_id;
        }
        void set_user_key(std::string user_key) {
            if (user_key.length() > 0) {
                clientState.user_key = hexstring_to_string(user_key);
            }
        }
        void set_card_key(std::string card_key) {
            if (card_key.length() > 0) {
                for(char &c : card_key)
                    c = tolower(c);
                clientState.card_key = card_key.substr(14, 32);
            }
        }
        void set_turn_left(bool turn_left) { clientState.turn_left = turn_left; }
        void set_key_horizontal(bool key_horizontal) { clientState.key_horizontal = key_horizontal; }
        void set_lock_turns(int lock_turns) { clientState.lock_turns = lock_turns; }


        void set_lock_ble_state_sensor(text_sensor::TextSensor *lock_ble_state_sensor) { this->lock_ble_state_sensor_ = lock_ble_state_sensor; }
        void set_low_battery_sensor(text_sensor::TextSensor *low_battery_sensor) { this->low_battery_sensor_ = low_battery_sensor; }
        void set_lock_status_sensor(text_sensor::TextSensor *lock_status_sensor) { this->lock_status_sensor_ = lock_status_sensor; }
        void set_user_key_sensor(text_sensor::TextSensor *user_key_sensor) { this->user_key_sensor_ = user_key_sensor; }
        void set_user_id_sensor(text_sensor::TextSensor *user_id_sensor) { this->user_id_sensor_ = user_id_sensor; }
        void set_mac_address_sensor(text_sensor::TextSensor *mac_address_sensor) { this->mac_address_sensor_ = mac_address_sensor; }

        void set_state(esphome::esp32_ble_tracker::ClientState st) {
            BLEClientBase::set_state(st);
            this->lock_ble_state_sensor_->publish_state(getClientState()); 
        };
        void dump_config() override;
        bool gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                                esp_ble_gattc_cb_param_t *param) override;

    protected: 
        text_sensor::TextSensor *lock_ble_state_sensor_{nullptr};                
        text_sensor::TextSensor *low_battery_sensor_{nullptr};
        text_sensor::TextSensor *lock_status_sensor_{nullptr};
        text_sensor::TextSensor *user_key_sensor_{nullptr};
        text_sensor::TextSensor *user_id_sensor_{nullptr};
        text_sensor::TextSensor *mac_address_sensor_{nullptr};

};


template<typename... Ts>
class EqivaSettings : public Action<Ts...>, public Parented<EqivaKeyBle> {
    TEMPLATABLE_VALUE(bool, turn_left)
    TEMPLATABLE_VALUE(bool, key_horizontal)
    TEMPLATABLE_VALUE(int, lock_turns)
    public:
        void play(const Ts &...x) override { 
            auto turn_left = this->turn_left_.value(x...);
            auto key_horizontal = this->key_horizontal_.value(x...);
            auto lock_turns = this->lock_turns_.value(x...);
            this->parent_->set_turn_left(turn_left);
            this->parent_->set_key_horizontal(key_horizontal);
            this->parent_->set_lock_turns(lock_turns);
            this->parent_->applySettings();
        }
};

template<typename... Ts>
class EqivaConnect : public Action<Ts...>, public Parented<EqivaKeyBle> {
    TEMPLATABLE_VALUE(std::string, mac_address)
    TEMPLATABLE_VALUE(int, user_id)
    TEMPLATABLE_VALUE(std::string, user_key)
    public:
        void play(const Ts &...x) override {

            auto mac_address = this->mac_address_.value(x...);
            auto current_mac_address = this->parent_->get_address();
            if (current_mac_address != 0 && (string_to_mac(mac_address) < current_mac_address || string_to_mac(mac_address) > current_mac_address)) {
                this->parent_->disconnect();
            }
            auto user_id = this->user_id_.value(x...);
            auto user_key = this->user_key_.value(x...);
            this->parent_->set_user_id(user_id);
            this->parent_->set_user_key(user_key);
            this->parent_->set_address(string_to_mac(mac_address));
            ESP_LOGD("ESP Eqiva", " Address: %s, %s", this->parent_->address_str(), mac_address.c_str());

        }
};

template<typename... Ts>
class EqivaDisconnect : public Action<Ts...>, public Parented<EqivaKeyBle> {
    public:
        void play(const Ts &...x) {
            // this->parent_->set_user_id(255);
            // this->parent_->set_user_key("");
            this->parent_->disconnect();
            this->parent_->set_address(1);
        }
};

template<typename... Ts>
class EqivaPair : public Action<Ts...>, public Parented<EqivaKeyBle> {
    TEMPLATABLE_VALUE(std::string, card_key)
    TEMPLATABLE_VALUE(std::string, mac_address)
    public:
        void play(const Ts &...x) { 
            auto card_key = this->card_key_.value(x...);
            auto mac_address = this->mac_address_.value(x...);
            this->parent_->set_card_key(card_key);
            this->parent_->startPair();
            this->parent_->set_address(string_to_mac(mac_address));
        }
};

template<typename... Ts>
class EqivaLock : public Action<Ts...>, public Parented<EqivaKeyBle> {
 public:
  void play(const Ts &...x) { this->parent_->sendCommand(LOCK); }
};

template<typename... Ts>
class EqivaUnlock : public Action<Ts...>, public Parented<EqivaKeyBle> {
 public:
  void play(const Ts &...x) { this->parent_->sendCommand(UNLOCK); }
};

template<typename... Ts>
class EqivaOpen : public Action<Ts...>, public Parented<EqivaKeyBle> {
 public:
  void play(const Ts &...x) { this->parent_->sendCommand(OPEN); }
};

template<typename... Ts>
class EqivaStatus : public Action<Ts...>, public Parented<EqivaKeyBle> {
 public:
  void play(const Ts &...x) { this->parent_->sendCommand(REQUEST_STATUS); }
};



}  // namespace eqiva_key_ble
}  // namespace esphome

#endif
