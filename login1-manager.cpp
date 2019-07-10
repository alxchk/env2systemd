#include <systemd/sd-login.h>
#include <exception>
#include <sys/types.h>
#include <unistd.h>

#include "login1-manager.hpp"

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
    std::function<void (bool)> lock_hook)
    : DBus::ObjectProxy(connection,
                        Login1::OBJECT,
                        Login1::BUS),
      __current_session(NULL),
      _manager(_manager)
{
    std::string session = this->__getCurrentSession();
    if (session.empty()) {
        std::cerr << "Login1: Couldn't find active session" << std::endl;
        return;
    }

    std::cerr << "Login1: attach to session: " << session << std::endl;

    try {
        __current_session = new Session(connection, GetSession(session), lock_hook);
    }
    catch (const std::exception &e) {
        std::cerr << "Login1: Error: " <<  e.what() << std::endl;
    }
}

std::string Login1::Manager::__getCurrentSession() {
    char * session = getenv("XDG_SESSION_ID");
    if (session != NULL) {
        return std::string(session);
    }

    int rval = sd_pid_get_session(getpid(), &session);
    if (!rval) {
        std::string s_session = std::string(session);
        free(session);
        return s_session;
    }

    for (std::string environ: _manager.Environment()) {
        size_t split = environ.find('=');

        if (split == std::string::npos)
            continue;

        if (environ.compare(0, split, "XDG_SESSION_ID") != 0)
            continue;

        return environ.substr(split+1);
    }

    return std::string();
}

Login1::Manager::~Manager()
{
    if (__current_session)
        delete __current_session;
}
