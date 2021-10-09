#include "zwave_processor.h"

#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#include <iostream>

//#include "ValueStore.h"
//#include "Value.h"
//#include "ValueBool.h"

using namespace OpenZWave;

namespace zwave_app {
namespace {
//-----------------------------------------------------------------------------
// <NotificationFunc>
// Callback that is triggered when a value, group or node changes
//-----------------------------------------------------------------------------
void NotificationFunc(Notification const *notification, void *context) {
  if (context == nullptr) {
    return;
  }
  reinterpret_cast<ZWaveProcessor *>(context)->OnNotification(notification);
}

}  // namespace

ZWaveProcessor::ZWaveProcessor(const std::string &zwave_dongle_dev_path)
    : zwave_dongle_dev_path_(zwave_dongle_dev_path) {}

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
      if (NodeInfo *nodeInfo = GetNodeInfo(notification)) {
        // Add the new value to our list
        nodeInfo->m_values.push_back(notification->GetValueID());
      }
      break;
    }

    case Notification::Type_ValueRemoved: {
      if (NodeInfo *nodeInfo = GetNodeInfo(notification)) {
        // Remove the value from out list
        for (list<ValueID>::iterator it = nodeInfo->m_values.begin();
             it != nodeInfo->m_values.end(); ++it) {
          if ((*it) == notification->GetValueID()) {
            nodeInfo->m_values.erase(it);
            break;
          }
        }
      }
      break;
    }

    case Notification::Type_ValueChanged: {
      // One of the node values has changed
      /*
    NodeInfo *nodeInfo = GetNodeInfo(notification);
    if (nodeInfo->m_nodeId == sensorNodeID) {
      bool *status;
      ValueID v = GetValueID(nodeInfo, 0x30);
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
      if (NodeInfo *nodeInfo = GetNodeInfo(notification)) {
        nodeInfo = nodeInfo;  // placeholder for real action
      }
      break;
    }

    case Notification::Type_NodeAdded: {
      // Add the new node to our list
      NodeInfo *nodeInfo = new NodeInfo();
      nodeInfo->m_homeId = notification->GetHomeId();
      nodeInfo->m_nodeId = notification->GetNodeId();
      nodeInfo->m_polled = false;
      nodes_.push_back(nodeInfo);
      /*
      if (nodeInfo->m_nodeId == sensorNodeID) {
        // Add an association between the sensor and the controller, if one is
        // not already present
        Manager::Get()->AddAssociation(nodeInfo->m_homeId, nodeInfo->m_nodeId,
      1, 1); Manager::Get()->RefreshNodeInfo(nodeInfo->m_homeId,
      nodeInfo->m_nodeId);
      }
      */
      break;
    }

    case Notification::Type_NodeRemoved: {
      // Remove the node from our list
      uint32 const homeId = notification->GetHomeId();
      uint8 const nodeId = notification->GetNodeId();
      for (list<NodeInfo *>::iterator it = nodes_.begin(); it != nodes_.end();
           ++it) {
        NodeInfo *nodeInfo = *it;
        if ((nodeInfo->m_homeId == homeId) && (nodeInfo->m_nodeId == nodeId)) {
          nodes_.erase(it);
          delete nodeInfo;
          break;
        }
      }
      break;
    }

    case Notification::Type_NodeEvent: {
      // We have received an event from the node, caused by a
      // basic_set or hail message.
      if (NodeInfo *nodeInfo = GetNodeInfo(notification)) {
        nodeInfo = nodeInfo;
      }
      break;
    }

    case Notification::Type_PollingDisabled: {
      if (NodeInfo *nodeInfo = GetNodeInfo(notification)) {
        nodeInfo->m_polled = false;
      }
      break;
    }

    case Notification::Type_PollingEnabled: {
      if (NodeInfo *nodeInfo = GetNodeInfo(notification)) {
        nodeInfo->m_polled = true;
      }
      break;
    }

    case Notification::Type_DriverReady: {
      homeId = notification->GetHomeId();
      break;
    }

    case Notification::Type_DriverFailed: {
      initFailed = true;
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
    if (item->m_nodeId != node_id) {
      continue;
    }

    for (auto v : item->m_values) {
      if (v.GetCommandClassId() != 0x25) {
        continue;
      }

      Manager::Get()->SetValue(v, value);
    }
  }
}

//-----------------------------------------------------------------------------
// <GetNodeInfo>
// Return the NodeInfo object associated with this notification
//-----------------------------------------------------------------------------
ZWaveProcessor::NodeInfo *ZWaveProcessor::GetNodeInfo(
    Notification const *notification) const {
  uint32 const homeId = notification->GetHomeId();
  uint8 const nodeId = notification->GetNodeId();
  for (auto it = nodes_.begin(); it != nodes_.end(); ++it) {
    NodeInfo *nodeInfo = *it;
    if ((nodeInfo->m_homeId == homeId) && (nodeInfo->m_nodeId == nodeId)) {
      return nodeInfo;
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
    NodeInfo *nodeInfo = *it;
    if ((nodeInfo->m_homeId == homeId) && (nodeInfo->m_nodeId == nodeId)) {
      return nodeInfo;
    }
  }

  return NULL;
}

//----------------------------------------------------------------------------
// <GetValueID>
// Return the ValueID of a given CommandClass and index
//----------------------------------------------------------------------------
/*
ValueID GetValueID(NodeInfo *nodeInfo, uint8 commandClassID, int index = 0) {
  for (list<ValueID>::iterator v = nodeInfo->m_values.begin();
       v != nodeInfo->m_values.end(); ++v) {
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

    case 'f': {
      TurnOnSwitchNode(2, true);
      break;
    }

    case 'g': {
      TurnOnSwitchNode(2, false);
      break;
    }

    default: {
      printf("unkown command %c\n", command_queue_.front());
      break;
    }
  }

  command_queue_.pop();
}

bool ZWaveProcessor::IsExit() { return is_exit_; }

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
  Manager::Get()->AddWatcher(NotificationFunc, processor);
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
  Manager::Get()->WriteConfig(processor->homeId);

  if (strcasecmp(port.c_str(), "usb") == 0) {
    Manager::Get()->RemoveDriver("HID Controller");
  } else {
    Manager::Get()->RemoveDriver(port);
  }
  Manager::Get()->RemoveWatcher(NotificationFunc, processor);
  Manager::Destroy();
  Options::Destroy();

  printf("done\n");
}

}  // namespace zwave_app
