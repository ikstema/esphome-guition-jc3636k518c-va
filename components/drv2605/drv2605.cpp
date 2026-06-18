#include "drv2605.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace drv2605 {

static const char *const TAG = "drv2605";

void DRV2605Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up DRV2605...");

  // Read status register to check communication
  uint8_t status;
  if (!this->read_register_(DRV2605_REG_STATUS, &status)) {
    ESP_LOGE(TAG, "Failed to communicate with DRV2605");
    this->mark_failed();
    return;
  }

  // Exit standby mode
  if (!this->write_register_(DRV2605_REG_MODE, 0x00)) {
    ESP_LOGE(TAG, "Failed to exit standby mode");
    this->mark_failed();
    return;
  }
  delay(1);

  // Configure the actuator. The K518 board uses an ERM motor in open-loop mode.
  if (this->is_lra_) {
    if (!this->write_register_(DRV2605_REG_RATEDV, 0x2F) ||
        !this->write_register_(DRV2605_REG_CLAMPV, 0x59) ||
        !this->write_register_(DRV2605_REG_FEEDBACK, 0xB6) ||
        !this->write_register_(DRV2605_REG_CONTROL4, 0x30)) {
      ESP_LOGE(TAG, "Failed to configure LRA registers");
      this->mark_failed();
      return;
    }
    // Auto-calibrate once at boot so the LRA actually moves.
    this->set_mode_(DRV2605_MODE_AUTOCAL);
    this->write_register_(DRV2605_REG_GO, 0x01);
    for (int i = 0; i < 400; i++) {
      uint8_t go = 1;
      if (!this->read_register_(DRV2605_REG_GO, &go))
        break;
      if ((go & 0x01) == 0)
        break;
      delay(5);
    }
    uint8_t status_after = 0;
    if (this->read_register_(DRV2605_REG_STATUS, &status_after) && (status_after & 0x08)) {
      ESP_LOGW(TAG, "DRV2605 LRA autocal reported DIAG fault (status=0x%02x)", status_after);
    } else {
      ESP_LOGCONFIG(TAG, "DRV2605 LRA autocal finished");
    }
  } else {
    uint8_t feedback = 0;
    uint8_t control3 = 0;
    if (!this->read_register_(DRV2605_REG_FEEDBACK, &feedback) ||
        !this->read_register_(DRV2605_REG_CONTROL3, &control3) ||
        !this->write_register_(DRV2605_REG_FEEDBACK, feedback & 0x7F) ||
        !this->write_register_(DRV2605_REG_CONTROL3, control3 | 0x20)) {
      ESP_LOGE(TAG, "Failed to configure ERM open-loop mode");
      this->mark_failed();
      return;
    }
  }

  if (!this->write_register_(DRV2605_REG_RTPIN, 0x00) ||
      !this->write_register_(DRV2605_REG_OVERDRIVE, 0x00) ||
      !this->write_register_(DRV2605_REG_SUSTAINPOS, 0x00) ||
      !this->write_register_(DRV2605_REG_SUSTAINNEG, 0x00) ||
      !this->write_register_(DRV2605_REG_BREAK, 0x00) ||
      !this->write_register_(DRV2605_REG_LIBRARY, this->library_) ||
      !this->write_register_(DRV2605_REG_MODE, DRV2605_MODE_INTTRIG)) {
    ESP_LOGE(TAG, "Failed to finish DRV2605 configuration");
    this->mark_failed();
    return;
  }

  ESP_LOGCONFIG(TAG, "DRV2605 setup complete");
}

void DRV2605Component::dump_config() {
  ESP_LOGCONFIG(TAG, "DRV2605:");
  LOG_I2C_DEVICE(this);
  ESP_LOGCONFIG(TAG, "  Motor Type: %s", this->is_lra_ ? "LRA" : "ERM");
  ESP_LOGCONFIG(TAG, "  Library: %d", this->library_);
  
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with DRV2605 failed!");
  }
}

bool DRV2605Component::write_register_(uint8_t reg, uint8_t value) {
  return this->write_byte(reg, value);
}

bool DRV2605Component::read_register_(uint8_t reg, uint8_t *value) {
  return this->read_byte(reg, value);
}

void DRV2605Component::select_library_(uint8_t library) {
  this->write_register_(DRV2605_REG_LIBRARY, library);
}

void DRV2605Component::set_mode_(uint8_t mode) {
  this->write_register_(DRV2605_REG_MODE, mode);
}

void DRV2605Component::play_effect(uint8_t effect) {
  if (effect == 0 || effect > 123) {
    ESP_LOGW(TAG, "Invalid effect ID: %d (must be 1-123)", effect);
    return;
  }

  // Ensure internal-trigger library mode before playing a ROM effect.
  this->set_mode_(DRV2605_MODE_INTTRIG);
  this->select_library_(this->library_);

  // Set the waveform in sequence register 1
  this->write_register_(DRV2605_REG_WAVESEQ1, effect);
  // End the sequence
  this->write_register_(DRV2605_REG_WAVESEQ2, 0);
  
  // Start playback
  this->write_register_(DRV2605_REG_GO, 0x01);
  
  ESP_LOGD(TAG, "Playing effect %d", effect);
}

void DRV2605Component::play_sequence(const std::vector<uint8_t> &effects) {
  if (effects.empty()) {
    ESP_LOGW(TAG, "Empty effect sequence");
    return;
  }

  this->set_mode_(DRV2605_MODE_INTTRIG);
  this->select_library_(this->library_);

  // Maximum 8 effects in sequence
  size_t count = std::min(effects.size(), size_t(8));
  
  // Write effects to sequence registers
  for (size_t i = 0; i < count; i++) {
    if (effects[i] == 0 || effects[i] > 123) {
      ESP_LOGW(TAG, "Invalid effect ID at position %d: %d", i, effects[i]);
      continue;
    }
    this->write_register_(DRV2605_REG_WAVESEQ1 + i, effects[i]);
  }
  
  // End the sequence if less than 8 effects
  if (count < 8) {
    this->write_register_(DRV2605_REG_WAVESEQ1 + count, 0);
  }
  
  // Start playback
  this->write_register_(DRV2605_REG_GO, 0x01);
  
  ESP_LOGD(TAG, "Playing sequence of %d effects", count);
}

void DRV2605Component::stop() {
  // Clear all sequences
  for (uint8_t i = 0; i < 8; i++) {
    this->write_register_(DRV2605_REG_WAVESEQ1 + i, 0);
  }
  
  // Stop playback
  this->write_register_(DRV2605_REG_GO, 0x00);
  
  ESP_LOGD(TAG, "Stopped playback");
}

void DRV2605Component::set_realtime_value(uint8_t value) {
  // Set to real-time playback mode
  this->set_mode_(DRV2605_MODE_REALTIME);
  
  // Set the RTP value
  this->write_register_(DRV2605_REG_RTPIN, value);
  
  ESP_LOGD(TAG, "Set realtime value: %d", value);
}

void DRV2605Component::standby() {
  this->set_mode_(DRV2605_MODE_STANDBY);
  ESP_LOGD(TAG, "Entered standby mode");
}

void DRV2605Component::wake() {
  this->set_mode_(DRV2605_MODE_INTTRIG);
  this->select_library_(this->library_);
  ESP_LOGD(TAG, "Woke from standby");
}

}  // namespace drv2605
}  // namespace esphome
