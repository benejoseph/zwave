#include "zwave_processor.h"

#include <iostream>
#include <thread>

int main() {
  zwave_app::ZWaveProcessor processor("/dev/ttyACM0");

  std::thread processor_thread(zwave_app::ZWaveProcessor::MainThreadFunc, &processor);

    while (true) {
      std::string buf;
      std::cin >> buf;
      processor.Command(buf[0]);

      if (buf[0] == 'q') {
        break;
      }

    }

  processor_thread.join();
}
