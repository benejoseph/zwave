#ifndef ZWAVE_PROCESSOR_H_
#define ZWAVE_PROCESSOR_H_

#include <condition_variable>
#include <cstdint>
#include <list>
#include <mutex>
#include <queue>
#include <thread>

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
  ZWaveProcessor(const std::string &zwave_dongle_dev_path);

  static void MainThreadFunc(ZWaveProcessor *processor);

  void OnNotification(OpenZWave::Notification const *notification);

  void Command(char command);

 private:
  void DoNextCommand();

  bool IsExit();

  void TurnOnSwitchNode(uint8_t node_id, bool value);

  struct NodeInfo {
    uint32_t m_homeId;
    uint8_t m_nodeId;
    bool m_polled;
    std::list<OpenZWave::ValueID> m_values;
  };

  NodeInfo *GetNodeInfo(OpenZWave::Notification const *notification) const;
  NodeInfo *GetNodeInfo(int nodeId) const;

  uint32_t homeId;
  bool initFailed = false;
  std::list<NodeInfo *> nodes_;
  std::string zwave_dongle_dev_path_;
  int port_;

  std::mutex command_mutex_;
  std::condition_variable command_condition_;
  std::queue<char> command_queue_;
  bool is_exit_ = false;
};

}  // namespace zwave_app

#endif  // ZWAVE_PROCESSOR_H_
