#include <iostream>
#include <glibmm.h>
#include <dbus-c++/glib-integration.h>
#include <cstring>

#include <systemd/sd-daemon.h>
#include <sys/signalfd.h>
#include <unistd.h>

#include "policy.hpp"

int main(int argc, char *argv[])
{
    static const std::string instance_system = "--system";
    sigset_t handled_signals;
    sigemptyset(&handled_signals);
    sigaddset(&handled_signals, SIGTERM);
    sigaddset(&handled_signals, SIGINT);

    const int sfd = signalfd(-1, &handled_signals, SFD_NONBLOCK | SFD_CLOEXEC);
    if (sfd == -1)
        return 1;

    if (sigprocmask(SIG_BLOCK, &handled_signals, NULL) == -1)
        return 1;

    DBus::Glib::BusDispatcher dispatcher;
    DBus::default_dispatcher = &dispatcher;

    auto eloop = Glib::MainLoop::create(false);
    dispatcher.attach(NULL);

    const auto io_source = Glib::IOSource::create(sfd, Glib::IO_IN | Glib::IO_HUP);
    io_source->connect([sfd,eloop](const Glib::IOCondition c) -> bool
                       {
                           struct signalfd_siginfo fdsi;
                           ssize_t s;

                           s = ::read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
                           assert(s == sizeof(struct signalfd_siginfo));

                           std::cerr << SD_NOTICE "Exiting with signal ("
                                     << fdsi.ssi_signo << ")"
                                     << std::endl;

                           eloop->quit();
                           return true;
                       });
    io_source->attach(eloop->get_context());

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

        auto p = policy(systemd, system, eloop);

        sd_notify(0, "READY=1");
        eloop->run();
    } catch (DBus::Error e) {
        std::cerr << SD_ERR << "Error: DBus (Global): " << e.message() << std::endl;
        sd_notifyf(0,
                   "READY=0\n"
                   "BUSERROR=%s", e.message());
        return -1;
    }

    return 0;
}
