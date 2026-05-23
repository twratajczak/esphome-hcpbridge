#pragma once

#include "esphome/core/datatypes.h"

namespace esphome {
namespace hcpbridge {

inline constexpr uint8_t FC_WRITE_REGS = 0x10;
inline constexpr uint8_t FC_READWRITE_REGS = 0x17;

inline constexpr uint16_t CMD_OPEN[] = {0x0110, 0x0210};
inline constexpr uint16_t CMD_CLOSE[] = {0x0120, 0x0220};
inline constexpr uint16_t CMD_TOGGLE[] = {0x0240, 0x0140};
inline constexpr uint16_t CMD_LAMP[] = {0x0102, 0x0802};

struct __attribute__((packed)) modbus_readwrite {
  uint8_t address;
  uint8_t command;
  uint8_t read_addr_hi;
  uint8_t read_addr_lo;
  uint8_t read_count_pad;
  uint8_t read_count;
  uint8_t write_addr_hi;
  uint8_t write_addr_lo;
  uint8_t write_count_pad;
  uint8_t write_count;
  uint8_t write_length;

  uint8_t counter;

  bool is_valid() const { return write_addr_hi == 0x9C && write_addr_lo == 0x41 && write_count_pad == 0 && read_addr_hi == 0x9C && read_addr_lo == 0xB9 && read_count_pad == 0; }
  bool is_poll() const { return is_valid() && write_count == 2 && read_count == 8; }
  bool is_ping() const { return is_valid() && write_count == 2 && read_count == 2; }
  bool is_scan() const { return is_valid() && write_count == 3 && read_count == 5; }
};
static_assert(sizeof(modbus_readwrite) == 11 + 1);

struct __attribute__((packed)) modbus_write {
  uint8_t address;
  uint8_t command;
  uint8_t write_addr_hi;
  uint8_t write_addr_lo;
  uint8_t write_count_pad;
  uint8_t write_count;
  uint8_t write_length;

  uint8_t counter;

  bool is_valid() const { return write_addr_hi == 0x9D && write_addr_lo == 0x31 && write_count_pad == 0; }
  bool is_broadcast() const { return is_valid() && address == 0x0 && write_count == 9; }
};
static_assert(sizeof(modbus_write) == 7 + 1);

struct __attribute__((packed)) modbus_response {
  uint8_t address;
  uint8_t command;
  uint8_t write_length;
};
static_assert(sizeof(modbus_response) == 3);

struct __attribute__((packed)) hcp_broadcast : modbus_write {
  uint8_t reg_0_lo;
  uint8_t position_target;
  uint8_t position;
  uint8_t state;
  uint8_t state_ext; // 0x61 for VENT
  uint8_t reg_3_hi;
  uint8_t reg_3_lo;
  uint8_t reg_4_hi;
  uint8_t reg_4_lo;
  uint8_t timer;
  uint8_t reg_5_lo;
  uint8_t reg_6_hi;
  uint8_t light;
  uint8_t reg_7_hi;
  uint8_t reg_7_lo;
  uint8_t reg_8_hi;
  uint8_t reg_8_lo;
  uint8_t crc_hi;
  uint8_t crc_lo;
};
static_assert(sizeof(hcp_broadcast) == 27);
struct __attribute__((packed)) hcp_device : modbus_readwrite {
  uint8_t reg_0_lo;
  uint8_t reg_1_hi;
  uint8_t reg_1_lo;
  uint8_t crc_hi;
  uint8_t crc_lo;
};
static_assert(sizeof(hcp_device) == 17);
struct __attribute__((packed)) hcp_scan : modbus_readwrite {
  uint8_t reg_0_lo;
  uint8_t reg_1_hi;
  uint8_t reg_1_lo;
  uint8_t reg_2_hi;
  uint8_t reg_2_lo;
  uint8_t crc_hi;
  uint8_t crc_lo;
};
static_assert(sizeof(hcp_scan) == 19);

} // namespace hcpbridge
} // namespace esphome
