#include "pi_board_switch_processor.h"

#include <wiringPi.h>

void PiBoardSwitchProcessor::DoSomething() {
  wiringPiSetup () ;
  pinMode (0, OUTPUT) ;
  for (int i = 0; i < 100; ++i) {
    digitalWrite (0, HIGH) ; delay (500) ;
    digitalWrite (0,  LOW) ; delay (500) ;
  }
}
