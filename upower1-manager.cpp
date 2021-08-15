#include "upower1-manager.hpp"

UPower1::Manager::Manager(DBus::Connection &connection,
                          std::function<void (bool)> battery_hook,
                          std::function<void (bool)> low_battery_hook)
  : DBus::ObjectProxy(connection,
                      UPower1::PATH,
                      UPower1::BUS),
    __battery_hook(battery_hook),
    __low_battery_hook(low_battery_hook)
{
  __last_battery = OnBattery();

  battery_hook(__last_battery);
}

void UPower1::Manager::DeviceAdded(const ::DBus::Path& device)
{}

void UPower1::Manager::DeviceRemoved(const ::DBus::Path& device)
{}

void UPower1::Manager::PropertiesChanged(
    const std::string& interface,
    const std::map< std::string, ::DBus::Variant >& changed_properties,
    const std::vector< std::string >& invalidated_properties
)
{
  bool battery = OnBattery();

  if (battery != __last_battery) {
    __last_battery = battery;
    __battery_hook(__last_battery);
  }
}
