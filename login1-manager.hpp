#pragma once

#include <functional>
#include <dbus-c++/dbus.h>
#include <sys/types.h>
#include "login1-proxy.hpp"
#include "login1-session-proxy.hpp"

namespace Login1
{
  static const char BUS[]       = "org.freedesktop.login1";
  static const char OBJECT[]    = "/org/freedesktop/login1";

  class Session
    : public org::freedesktop::login1::Session_proxy,
      public DBus::ObjectProxy
  {
    std::function<void (bool)> __hook;

  public:
    Session(DBus::Connection &connection,
            DBus::Path path,
            std::function<void (bool)>);

  protected:
    virtual void Lock();
    virtual void Unlock();
  };

  class Manager
    : public org::freedesktop::login1::Manager_proxy,
      public DBus::ObjectProxy
  {
    Session * __current_session;

  public:
    Manager(DBus::Connection &connection,
            std::function<void (bool)> lock_hook);
    ~Manager();

  protected:
    virtual void SessionNew(const std::string&, const DBus::Path&) {}
    virtual void SessionRemoved(const std::string&, const DBus::Path&) {}
    virtual void UserNew(const uint32_t&, const DBus::Path&) {}
    virtual void UserRemoved(const uint32_t&, const DBus::Path&) {}
    virtual void SeatNew(const std::string&, const DBus::Path&) {}
    virtual void SeatRemoved(const std::string&, const DBus::Path&) {}
    virtual void PrepareForShutdown(const bool&) {}
    virtual void PrepareForSleep(const bool&) {}

  private:
    Session __getCurrentSession();
  };
}
