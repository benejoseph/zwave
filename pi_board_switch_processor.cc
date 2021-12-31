#include "pi_board_switch_processor.h"

#include <assert.h>
#include <iostream>
#include "pigpio/pigpio.h"

// BCM numbers aka "GPIO 22", not the pin number
constexpr int kSwitchBcm1 = 17;
constexpr int kSwitchBcm2 = 27;
constexpr int kSwitchBcm3 = 22;
constexpr int kSwitchBcm4 = 25;

constexpr int kNumGpios = 4;

constexpr int kSampleRateUs = 1000;
constexpr int kGlitchPeriodUs = 5000;



namespace  {
struct GpioConfig {
  int gpio;
  int sample_period_us;
  int glitch_period_us;
};

constexpr GpioConfig kGpioConfig[kNumGpios] = {
    {.gpio = kSwitchBcm1,
     .sample_period_us = kSampleRateUs,
     .glitch_period_us = kGlitchPeriodUs},
    {.gpio = kSwitchBcm2,
     .sample_period_us = kSampleRateUs,
     .glitch_period_us = kGlitchPeriodUs},
    {.gpio = kSwitchBcm3,
     .sample_period_us = kSampleRateUs,
     .glitch_period_us = kGlitchPeriodUs},
    {.gpio = kSwitchBcm4,
     .sample_period_us = kSampleRateUs,
     .glitch_period_us = kGlitchPeriodUs},
};


void OnEdge(int gpio, int level, uint32_t tick) {
  PiBoardSwitchProcessor::GetInstance().OnGpioChange(gpio, level, tick);
}
}  // namespace


PiBoardSwitchProcessor::PiBoardSwitchProcessor() {

}

PiBoardSwitchProcessor& PiBoardSwitchProcessor::GetInstance() {
    static PiBoardSwitchProcessor processor;
    return processor;
}


void PiBoardSwitchProcessor::OnGpioChange(int gpio, int level, uint32_t tick) {
  std::cout << "gpio:" << gpio << ", level:" << level << ", tick:" << tick << std::endl;
}

void PiBoardSwitchProcessor::DoSomething() {

}

void PiBoardSwitchProcessor::Start() {
    std::cerr << "Starting..." << std::endl;
    gpioCfgClock(kSampleRateUs, 1, 1);


    if (gpioInitialise() < 0) {
        std::cerr << "FATAL: gpio failed to initialize." << std::endl;
        assert(0);
    }

    for (int i = 0; i < kNumGpios; ++i) {
        const GpioConfig& config = kGpioConfig[i];
      gpioSetMode(config.gpio, PI_INPUT);
      gpioGlitchFilter(config.gpio,config.glitch_period_us);
      gpioSetAlertFunc(config.gpio, OnEdge);
    }
}
