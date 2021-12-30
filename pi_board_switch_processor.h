#ifndef _PIBOARD_SWITCH_PROCESSOR_H_
#define _PIBOARD_SWITCH_PROCESSOR_H_

#include <cstdint>

class PiBoardSwitchProcessor {
  private:
    PiBoardSwitchProcessor();

 public:
   // Singleton accessor. There can be only one!
   static PiBoardSwitchProcessor& GetInstance();

   // Sets ups the GPI pins to be in the proper mode (read, etc.)
   void Start();

   void OnGpioChange(int gpio, int level, uint32_t tick);

  void DoSomething();

};


#endif //_PIBOARD_SWITCH_PROCESSOR_H_
