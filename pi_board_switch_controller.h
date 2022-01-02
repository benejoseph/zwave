#ifndef _PI_BOARD_SWITCH_CONTROLLER_H_
#define _PI_BOARD_SWITCH_CONTROLLER_H_

#include "pi_board_switch_processor.h"
#include "zwave_processor.h"

class PiBoardSwitchController {
public:
  PiBoardSwitchController(zwave_app::ZWaveProcessor &zwave_processor);

private:
  constexpr static int kNumPushesToPair = 3;

  struct PushButtonDuration {
    uint32_t start_tick;
    uint32_t end_tick;
  };

  void OnSwitch(SwitchId switch_id, int level, uint32_t tick_us);

  void OnPushButton(int level, uint32_t tick_us);

  PushButtonDuration current_push_;

  zwave_app::ZWaveProcessor &zwave_processor_;

  uint32_t on_push_ticks_us_[kNumPushesToPair] = {0, 0, 0};
  uint32_t on_push_idx_ = 0;
  uint32_t on_push_tick_counts_ = 0;
  bool is_waiting_for_pairing_long_press_ = false;
};

#endif // _PI_BOARD_SWITCH_CONTROLLER_H_
