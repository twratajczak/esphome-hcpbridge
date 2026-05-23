import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID, CONF_NAME, CONF_DEVICE_CLASS, ENTITY_CATEGORY_DIAGNOSTIC, DEVICE_CLASS_CONNECTIVITY
from .. import hcpbridge_ns, CONF_HCPBridge_ID, HCPBridge

DEPENDENCIES = ["hcpbridge"]

HCPBridgeConnected = hcpbridge_ns.class_("HCPBridgeConnected", binary_sensor.BinarySensor, cg.Component)

CONFIG_SCHEMA = binary_sensor.binary_sensor_schema(HCPBridgeConnected, entity_category=ENTITY_CATEGORY_DIAGNOSTIC).extend({
  cv.GenerateID(): cv.declare_id(HCPBridgeConnected),
  cv.Optional(CONF_NAME, default="Connected"): cv.string,
  cv.Optional(CONF_HCPBridge_ID, default=CONF_HCPBridge_ID): cv.use_id(HCPBridge),
  cv.Optional(CONF_DEVICE_CLASS, default=DEVICE_CLASS_CONNECTIVITY): cv.string,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
  var = cg.new_Pvariable(config[CONF_ID])
  await cg.register_component(var, config)
  await binary_sensor.register_binary_sensor(var, config)

  hcp = await cg.get_variable(config[CONF_HCPBridge_ID])
  cg.add(hcp.set_connected_sensor(var))
