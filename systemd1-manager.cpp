#include "systemd1-manager.hpp"

Systemd1::Manager::Manager(DBus::Connection &connection)
  : DBus::ObjectProxy(connection, Systemd1::PATH, Systemd1::NAME)
{
}
