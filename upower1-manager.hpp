#pragma once

#include <functional>
#include <memory>
#include <dbus-c++/dbus.h>
#include "upower1-proxy.hpp"
#include "upower1-device-proxy.hpp"
#include "dbus-properties-proxy.hpp"

namespace UPower1 {
  static const char BUS[] = "org.freedesktop.UPower";
  static const char PATH[] = "/org/freedesktop/UPower";

  class Device
   : public org::freedesktop::UPower::Device_proxy,
     public org::freedesktop::DBus::Properties_proxy,
     public DBus::ObjectProxy
  {
      std::function<void ()> __on_change;
      std::string __native_string;

    public:
      Device(
          DBus::Connection &connection,
          const DBus::Path &path,
          std::function<void ()> on_change);

      const std::string &SavedName() const;

      virtual void PropertiesChanged(
        const std::string& interface,
        const std::map< std::string, ::DBus::Variant >& changed_properties,
        const std::vector< std::string >& invalidated_properties
    );
  };

  class Manager
    : public org::freedesktop::UPower_proxy,
      public org::freedesktop::DBus::Properties_proxy,
      public DBus::ObjectProxy
  {
    std::function<void (bool)> __battery_hook;
    std::function<void (bool)> __low_battery_hook;

    std::list<std::unique_ptr<Device>> __ac_devices;

    bool __last_battery;
    bool __last_low_battery;

  public:
    Manager(DBus::Connection &connection,
            std::function<void (bool)>,
            std::function<void (bool)>);

    void OnChanges();

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
