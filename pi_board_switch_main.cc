#include "pi_board_switch_controller.h"
#include "pi_board_switch_processor.h"
#include "zwave_processor.h"

int main() {
  zwave_app::ZWaveProcessor zwave_processor("/dev/ttyACM0");

  PiBoardSwitchProcessor::GetInstance().Start();

  PiBoardSwitchController controller(zwave_processor);

  std::thread zwave_processor_thread(zwave_app::ZWaveProcessor::MainThreadFunc,
                                     &zwave_processor);

  while (true) {
    std::string buf;
    std::cin >> buf;
    zwave_processor.Command(buf[0]);

    if (buf[0] == 'q') {
      break;
    }
  }

  zwave_processor_thread.join();

  return 0;
}
