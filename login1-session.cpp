#include <systemd/sd-login.h>
#include <exception>
#include "login1-session.hpp"

DBus::Path Login1::Session::__currentSessionPath() {
    char * session = NULL;
    int rval = sd_pid_get_session(getpid(), &session);
    if (rval)
        return NULL;

    const std::string r = std::string(Login1::OBJECT) + std::string(session);
    free(session);
    return r;
}


Login1::Session::Session(DBus::Connection &connection,
            std::function<void (bool)> hook)
    : DBus::ObjectProxy(connection,
                        Login1::Session::__currentSessionPath(),
                        Login1::BUS),
    __hook(hook)
{}

void Login1::Session::Lock() {
    __hook(true);
}

void Login1::Session::Unlock() {
    __hook(false);
}
