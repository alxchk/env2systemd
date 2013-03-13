#pragma once

#include <dbus-c++/dbus.h>
#include "systemd1-manager-unit-proxy.hpp"
#include "systemd1-manager-proxy.hpp"

namespace Systemd1
{
  static const char * NAME = "org.freedesktop.systemd1";
  static const char * PATH = "/org/freedesktop/systemd1";

  class Unit
    : public org::freedesktop::systemd1::Unit_proxy,
      public DBus::ObjectProxy {
  public:
    Unit(DBus::Connection &connection,
         DBus::Path &path);
  };

  class Manager
    : public org::freedesktop::systemd1::Manager_proxy,
      public DBus::ObjectProxy
  {
    DBus::Connection &_connection;

  public:
    Manager(DBus::Connection &connection);
    Unit getUnit(const std::string &);

  protected:
    virtual void UnitNew(const std::string& unit, const DBus::Path& path) {}
    virtual void UnitRemoved(const std::string& unit, const DBus::Path& path) {}
    virtual void JobNew(const uint32_t& id, const DBus::Path& path, const std::string& job) {}
    virtual void JobRemoved(const uint32_t& id, const DBus::Path& path, const std::string& job, const std::string& status) {}
    virtual void StartupFinished(const uint64_t& t1, const uint64_t& t2, const uint64_t& t3, const uint64_t& t4) {}
    virtual void UnitFilesChanged() {}
    virtual void StartupFinished(const uint64_t& firmware, const uint64_t& loader, const uint64_t& kernel, const uint64_t& initrd, const uint64_t& userspace, const uint64_t& total){};
  };
}
