import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import (
    ENTITY_CATEGORY_NONE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_BATTERY,
    ICON_SIGNAL_DISTANCE_VARIANT,
)
from . import CONF_EQIVA_KEY_BLE_ID, EqivaKeyBle

DEPENDENCIES = ["eqiva_key_ble"]

CONF_LOCK_BLE_STATE = "lock_ble_state"
CONF_LOW_BATTERY = "low_battery"
CONF_LOCK_STATUS = "lock_status"
CONF_USER_KEY = "user_key"
CONF_USER_ID = "user_id"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_EQIVA_KEY_BLE_ID): cv.use_id(EqivaKeyBle),
        cv.Optional(CONF_LOCK_BLE_STATE): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_NONE,
        ),
        cv.Optional(CONF_LOW_BATTERY): text_sensor.text_sensor_schema(
            icon=ICON_BATTERY,
            entity_category=ENTITY_CATEGORY_NONE,
        ),
        cv.Optional(CONF_LOCK_STATUS): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_NONE,
        ),
        cv.Optional(CONF_USER_KEY): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_NONE,
        ),
        cv.Optional(CONF_USER_ID): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_NONE,
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_EQIVA_KEY_BLE_ID])

    for key in [
        CONF_LOCK_BLE_STATE,
        CONF_LOW_BATTERY,
        CONF_LOCK_STATUS,
        CONF_USER_KEY,
        CONF_USER_ID
    ]:
        if key not in config:
            continue
        conf = config[key]
        sens = await text_sensor.new_text_sensor(conf)
        cg.add(getattr(hub, f"set_{key}_sensor")(sens))
