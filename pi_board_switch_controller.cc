#include "pi_board_switch_controller.h"

#include <iostream>
#include <limits>

namespace {
constexpr uint32_t FromSeconds(uint32_t seconds) { return seconds * 1000000; }

constexpr uint32_t kPairMultiPressPeriodUs = FromSeconds(3);
constexpr uint32_t kPairPressHoldPeriodUs = FromSeconds(10);
constexpr uint32_t kResetPressHoldPeriodUs = FromSeconds(20);

constexpr int kLevelPushed = 0;
constexpr int kLevelUnpushed = 1;

// Perform a - b (a minus b)
uint32_t SubtractTicks(uint32_t a, uint32_t b) {
  if (b > a) {
    // (0xFF + 0x01) - 0xFA
    // looks like
    //  0x01 - 0xFA
    return std::numeric_limits<uint32_t>::max() - b + a;
  }

  return a - b;
}
} // namespace

PiBoardSwitchController::PiBoardSwitchController(
    zwave_app::ZWaveProcessor &zwave_processor)
    : zwave_processor_(zwave_processor) {
  current_push_ = {};
  PiBoardSwitchProcessor::GetInstance().SetOnSwitchChange(
      [&](SwitchId switch_id, int level, uint32_t tick_us) {
        OnSwitch(switch_id, level, tick_us);
      });
}

void PiBoardSwitchController::OnSwitch(SwitchId switch_id, int level,
                                       uint32_t tick_us) {
  switch (switch_id) {
  case SwitchId::kLightSwitch1: {
    zwave_processor_.TurnOnSwitchNode(2, static_cast<bool>(level));
    break;
  }

  case SwitchId::kLightSwitch2: {
    zwave_processor_.TurnOnSwitchNode(3, static_cast<bool>(level));
    break;
  }

  case SwitchId::kLightSwitch3: {
    zwave_processor_.TurnOnSwitchNode(4, static_cast<bool>(level));
    break;
  }

  case SwitchId::kLightSwitch4: {
    zwave_processor_.TurnOnSwitchNode(5, static_cast<bool>(level));
    break;
  }

  case SwitchId::kPushButton: {
    OnPushButton(level, tick_us);
    break;
  }

  default:
    break;
  }
}

void PiBoardSwitchController::OnPushButton(int level, uint32_t tick_us) {
  // Look for kNumPushesToPair presses within kPairMultiPressPeriodUs with the
  // last press being in duration of of kPairPressHoldPeriod. i.e.  press,
  // press, press and hold......... unpress.
  if (level == kLevelPushed) {
    is_waiting_for_pairing_long_press_ = false;

    current_push_.start_tick = tick_us;
    current_push_.end_tick = tick_us;

    // Circular buffer of size kNumPushesToPair
    on_push_ticks_us_[on_push_idx_] = tick_us;
    on_push_idx_ = (on_push_idx_ + 1) % kNumPushesToPair;
    if (++on_push_tick_counts_ >= kNumPushesToPair) {
      on_push_tick_counts_ = kNumPushesToPair;
      // Latest tick in the circular buffer minus first tick in the circular
      // buffer.
      const uint32_t n_click_duration_us =
          SubtractTicks(tick_us, on_push_ticks_us_[on_push_idx_]);

      if (n_click_duration_us < kPairMultiPressPeriodUs) {
        is_waiting_for_pairing_long_press_ = true;
      }
    }
  }

  if (level == kLevelUnpushed && current_push_.start_tick > 0) {
    current_push_.end_tick = tick_us;
    const uint32_t press_duration_us =
        SubtractTicks(current_push_.end_tick, current_push_.start_tick);

    if (is_waiting_for_pairing_long_press_ &&
        press_duration_us > kPairPressHoldPeriodUs) {
      zwave_processor_.StartInclusion();
      PiBoardSwitchProcessor::GetInstance().SetBlinkStatusLed();
    } else if (press_duration_us > kResetPressHoldPeriodUs) {
      zwave_processor_.ResetNetwork();
      PiBoardSwitchProcessor::GetInstance().SetBlinkStatusLed();
    }
  }
}
