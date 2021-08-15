#pragma once

#include <functional>
#include <dbus-c++/dbus.h>
#include "upower1-proxy.hpp"
#include "dbus-properties-proxy.hpp"

namespace UPower1 {
  static const char BUS[] = "org.freedesktop.UPower";
  static const char PATH[] = "/org/freedesktop/UPower";

  class Manager
    : public org::freedesktop::UPower_proxy,
      public org::freedesktop::DBus::Properties_proxy,
      public DBus::ObjectProxy
  {
    std::function<void (bool)> __battery_hook;
    std::function<void (bool)> __low_battery_hook;

    bool __last_battery;
    bool __last_low_battery;

  public:
    Manager(DBus::Connection &connection,
            std::function<void (bool)>,
            std::function<void (bool)>);

  protected:
    virtual void DeviceAdded(const ::DBus::Path& device);
    virtual void DeviceRemoved(const ::DBus::Path& device);
    virtual void PropertiesChanged(
        const std::string& interface,
        const std::map< std::string, ::DBus::Variant >& changed_properties,
        const std::vector< std::string >& invalidated_properties
    );
  };
}
