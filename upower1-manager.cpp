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

void UPower1::Manager::DeviceChanged(const ::DBus::Path& device)
{}

void UPower1::Manager::Changed()
{
  bool battery = OnBattery();

  if (battery != __last_battery) {
    __last_battery = battery;
    __battery_hook(__last_battery);
  }
}

/* Deprecated */
void UPower1::Manager::Sleeping()
{}

void UPower1::Manager::Resuming()
{}

void UPower1::Manager::NotifySleep(const std::string& reason)
{}

void UPower1::Manager::NotifyResume(const std::string& reason)
{}
