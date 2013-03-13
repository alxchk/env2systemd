#include "systemd1-manager.hpp"

Systemd1::Manager::Manager(DBus::Connection &connection)
  : DBus::ObjectProxy(connection, Systemd1::PATH, Systemd1::NAME),
    _connection(connection) {
}

Systemd1::Unit Systemd1::Manager::getUnit(const std::string &unit) {
  DBus::Path p = LoadUnit(unit);
  return Unit(_connection, p);
}

Systemd1::Unit::Unit(DBus::Connection &connection,
                     DBus::Path &path)
  :  DBus::ObjectProxy(connection, path, Systemd1::NAME) {
}
