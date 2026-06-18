#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace drv2605 {

#ifndef VERSION_CODE
  #define VERSION_CODE(major, minor, patch) 0
#endif

#ifndef ESPHOME_VERSION_CODE
  #define ESPHOME_VERSION_CODE VERSION_CODE(0, 0, 0)
#endif

// DRV2605 Register addresses
static const uint8_t DRV2605_REG_STATUS = 0x00;
static const uint8_t DRV2605_REG_MODE = 0x01;
static const uint8_t DRV2605_REG_RTPIN = 0x02;
static const uint8_t DRV2605_REG_LIBRARY = 0x03;
static const uint8_t DRV2605_REG_WAVESEQ1 = 0x04;
static const uint8_t DRV2605_REG_WAVESEQ2 = 0x05;
static const uint8_t DRV2605_REG_WAVESEQ3 = 0x06;
static const uint8_t DRV2605_REG_WAVESEQ4 = 0x07;
static const uint8_t DRV2605_REG_WAVESEQ5 = 0x08;
static const uint8_t DRV2605_REG_WAVESEQ6 = 0x09;
static const uint8_t DRV2605_REG_WAVESEQ7 = 0x0A;
static const uint8_t DRV2605_REG_WAVESEQ8 = 0x0B;
static const uint8_t DRV2605_REG_GO = 0x0C;
static const uint8_t DRV2605_REG_OVERDRIVE = 0x0D;
static const uint8_t DRV2605_REG_SUSTAINPOS = 0x0E;
static const uint8_t DRV2605_REG_SUSTAINNEG = 0x0F;
static const uint8_t DRV2605_REG_BREAK = 0x10;
static const uint8_t DRV2605_REG_AUDIOCTRL = 0x11;
static const uint8_t DRV2605_REG_AUDIOLVL = 0x12;
static const uint8_t DRV2605_REG_AUDIOMAX = 0x13;
static const uint8_t DRV2605_REG_RATEDV = 0x16;
static const uint8_t DRV2605_REG_CLAMPV = 0x17;
static const uint8_t DRV2605_REG_AUTOCALCOMP = 0x18;
static const uint8_t DRV2605_REG_AUTOCALEMP = 0x19;
static const uint8_t DRV2605_REG_FEEDBACK = 0x1A;
static const uint8_t DRV2605_REG_CONTROL1 = 0x1B;
static const uint8_t DRV2605_REG_CONTROL2 = 0x1C;
static const uint8_t DRV2605_REG_CONTROL3 = 0x1D;
static const uint8_t DRV2605_REG_CONTROL4 = 0x1E;
static const uint8_t DRV2605_REG_VBAT = 0x21;
static const uint8_t DRV2605_REG_LRARESON = 0x22;

// Mode register values
static const uint8_t DRV2605_MODE_INTTRIG = 0x00;
static const uint8_t DRV2605_MODE_EXTTRIGEDGE = 0x01;
static const uint8_t DRV2605_MODE_EXTTRIGLVL = 0x02;
static const uint8_t DRV2605_MODE_PWMANALOG = 0x03;
static const uint8_t DRV2605_MODE_AUDIOVIBE = 0x04;
static const uint8_t DRV2605_MODE_REALTIME = 0x05;
static const uint8_t DRV2605_MODE_DIAGNOS = 0x06;
static const uint8_t DRV2605_MODE_AUTOCAL = 0x07;
static const uint8_t DRV2605_MODE_STANDBY = 0x40;

