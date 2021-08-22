#include "upower1-manager.hpp"

UPower1::Device::Device(DBus::Connection &connection,
                        const DBus::Path &path,
                        std::function<void ()> on_change)
        : DBus::ObjectProxy(connection, path,
                            UPower1::BUS),
          __on_change(on_change),
          __native_string(NativePath())
{}

const std::string &UPower1::Device::SavedName() const {
    return this->__native_string;
}

void UPower1::Device::PropertiesChanged(
    const std::string& interface,
    const std::map< std::string, ::DBus::Variant >& changed_properties,
    const std::vector< std::string >& invalidated_properties)
{
    __on_change();
}

UPower1::Manager::Manager(DBus::Connection &connection,
                          std::function<void (bool)> battery_hook,
                          std::function<void (bool)> low_battery_hook)
  : DBus::ObjectProxy(connection,
                      UPower1::PATH,
                      UPower1::BUS),
    __battery_hook(battery_hook),
    __low_battery_hook(low_battery_hook)
{
    bool on_battery = true;

    for (auto path: this->EnumerateDevices()) {
        auto obj = std::make_unique<Device>(
            this->conn(), path, [this]() {
                this->OnChanges();
            });

        if (obj->Type() == 1) {
            std::cout << "[Add AC Device] " << obj->SavedName()
                      << " Online=" << obj->Online() << std::endl;

            if (obj->Online())
                on_battery = false;

            this->__ac_devices.push_back(std::move(obj));
        }
    }

    this->__last_battery = on_battery;
    this->__battery_hook(on_battery);
}

void UPower1::Manager::DeviceAdded(const ::DBus::Path& device)
{
    auto obj = std::make_unique<Device>(
        this->conn(), device, [this]() {
            this->OnChanges();
        });

    if (obj->Type() == 1) {
        std::cout << "[Add AC Device] " << obj->SavedName()
                  << " Online=" << obj->Online() << std::endl;
        this->__ac_devices.push_back(std::move(obj));
    }

    this->OnChanges();
}

void UPower1::Manager::OnChanges()
{
    bool on_battery = true;
    for (auto &obj : this->__ac_devices) {
        if (obj->Online()) {
            on_battery = false;
            break;
        }
    }

    if (this->__last_battery != on_battery) {
        this->__last_battery = on_battery;
        this->__battery_hook(on_battery);
    }
}

void UPower1::Manager::DeviceRemoved(const ::DBus::Path& device)
{
    for (auto &obj: this->__ac_devices) {
        if (obj->path() == device) {
            std::cout << "[Remove AC Device] " << obj->SavedName() << std::endl;
            this->__ac_devices.remove(obj);
            break;
        }
    }

    this->OnChanges();
}

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
