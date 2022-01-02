#include "pi_board_switch_controller.h"

#include <iostream>
#include <limits>

namespace {
constexpr uint32_t kPairTickPeriodUs = 3000000;
constexpr uint32_t kResetTickPeriodUs = 10000000;
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
}

PiBoardSwitchController::PiBoardSwitchController(zwave_app::ZWaveProcessor& zwave_processor)
 : zwave_processor_(zwave_processor) {
    current_push_ = {};
    PiBoardSwitchProcessor::GetInstance().SetOnSwitchChange([&](SwitchId switch_id, int level, uint32_t tick_us) {
        OnSwitch(switch_id, level, tick_us);
    });

    zwave_processor.SetOnAddNodeCallback([&]() {
      for (auto node_id : zwave_processor.GetNodeSwitchIds()) {
        PiBoardSwitchProcessor::GetInstance().SetBlinkStatusLed(1);
      }
    });

}


void PiBoardSwitchController::OnSwitch(SwitchId switch_id, int level, uint32_t tick_us) {
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
 // Look for kNumPushesToPair presses within kPairTickPeriodUs.
 if (level == kLevelPushed) {
   current_push_.start_tick = tick_us;
   current_push_.end_tick = tick_us;

   push_ticks_us_[push_idx_] = tick_us;
   push_idx_ = (push_idx_ + 1) % kNumPushesToPair;
   if (++push_tick_counts_ >= kNumPushesToPair) {
     push_tick_counts_ = kNumPushesToPair;
     // Latest minus oldest
     const uint32_t n_click_duration_us = SubtractTicks(tick_us, push_ticks_us_[push_idx_]);
     std::cerr << "duration (ms): " << n_click_duration_us / 1000 << std::endl;
     if (n_click_duration_us < kPairTickPeriodUs) {
       zwave_processor_.StartInclusion();
       PiBoardSwitchProcessor::GetInstance().SetBlinkStatusLed(kNumPushesToPair);
     }
   }
 }

 if (level == kLevelUnpushed && current_push_.start_tick > 0) {
    current_push_.end_tick = tick_us;

    if (SubtractTicks(current_push_.end_tick, current_push_.start_tick) > kResetTickPeriodUs) {
      zwave_processor_.ResetNetwork();
    }
 }
}

