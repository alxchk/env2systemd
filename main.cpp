#include <iostream>
#include <dbus-c++/dbus.h>
#include <sys/signal.h>

#include "policy.hpp"

DBus::BusDispatcher dispatcher;

void terminate(int sig) {
    dispatcher.leave();
}

int main(int argc, char *argv[])
{
    signal(SIGINT, terminate);
    signal(SIGTERM, terminate);

    static const std::string instance_system = "--system";

    DBus::default_dispatcher = &dispatcher;

    DBus::Connection system  = DBus::Connection::SystemBus();
    DBus::Connection session = DBus::Connection::SessionBus();

    try {
        auto p =
            policy((argc == 2 && argv[1] == instance_system)
                   ? system
                   : session,
                   system);
        dispatcher.enter();
    } catch (DBus::Error e) {
        std::cerr << "Get unhandled DBus exception: " << e.message() << std::endl;
        return -1;
    }

    std::cout << "Goodbye" << std::endl;

    return 0;
}
