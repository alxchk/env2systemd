#include <systemd/sd-daemon.h>

#include <iostream>
#include <dbus-c++/dbus.h>
#include <sys/signal.h>
#include <cstring>

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

    auto r = sd_booted();

    if (r == 0) {
        std::cerr << SD_ERR "Service unusable without systemd" << std::endl;
        return -1;
    } else if (r < 0) {
        std::cerr << SD_ERR "Couldn't get systemd booted status: " << ::strerror(errno) << std::endl;
        return -errno;
    }

    bool system_instance = true;

    try {
        system_instance = (argc > 1 && instance_system == argv[1])
            || std::stoi(getenv("MANAGERPID")) == 1;
    } catch (...) {}

    if (system_instance) {
        std::cerr << SD_NOTICE "Starting system-wide instance" << std::endl;
        sd_notify(0,
                  "STATUS=System-wide instance\n"
                  "READY=0");
    } else {
        sd_notify(0,
                  "STATUS=Session-wide instance\n"
                  "READY=0");
    }

    try {
        DBus::Connection system = DBus::Connection::SystemBus();
        DBus::Connection systemd = system_instance
            ? system
            : DBus::Connection::SessionBus()
            ;

        auto p = policy(systemd, system);

        sd_notify(0, "READY=1");
        dispatcher.enter();
    } catch (DBus::Error e) {
        std::cerr << SD_ERR << "Error: DBus (Global): " << e.message() << std::endl;
        sd_notifyf(0,
                   "READY=0\n"
                   "BUSERROR=%s", e.message());
        return -1;
    }

    return 0;
}
