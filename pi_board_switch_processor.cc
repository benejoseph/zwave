#include "pi_board_switch_processor.h"

#include <assert.h>
#include <iostream>
#include "pigpio/pigpio.h"

// BCM numbers aka "GPIO 22", not the pin number
constexpr int kSwitchBcm1 = 27;
constexpr int kSwitchBcm2 = 18;
constexpr int kSwitchBcm3 = 22;
constexpr int kSwitchBcm4 = 17;
constexpr int kPushButtonSwitchBcm = 16;
//constexpr int kStatusLedBcm =

constexpr int kNumInputGpios = 5;

constexpr int kSampleRateUs = 1000;
constexpr int kGlitchPeriodUs = 5000;
constexpr int kPushButtonGlitchPeriodUs = 20000;


namespace  {

struct GpioConfig {
  SwitchId switch_id;
  int gpio;
  int sample_period_us;
  int glitch_period_us;
};

constexpr GpioConfig kGpioInputConfigs[kNumInputGpios] = {
    {.switch_id = SwitchId::kLightSwitch1,
     .gpio = kSwitchBcm1,
     .sample_period_us = kSampleRateUs,
     .glitch_period_us = kGlitchPeriodUs},
    {.switch_id = SwitchId::kLightSwitch2,
     .gpio = kSwitchBcm2,
     .sample_period_us = kSampleRateUs,
     .glitch_period_us = kGlitchPeriodUs},
    {.switch_id = SwitchId::kLightSwitch3,
     .gpio = kSwitchBcm3,
     .sample_period_us = kSampleRateUs,
     .glitch_period_us = kGlitchPeriodUs},
    {.switch_id = SwitchId::kLightSwitch4,
     .gpio = kSwitchBcm4,
     .sample_period_us = kSampleRateUs,
     .glitch_period_us = kGlitchPeriodUs},
    {.switch_id = SwitchId::kPushButton,
     .gpio = kPushButtonSwitchBcm,
     .sample_period_us = kSampleRateUs,
     .glitch_period_us = kPushButtonGlitchPeriodUs},
};


}  // namespace


PiBoardSwitchProcessor::PiBoardSwitchProcessor() {

}

PiBoardSwitchProcessor& PiBoardSwitchProcessor::GetInstance() {
    static PiBoardSwitchProcessor processor;
    return processor;
}

// static
void PiBoardSwitchProcessor::OnGpioChangeCallback(int gpio, int level, uint32_t tick_us) {
  std::cout << "gpio:" << gpio << ", level:" << level << ", tick_us:" << tick_us << std::endl;
  PiBoardSwitchProcessor::GetInstance().OnGpioChange(gpio, level, tick_us);
}

void PiBoardSwitchProcessor::OnGpioChange(int gpio, int level, uint32_t tick_us) {
  // Stupid linear search.
  for (int i = 0; i < kNumInputGpios; ++i) {
    if (kGpioInputConfigs[i].gpio == gpio && on_switch_change_) {
      on_switch_change_(kGpioInputConfigs[i].switch_id, level, tick_us);
    }
  }
}

void PiBoardSwitchProcessor::SetBlinkStatusLed(int num_blinks, int duration_ms) {
    // TODO implement this
    // make output gpio be writable
}

void PiBoardSwitchProcessor::Start() {
    std::cerr << "Starting..." << std::endl;
    gpioCfgClock(kSampleRateUs, 1, 1);


    if (gpioInitialise() < 0) {
        std::cerr << "FATAL: gpio failed to initialize." << std::endl;
        assert(0);
    }

    for (int i = 0; i < kNumInputGpios; ++i) {
        const GpioConfig& config = kGpioInputConfigs[i];
      gpioSetMode(config.gpio, PI_INPUT);
      gpioGlitchFilter(config.gpio,config.glitch_period_us);
      gpioSetAlertFunc(config.gpio, &PiBoardSwitchProcessor::OnGpioChangeCallback);
    }
}
