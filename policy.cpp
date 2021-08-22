#include <systemd/sd-daemon.h>

#include <iostream>
#include <string>
#include <algorithm>
#include <functional>
#include <thread>
#include <sstream>
#include <vector>

#include <glibmm.h>
#include <dbus-c++/glib-integration.h>

#include "login1-manager.hpp"
#include "upower1-manager.hpp"
#include "systemd1-manager.hpp"
#include "network-manager.hpp"
#include "acpi-manager.hpp"

#include "policy.hpp"

#include "util.c"


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

class DockDispatch
{
  std::string          _dock_target;
  Systemd1::Manager & _manager;

public:
  DockDispatch(const char * const dock_target, Systemd1::Manager & manager)
    : _dock_target(dock_target),
      _manager(manager)
  {}

public:
  void TriggerDock(bool docked) {
    try {
      if (docked) {
        std::cerr << "System docked" << std::endl;
        _manager.StartUnit(_dock_target, SYSTEMD_OVERRIDE);
      } else {
        std::cerr << "System undocked" << std::endl;
        _manager.StopUnit(_dock_target, SYSTEMD_OVERRIDE);
      }
    } catch(DBus::Error e) {
      std::cerr << "DockDispatch->Systemd DBus Error: " << e.message() << std::endl;
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
  void dispatch(const std::vector<std::string> &message)
  {
    if (message.size() < 3) {
      std::cerr << SD_ERR "ACPI Dispatch: malformed ACPI message, elems: " <<
          message.size() << std::endl;
      return;
    }

    std::string klass(message[0]);
    std::string subklass;
    std::string object;
    std::string action;

    if (message.size() == 3) {
        size_t pos = klass.find('/');
        if (pos == std::string::npos) {
          subklass = "";
        } else {
          subklass = klass.substr(pos+1);
          klass = klass.substr(0, pos);
        }
        object = message[1];
        action = message[2];
    } else {
        subklass = message[1];
        object = message[2];
        action = message[3];
    }

    const static std::string button = "button";
    const static std::string video = "video";
    const static std::string lid = "lid";
    const static std::string hkey = "hkey";
    const static std::string open = "open";
    const static std::string close = "close";

    try {
      if (cmpstr(klass, button) == 0 || cmpstr(klass, video) == 0 || cmpstr(subklass, hkey)) {
        if (cmpstr(subklass, lid)) {
          if(cmpstr(action, open)) {
            /* LID close */
            std::cerr << SD_INFO "ACPI->SYSTEMD: LID OPENED" << std::endl;
            _manager.StartUnit(UNIT_ACPI_LID, SYSTEMD_OVERRIDE);
          } else {
            /* Lid open */
            std::cerr << SD_INFO "ACPI->SYSTEMD: LID CLOSED" << std::endl;
            _manager.StopUnit(UNIT_ACPI_LID, SYSTEMD_OVERRIDE);
          }
        } else {
          std::string unit = UNIT_ACPI_HKEY "-" + subklass + "." UNIT_ACPI_HKEY_TYPE;
          if (_manager.getUnit(unit).ActiveState() != "active") {
            _manager.StartUnit(unit, SYSTEMD_OVERRIDE);
          } else {
            _manager.StopUnit(unit, SYSTEMD_OVERRIDE);
          }
        }
      } else {
        std::cerr << SD_INFO << "ACPI: Unknown <" << klass << "> ("
                  << subklass << ", "
                  << object << ", "
                  << action << ")" << std::endl;
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
      _dk(UNIT_DOCK_SESSION,
          _manager),
      _nd(UNIT_NETWORK_PLACE,
          UNIT_NETWORK_STATE,
          _manager),
      _up(UNIT_ON_BATTERY,
          UNIT_ON_LOW_BATTERY,
          _manager),
      _ad(_manager),
      _login1(_manager, system,
          [this](bool locked) {
            this->_lk.TriggerLock(locked);
          },
          [this](bool docked) {
            this->_dk.TriggerDock(docked);
          }
      ),
      _nm(system,
          [this](const std::string &id, bool active) {
            if (active || !this->_eloop->is_running())
              {
                this->_nd.TriggerNetwork(id, active);
                return;
              }

            Glib::signal_timeout()
                .connect([id, this]() -> bool
                         {
                           if (this->_nm.isActivating(id))
                             return true;

                           if(! this->_nm.isActive(id))
                             this->_nd.TriggerNetwork(id, false);

                           return false;
                         }, DEFAULT_ACTION_TIMEOUT);
          },
          [this](bool active) {
            if (active || !this->_eloop->is_running())
              {
                this->_nd.TriggerState(active);
                return;
              }

            Glib::signal_timeout()
                .connect([this]() -> bool
                         {
                           if (this->_nm.isActivating())
                             return true;

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
   _acpi([this](const std::vector<std::string> & message)
         {
           this->_ad.dispatch(message);
         })
  {

  }

private:
  /* Services Control */
  Systemd1::Manager _manager;

  /* Event loop */
  Glib::RefPtr< Glib::MainLoop > _eloop;

  /* Dispatchers */
  DockDispatch _dk;
  SessionLockDispatch _lk;
  NetworkDispatch _nd;
  UPowerDispatch _up;
  AcpiDispatch _ad;

  /* Event sources */
  Login1::Manager _login1;
  NetworkManager::Manager _nm;
  UPower1::Manager _upower1;
  Acpi::Manager _acpi;
};

std::shared_ptr<Policy> policy(DBus::Connection &systemd_session,
                               DBus::Connection &system,
                               Glib::RefPtr< Glib::MainLoop > eloop)
{
  return std::shared_ptr<Policy>(new DefaultPolicy(systemd_session,
                                                   system, eloop));
}
