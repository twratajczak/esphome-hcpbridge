#pragma once

#include "esphome/components/light/light_output.h"
#include "esphome/core/component.h"

namespace esphome {
namespace hcpbridge {

class HCPBridge;
struct hcp_broadcast;

class HCPBridgeLight : public light::LightOutput, public Component {
 public:
  light::LightTraits get_traits() override;
  void setup_state(light::LightState *) override;
  void write_state(light::LightState *) override;
  void set_parent(HCPBridge *);
  void update(const hcp_broadcast *);

 protected:
  HCPBridge *parent_{nullptr};
  light::LightState *light_state_{nullptr};
  bool is_on_{false};
};

} // namespace hcpbridge
} // namespace esphome
