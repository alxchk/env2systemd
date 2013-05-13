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
    __hook(true);
}

void Login1::Session::Unlock() {
    __hook(false);
}

Login1::Manager::Manager(DBus::Connection &connection,
            std::function<void (bool)> lock_hook)
    : DBus::ObjectProxy(connection,
                        Login1::OBJECT,
                        Login1::BUS),
      __current_session(NULL)
{
    char * session = NULL;
    int rval = sd_pid_get_session(getpid(), &session);
    if (rval) {
        free(session);
        std::cerr << "Login1: Couldn't find active session" << std::endl;
        return;
    }

    try {
        __current_session = new Session(connection,
                                        GetSession(session),
                                        lock_hook);
    }
    catch (const std::exception &e) {
        std::cerr << "Login1: Error: " <<  e.what() << std::endl;
    }

    free(session);
}

Login1::Manager::~Manager()
{
    if (__current_session)
        delete __current_session;
}
