#pragma once
#include "config.h"
#include <memory>

#include <dbus-c++/glib-integration.h>
#include <glibmm.h>

class Policy
{
protected:
  Policy() {}
};

std::shared_ptr<Policy> policy(DBus::Connection &systemd_session,
                               DBus::Connection &system,
                               Glib::RefPtr< Glib::MainLoop > eloop);
