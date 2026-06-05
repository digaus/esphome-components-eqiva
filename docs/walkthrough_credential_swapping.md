# Eqiva Lock Integration Walkthrough

This document summarizes all the major features, optimizations, and bug fixes implemented for the `esphome-eqiva-lock` component over our recent sessions.

## 1. Configurable Connection Modes (`disconnect_timeout`)
We added two schema options in `__init__.py` to support both "Always-On" and "On-Demand" connection strategies, simplifying your YAML setup:
*   `disconnect_timeout` (Time Period, default: `0s`):
    *   If `0s` (default): Stay connected continuously (`set_auto_connect(true)`). In this mode, the C++ component natively queries the lock status every 4 minutes in the background, keeping Home Assistant perfectly in sync without needing YAML cron scripts.
    *   If > `0s` (e.g. `10s`): Disconnect from the lock after that period of inactivity to conserve the lock's battery.
*   `status_update_interval` (Time Period, default: `2h`):
    *   Used only when `disconnect_timeout` is active. Defines how often the ESP32 briefly reconnects in the background to refresh status.

## 2. Auto-Connection on Command Trigger
*   In `sendMessage()`, if a command (e.g. lock/unlock/open/status) is triggered while the BLE client is `IDLE`, a connection is automatically opened.
*   The command is safely buffered and transmitted as soon as the handshake completes.

## 3. Transition State Reporting (`LOCKING` / `UNLOCKING`)
State tracking is moved from YAML globals into the C++ component:
*   We track the last command sent (`last_command_sent_`) and the previous state (`previous_lock_state_`).
*   When the motor turns, the status sensor publishes `"LOCKING"` or `"UNLOCKING"` directly.
*   This allows the Home Assistant template lock to report accurate transitional states natively in the UI.

## 4. Dynamic Credential Swapping & Cache Management
Modified `eqiva_key_ble.cpp` to support seamless credential swapping between multiple locks (e.g., swapping from the Front Door MAC to the Back Door MAC):
* Implemented `clear_bonds_and_cache(mac_str)` which clears both the old and new MAC addresses' GATTC caches and BLE controller bond lists at the ESP-IDF level.
* Introduced `pending_mac_address_` and `pending_connect_` variables to support deferred connections, safely tearing down the active connection before opening the new one.
* Added `configured_mac_address_` to remember the *intended* lock MAC. This prevents the component from erasing the ESP-IDF bonding memory unnecessarily when the ESPHome tracker temporarily resets its state during idle periods.

## 5. YAML Component Warning Fixes
* Replaced deprecated `.state` access with `.current_option()` for the `direction`, `position`, and `turns` select components in `example.yaml`.

## 6. ⚡ Latency & Connection Stability Fixes
During testing, we discovered the ESPHome `BLEClientBase` was racing the radio and wiping its cache unnecessarily on every command. We implemented the following fixes to achieve maximum theoretical performance:

* **Proper Parent Loop Call**: Added `BLEClientBase::loop()` to the `EqivaKeyBle::loop()` override to ensure ESP-IDF GATT application registration actually occurs (fixing an `INIT` state limbo).
* **Restored Direct Connect**: Bypassed the ESPHome tracker's passive scanning delay by calling `esp_ble_gattc_open(..., true)` directly under the hood. This allows the ESP32 hardware to seamlessly wait for the lock's brief advertisement window and fire a connection request natively in the background, shaving off an entire second of scan latency.

### Performance Results
By properly utilizing the ESP-IDF GATT cache and bypassing redundant service discoveries, we drastically reduced latency.

| Metric | Before Fixes (Full Service Discovery) | After Fixes (Cached + Direct Connect) | Improvement |
| :--- | :--- | :--- | :--- |
| **Physical Connect** | ~1.1 s | ~1.2 s* | (Hardware dependent) |
| **Handshake & Cache Load** | 1,289 ms | **5 ms** | **🚀 -1,284 ms** |
| **Time until Motor Starts** | 2.6+ s | **~1.5 s** | **🚀 -1.1 s** |

*\*Note: The physical connection time fluctuates (ranging from 200ms to 2.5s) purely based on the lock's sleep cycle and when it decides to broadcast its BLE advertisement. The ESP32 hardware passively listens and connects instantly upon detection.*
