#include <iostream>
#include <string>
#include <algorithm>
#include <functional>
#include <thread>

#include <sys/signal.h>

#include "login1-manager.hpp"
#include "upower1-manager.hpp"
#include "systemd1-manager.hpp"
#include "network-manager.hpp"
#include "acpi-manager.hpp"

#include "util.c"

#include "config.h"

class SessionLockDispatch
{
    std::string         _lock_target;
    Systemd1::Manager & _manager;

public:
    SessionLockDispatch(const char * const lock_target, Systemd1::Manager & manager)
        : _lock_target(lock_target),
          _manager(manager)
        {}

public:
    void TriggerLock(bool locked)   {
        try {
            if (locked)
                _manager.StartUnit(_lock_target, SYSTEMD_OVERRIDE);
            else
                _manager.StopUnit(_lock_target, SYSTEMD_OVERRIDE);
        }
        catch(DBus::Error e) {
            std::cerr << "NM->Systemd DBus Error: " << e.message() << std::endl;
        }
    }
};

class NetworkDispatch {
    std::string         _dispatch_template;
    std::string         _dispatch_global;
    Systemd1::Manager & _manager;

public:
    NetworkDispatch(const char * const dispatch_template,
                    const char * const dispatch_global,
                    Systemd1::Manager & manager)
        : _dispatch_template(std::string(dispatch_template)),
          _dispatch_global(std::string(dispatch_global)),
          _manager(manager)
        {}

    void TriggerNetwork(const std::string & id, bool active) {
        auto pos = _dispatch_template.find("%i");
        if (pos == std::string::npos)
            return;

        char * cid = xescape(id.c_str()," /");
        if (! cid)
            return;

        auto unit = _dispatch_template;
        unit.replace(pos, sizeof("%i")-1, cid);
        free(cid);

        try {
            if (active) {
                _manager.StartUnit(unit, SYSTEMD_OVERRIDE);
                std::cout << "NM Dispatch: start " << unit << std::endl;
            } else {
                _manager.StopUnit(unit, SYSTEMD_OVERRIDE);
                std::cout << "NM Dispatch: stop " << unit << std::endl;
            }
        }
        catch(DBus::Error e) {
            std::cerr << "NM->Systemd DBus Error: " << e.message() << std::endl;
        }
    }

    void TriggerState(bool active) {
        try {
            if (active) {
                _manager.StartUnit(_dispatch_global, SYSTEMD_OVERRIDE);
                std::cout << "NM Dispatch: Enable networking" << std::endl;
            } else {
                _manager.StopUnit(_dispatch_global, SYSTEMD_OVERRIDE);
                std::cout << "NM Dispatch: Disable networking" << std::endl;
            }
        }
        catch(DBus::Error e) {
            std::cerr << "NM->Systemd DBus Error: " << e.message() << std::endl;
        }
    }
};

class UPowerDispatch {
    std::string _battery_target;
    std::string _low_battery_target;

    Systemd1::Manager & _manager;

public:
    UPowerDispatch(const char * const battery_target,
                   const char * const low_battery_target,
                   Systemd1::Manager & manager)
        : _battery_target(battery_target),
          _low_battery_target(low_battery_target),
          _manager(manager)
        {}

    void on_battery(bool state) {
        try {
            if (state) {
                _manager.StartUnit(_battery_target, SYSTEMD_OVERRIDE);
                std::cout << "UP: On Battery" << std::endl;
            } else {
                _manager.StopUnit(_battery_target, SYSTEMD_OVERRIDE);
                std::cout << "UP: Disable On Battery" << std::endl;
            }
        }
        catch(DBus::Error e) {
            std::cerr << "UP->Systemd DBus Error: " << e.message() << std::endl;
        }
    }

