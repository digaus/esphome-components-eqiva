# Optimize ESPHome BLE Component Integration

The goal is to optimize how the `esphome-eqiva-lock` component interacts with the ESPHome configuration, specifically removing inefficient `on_state` polling loops and replacing them with native event triggers.

## User Review Required
The connection hang bug has been fully resolved via a separate set of stability fixes (now tracked in a separate branch). We can now proceed with the original optimization plan. Please review this plan to ensure it aligns with your expectations.

## Proposed Changes

We will introduce new automation triggers to the `eqiva_key_ble` component to allow the YAML configuration to react to events without polling.

### Component Updates (`eqiva_key_ble`)

#### [MODIFY] `components/eqiva_key_ble/__init__.py`
- Register new automation triggers in the ESPHome Python generation script:
  - `on_connection_established`
  - `on_disconnected`
  - `on_status_update`

#### [MODIFY] `components/eqiva_key_ble/eqiva_key_ble.h` & `.cpp`
- Add trigger member variables (e.g., `Trigger<> *connection_established_trigger_`).
- Fire these triggers at the appropriate moments in the component lifecycle (e.g., in `node_state_update`, `set_state`, or when receiving status updates from the lock).

### YAML Configuration Updates

#### [MODIFY] `example.yaml` (or the user's equivalent config)
- Remove `on_state` update loops that continuously check the lock's state.
- Implement the new `on_connection_established` and `on_status_update` triggers to run actions (like publishing to Home Assistant) only when necessary.

## Verification Plan

### Automated Tests
- Build the ESPHome configuration to verify the C++ generation logic correctly wires up the new triggers.

### Manual Verification
- Deploy the updated component to the ESP32.
- Verify through logs that Home Assistant updates are pushed immediately upon connection or status change, rather than being polled constantly.
- Verify that the overall system responsiveness improves and unnecessary log spam from polling is eliminated.
