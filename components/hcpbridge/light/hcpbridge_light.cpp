#include "../hcpbridge.h"
#include "hcpbridge_light.h"

namespace esphome {
namespace hcpbridge {


light::LightTraits HCPBridgeLight::get_traits() {
  auto traits = light::LightTraits();
  traits.set_supported_color_modes({light::ColorMode::ON_OFF});
  return traits;
}

void HCPBridgeLight::setup_state(light::LightState *state) {
  this->light_state_ = state;
}

void HCPBridgeLight::write_state(light::LightState *state) {
  bool is_on;
  state->current_values_as_binary(&is_on);

  if (this->is_on_ != is_on) {
    this->parent_->set_command(CMD_LAMP);
  }
}

void HCPBridgeLight::set_parent(HCPBridge *parent) {
  this->parent_ = parent;
}

void HCPBridgeLight::update(const hcp_broadcast *bcast) {
  const bool is_on = bcast->light & 0xF0; // lower nibble is external light?
  if (this->is_on_ != is_on) {
    this->is_on_ = is_on;
    if (this->light_state_) {
      auto call = this->light_state_->make_call();
      call.set_state(is_on);
      call.perform();
    }
  }
}

} // namespace hcpbridge
} // namespace esphome
