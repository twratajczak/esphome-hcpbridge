import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]
MULTI_CONF = True

CONF_ADDRESS = "address"
CONF_HCPBridge_ID = "hcpbridge_id"

hcpbridge_ns = cg.esphome_ns.namespace("hcpbridge")
HCPBridge = hcpbridge_ns.class_("HCPBridge", cg.Component, uart.UARTDevice)

CONFIG_SCHEMA = cv.Schema({
  cv.Optional(CONF_ID, default=CONF_HCPBridge_ID): cv.declare_id(HCPBridge),
  cv.Optional(CONF_ADDRESS, default=0x02): cv.hex_uint8_t,
}).extend(uart.UART_DEVICE_SCHEMA).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
  var = cg.new_Pvariable(config[CONF_ID])
  await cg.register_component(var, config)
  await uart.register_uart_device(var, config)
  cg.add(var.set_address(config[CONF_ADDRESS]))
