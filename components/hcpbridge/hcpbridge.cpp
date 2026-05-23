#include "hcpbridge.h"
#include "cover/hcpbridge_cover.h"
#include "light/hcpbridge_light.h"
#include "esphome/core/log.h"

namespace esphome {
namespace hcpbridge {

static const char *const TAG = "hcpbridge";
static const uint32_t HCP_BAUD = 57600;
static const uint32_t MIN_RESP_US = 2000;
static const uint32_t MAX_RESP_US = 9000;
static const uint32_t INTER_FRAME_US = 1750; // MODBUS over Serial Line Specification and Implementation Guide
static const uint32_t INTER_CHAR_US = 750;
static const uint32_t TIMEOUT_MS = 3000;

static const uint8_t S_REG = 2;
static const uint8_t S_CRC = 2;

static char log_buffer[FRAME_SIZE * 3];
#define FORMAT_HEX(data, len) esphome::format_hex_pretty_to(log_buffer, sizeof(log_buffer), data, len)

uint16_t hcp_modbus_crc16(const uint8_t *data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t pos = 0; pos < len; pos++) {
    crc ^= (uint16_t)data[pos];
    for (int i = 8; i != 0; i--) {
      if ((crc & 0x0001) != 0) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

void HCPBridge::setup() {
  this->rx_len_ = 0;
  this->cmd_len_ = 0;
  this->parent_->set_baud_rate(HCP_BAUD);
  this->parent_->set_data_bits(8);
  this->parent_->set_stop_bits(1);
  this->parent_->set_parity(esphome::uart::UART_CONFIG_PARITY_EVEN);
}

void HCPBridge::loop() {
  do {
    while (this->available()) {
      this->read_byte(&this->rx_buffer_[this->rx_len_]);
      this->rx_len_ += 1;
      this->last_rx_us_ = micros();

      if (this->rx_len_ >= FRAME_SIZE) {
        ESP_LOGW(TAG, "Receive buffer overflow after %d!", micros() - this->last_rx_us_);
        this->rx_len_ = 0;
      }
    }
    if (this->has_hcp_frame()) this->process_hcp_frame();
  } while (this->rx_len_ > 0 && micros() - this->last_rx_us_ < INTER_CHAR_US);

  if (this->rx_len_ > 0) {
    this->status_set_warning(LOG_STR("incomplete frame"));
    this->rx_len_ = 0;
  }

  if (this->status_ == HCPBridgeStatus::OK && millis() - this->last_frame_ms_ > TIMEOUT_MS) {
    this->status_set_error(LOG_STR("communication timeout"));
    this->status_ = HCPBridgeStatus::ERROR;
    if (this->connected_) this->connected_->publish_state(false);
  }
}

bool HCPBridge::has_hcp_frame() {
  if (this->rx_len_ < 11) return false;

  if (this->rx_buffer_[1] == FC_WRITE_REGS) {
    const auto *frame = reinterpret_cast<modbus_write *>(this->rx_buffer_);
    return this->rx_len_ >= sizeof(frame) + frame->write_length + S_CRC;
  } else if (this->rx_buffer_[1] == FC_READWRITE_REGS) {
    const auto *frame = reinterpret_cast<modbus_readwrite *>(this->rx_buffer_);
    return this->rx_len_ >= sizeof(frame) + frame->write_length + S_CRC;
  }

  return false;
}

void HCPBridge::process_hcp_frame() {
  uint16_t calculated_crc = hcp_modbus_crc16(this->rx_buffer_, this->rx_len_ - 2);
  uint16_t received_crc = this->rx_buffer_[this->rx_len_ - 2] | (this->rx_buffer_[this->rx_len_ - 1] << 8);

  if (calculated_crc != received_crc) {
    this->status_set_warning(LOG_STR("invalid frame crc"));
    this->rx_len_ = 0;
    return;
  } else this->status_clear_warning();

  uint8_t address = this->rx_buffer_[0];
  uint8_t func_code = this->rx_buffer_[1];

  if (func_code == FC_WRITE_REGS) {
    const auto *frame = reinterpret_cast<modbus_write *>(this->rx_buffer_);
    if (frame->is_broadcast()) {
      const auto *bcast = reinterpret_cast<hcp_broadcast *>(this->rx_buffer_);
      if (this->cover_) this->cover_->update(bcast);
      if (this->light_) this->light_->update(bcast);

#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_DEBUG
      if (memcmp(this->last_bcast_, this->rx_buffer_ + sizeof(modbus_write), BCAST_SIZE)) {
        memcpy(this->last_bcast_, this->rx_buffer_ + sizeof(modbus_write), BCAST_SIZE);
        ESP_LOGD(TAG, "Received bcast [**:%s]", FORMAT_HEX(this->last_bcast_, BCAST_SIZE));
      }
#endif
    }
  } else if (func_code == FC_READWRITE_REGS && address == this->address_) {
    const auto *frame = reinterpret_cast<modbus_readwrite *>(this->rx_buffer_);

    if (frame->is_poll()) {
      this->tx_buffer_[5] = 0x03;
      this->tx_buffer_[6] = 0x01;
      if (this->cmd_len_ >= 2) {
        this->tx_buffer_[7] = this->cmd_buffer_[QUEUE_SIZE - this->cmd_len_--];
        this->tx_buffer_[8] = this->cmd_buffer_[QUEUE_SIZE - this->cmd_len_--];
        if (this->tx_buffer_[7] != 0x0 || this->tx_buffer_[8] != 0x0) {
          ESP_LOGI(TAG, "Send command [%02X%02X]", this->tx_buffer_[7], this->tx_buffer_[8]);
        }
      }
      this->respond(frame);
    } else if (frame->is_ping()) {
      this->respond(frame);
    } else if (frame->is_scan()) {
      this->tx_buffer_[5] = 0x02;
      this->tx_buffer_[6] = 0x05;
      this->tx_buffer_[7] = 0x04;
      this->tx_buffer_[8] = 0x30;
      this->tx_buffer_[9] = 0x10;
      this->respond(frame);
      ESP_LOGI(TAG, "Received bus scan: [%s]", FORMAT_HEX(this->rx_buffer_, this->rx_len_));
    } else {
      ESP_LOGI(TAG, "Unknown frame read=%d at %02X%02X, write %d at %02X%02X, (Size: %d): [ %s]",
          frame->read_count, frame->read_addr_hi, frame->read_addr_lo, frame->write_count, frame->write_addr_hi, frame->write_addr_lo,
          this->rx_len_, FORMAT_HEX(this->rx_buffer_, this->rx_len_));
    }

    this->last_frame_ms_ = millis();
    if (this->status_ != HCPBridgeStatus::OK) {
      this->status_ = HCPBridgeStatus::OK;
      this->status_clear_error();
      if (this->connected_) this->connected_->publish_state(true);
    }
  }
  this->rx_len_ = 0;
}

void HCPBridge::respond(const modbus_readwrite *frame) {
  const int len = sizeof(modbus_response) + S_REG * frame->read_count + S_CRC;

  this->tx_buffer_[0] = frame->address;
  this->tx_buffer_[1] = frame->command;
  this->tx_buffer_[2] = 2 * frame->read_count;
  this->tx_buffer_[3] = frame->counter;
  const uint16_t crc = hcp_modbus_crc16(this->tx_buffer_, len - 2);
  this->tx_buffer_[len - 2] = crc & 0xFF;
  this->tx_buffer_[len - 1] = crc >> 8;

  const uint32_t tx_delay = micros() - this->last_rx_us_;
  if (tx_delay < MIN_RESP_US) delayMicroseconds(MIN_RESP_US - tx_delay);

  this->write_array(this->tx_buffer_, len);
  this->flush();
  this->last_tx_us_ = micros();

  if (this->last_tx_us_ - this->last_rx_us_ > MAX_RESP_US) {
    ESP_LOGW(TAG, "Slow response (%d)", this->last_tx_us_ - this->last_rx_us_);
  }

  memset(this->tx_buffer_, 0, FRAME_SIZE);
}

bool HCPBridge::can_set_command() {
  if (this->status_has_error()) return false;
  if (this->cmd_len_ > 0) return false;

  return true;
}

void HCPBridge::set_command(const uint16_t cmd[2]) {
  ESP_LOGD(TAG, "CMD %04X%04X", cmd[0], cmd[1]);
  if (this->cmd_len_ > 0) {
    ESP_LOGW(TAG, "CMD %04X%04X ignored, send pending (%d)", cmd[0], cmd[1], this->cmd_len_);
    return;
  }
  memset(this->cmd_buffer_, 0, QUEUE_SIZE);
  this->cmd_buffer_[0] = cmd[0] >> 8;
  this->cmd_buffer_[1] = cmd[0] & 0xFF;
  this->cmd_buffer_[10] = cmd[1] >> 8;
  this->cmd_buffer_[11] = cmd[1] & 0xFF;
  this->cmd_len_ = QUEUE_SIZE;
}

} // namespace hcpbridge
} // namespace esphome
