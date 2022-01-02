#include "zwave_processor.h"

#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#include <iostream>
#include <set>


using namespace OpenZWave;

namespace zwave_app {
namespace {
constexpr uint8_t kPowerSwitchCommandClass = 0x25;

}  // namespace

ZWaveProcessor::ZWaveProcessor(const std::string &zwave_dongle_dev_path)
    : zwave_dongle_dev_path_(zwave_dongle_dev_path) {}

void ZWaveProcessor::NotificationFunc(Notification const *notification, void *context) {
  if (context == nullptr) {
    return;
  }
  reinterpret_cast<ZWaveProcessor *>(context)->OnNotification(notification);
}

void ZWaveProcessor::Command(char command) {
  std::lock_guard<std::mutex> lock(command_mutex_);
  command_queue_.push(command);
  command_condition_.notify_one();
}

void ZWaveProcessor::OnNotification(
    OpenZWave::Notification const *notification) {
  const auto notification_type = notification->GetType();

  switch (notification_type) {
    case Notification::Type_ValueAdded: {
      if (NodeInfo *node_info = GetNodeInfo(notification)) {
        // Add the new value to our list
        node_info->values.push_back(notification->GetValueID());
      }
      break;
    }

    case Notification::Type_ValueRemoved: {
      if (NodeInfo *node_info = GetNodeInfo(notification)) {
        // Remove the value from out list
        for (list<ValueID>::iterator it = node_info->values.begin();
             it != node_info->values.end(); ++it) {
          if ((*it) == notification->GetValueID()) {
            node_info->values.erase(it);
            break;
          }
        }
      }
      break;
    }

    case Notification::Type_ValueChanged: {
      // One of the node values has changed
      /*
    NodeInfo *node_info = GetNodeInfo(notification);
    if (node_info->node_id == sensorNodeID) {
      bool *status;
      ValueID v = GetValueID(node_info, 0x30);
      bool val_get = Manager::Get()->GetValueAsBool(v, status);
      if (*status) {
        printf("\n Sensor is now: ACTIVE");
        SetPowerSwitch(GetValueID(GetNodeInfo(powerSwitchNodeID), 0x25),
                       true);
      } else {
        printf("\n Sensor is now: INACTIVE");
        SetPowerSwitch(GetValueID(GetNodeInfo(powerSwitchNodeID), 0x25),
                       false);
      }
    }
    */
      break;
    }

    case Notification::Type_Group: {
      // One of the node's association groups has changed
      if (NodeInfo *node_info = GetNodeInfo(notification)) {
        node_info = node_info;  // placeholder for real action
      }
      break;
    }

    case Notification::Type_NodeAdded: {
      // Update state.
      NodeInfo *node_info = new NodeInfo();
      node_info->home_id = notification->GetHomeId();
      node_info->node_id = notification->GetNodeId();
      node_info->polled = false;
      nodes_.push_back(node_info);

      // Let subscribers know that there's a new kid on the block.
      if (on_add_node_callback_) {
        on_add_node_callback_();
      }

      break;
    }

    case Notification::Type_NodeRemoved: {
      // Remove the node from our list
      uint32 const home_id = notification->GetHomeId();
      uint8 const nodeId = notification->GetNodeId();
      for (list<NodeInfo *>::iterator it = nodes_.begin(); it != nodes_.end();
           ++it) {
        NodeInfo *node_info = *it;
        if ((node_info->home_id == home_id_) && (node_info->node_id == nodeId)) {
          nodes_.erase(it);
          delete node_info;
          break;
        }
      }
      break;
    }

    case Notification::Type_NodeEvent: {
      // We have received an event from the node, caused by a
      // basic_set or hail message.
      if (NodeInfo *node_info = GetNodeInfo(notification)) {
        node_info = node_info;
      }
      break;
    }

    case Notification::Type_PollingDisabled: {
      if (NodeInfo *node_info = GetNodeInfo(notification)) {
        node_info->polled = false;
      }
      break;
    }

    case Notification::Type_PollingEnabled: {
      if (NodeInfo *node_info = GetNodeInfo(notification)) {
        node_info->polled = true;
      }
      break;
    }

    case Notification::Type_DriverReady: {
      home_id_ = notification->GetHomeId();
      break;
    }

    case Notification::Type_DriverFailed: {
      init_failed_ = true;
      //  pthread_cond_broadcast(&initCond);
      break;
    }

    case Notification::Type_AwakeNodesQueried:
    case Notification::Type_AllNodesQueried:
    case Notification::Type_AllNodesQueriedSomeDead: {
      // pthread_cond_broadcast(&initCond);
      break;
    }

    case Notification::Type_ControllerCommand: {
      OpenZWave::Log::Write(LogLevel_Info, "ControllerCommand");
    }

    case Notification::Type_DriverReset:
    case Notification::Type_Notification:
    case Notification::Type_NodeNaming:
    case Notification::Type_NodeProtocolInfo:
    case Notification::Type_NodeQueriesComplete:
    default: {
      OpenZWave::Log::Write(LogLevel_Info, "Unknown notifcation: %d",
                            static_cast<int>(notification_type));
    }
  }
}

void ZWaveProcessor::TurnOnSwitchNode(uint8_t node_id, bool value) {
  for (const auto &item : nodes_) {
    if (item->node_id != node_id) {
      continue;
    }

    for (auto v : item->values) {
      if (v.GetCommandClassId() == kPowerSwitchCommandClass) {
        Manager::Get()->SetValue(v, value);
      }
    }
  }
}

void ZWaveProcessor::QueryHomeIds() {
    // Remove from all networks.
    std::set<uint32_t> home_ids;
    for (auto it = nodes_.begin(); it != nodes_.end(); ++it) {
      NodeInfo *node_info = *it;
      OpenZWave::Log::Write(LogLevel_Info, "Node homeid: %u,  nodeid: %u", node_info->home_id, node_info->node_id);
      home_ids.insert(node_info->home_id);
    }

    OpenZWave::Log::Write(LogLevel_Info, "Found %d home ids", home_ids.size());
}

void ZWaveProcessor::ResetNetwork() {
  Manager::Get()->ResetController(home_id_);
}


//-----------------------------------------------------------------------------
// <GetNodeInfo>
// Return the NodeInfo object associated with this notification
//-----------------------------------------------------------------------------
ZWaveProcessor::NodeInfo *ZWaveProcessor::GetNodeInfo(
    Notification const *notification) const {
  uint32 const home_id = notification->GetHomeId();
  uint8 const nodeId = notification->GetNodeId();
  for (auto it = nodes_.begin(); it != nodes_.end(); ++it) {
    NodeInfo *node_info = *it;
    if ((node_info->home_id == home_id_) && (node_info->node_id == nodeId)) {
      return node_info;
    }
  }

  return NULL;
}

//-----------------------------------------------------------------------------
// <GetNodeInfo>
// Return the NodeInfo object associated with this ID number
//-----------------------------------------------------------------------------
ZWaveProcessor::NodeInfo *ZWaveProcessor::GetNodeInfo(int nodeId) const {
  for (auto it = nodes_.begin(); it != nodes_.end(); ++it) {
    NodeInfo *node_info = *it;
    if ((node_info->home_id == home_id_) && (node_info->node_id == nodeId)) {
      return node_info;
    }
  }

  return NULL;
}

//----------------------------------------------------------------------------
// <GetValueID>
// Return the ValueID of a given CommandClass and index
//----------------------------------------------------------------------------
/*
ValueID GetValueID(NodeInfo *node_info, uint8 commandClassID, int index = 0) {
  for (list<ValueID>::iterator v = node_info->values.begin();
       v != node_info->values.end(); ++v) {
    ValueID vid = *v;
    if (vid.GetCommandClassId() == commandClassID && vid.GetIndex() == index)
      return vid;
  }
  throw 1;
}
*/

void ZWaveProcessor::DoNextCommand() {
  //   std::lock_guard<std::mutex> lock(command_mutex_);

  if (command_queue_.empty()) {
    return;
  }

  switch (command_queue_.front()) {
    case 'q': {
      is_exit_ = true;
      break;
    }

    case '2': {
      TurnOnSwitchNode(2, true);
      break;
    }

    case '@': {
      TurnOnSwitchNode(2, false);
      break;
    }

  case '3': {
    TurnOnSwitchNode(3, true);
    break;
  }

  case '#': {
    TurnOnSwitchNode(3, false);
    break;
  }

  case '4': {
    TurnOnSwitchNode(3, true);
    break;
  }

  case '$': {
    TurnOnSwitchNode(3, false);
    break;
  }

  case '5': {
    TurnOnSwitchNode(3, true);
    break;
  }

  case '%': {
    TurnOnSwitchNode(3, false);
    break;
  }

    case 'r': {
      ResetNetwork();
      break;
    }

    case 'h': {
      QueryHomeIds();
      break;
    }

  case 'a': {
    StartInclusion();
  }

    default: {
      printf("unkown command %c\n", command_queue_.front());
      break;
    }
  }

  command_queue_.pop();
}

bool ZWaveProcessor::IsExit() { return is_exit_; }


std::set<uint8_t> ZWaveProcessor::GetNodeSwitchIds() const {
    // Remove from all networks.
    std::set<uint8_t> node_ids;
    for (auto it = nodes_.begin(); it != nodes_.end(); ++it) {
      NodeInfo *node_info = *it;

      for (auto v : node_info->values) {
        if (v.GetCommandClassId() == kPowerSwitchCommandClass) {
            node_ids.insert(node_info->node_id);
        }
      }
    }

    return node_ids;
}

void ZWaveProcessor::StartInclusion() {
    Manager::Get()->AddNode(home_id_);
}

void ZWaveProcessor::MainThreadFunc(ZWaveProcessor *processor) {
  std::unique_lock<std::mutex> lock(processor->command_mutex_);

  const string port = processor->zwave_dongle_dev_path_;

  printf("\n Creating Options \n");

  Options::Create("/usr/local/etc/openzwave", "../meta/", "");
  Options::Get()->AddOptionInt("SaveLogLevel", LogLevel_Detail);
  Options::Get()->AddOptionInt("QueueLogLevel", LogLevel_Debug);
  Options::Get()->AddOptionInt("DumpTrigger", LogLevel_Error);
  Options::Get()->AddOptionInt("PollInterval", 5000);

  // Comment the following line out if you want console logging
  // Options::Get()->AddOptionBool( "ConsoleOutput", false );

  Options::Get()->AddOptionBool("IntervalBetweenPolls", true);
  Options::Get()->AddOptionBool("ValidateValueChanges", true);
  Options::Get()->Lock();

  printf("\n Creating Manager \n");

  Manager::Create();

  Manager::Get()->AddWatcher(&ZWaveProcessor::NotificationFunc, processor);
  Manager::Get()->AddDriver(port);

  while (true) {
    printf("start waiting!\n");

    processor->command_condition_.wait(
        lock, [processor]() { return !processor->command_queue_.empty(); });

    printf("not waiting!\n");
    processor->DoNextCommand();

    if (processor->IsExit()) {
      break;
    }
  }

  printf("cleaning up\n");
  // program exit (clean up)
  Manager::Get()->WriteConfig(processor->home_id_);

  if (strcasecmp(port.c_str(), "usb") == 0) {
    Manager::Get()->RemoveDriver("HID Controller");
  } else {
    Manager::Get()->RemoveDriver(port);
  }
  Manager::Get()->RemoveWatcher(&ZWaveProcessor::NotificationFunc, processor);
  Manager::Destroy();
  Options::Destroy();

  printf("done\n");
}

}  // namespace zwave_app
