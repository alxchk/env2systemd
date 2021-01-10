#include <systemd/sd-login.h>
#include <systemd/sd-daemon.h>
#include <exception>
#include <sys/types.h>
#include <unistd.h>

#include "login1-manager.hpp"

Login1::User::User(DBus::Connection &connection, std::function<void (bool)> lock_hook)
    : DBus::ObjectProxy(connection, Login1::SELF, Login1::BUS),
    _connection(connection),
    __lock_hook(lock_hook)
{
    for (auto session: Sessions()) {
        std::cerr << "Handle new session: " << session._1 << std::endl;
        __handled_sessions.push_back(
            new Session(_connection, session._2, __lock_hook)
        );
    }
}

void Login1::User::add(const DBus::Path &new_session) {
    for (auto session: __handled_sessions) {
        if (session->path() == new_session) {
            return;
        }
    }

    std::cerr << "Handle new session: " << new_session << std::endl;
    __handled_sessions.push_back(
        new Session(_connection, new_session, __lock_hook)
    );
}

void Login1::User::remove(const DBus::Path &removed_session) {
    for (auto session: __handled_sessions) {
        if (session->path() == removed_session) {
            std::cerr << "Remove handled session " << removed_session << std::endl;
            delete session;
            __handled_sessions.remove(session);
            return;
        }
    }
}

Login1::User::~User() {
    for (auto handled_session: __handled_sessions) {
        delete handled_session;
    }
    __handled_sessions.clear();
}

Login1::Session::Session(DBus::Connection &connection,
                         DBus::Path path,
                         std::function<void (bool)> lock_hook)
    : DBus::ObjectProxy(connection,
                        path,
                        Login1::BUS),
    __hook(lock_hook)
{}

void Login1::Session::Lock() {
    std::cerr << "Session lock request" << std::endl;
    __hook(true);
}

void Login1::Session::Unlock() {
    std::cerr << "Session unlock request" << std::endl;
    __hook(false);
}

Login1::Manager::Manager(
    Systemd1::Manager & _manager, DBus::Connection &connection,
    std::function<void (bool)> lock_hook,
    std::function<void (bool)> dock_hook)
    : DBus::ObjectProxy(connection,
                        Login1::OBJECT,
                        Login1::BUS),
      _manager(_manager),
      _connection(connection),
      __dock_hook(dock_hook),
      __is_docked(Docked()),
      __current_user(nullptr)
{
    try {
        this->__current_user = new User(connection, lock_hook);
    } catch (DBus::Error e) {
        std::cerr << SD_ERR << "Error: DBus (Login1): " << e.message() << std::endl;
    }
}

void Login1::Manager::update() {
    bool current_is_docked = Docked();
    if (current_is_docked != __is_docked) {
        __dock_hook(current_is_docked);
        __is_docked = current_is_docked;
    }
}

void Login1::Manager::SessionNew(const std::string &session, const DBus::Path &object)
{
    SessionViewOnly sobj = SessionViewOnly(_connection, object);
    auto uid = sobj.User()._1;
    std::cerr << "New session (global) " << session << "; uid: " << uid << std::endl;
    if (uid == getuid()) {
        if (this->__current_user) {
            std::cerr << "Handle new session: " << session << std::endl;
            __current_user->add(object);
        } else {
            std::cerr << "Handle new session: current user is not tracked" << std::endl;
        }
    }
}

void Login1::Manager::SessionRemoved(const std::string &session, const DBus::Path &object)
{
    if (this->__current_user) {
        std::cerr << "Removed session (global)" << session << std::endl;
        __current_user->remove(object);
    }
}

Login1::Manager::~Manager()
{
    if (this->__current_user) {
        delete this->__current_user;
        this->__current_user = nullptr;
    }
}
