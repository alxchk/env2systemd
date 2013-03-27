#include "acpi-manager.hpp"

#include <systemd/sd-daemon.h>

#include <sstream>
#include <iostream>

#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>

#include "config.h"
#include "__hacks.hpp"

Acpi::Manager::Manager(std::function<void (const std::vector<std::string> &)> dispatch)
    : _stream(NULL), _dispatch(dispatch)
{
  if (! _reconnect())
    return;
}

Acpi::Manager::~Manager()
{
  if (_stream) {
    _stream->close();
    delete _stream;
  }
}

void Acpi::Manager::_reconnect_multiple() {
  Glib::signal_timeout()
      .connect([this]{
          return !this->_reconnect();
        }, DEFAULT_ACPI_CONNECT_DELAY);
}

bool Acpi::Manager::_reconnect()
{
  int fd, r;
  struct sockaddr_un addr;
  static const char unp_acpi[] = "/run/acpid.socket";

  std::cerr << SD_NOTICE "ACPI: Connecting to daemon" << std::endl;

  if (_stream) {
    _stream->close();
    delete _stream;
    _stream = NULL;
  }

  fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (fd < 0) {
      std::cerr << SD_NOTICE "ACPI: Connecting to daemon failed: "
                << strerror(errno)
                << std::endl;
      return false;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, unp_acpi, sizeof(addr.sun_path) - 1);

  r = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
  if (r < 0) {
    std::cerr << SD_NOTICE "ACPI: Connecting to daemon failed: "
              << strerror(errno)
              << std::endl;
    close(fd);
    return false;
  }

  _stream = new __gnu_cxx::stdio_filebuf<char>(fd,std::ios::in);

  auto _source = Glib::IOSource::create(fd, Glib::IO_IN | Glib::IO_HUP);
  _source->connect([this](const Glib::IOCondition c) -> bool
                   {
                     if (c == Glib::IO_HUP) {
                       std::cerr << SD_ERR "ACPI Manager: HUP [1]"
                                 << std::endl;
                       this->_reconnect_multiple();
                       return false;
                     }

                     std::istream in(this->_stream);
                     std::string buffer, item;
                     std::getline(in, buffer);
                     std::istringstream line(buffer);
                     std::vector<std::string> elems;

                     while(std::getline(line, item, ' '))
                       elems.push_back(item);

                     _dispatch(elems);

                     if (c & Glib::IO_HUP) {
                       this->_reconnect_multiple();
                       return false;
                     }

                     return true;
                   });
  _source->attach(Glib::MainContext::get_default());

  return _stream->is_open();
}
