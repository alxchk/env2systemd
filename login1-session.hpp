#pragma once

#include <functional>
#include <dbus-c++/dbus.h>
#include "login1-session-proxy.hpp"

namespace Login1
{
  static const char BUS[]       = "org.freedesktop.login1";
  static const char OBJECT[]    = "/org/freedesktop/login1/session/";

  class Session
    : public org::freedesktop::login1::Session_proxy,
      public DBus::ObjectProxy
  {
    std::function<void (bool)> __hook;

  public:
    Session(DBus::Connection &connection,
            std::function<void (bool)>);

  protected:
    virtual void Lock();
    virtual void Unlock();

  private:
    static DBus::Path __currentSessionPath();

  };
}
