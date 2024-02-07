Big thanks to previous work done by:  @MariusSchiffer, @tc-maxx, @RoP09, @lumokitho and the original creator @oyooyo




# Use esp-idf framework:
Need to use esp-idf framework due to flash limitations
```
esp32:
  # ...
  framework:
    type: esp-idf
    # Uncomment below for ESP32-C3 if you have unexpected reboots when encrypting data
    # sdkconfig_options:
      # CONFIG_BOOTLOADER_WDT_TIME_MS: "60000"
```

# Webserver:
Using webserver to set parameters from UI

```
web_server:
  include_internal: true
  # needs to be false due to a bug: https://github.com/esphome/issues/issues/5188
  local: false 
  port: 80
```

# Wifi:
We use on_connect and on_disconnect to start/stop BLE scan and set the current parameters
```
wifi:
  # ...
  # We only start BLE scan after WiFi connected, see https://github.com/esphome/issues/issues/2941#issuecomment-1842369092
  on_connect:
    - esp32_ble_tracker.start_scan:
       continuous: true
    # Use below to apply saved input parameters
    - eqiva_key_ble.connect:
      mac_address: !lambda 'return id(mac_address).state;' 
      user_id: !lambda 'return id(user_id).state;'
      user_key: !lambda 'return id(user_key).state;'
  on_disconnect:
    - esp32_ble_tracker.stop_scan:
```

# UI:
We use buttons and inputs to be able to control from UI
```
# button, number and text input for pairing and setting mac/user_id/user-key via UI
button:
  - platform: template
    id: ble_settings
    name: BLE Settings _Save_
    icon: "mdi:content-save"
    on_press:
      - eqiva_key_ble.connect:
          mac_address: !lambda 'return id(mac_address).state;' 
          user_id: !lambda 'return id(user_id).state;'
          user_key: !lambda 'return id(user_key).state;'
  
  - platform: template
    id: ble_disconnect
    name: BLE Settings _Disconnect_
    icon: "mdi:clear"
    on_press:
      - eqiva_key_ble.disconnect:

  - platform: template
    id: ble_pair
    name: BLE Pair _Start_
    icon: "mdi:check-underline"
    on_press:
      - eqiva_key_ble.pair:
          card_key: !lambda 'return id(card_key).state;' 

  - platform: template
    id: lock_settings
    name: Lock Settings _Apply_
    icon: "mdi:check-underline"
    on_press:
      - eqiva_key_ble.settings:
          turn_left: !lambda 'return id(direction).state == "Left";' 
          key_horizontal: !lambda 'return id(position).state == "Horizontal";'
          lock_turns: !lambda 'return atoi(id(turns).state.c_str());'
          
number:
  - platform: template
    mode: box
    name: BLE Settings User ID
    id: user_id
    max_value: 7
    min_value: 0
    step: 1
    optimistic: true
    restore_value: true

text:
  - platform: template
    mode: text
    name: BLE Settings Mac Address
    id: mac_address
    optimistic: true
    restore_value: true
  - platform: template
    mode: text
    name: BLE Settings User Key
    id: user_key
    optimistic: true
    restore_value: true
  - platform: template
    mode: text
    name: BLE Pair Card Key
    id: card_key
    optimistic: true

select:
  - platform: template
    name: Lock Settings Close Direction
    id: direction
    options:
     - "Left"
     - "Right"
    optimistic: true
  - platform: template
    name: Lock Settings Key Position
    id: position
    options:
     - "Vertical"
     - "Horizontal"
    optimistic: true
  - platform: template
    name: Lock Settings Turns
    id: turns
    options:
     - "1"
     - "2"
     - "3"
     - "4"
    optimistic: true
```
# Add external component:
Add this external component so you can use it
```
external_components:
  source: github://digaus/esphome-components-eqiva
  # use refresh when you do not get latest version
  # refresh: 0s

eqiva_key_ble:
  id: key_ble
```

# Discover new lock:

```
esp32_ble_tracker:
   scan_parameters:
    window: 300ms
    # We do not start scan initialy, see https://github.com/esphome/issues/issues/2941#issuecomment-1842369092
    continuous: false

# used for discovering the lock and get the mac_address, check logger for it
eqiva_ble:
  
```

# Sensors:
```
text_sensor: 
  - platform: eqiva_key_ble
    mac_address: 
      name: "Mac Address"
    lock_status:
      id: lock_status
      name: "Lock Status"
    low_battery:
      name: "Low Battery"
    lock_ble_state:
      name: "Lock BLE State"
    user_id:
      name: "User ID"
    user_key:
      name: "User Key"
```

# Refresh lock status:
Call status every 4 minutes because lock seems to disconnect after 5 minutes of inactivity.
Need to watch battery consumption, could also do some other time or present based approaches.

```

time:
  - platform: sntp
    on_time:
      # Every 4 minutes
      - seconds: 0
        minutes: /4
        then:
          - eqiva_key_ble.status:
```

# Create lock component:
```
# Lock component for HA, can also create two locks and use connect service to connect/control two different locks
# One ESP per lock is still recommended
lock:
  - platform: template
    name: "Lock 1"
    lambda: |-
      if (id(lock_status).state == "LOCKED") {
        return LOCK_STATE_LOCKED;
      } else if (id(lock_status).state == "UNLOCKED" || id(lock_status).state == "OPENED") {
        return LOCK_STATE_UNLOCKED;
      } else if(id(lock_status).state == "MOVING") {
        return {};
      } else if (id(lock_status).state == "UNKNOWN") {
        return LOCK_STATE_JAMMED;
      } 
      return LOCK_STATE_LOCKED;
    lock_action:
      - eqiva_key_ble.lock:
    unlock_action:
      - eqiva_key_ble.unlock:
    open_action:
      - eqiva_key_ble.open:
```


# Initial Pairing:
Please do previous steps first!

For connecting and controlling a lock we need the mac_address (see # Discover new lock), the user_id and the user_key
The user_id and user_key can only be retrieved while pairing for which we need the card_key.

1. Get the mac_address
2. Enter mac_address via UI and press save
3. Wait for ESP to connect to lock
4. Start pairing on the lock (hold open button for 5 seconds)
5. Enter card_key (need to scan QR code of the card and copy the result) in the input
6. Press Pair Button on UI
7. ESP should pair and display the user_key and user_id via UI
8. Copy the values to the inputs and press Save Button
