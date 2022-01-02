#ifndef ZWAVE_PROCESSOR_H_
#define ZWAVE_PROCESSOR_H_

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <list>
#include <mutex>
#include <queue>
#include <set>
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
  // Provide path to zwave dongle, i.e. "/dev/ttyACM0"
  ZWaveProcessor(const std::string &zwave_dongle_dev_path);

  // Main thread loop.  From main, create a ZWaveProcessor and call this method
  // in a new thread. It will run forever, or until the user quits.
  static void MainThreadFunc(ZWaveProcessor *processor);

  // Give the processor a single character command from the keyboard.
  void Command(char command);

  // This will reset the controller and forget all the nodes that have
  // joined the network.
  void ResetNetwork();

  // Calling AddNewNode starts the inclusion process, i.e. adding a new node.
  void StartInclusion();

  // Turns on or off a power switch node.
  void TurnOnSwitchNode(uint8_t node_id, bool value);

  // Returns set of unique node ids for paired switches.
  std::set<uint8_t> GetNodeSwitchIds() const;

  // Register a callback for when a new node is added.
  void SetOnAddNodeCallback(std::function<void()> on_add_node_callback) {
    on_add_node_callback_ = on_add_node_callback;
  }

private:
  struct NodeInfo {
    uint32_t home_id;
    uint8_t node_id;
    bool polled;
    std::list<OpenZWave::ValueID> values;
  };

  // Callback from the zwave library.
  static void NotificationFunc(OpenZWave::Notification const *notification,
                               void *context);
  void OnNotification(OpenZWave::Notification const *notification);

  // Proccess the next command.
  void DoNextCommand();

  // Exit condition checked by thread loop.
  bool IsExit();

  // Prints out list of home ids and nodes.
  void QueryHomeIds();

  NodeInfo *GetNodeInfo(OpenZWave::Notification const *notification) const;
  NodeInfo *GetNodeInfo(int nodeId) const;

  std::function<void()> on_add_node_callback_;

  uint32_t home_id_;
  bool init_failed_ = false;
  std::list<NodeInfo *> nodes_;
  std::string zwave_dongle_dev_path_;
  int port_;

  std::mutex command_mutex_;
  std::condition_variable command_condition_;
  std::queue<char> command_queue_;
  bool is_exit_ = false;
};

} // namespace zwave_app

#endif // ZWAVE_PROCESSOR_H_
