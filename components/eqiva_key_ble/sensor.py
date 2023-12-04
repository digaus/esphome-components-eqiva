import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    ENTITY_CATEGORY_NONE,
    ICON_BATTERY,
)
from . import CONF_EQIVA_KEY_BLE_ID, EqivaKeyBle

DEPENDENCIES = ["eqiva_key_ble"]

CONF_STATUS = "status"
CONF_LOW_BATTERY = "low_battery"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_EQIVA_KEY_BLE_ID): cv.use_id(EqivaKeyBle),
        cv.Optional(CONF_STATUS): sensor.sensor_schema(
            accuracy_decimals=0,
            entity_category=ENTITY_CATEGORY_NONE,
        ),
        cv.Optional(CONF_LOW_BATTERY): sensor.sensor_schema(
            icon=ICON_BATTERY,
            accuracy_decimals=0,
            entity_category=ENTITY_CATEGORY_NONE,
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_EQIVA_KEY_BLE_ID])

    for key in [
        CONF_STATUS,
        CONF_LOW_BATTERY,
    ]:
        if key not in config:
            continue
        conf = config[key]
        sens = await sensor.new_sensor(conf)
        cg.add(getattr(hub, f"set_{key}_sensor")(sens))
