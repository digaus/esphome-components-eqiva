#include "eqiva_key_ble.h"
#include "esphome/core/log.h"
#include <cinttypes>
#include "eQ3_util.h"
#include "eQ3_message.h"
#include <sstream>

#ifdef USE_ESP32

namespace esphome {
namespace eqiva_key_ble {

static const char *const TAG = "eqiva_key_ble";

void EqivaKeyBle::dump_config() {
  ESP_LOGCONFIG(TAG, "Eqiva Key-BLE:");
  ESP_LOGCONFIG(TAG, "  Address: %s", this->address_str().c_str());
  ESP_LOGCONFIG(TAG, "  UserKey: %s", clientState.user_key.length() > 0 ? string_to_hex(clientState.user_key).c_str() : "");
  ESP_LOGCONFIG(TAG, "  UserId: %d", clientState.user_id);
  ESP_LOGCONFIG(TAG, "  CardKey: %s", clientState.card_key.c_str());

}

bool EqivaKeyBle::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t esp_gattc_if,
                                    esp_ble_gattc_cb_param_t *param) {

  this->mac_address_sensor_->publish_state(this->address_str());

  if (!BLEClientBase::gattc_event_handler(event, esp_gattc_if, param))
    return false;

  this->lock_ble_state_sensor_->publish_state(getClientState());

  switch (event) {
    case ESP_GATTC_SEARCH_CMPL_EVT: {
      if (this->state() == espbt::ClientState::ESTABLISHED) {
        clientState.remote_session_nonce.clear();
        clientState.local_session_nonce.clear();
        write = this->get_characteristic(esp32_ble_tracker::ESPBTUUID::from_raw("58e06900-15d8-11e6-b737-0002a5d5c51b"), esp32_ble_tracker::ESPBTUUID::from_raw("3141dd40-15db-11e6-a24b-0002a5d5c51b"));
        ESP_LOGD(TAG, "Write (UUID): %s  ",  write->uuid.to_string().c_str());
        read = this->get_characteristic(esp32_ble_tracker::ESPBTUUID::from_raw("58e06900-15d8-11e6-b737-0002a5d5c51b"), esp32_ble_tracker::ESPBTUUID::from_raw("359d4820-15db-11e6-82bd-0002a5d5c51b"));
        esp_err_t errRc = ::esp_ble_gattc_register_for_notify(
          this->gattc_if_,
          this->remote_bda_,
          read->handle
        );
        ESP_LOGD(TAG, "Read (UUID): %s  ",  read->uuid.to_string().c_str());
        init();
        if (currentMsg == NULL && requestPair == false && clientState.user_key.length() > 0 && clientState.user_id < 255) {
          auto * msg = new eQ3Message::StatusRequestMessage;
          sendMessage(msg, false);
        }
  
      }
      break;
    }
    case ESP_GATTC_DISCONNECT_EVT: {
      ESP_LOGD(TAG, "ESP_GATTC_DISCONNECT_EVT");
    
      break;
    }
    case ESP_GATTC_SEARCH_RES_EVT: {
      ESP_LOGD(TAG, "ESP_GATTC_REG_FOR_NOTIFY_EVT");
      break;
    }
    case ESP_GATTC_WRITE_CHAR_EVT: {
      ESP_LOGD(TAG, "ESP_GATTC_WRITE_CHAR_EVT");
      sending = false;
      sendFragment();
      break;
    }
    case ESP_GATTC_WRITE_DESCR_EVT: {
      ESP_LOGD(TAG, "ESP_GATTC_WRITE_DESCR_EVT");
      break;
    }
    case ESP_GATTC_NOTIFY_EVT: {
      ESP_LOGD(TAG, "ESP_GATTC_NOTIFY_EVT");
      if (param != NULL) {
        eQ3Message::MessageFragment frag;

        frag.data = std::string((char *) param->notify.value, param->notify.value_len);
        auto msgtype = frag.getType();
        if (frag.isLast()) {
          ESP_LOGD(TAG, "LAST");
        } else {
          ESP_LOGD(TAG, "NOT_LAST, TODO");
          /*eQ3Message::FragmentAckMessage ack(frag.getStatusByte());
          ESP_LOGD(TAG, "Send message: %s ", string_to_hex(ack.data).c_str());
          auto *write = this->get_characteristic(esp32_ble_tracker::ESPBTUUID::from_raw("58e06900-15d8-11e6-b737-0002a5d5c51b"), esp32_ble_tracker::ESPBTUUID::from_raw("3141dd40-15db-11e6-a24b-0002a5d5c51b"));
          write->write_value((uint8_t *) (ack.data.c_str()), 16, ESP_GATT_WRITE_TYPE_RSP);*/
        }
        std::stringstream ss;
        ss << frag.getData();
        std::string msgdata = ss.str();
        if (eQ3Message::Message::isTypeSecure(msgtype)) {
          auto msg_security_counter = static_cast<uint16_t>(msgdata[msgdata.length() - 6]);
          msg_security_counter <<= 8;
          msg_security_counter += msgdata[msgdata.length() - 5];
          if (msg_security_counter <= clientState.remote_security_counter) {
              ESP_LOGD(TAG,"Remote security counter missmatch");
              return true;
          }
          clientState.remote_security_counter = msg_security_counter;
          string msg_auth_value = msgdata.substr(msgdata.length() - 4, 4);
          ESP_LOGD(TAG, "# Auth value: ");
          ESP_LOGD(TAG, string_to_hex(msg_auth_value).c_str());
          //std::string decrypted = crypt_data(msgdata.substr(0, msgdata.length() - 6), msgtype,
          std::string decrypted = crypt_data(msgdata.substr(1, msgdata.length() - 7), msgtype, clientState.local_session_nonce, clientState.remote_security_counter, clientState.user_key);
          ESP_LOGD(TAG, "# Crypted data: ");
          ESP_LOGD(TAG, string_to_hex(msgdata.substr(1, msgdata.length() - 7)).c_str());
          std::string computed_auth_value = compute_auth_value(decrypted, msgtype, clientState.local_session_nonce, clientState.remote_security_counter, clientState.user_key);
          if (msg_auth_value != computed_auth_value) {
              ESP_LOGD(TAG,"# Auth value mismatch");
              clientState.remote_session_nonce.clear();
              clientState.remote_security_counter = 0;
              return true;
          }
          if(clientState.card_key.length() > 0 && clientState.user_id != 255) {
            ESP_LOGI(TAG, "Pairing successfull, please copy user_key: %s and user_id: %d into your yaml", string_to_hex(clientState.user_key).c_str(), clientState.user_id);
            clientState.card_key.clear();
          }
          msgdata = decrypted;
          ESP_LOGD(TAG, "# Decrypted: ");
          ESP_LOGD(TAG, string_to_hex(msgdata).c_str());

        }
        switch (msgtype) {
          case 0: {
              ESP_LOGD(TAG, "Case 0");
              break;
          }

          case 0x81: // answer with security
              // TODO call callback to user that pairing succeeded
              ESP_LOGD(TAG, "Case 0x81");
              break;

          case 0x01: // answer without security
              // TODO report error
              ESP_LOGD(TAG, "Case 0x01");
              break;
  
          case 0x05: {
              ESP_LOGD(TAG, "Case 0x05");
              auto * message = new eQ3Message::StatusRequestMessage;
              sendMessage(message, false);
              break;
          }

          case 0x03: {
              // Nonce success
              ESP_LOGD(TAG, "Case 0x03");
              eQ3Message::Connection_Info_Message message;
              message.data = msgdata;
              clientState.user_id = message.getUserId();
              clientState.remote_session_nonce = message.getRemoteSessionNonce();
              clientState.local_security_counter = 1;
              clientState.remote_security_counter = 0;
              if (clientState.remote_session_nonce.length() == 8) {
                ESP_LOGD(TAG,"Nonce exchanged: %s",  string_to_hex(clientState.remote_session_nonce).c_str());
                ESP_LOGD(TAG,"Remote user_id: %d",  clientState.user_id);
           
              } else {
                ESP_LOGD(TAG,"error Nonce exchanged: %s",  string_to_hex(clientState.remote_session_nonce).c_str());
              }

              int user_id = message.getUserId();
              this->user_key_sensor_->publish_state(string_to_hex(clientState.user_key).c_str());
              this->user_id_sensor_->publish_state(std::to_string(user_id));
  
              sendingNonce = false;
              if (currentMsg != NULL) {
                sendMessage(currentMsg, false);
                currentMsg = NULL;
              } else if (requestPair) {
                finishPair();
                requestPair = false;
              }
              break;
          }

          case 0x83: {
              ESP_LOGD(TAG, "Case 0x83");
              // status info
              eQ3Message::Status_Info_Message message;
              message.data = msgdata;
              std::string lockStatus;
              switch(message.getLockStatus()) {
                case 0: {
                  lockStatus = "UNKNOWN";
                  break;
                }
                case 1: {
                  lockStatus = "MOVING";
                  break;
                }
                case 2: {
                  lockStatus = "UNLOCKED";
                  break;
                }
                case 3: {
                  lockStatus = "LOCKED";
                  break;
                }
                case 4: {
                  lockStatus = "OPENED";
                  break;
                }
              }
              this->lock_status_sensor_->publish_state(lockStatus);
              this->low_battery_sensor_->publish_state(message.isBatteryLow() ? "true" : "false");

              ESP_LOGD(TAG, "# Lock state: %d", message.getLockStatus());
              ESP_LOGD(TAG, "# Battery low: %s", message.isBatteryLow() ? "true" : "false");
              break;
          }

          default: { // user info
              ESP_LOGD(TAG, "Case default");
              break;
          }
        }
  
        sendFragment();
      }
      break;
    }
    default: {
      ESP_LOGD(TAG, "OTHER EVENT %d", static_cast<int>(event));
      break;
    }
  }
  return true;
}
void EqivaKeyBle::init() {
    if(clientState.user_key.length() == 16) {
      sendNonce();
    } else {
      ESP_LOGE(TAG, "User Error: (Key: %s, ID:  %d)", clientState.user_key.c_str(), clientState.user_id);
    }
}
void EqivaKeyBle::sendCommand(CommandType command) {
  if (command == REQUEST_STATUS) {
      auto * msg = new eQ3Message::StatusRequestMessage;
      sendMessage(msg, false);
  } else {
      auto msg = new eQ3Message::CommandMessage((char) command);
      sendMessage(msg, false);
  }
}
void EqivaKeyBle::applySettings() {
    auto * msg = new eQ3Message::Mount_Options_Set_Message;
    sendMessage(msg, false);
}
void EqivaKeyBle::startPair() {
    if (clientState.card_key.length() > 0) {
      clientState.user_id = 255;
      clientState.user_key.clear();
      clientState.remote_session_nonce.clear();
        srand((unsigned int)time(NULL));
      auto randchar = []() -> char
      {
          const char charset[] =
          "0123456789"
          "abcdefghijklmnopqrstuvwxyz";
          const size_t max_index = (sizeof(charset) - 1);
          return charset[ rand() % max_index ];
      };
      std::string str(16,0);
      std::generate_n( str.begin(), 16, randchar );
      clientState.user_key = str;
      ESP_LOGI(TAG, "CardKey: %s", clientState.card_key.c_str());
      ESP_LOGI(TAG, "Please press and hold open button for 5 seconds to enter pairing mode");
      ESP_LOGI(TAG, "Trying to pair...");
      init();
      finishPair();
    } else {
      ESP_LOGI(TAG, "Card key missing!");
    }

}

void EqivaKeyBle::finishPair() {
    if (sendingNonce == false && clientState.remote_session_nonce.length() > 0) {
      auto *message = new eQ3Message::PairingRequestMessage();
      message->data.append(1, clientState.user_id);   
      std::string cardKey = hexstring_to_string(clientState.card_key);
      std::string encrypted_pair_key = crypt_data(clientState.user_key, 0x04, clientState.remote_session_nonce, clientState.local_security_counter, cardKey);
      if (encrypted_pair_key.length() < 22)
          encrypted_pair_key.append(22 - encrypted_pair_key.length(), 0);
      message->data.append(encrypted_pair_key);

      // counter
      message->data.append(1, (char) (clientState.local_security_counter >> 8));
      message->data.append(1, (char) (clientState.local_security_counter));

      // auth value
      std::string  extra;
      extra.append(1, clientState.user_id);
      extra.append(clientState.user_key);
      if (extra.length() < 23)
          extra.append(23 - extra.length(), 0);
      std::string auth_value = compute_auth_value(extra, 0x04, clientState.remote_session_nonce, clientState.local_security_counter, cardKey);
      message->data.append(auth_value);
      sendMessage(message, false);
    } else {
      requestPair = true;
    }
  }

void EqivaKeyBle::sendNonce() {
    sendingNonce = true;
    clientState.local_session_nonce.clear();
    for (int i = 0; i < 8; i++)
        clientState.local_session_nonce.append(1,esp_random());
  
    auto *noncemsg = new eQ3Message::Connection_Request_Message;
    sendMessage(noncemsg, true);
}

bool EqivaKeyBle::sendMessage(eQ3Message::Message *msg, bool nonce) {
    if (((sendingNonce == false && clientState.remote_session_nonce.length() > 0) || nonce) && this->state() == espbt::ClientState::ESTABLISHED) {
      std::string data;
      if (msg->isSecure()) {
          std::string padded_data;
          padded_data.append(msg->encode(&clientState));
          int pad_to = generic_ceil(padded_data.length(), 15, 8);
          if (pad_to > padded_data.length())
              padded_data.append(pad_to - padded_data.length(), 0);
          // crypt_data(padded_data, msg->id, clientState.remote_session_nonce, clientState.local_security_counter, clientState.user_key);
          data.append(1, msg->id);
          data.append(crypt_data(padded_data, msg->id, clientState.remote_session_nonce, clientState.local_security_counter, clientState.user_key));
          data.append(1, (char) (clientState.local_security_counter >> 8));
          data.append(1, (char) clientState.local_security_counter);
          data.append(compute_auth_value(padded_data, msg->id, clientState.remote_session_nonce, clientState.local_security_counter, clientState.user_key));
          clientState.local_security_counter++;
      } else {
          data.append(1, msg->id);
          data.append(msg->encode(&clientState));
      }
    
      // fragment
      int chunks = data.length() / 15;
      if (data.length() % 15 > 0)
          chunks += 1;
      for (int i = 0; i < chunks; i++) {
          eQ3Message::MessageFragment frag;
          frag.data.append(1, (i ? 0 : 0x80) + (chunks - 1 - i)); // fragment status byte
          frag.data.append(data.substr(i * 15, 15));
          if (frag.data.length() < 16)
              frag.data.append(16 - (frag.data.length() % 16), 0);  // padding
          sendQueue.push(frag);
          sendFragment();
      }
      free(msg);
      return true;
    } else {
      ESP_LOGI(TAG, "Waiting for connection... (sendingNonce: %s) (remoteSessionNonce: %s) (remoteSessionNonce: %s) (clientStateEstablished: %s)", sendingNonce ? 'true' : 'false', clientState.remote_session_nonce.length() > 0 ? 'true' : 'false', this->state() == espbt::ClientState::ESTABLISHED ? 'true' : 'false');
      currentMsg = msg;
      return false;
    }
}

void EqivaKeyBle::sendFragment() {
    ESP_LOGD(TAG, "Check send frag: %s, %s", sendQueue.empty()  ? "empty" : "not-empty", sending ? "sending" : "not-sending");

    if (sendQueue.empty() || sending || this->state_ != espbt::ClientState::ESTABLISHED)
      return;
    sending = true;
    std::string data = sendQueue.front().data;
    sendQueue.pop();
    write->write_value((uint8_t *) (data.c_str()), 16, ESP_GATT_WRITE_TYPE_RSP);
}

}  // namespace eqiva_key_ble
}  // namespace esphome

#endif
