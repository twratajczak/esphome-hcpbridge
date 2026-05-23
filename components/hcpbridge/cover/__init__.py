import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import cover
from esphome.const import CONF_ID, CONF_NAME
from .. import hcpbridge_ns, CONF_HCPBridge_ID, HCPBridge

DEPENDENCIES = ["hcpbridge"]

HCPBridgeCover = hcpbridge_ns.class_("HCPBridgeCover", cover.Cover, cg.Component)

CONFIG_SCHEMA = cover.cover_schema(HCPBridgeCover).extend({
  cv.GenerateID(): cv.declare_id(HCPBridgeCover),
  cv.Optional(CONF_NAME, default="Garage Cover"): cv.string,
  cv.Optional(CONF_HCPBridge_ID, default=CONF_HCPBridge_ID): cv.use_id(HCPBridge),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
  var = cg.new_Pvariable(config[CONF_ID])
  await cg.register_component(var, config)
  await cover.register_cover(var, config)

  hcp = await cg.get_variable(config[CONF_HCPBridge_ID])
  cg.add(var.set_parent(hcp))
  cg.add(hcp.set_cover(var))
