#include <NetworkManager.h>
#include <algorithm>
#include <exception>
#include "network-manager.hpp"

NetworkManager::Manager::Manager(DBus::Connection &connection,
                                 std::function<void (const std::string &, bool)> hook_network,
                                 std::function<void (bool)> hook_global)
  : DBus::ObjectProxy(connection, NetworkManager::PATH, NetworkManager::NAME),
    __connection(connection),
    __hook_network(hook_network),
    __hook_global(hook_global)
{
  __hook_global(isActive());
  __registerActiveConnections(ActiveConnections());
}

NetworkManager::Manager::~Manager()
{
  __hook_global(false);
  for (auto &c: __tracked_connections) {
    __hook_network(c.second.first, false);
    delete c.second.second;
  }
}

bool NetworkManager::Manager::isActive(const std::string &id)
{
  if (id == "")
    return State() == NM_STATE_CONNECTED_GLOBAL;
  else
    return __connectionStateByName(id) == NM_ACTIVE_CONNECTION_STATE_ACTIVATED;
}

bool NetworkManager::Manager::isActivating(const std::string &id)
{
  if (id == "")
    return State() == NM_STATE_CONNECTING;
  else
    return __connectionStateByName(id) == NM_ACTIVE_CONNECTION_STATE_ACTIVATING;
}

NetworkManager::ActiveConnection::ActiveConnection(DBus::Connection& connection,
                                                   DBus::Path const& path,
                                                   std::function<void (uint32_t state)> trigger)
  : DBus::ObjectProxy(connection, path, NetworkManager::NAME),
    __trigger(trigger)
{}

void NetworkManager::ActiveConnection::PropertiesChanged(
    const std::string& interface,
    const std::map< std::string, ::DBus::Variant >& changed_properties,
    const std::vector< std::string >& invalidated_properties
) {
  for (auto &p : changed_properties)
    if (p.first == "State")
      __trigger(p.second);
}

NetworkManager::Settings::Settings(DBus::Connection& connection, DBus::Path const& path)
  : DBus::ObjectProxy(connection, path, NetworkManager::NAME) {}

void NetworkManager::Manager::DeviceRemoved(const ::DBus::Path& argin0) {}

void NetworkManager::Manager::DeviceAdded(const ::DBus::Path& argin0) {}

void NetworkManager::Manager::PropertiesChanged(
    const std::string& interface,
    const std::map< std::string, ::DBus::Variant >& changed_properties,
    const std::vector< std::string >& invalidated_properties
) {
  for (auto &i : changed_properties) {
    try {
      if (i.first == "ActiveConnections") {
        const std::vector< ::DBus::Path > payload = i.second;
        __registerActiveConnections(payload);
      }
    }
    catch (DBus::Error e) {
      std::cout << "Warning: " << e.message() << std::endl;
    }
  }
}

void NetworkManager::Manager::StateChanged(const uint32_t& argin0) {
  switch(argin0) {
  case NM_STATE_CONNECTED_GLOBAL:
    __hook_global(true);
    break;
  case NM_STATE_DISCONNECTING:
  case NM_STATE_CONNECTING:
    break;
  default:
    __hook_global(false);
  }
}

void NetworkManager::Manager::CheckPermissions() {
}

uint32_t NetworkManager::Manager::__connectionStateByName(const std::string &id) {
  for (const auto &c : __tracked_connections) {
    if (c.second.first == id)
      return c.second.second->State();
  }

  return 0;
}

void NetworkManager::Manager::__triggerActiveConnection(::DBus::Path path, uint32_t state) {
  auto i = __tracked_connections.find(path);
  assert(i != __tracked_connections.end());
  auto c = i->second.second;
  assert(c);

  std::cerr << "Event on "  << path << " (" << state << ")" << std::endl;
  __processActiveConnection(i->second.first, state);

  if (state == NM_ACTIVE_CONNECTION_STATE_DEACTIVATED) {
    std::cout << "NM: Dropping " << path << std::endl;
    __tracked_connections.erase(i);
    delete c;
  }
}

std::string NetworkManager::Manager::__nameActiveConnection(ActiveConnection * c) {
    try {
      std::cerr << "c->Connection(): " << c->Connection() << std::endl;
      auto settings = NetworkManager::Settings(__connection,
                                               c->Connection()
                                               ).GetSettings();
      return std::string(static_cast<const std::string&>(settings["connection"]["id"]));
    }
    catch (DBus::Error e) {
      return "";
    }
}

void NetworkManager::Manager::__processActiveConnection(const std::string &name, uint32_t state) {
  switch(state) {
      case NM_ACTIVE_CONNECTION_STATE_ACTIVATED:
        std::cerr << "NM: Activating: " << name << std::endl;
        __hook_network(name, true); break;
      case 0:
      case NM_ACTIVE_CONNECTION_STATE_DEACTIVATED:
        std::cerr << "NM: Deactivating: " << name << std::endl;
        __hook_network(name, false); break;
  default: break;
  }
}

void NetworkManager::Manager::__registerActiveConnections(const std::vector< ::DBus::Path> &active) {
  for (auto &c : active) {
    if (__tracked_connections.find(c) == __tracked_connections.end()) {
      auto nconn = new ActiveConnection(__connection, c,
                                        [this,c](uint32_t state) {
                                          this->__triggerActiveConnection(c, state);
                                        });
      std::string name = __nameActiveConnection(nconn);
      if (name == "") {
        std::cerr << "Couldn't get id for " << c << ", give up" << std::endl;
        delete nconn;
        continue;
      }

      std::cerr << "NM: Wating for events at " << c << " (" << name << ")" << std::endl;
      __tracked_connections[c] = std::pair<std::string, ActiveConnection *>(name, nconn);
      __processActiveConnection(name, nconn->State());
    }
  }
}
