#pragma once
#include "config.h"
#include <memory>

class Policy
{
protected:
  Policy() {}
};

std::shared_ptr<Policy> policy(DBus::Connection &systemd_session,
                               DBus::Connection &system);
