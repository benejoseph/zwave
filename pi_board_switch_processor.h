#ifndef _PIBOARD_SWITCH_PROCESSOR_H_
#define _PIBOARD_SWITCH_PROCESSOR_H_

#include <cstdint>
#include <functional>

// Values for light switches are the hard-coded node values.
enum class SwitchId : uint8_t {
  kUnknown = 0,
  kLightSwitch1 = 2,
  kLightSwitch2 = 3,
  kLightSwitch3 = 4,
  kLightSwitch4 = 5,
  kPushButton
};

// A wrapper around pgpio listen for switches
// and to blink the status LED.
class PiBoardSwitchProcessor {
private:
  PiBoardSwitchProcessor();

public:
  // Singleton accessor. There can be only one!
  static PiBoardSwitchProcessor &GetInstance();

  // Sets ups the GPIO pins to be in the proper mode (read, etc.)
  void Start();

  // Registers callback that will translate switch changes to their appropriate
  // hard-coded ids.
  void SetOnSwitchChange(
      std::function<void(SwitchId switch_id, int level, uint32_t tick_us)>
          on_switch_change) {
    on_switch_change_ = on_switch_change;
  }

  // Should blink the status LED for a couple of seconds.
  void SetBlinkStatusLed();

private:
  // Callback target for when a GPIO changes level.
  static void OnGpioChangeCallback(int gpio, int level, uint32_t tick_us);
  void OnGpioChange(int gpio, int level, uint32_t tick_us);

  // Callback for switch changes.
  std::function<void(SwitchId switch_id, int level, uint32_t tick_us)>
      on_switch_change_;

  int blink_wave_id_ = -1;
};

#endif //_PIBOARD_SWITCH_PROCESSOR_H_
