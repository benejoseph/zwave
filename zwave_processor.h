#ifndef ZWAVE_PROCESSOR_H_
#define ZWAVE_PROCESSOR_H_

#include <cstdint>
#include <list>
#include <thread>
#include <mutex>

#include "Driver.h"
#include "Group.h"
#include "Manager.h"
#include "Node.h"
#include "Notification.h"
#include "Options.h"
#include "platform/Log.h"

namespace zwave_app {

class ZWaveProcessor {
public:
  ZWaveProcessor(const std::string& zwave_dongle_dev_path);

  void OnNotification(OpenZWave::Notification const *notification);


  private:
  struct NodeInfo {
    uint32_t m_homeId;
    uint8_t m_nodeId;
    bool m_polled;
    std::list<OpenZWave::ValueID> m_values;
  };


  NodeInfo *GetNodeInfo(OpenZWave::Notification const* notification) const;
  NodeInfo *GetNodeInfo(int nodeId) const;

  uint32_t homeId;
  bool initFailed = false;


  std::list<NodeInfo *> nodes_;
  pthread_mutex_t critical_section_;
  pthread_cond_t initCond = PTHREAD_COND_INITIALIZER;
  pthread_mutex_t initMutex = PTHREAD_MUTEX_INITIALIZER;
  std::mutex mutex_;
};

} // namespace zwave_app

#endif //ZWAVE_PROCESSOR_H_

