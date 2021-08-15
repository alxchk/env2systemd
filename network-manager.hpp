#pragma once

#include <functional>
#include <dbus-c++/dbus.h>
#include "dbus-properties-proxy.hpp"
#include "network-manager-proxy.hpp"
#include "network-manager-settings-proxy.hpp"
#include "network-manager-active-connection-proxy.hpp"

namespace NetworkManager
{
  static const char * PATH = "/org/freedesktop/NetworkManager";
  static const char * NAME = "org.freedesktop.NetworkManager";

  class Settings
    : public org::freedesktop::NetworkManager::Settings::Connection_proxy,
      public org::freedesktop::DBus::Properties_proxy,
      public DBus::ObjectProxy
  {
  public:
    Settings(DBus::Connection &connection, const DBus::Path &path);

  protected:
    virtual void Removed() {}
    virtual void Updated() {}
	void PropertiesChanged(
            const std::string& interface,
            const std::map< std::string, ::DBus::Variant >& changed_properties,
            const std::vector< std::string >& invalidated_properties
        ) {}
  };

  class ActiveConnection
    : public org::freedesktop::NetworkManager::Connection::Active_proxy,
      public org::freedesktop::DBus::Properties_proxy,
      public DBus::ObjectProxy
  {
  public:
    ActiveConnection(DBus::Connection &connection,
                     const DBus::Path &path,
                     std::function<void (uint32_t)> trigger);

  protected:
    virtual void PropertiesChanged(
        const std::string& interface,
        const std::map< std::string, ::DBus::Variant >& changed_properties,
        const std::vector< std::string >& invalidated_properties
    );

  private:
    std::function<void (uint32_t active)> __trigger;
  };

  class Manager
    : private org::freedesktop::NetworkManager_proxy,
      private org::freedesktop::DBus::Properties_proxy,
      private DBus::ObjectProxy
  {
  public:
    struct Behavior {
      virtual void Connect(const DBus::Path & connection) = 0;
      virtual void Disconnect(const DBus::Path & connection) = 0;
    };

  public:
    Manager(DBus::Connection &connection,
            std::function<void (const std::string &, bool)>,
            std::function<void (bool)>);
    ~Manager();

    bool isActive(const std::string &id = "");
    bool isActivating(const std::string &id = "");

  protected:
    virtual void DeviceRemoved(const ::DBus::Path& argin0);
    virtual void DeviceAdded(const ::DBus::Path& argin0);
    virtual void PropertiesChanged(
        const std::string& interface,
        const std::map< std::string, ::DBus::Variant >& changed_properties,
        const std::vector< std::string >& invalidated_properties
    );
    virtual void StateChanged(const uint32_t& argin0);
    virtual void CheckPermissions();

  private:
    DBus::Connection &__connection;
    std::map< ::DBus::Path,
              std::pair<std::string, ActiveConnection* > > __tracked_connections;
    std::function<void (const std::string &, bool)> __hook_network;
    std::function<void (uint32_t)> __hook_global;

  private:
    void __registerActiveConnections(const std::vector< ::DBus::Path > &);
    void __triggerActiveConnection(::DBus::Path, uint32_t);
    void __processActiveConnection(const std::string&, uint32_t);
    uint32_t __connectionStateByName(const std::string &id);
    std::string __nameActiveConnection(ActiveConnection *);
  };
}
