#pragma once

#include "esphome/core/component.h"
#include "esphome/components/cover/cover.h"

namespace esphome {
namespace hcpbridge {

class HCPBridge;
struct hcp_broadcast;

enum HCPState {
  STOPPED = 0x00,
  OPENING = 0x01,
  CLOSING = 0x02,
  MOVE_HALF = 0x05,
  MOVE_VENTING = 0x09,
  VENT = 0x0A,
  OPEN = 0x20,
  CLOSED = 0x40,
  HALFOPEN = 0x80,
  UNKNOWN = 0xFF,
};

inline constexpr uint8_t POSITION_OPEN = 0xC8;
inline constexpr uint8_t POSITION_NULL = 0xFF;

class HCPBridgeCover : public cover::Cover, public Component {
 public:
  cover::CoverTraits get_traits() override;
  void control(const cover::CoverCall &) override;
  void set_parent(HCPBridge *);
  void update(const hcp_broadcast *);

 protected:
  HCPBridge *parent_{nullptr};

  HCPState last_state_{UNKNOWN};
  uint8_t last_position_{POSITION_NULL};
  uint8_t target_position_{POSITION_NULL};
};

} // namespace hcpbridge
} // namespace esphome
