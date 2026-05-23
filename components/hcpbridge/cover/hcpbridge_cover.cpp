#include "../hcpbridge.h"
#include "hcpbridge_cover.h"

namespace esphome {
namespace hcpbridge {

cover::CoverTraits HCPBridgeCover::get_traits() {
  auto traits = cover::CoverTraits();
  traits.set_supports_stop(true);
  traits.set_supports_position(true);
  return traits;
}

void HCPBridgeCover::control(const cover::CoverCall &call) {
  if (this->parent_ == nullptr) return;

  if (call.get_stop()) {
    if (this->current_operation == esphome::cover::COVER_OPERATION_OPENING) {
      this->parent_->set_command(CMD_OPEN);
    } else if (this->current_operation == esphome::cover::COVER_OPERATION_CLOSING) {
      this->parent_->set_command(CMD_CLOSE);
    }
    return;
  }

  if (call.get_toggle()) {
    this->parent_->set_command(CMD_TOGGLE);
    return;
  }

  if (call.get_position().has_value()) {
    float target_position = *call.get_position();
    if (target_position == cover::COVER_OPEN) {
      this->parent_->set_command(CMD_OPEN);
    } else if (target_position == cover::COVER_CLOSED) {
      this->parent_->set_command(CMD_CLOSE);
    } else {
      this->target_position_ = POSITION_OPEN * target_position;
    }
  }
}

void HCPBridgeCover::set_parent(HCPBridge *parent) {
  this->parent_ = parent;
}

void HCPBridgeCover::update(const hcp_broadcast *bcast) {
  bool changed = false;

  if (this->last_position_ != bcast->position) {
    changed = true;
    this->last_position_ = bcast->position;

    this->position = this->last_position_ / (float) POSITION_OPEN;
  }

  if (this->last_state_ != bcast->state) {
    changed = true;
    this->last_state_ = (HCPState) bcast->state;

    switch (this->last_state_) {
    case HCPState::OPENING:
      this->current_operation = esphome::cover::COVER_OPERATION_OPENING;
      break;
    case HCPState::CLOSING:
      this->current_operation = esphome::cover::COVER_OPERATION_CLOSING;
      break;
    case HCPState::MOVE_HALF:
    case HCPState::MOVE_VENTING:
      if (bcast->position < bcast->position_target) {
        this->current_operation = esphome::cover::COVER_OPERATION_OPENING;
      } else {
        this->current_operation = esphome::cover::COVER_OPERATION_CLOSING;
      }
      break;
    default:
      this->current_operation = esphome::cover::COVER_OPERATION_IDLE;
    }
  }

  if (this->parent_->can_set_command() && this->last_state_ != HCPState::UNKNOWN) {
    if (this->target_position_ == POSITION_NULL) {
      // noop
    } else if (this->last_position_ < this->target_position_) {
      if (this->last_state_ != HCPState::OPENING) this->parent_->set_command(CMD_OPEN);
    } else if (this->last_position_ > this->target_position_) {
      if (this->last_state_ != HCPState::CLOSING) this->parent_->set_command(CMD_CLOSE);
    } else if (this->current_operation != esphome::cover::COVER_OPERATION_IDLE) {
      this->parent_->set_command(CMD_TOGGLE);
      this->target_position_ = POSITION_NULL;
    }
  }

  if (changed) this->publish_state();
}

} // namespace hcpbridge
} // namespace esphome
