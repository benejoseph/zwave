#include "pi_board_switch_controller.h"

#include <limits>

namespace {
constexpr uint32_t kPairTickPeriodUs = 3000000;
constexpr uint32_t kResetTickPeriodUs = 10000000;

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
    case kLightSwitch1:
    case kLightSwitch2:
    case kLightSwitch3:
    case kLightSwitch4: {
      zwave_processor_.TurnOnSwitchNode(static_cast<uint8_t>(switch_id), static_cast<bool>(level));
      break;
    }

    case kPushButton: {
      OnPushButton(level, tick_us);
      break;
    }

    default:
     break;
  }
}

void PiBoardSwitchController::OnPushButton(int level, uint32_t tick_us) {
 // Look for kNumPushesToPair presses within kPairTickPeriodUs.
 if (level == 1) {
   current_push_.start_tick = tick_us;
   current_push_.end_tick = tick_us;

   push_ticks_us_[push_idx_] = tick_us;
   push_idx_ = (push_idx_ + 1) % kNumPushesToPair;
   if (++push_tick_counts_ >= kNumPushesToPair) {
     push_tick_counts_ = kNumPushesToPair;
     // Latest minus oldest
     if (SubtractTicks(tick_us, push_ticks_us_[push_idx_]) < kPairTickPeriodUs) {
       zwave_processor_.StartInclusion();
       PiBoardSwitchProcessor::GetInstance().SetBlinkStatusLed(kNumPushesToPair);
     }
   }
 }

 if (level == 0 && current_push_.start_tick > 0) {
    current_push_.end_tick = tick_us;

    if (SubtractTicks(current_push_.end_tick, current_push_.start_tick) > kResetTickPeriodUs) {
      zwave_processor_.ResetNetwork();
    }
 }
}

