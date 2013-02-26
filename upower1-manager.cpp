#include "upower1-manager.hpp"

UPower1::Manager::Manager(DBus::Connection &connection,
                          std::function<void (bool)> battery_hook,
                          std::function<void (bool)> low_battery_hook,
                          std::function<void (const std::string&)> resuming_hook,
                          std::function<void (const std::string&)> sleeping_hook)
  : DBus::ObjectProxy(connection,
                      UPower1::PATH,
                      UPower1::BUS),
    __battery_hook(battery_hook),
    __low_battery_hook(low_battery_hook),
    __resuming_hook(resuming_hook),
  __sleeping_hook(sleeping_hook)
{
  __last_battery = OnBattery();
  __last_low_battery = OnLowBattery();

  battery_hook(__last_battery);
  low_battery_hook(__last_low_battery);
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
  bool low_battery = OnLowBattery();

  if (battery != __last_battery) {
    __last_battery = battery;
    __battery_hook(__last_battery);
  }

  if (low_battery != __last_low_battery) {
    __last_low_battery = low_battery;
    __low_battery_hook(__last_low_battery);
  }
}

/* Deprecated */
void UPower1::Manager::Sleeping()
{
  __sleeping_hook("");
}

void UPower1::Manager::Resuming()
{
  __resuming_hook("");
}

void UPower1::Manager::NotifySleep(const std::string& reason)
{
  __sleeping_hook(reason);
}

void UPower1::Manager::NotifyResume(const std::string& reason)
{
  __resuming_hook(reason);
}