    void on_low_battery(bool state) {
        try {
            if (state) {
                _manager.StartUnit(_low_battery_target, SYSTEMD_OVERRIDE);
                std::cout << "UP: On Low Battery" << std::endl;
            } else {
                _manager.StopUnit(_low_battery_target, SYSTEMD_OVERRIDE);
                std::cout << "UP: Disable On Low Battery" << std::endl;
            }
        }
        catch(DBus::Error e) {
            std::cerr << "UP->Systemd DBus Error: " << e.message() << std::endl;
        }
    }
};

class AcpiDispatch {

public:
    AcpiDispatch(Systemd1::Manager & manager)
        : _manager(manager)
    {}

public:
    void thread() {
        std::thread([this](){
                Acpi::Manager([this](const std::vector<std::string> &message) {
                        this->dispatch(message);
                    }).dispatch();
            }).detach();
    }

private:
    void dispatch(const std::vector<std::string> &message)
    {
        if (message.size() != 4) return;

        const static std::string button = "button";
        const static std::string video = "video";
        const static std::string lid = "LID";

        try {
            if ((message[0].compare(0, button.size(), button) == 0) ||
                (message[1] == "HKEY")) {
                if (message[1].compare(0, lid.size(), lid) == 0) {
                    if(std::stoi(message[3]) & 1) {
                        /* LID close */
                        _manager.StartUnit(UNIT_ACPI_LID, SYSTEMD_OVERRIDE);
                    } else {
                        /* Lid open */
                        _manager.StopUnit(UNIT_ACPI_LID, SYSTEMD_OVERRIDE);
                    }
                } else {
                    std::string unit = UNIT_ACPI_HKEY "-" + message[2] + "-" + message[3] + "." UNIT_ACPI_HKEY_TYPE;
                    if (_manager.getUnit(unit).ActiveState() != "active") {
                        _manager.StartUnit(unit, SYSTEMD_OVERRIDE);
                    } else {
                        _manager.StopUnit(unit, SYSTEMD_OVERRIDE);
                    }
                }
            } else if (message[0].compare(0, video.size(), video) == 0) {
            }
            else {
                std::cerr << "ACPI: Unknown <" << message[0] << "> ("
                          << message[1] << ", "
                          << message[2] << ", "
                          << message[3] << ")" << std::endl;
                return;
            }
        }
        catch (const DBus::Error &e) {
            std::cerr << "ACPI->SYSTEMD: DBus Error: " << e.message() << std::endl;
        }

    }

private:
    Systemd1::Manager & _manager;
};

DBus::BusDispatcher dispatcher;

void terminate(int sig) {
    dispatcher.leave();
}

int main(int argc, char *argv[])
{
    signal(SIGINT, terminate);
    signal(SIGTERM, terminate);

    DBus::default_dispatcher = &dispatcher;

    DBus::Connection system  = DBus::Connection::SystemBus();
    DBus::Connection session = DBus::Connection::SessionBus();

    Systemd1::Manager manager(session);
    SessionLockDispatch lock(UNIT_LOCK_SESSION,
                             manager);

    NetworkDispatch nd(UNIT_NETWORK_PLACE,
                       UNIT_NETWORK_STATE,
                       manager);

    UPowerDispatch up(UNIT_ON_BATTERY,
                      UNIT_ON_LOW_BATTERY,
                      manager);

    Login1::Manager login1(system, [&lock](bool locked) {
            lock.TriggerLock(locked);
        });

    NetworkManager::Manager nm(system,
                               [&nd](const std::string &id, bool active) {
                                   nd.TriggerNetwork(id, active);
                               },
                               [&nd](bool active) {
                                   nd.TriggerState(active);
                               });

    UPower1::Manager upower1(system,
                             [&up](bool active) {
                                 up.on_battery(active);
                             },
                             [&up](bool active) {
                                 up.on_low_battery(active);
                             });

    AcpiDispatch ad(manager);

    try {
        ad.thread();
        dispatcher.enter();
    } catch (DBus::Error e) {
        std::cerr << "Get unhandled DBus exception: " << e.message() << std::endl;
        return 1;
    }

    std::cout << "Goodbye" << std::endl;

    return 0;
}
