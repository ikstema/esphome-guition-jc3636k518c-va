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
  ESP_LOGCONFIG(TAG, "Detected device status=0x%02X, chip_id=0x%02X", status, status >> 5);

  // Reset the DRV2605 itself. It remains powered across an ESP OTA reboot and
  // otherwise keeps stale actuator/open-loop settings from older firmware.
  if (!this->write_register_(DRV2605_REG_MODE, DRV2605_MODE_RESET)) {
    ESP_LOGE(TAG, "Failed to reset DRV2605");
    this->mark_failed();
    return;
  }
  delay(2);

  // The Waveshare/Guition K518 schematic identifies the actuator as LRA.
  // Use the same minimal closed-loop setup as the working RAR driver:
  // N_ERM_LRA in FEEDBACK plus ROM library 6. Do not run calibration with
  // guessed voltage limits; the chip's LRA library handles resonance tracking.
  const uint8_t feedback = this->is_lra_ ? 0x80 : 0x00;
  if (!this->write_register_(DRV2605_REG_MODE, DRV2605_MODE_INTTRIG) ||
      !this->write_register_(DRV2605_REG_FEEDBACK, feedback) ||
      !this->write_register_(DRV2605_REG_RTPIN, 0x00) ||
      !this->write_register_(DRV2605_REG_GO, 0x00) ||
      !this->write_register_(DRV2605_REG_LIBRARY, this->library_) ||
      !this->write_register_(DRV2605_REG_MODE, DRV2605_MODE_INTTRIG)) {
    ESP_LOGE(TAG, "Failed to configure DRV2605 actuator");
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

  uint8_t status = 0, mode = 0, library = 0, feedback = 0, control3 = 0;
  if (this->read_register_(DRV2605_REG_STATUS, &status) &&
      this->read_register_(DRV2605_REG_MODE, &mode) &&
      this->read_register_(DRV2605_REG_LIBRARY, &library) &&
      this->read_register_(DRV2605_REG_FEEDBACK, &feedback) &&
      this->read_register_(DRV2605_REG_CONTROL3, &control3)) {
    ESP_LOGCONFIG(TAG, "  Registers: STATUS=0x%02X MODE=0x%02X LIB=0x%02X FEEDBACK=0x%02X CONTROL3=0x%02X",
                  status, mode, library, feedback, control3);
  } else {
    ESP_LOGE(TAG, "Failed to read diagnostic registers");
  }

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

  if (!this->write_register_(DRV2605_REG_GO, 0x00) ||
      !this->write_register_(DRV2605_REG_MODE, DRV2605_MODE_INTTRIG) ||
      !this->write_register_(DRV2605_REG_LIBRARY, this->library_) ||
      !this->write_register_(DRV2605_REG_WAVESEQ1, effect) ||
      !this->write_register_(DRV2605_REG_WAVESEQ2, 0) ||
      !this->write_register_(DRV2605_REG_GO, 0x01)) {
    ESP_LOGE(TAG, "Failed to play effect %d", effect);
    return;
  }

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
  // RTP keeps driving until RTPIN is cleared, so stop both playback paths.
  this->write_register_(DRV2605_REG_RTPIN, 0x00);

  // Clear all sequences
  for (uint8_t i = 0; i < 8; i++) {
    this->write_register_(DRV2605_REG_WAVESEQ1 + i, 0);
  }
  
  // Stop playback
  this->write_register_(DRV2605_REG_GO, 0x00);
  
  ESP_LOGD(TAG, "Stopped playback");
}

void DRV2605Component::set_realtime_value(uint8_t value) {
  if (!this->write_register_(DRV2605_REG_GO, 0x00) ||
      !this->write_register_(DRV2605_REG_MODE, DRV2605_MODE_REALTIME) ||
      !this->write_register_(DRV2605_REG_RTPIN, value)) {
    ESP_LOGE(TAG, "Failed to set realtime value: %d", value);
    return;
  }

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
