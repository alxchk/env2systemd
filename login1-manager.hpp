#pragma once

#include <functional>
#include <list>
#include <dbus-c++/dbus.h>
#include <sys/types.h>

#include <memory>

#include "systemd1-manager.hpp"
#include "login1-proxy.hpp"
#include "login1-session-proxy.hpp"
#include "login1-user-proxy.hpp"

namespace Login1
{
  static const char BUS[]       = "org.freedesktop.login1";
  static const char OBJECT[]    = "/org/freedesktop/login1";
  static const char SELF[]      = "/org/freedesktop/login1/user/self";

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

  class SessionViewOnly
    : public org::freedesktop::login1::Session_proxy,
      public DBus::ObjectProxy
  {

  public:
    SessionViewOnly(DBus::Connection &connection,
            DBus::Path path)
    : DBus::ObjectProxy(connection, path, Login1::BUS)
    {};

  protected:
    virtual void Lock() {};
    virtual void Unlock() {};
  };


  class User
    : public org::freedesktop::login1::User_proxy,
      public DBus::ObjectProxy
  {
    DBus::Connection &_connection;
    std::function<void (bool)> __lock_hook;
    std::list<std::unique_ptr<Session>> __handled_sessions;

  public:
    User(
      DBus::Connection &connection,
      std::function<void (bool)> lock_hook
    );

    void add(const DBus::Path &session);
    void remove(const DBus::Path &session);
  };

  class Manager
    : public org::freedesktop::login1::Manager_proxy,
      public DBus::ObjectProxy
  {
    Systemd1::Manager & _manager;
    DBus::Connection & _connection;
    std::function<void (bool)> __dock_hook;
    bool __is_docked;
    std::unique_ptr<User> __current_user;

  public:
    Manager(
      Systemd1::Manager & _manager,
      DBus::Connection &connection,
      std::function<void (bool)> lock_hook,
      std::function<void (bool)> dock_hook
    );

    void update();

  protected:
    virtual void SessionNew(const std::string&, const DBus::Path&);
    virtual void SessionRemoved(const std::string&, const DBus::Path&);

    virtual void UserNew(const uint32_t&, const DBus::Path&) {}
    virtual void UserRemoved(const uint32_t&, const DBus::Path&) {}
    virtual void SeatNew(const std::string&, const DBus::Path&) {}
    virtual void SeatRemoved(const std::string&, const DBus::Path&) {}
    virtual void PrepareForShutdown(const bool&) {}
    virtual void PrepareForSleep(const bool&) {}
  };
}
