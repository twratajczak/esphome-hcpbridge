#pragma once

#include "hcpbridge_types.h"
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/cover/cover.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace hcpbridge {

inline constexpr int FRAME_SIZE = 32;
inline constexpr int QUEUE_SIZE = 2 * (1 + 4 + 1); // 2 bytes * cmd+delay+release
inline constexpr int BCAST_SIZE = 2 * 9 - 1;

class HCPBridge;
class HCPBridgeCover;
class HCPBridgeLight;

class HCPBridgeConnected: public binary_sensor::BinarySensor, public Component { };

enum HCPBridgeStatus {
  INIT,
  OK,
  ERROR,
};

class HCPBridge : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void set_cover(HCPBridgeCover *cover) { this->cover_ = cover; }
  void set_light(HCPBridgeLight *light) { this->light_ = light; }
  void set_connected_sensor(HCPBridgeConnected *connected) { this->connected_ = connected; }
  void set_address(uint8_t address) { this->address_ = address; }

  bool can_set_command();
  void set_command(const uint16_t cmd[2]);

 protected:
  HCPBridgeStatus status_{INIT};
  HCPBridgeCover *cover_{nullptr};
  HCPBridgeLight *light_{nullptr};
  HCPBridgeConnected *connected_{nullptr};
  uint8_t rx_len_{0};
  uint8_t rx_buffer_[FRAME_SIZE];
  uint8_t tx_buffer_[FRAME_SIZE];
  uint8_t cmd_buffer_[QUEUE_SIZE];
  uint8_t cmd_len_{0};
  uint8_t last_bcast_[BCAST_SIZE];
  uint32_t last_rx_us_{0};
  uint32_t last_tx_us_{0};
  uint32_t last_frame_ms_{0};
  uint8_t address_{0x02};
  uint8_t current_position_{0x00};
  uint8_t target_position_{0x00};
  void process_hcp_frame();
  bool has_hcp_frame();
  void respond(const modbus_readwrite *);
};

} // namespace hcpbridge
} // namespace esphome
