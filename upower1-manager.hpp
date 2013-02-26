#pragma once

#include <functional>
#include <dbus-c++/dbus.h>
#include "upower1-proxy.hpp"

namespace UPower1 {
  static const char BUS[] = "org.freedesktop.UPower";
  static const char PATH[] = "/org/freedesktop/UPower";

  class Manager
    : public org::freedesktop::UPower_proxy,
      public DBus::ObjectProxy
  {
    std::function<void (bool)> __battery_hook;
    std::function<void (bool)> __low_battery_hook;
    std::function<void (const std::string&)> __resuming_hook;
    std::function<void (const std::string&)> __sleeping_hook;

    bool __last_battery;
    bool __last_low_battery;

  public:
    Manager(DBus::Connection &connection,
            std::function<void (bool)>,
            std::function<void (bool)>,
            std::function<void (const std::string &)>,
            std::function<void (const std::string &)>);

  protected:
    virtual void DeviceAdded(const ::DBus::Path& device);
    virtual void DeviceRemoved(const ::DBus::Path& device);
    virtual void DeviceChanged(const ::DBus::Path& device);
    virtual void Changed();
    virtual void Sleeping();
    virtual void NotifySleep(const std::string& action);
    virtual void Resuming();
    virtual void NotifyResume(const std::string& action);
  };
}
