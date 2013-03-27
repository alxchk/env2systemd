#include <systemd/sd-daemon.h>

#include <iostream>
#include <string>
#include <algorithm>
#include <functional>
#include <thread>

#include <dbus-c++/glib-integration.h>

#include "login1-manager.hpp"
#include "upower1-manager.hpp"
#include "systemd1-manager.hpp"
#include "network-manager.hpp"
#include "acpi-manager.hpp"

#include "policy.hpp"

#include "util.c"
#include "__hacks.hpp"

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
        std::cout << SD_INFO "NM Dispatch: start " << unit << std::endl;
      } else {
        _manager.StopUnit(unit, SYSTEMD_OVERRIDE);
        std::cout << SD_INFO "NM Dispatch: stop " << unit << std::endl;
      }
    }
    catch(DBus::Error e) {
      std::cerr << SD_WARNING "NM->Systemd DBus Error: " << e.message() << std::endl;
    }
  }

  void TriggerState(bool active) {
    try {
      if (active) {
        _manager.StartUnit(_dispatch_global, SYSTEMD_OVERRIDE);
        std::cout << SD_INFO "NM Dispatch: Enable networking" << std::endl;
      } else {
        _manager.StopUnit(_dispatch_global, SYSTEMD_OVERRIDE);
        std::cout << SD_INFO "NM Dispatch: Disable networking" << std::endl;
      }
    }
    catch(DBus::Error e) {
      std::cerr << SD_WARNING "NM->Systemd DBus Error: " << e.message() << std::endl;
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
        std::cout << SD_INFO "UP: On Battery" << std::endl;
      } else {
        _manager.StopUnit(_battery_target, SYSTEMD_OVERRIDE);
        std::cout << SD_INFO "UP: Disable On Battery" << std::endl;
      }
    }
    catch(DBus::Error e) {
      std::cerr << SD_WARNING "UP->Systemd DBus Error: " << e.message() << std::endl;
    }
  }

  void on_low_battery(bool state) {
    try {
      if (state) {
        _manager.StartUnit(_low_battery_target, SYSTEMD_OVERRIDE);
        std::cout << SD_INFO "UP: On Low Battery" << std::endl;
      } else {
        _manager.StopUnit(_low_battery_target, SYSTEMD_OVERRIDE);
        std::cout << SD_INFO "UP: Disable On Low Battery" << std::endl;
      }
    }
    catch(DBus::Error e) {
      std::cerr << SD_WARNING "UP->Systemd DBus Error: " << e.message() << std::endl;
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
      } else {
        std::cerr << SD_INFO << "ACPI: Unknown <" << message[0] << "> ("
                  << message[1] << ", "
                  << message[2] << ", "
                  << message[3] << ")" << std::endl;
        return;
      }
    } catch (const DBus::Error &e) {
      std::cerr << SD_INFO "ACPI->SYSTEMD: DBus Error: " << e.message() << std::endl;
    }
  }

private:
  Systemd1::Manager & _manager;
};

class DefaultPolicy : public Policy
{
public:
  DefaultPolicy(DBus::Connection &systemd_session,
                DBus::Connection &system,
                Glib::RefPtr< Glib::MainLoop > eloop)
    : _manager(systemd_session),
      _eloop(eloop),
      _lk(UNIT_LOCK_SESSION,
          _manager),
      _nd(UNIT_NETWORK_PLACE,
          UNIT_NETWORK_STATE,
          _manager),
      _up(UNIT_ON_BATTERY,
          UNIT_ON_LOW_BATTERY,
          _manager),
      _login1(system, [this](bool locked) {
          this->_lk.TriggerLock(locked);
        }),
      _nm(system,
          [this](const std::string &id, bool active) {
            if (active || !this->_eloop->is_running())
              {
                std::cerr << SD_DEBUG "Network " << id << ": Forsing status" << std::endl;
                this->_nd.TriggerNetwork(id, active);
                return;
              }

            Glib::signal_timeout()
                .connect([id, this]() -> bool
                         {
                           std::cerr << SD_DEBUG "Network " << id << ": Timeout passed" << std::endl;
                           if (this->_nm.isActivating(id))
                             return true;

                           std::cerr << SD_DEBUG "Network " << id << ": Forsing status" << std::endl;
                           if(! this->_nm.isActive(id))
                             this->_nd.TriggerNetwork(id, false);

                           return false;
                         }, DEFAULT_ACTION_TIMEOUT);
          },
          [this](bool active) {
            if (active || !this->_eloop->is_running())
              {
                std::cerr << SD_DEBUG "Global: Forsing status" << std::endl;
                this->_nd.TriggerState(active);
                return;
              }

            Glib::signal_timeout()
                .connect([this]() -> bool
                         {
                           std::cerr << SD_DEBUG "Global: Timeout passed" << std::endl;
                           if (this->_nm.isActivating())
                             return true;

                           std::cerr << SD_DEBUG "Global: Forsing status" << std::endl;
                           if (! this->_nm.isActive())
                             this->_nd.TriggerState(false);
                           return false;
                         }, DEFAULT_ACTION_TIMEOUT);
          }),
    _upower1(system,
             [this](bool active) {
               this->_up.on_battery(active);
             },
             [this](bool active) {
               this->_up.on_low_battery(active);
             }),
    _ad(_manager) {
    _ad.thread();
  }

private:
  /* Services Control */
  Systemd1::Manager _manager;

  /* Event loop */
  Glib::RefPtr< Glib::MainLoop > _eloop;

  /* Dispatchers */
  SessionLockDispatch _lk;
  NetworkDispatch _nd;
  UPowerDispatch _up;
  AcpiDispatch _ad;

  /* Event sources */
  Login1::Manager _login1;
  NetworkManager::Manager _nm;
  UPower1::Manager _upower1;

};

std::shared_ptr<Policy> policy(DBus::Connection &systemd_session,
                               DBus::Connection &system,
                               Glib::RefPtr< Glib::MainLoop > eloop)
{
  return std::shared_ptr<Policy>(new DefaultPolicy(systemd_session,
                                                   system, eloop));
}
