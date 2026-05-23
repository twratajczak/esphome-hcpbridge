import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_OUTPUT_ID, CONF_NAME
from .. import hcpbridge_ns, CONF_HCPBridge_ID, HCPBridge

DEPENDENCIES = ["hcpbridge"]

HCPBridgeLight = hcpbridge_ns.class_("HCPBridgeLight", light.LightOutput, cg.Component)

CONFIG_SCHEMA = light.light_schema(HCPBridgeLight, light.LightState).extend({
  cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(HCPBridgeLight),
  cv.Optional(CONF_NAME, default="Garage Light"): cv.string,
  cv.Optional(CONF_HCPBridge_ID, default=CONF_HCPBridge_ID): cv.use_id(HCPBridge),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
  var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
  await cg.register_component(var, config)
  await light.register_light(var, config)

  hcp = await cg.get_variable(config[CONF_HCPBridge_ID])
  cg.add(var.set_parent(hcp))
  cg.add(hcp.set_light(var))