class DRV2605Component : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_motor_type(bool is_lra) { this->is_lra_ = is_lra; }
  void set_library(uint8_t library) { this->library_ = library; }

  // Play a single haptic effect by ID (1-123)
  void play_effect(uint8_t effect);
  
  // Play a sequence of up to 8 effects
  void play_sequence(const std::vector<uint8_t> &effects);
  
  // Stop any playing effects
  void stop();
  
  // Set real-time value (0-255) for continuous vibration
  void set_realtime_value(uint8_t value);
  
  // Go to standby mode (low power)
  void standby();
  
  // Wake from standby
  void wake();

 protected:
  bool is_lra_{false};
  uint8_t library_{1};
  
  bool write_register_(uint8_t reg, uint8_t value);
  bool read_register_(uint8_t reg, uint8_t *value);
  void select_library_(uint8_t library);
  void set_mode_(uint8_t mode);
};

// Automation actions
template<typename... Ts> class PlayEffectAction : public Action<Ts...> {
 public:
  explicit PlayEffectAction(DRV2605Component *parent) : parent_(parent) {}
  
  TEMPLATABLE_VALUE(uint8_t, effect)

#if defined(ESPHOME_VERSION_CODE) && ESPHOME_VERSION_CODE >= VERSION_CODE(2025,11,0)
  void play(const Ts&... x) override {
#else
  void play(Ts... x) override {
#endif
    uint8_t effect = this->effect_.value(x...);
    this->parent_->play_effect(effect);
  }

 protected:
  DRV2605Component *parent_;
};

template<typename... Ts> class PlaySequenceAction : public Action<Ts...> {
 public:
  explicit PlaySequenceAction(DRV2605Component *parent) : parent_(parent) {}
  
  void set_effects(const std::vector<uint8_t> &effects) { this->effects_ = effects; }

#if defined(ESPHOME_VERSION_CODE) && ESPHOME_VERSION_CODE >= VERSION_CODE(2025,11,0)
  void play(const Ts&... x) override {
#else
  void play(Ts... x) override {
#endif
    this->parent_->play_sequence(this->effects_);
  }

 protected:
  DRV2605Component *parent_;
  std::vector<uint8_t> effects_;
};

template<typename... Ts> class StopAction : public Action<Ts...> {
 public:
  explicit StopAction(DRV2605Component *parent) : parent_(parent) {}

#if defined(ESPHOME_VERSION_CODE) && ESPHOME_VERSION_CODE >= VERSION_CODE(2025,11,0)
  void play(const Ts&... x) override {
#else
  void play(Ts... x) override {
#endif
    this->parent_->stop();
  }

 protected:
  DRV2605Component *parent_;
};

template<typename... Ts> class SetRealtimeValueAction : public Action<Ts...> {
 public:
  explicit SetRealtimeValueAction(DRV2605Component *parent) : parent_(parent) {}

  TEMPLATABLE_VALUE(uint8_t, value)

#if defined(ESPHOME_VERSION_CODE) && ESPHOME_VERSION_CODE >= VERSION_CODE(2025,11,0)
  void play(const Ts&... x) override {
#else
  void play(Ts... x) override {
#endif
    uint8_t value = this->value_.value(x...);
    this->parent_->set_realtime_value(value);
  }

 protected:
  DRV2605Component *parent_;
};

template<typename... Ts> class StandbyAction : public Action<Ts...> {
 public:
  explicit StandbyAction(DRV2605Component *parent) : parent_(parent) {}

#if defined(ESPHOME_VERSION_CODE) && ESPHOME_VERSION_CODE >= VERSION_CODE(2025,11,0)
  void play(const Ts&... x) override {
#else
  void play(Ts... x) override {
#endif
    this->parent_->standby();
  }

 protected:
  DRV2605Component *parent_;
};

template<typename... Ts> class WakeAction : public Action<Ts...> {
 public:
  explicit WakeAction(DRV2605Component *parent) : parent_(parent) {}

#if defined(ESPHOME_VERSION_CODE) && ESPHOME_VERSION_CODE >= VERSION_CODE(2025,11,0)
  void play(const Ts&... x) override {
#else
  void play(Ts... x) override {
#endif
    this->parent_->wake();
  }

 protected:
  DRV2605Component *parent_;
};

}  // namespace drv2605
}  // namespace esphome
