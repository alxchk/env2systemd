#include "acpi-manager.hpp"

#include <ext/stdio_filebuf.h>
#include <sstream>

#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>

Acpi::Manager::Manager(std::function<void (const std::vector<std::string> &)> dispatch)
  : _fd(-1), _dispatch(dispatch)
{}

Acpi::Manager::~Manager()
{
  if (_fd != -1)
    close(_fd);
}

void Acpi::Manager::stop()
{
  _active = false;
}

bool Acpi::Manager::dispatch()
{
  if (! _connect())
    return false;

  __gnu_cxx::stdio_filebuf<char> filebuf(_fd, std::ios::in);
  std::istream in(&filebuf);

  _active = true;

  while (_active) {
    std::string buffer, item;
    std::getline(in, buffer);
    std::istringstream line(buffer);
    std::vector<std::string> elems;
    while(std::getline(line, item, ' ')) {
      elems.push_back(item);
    }

    _dispatch(elems);
  }

  return true;
}

bool Acpi::Manager::_connect()
{
  int fd, r;
  struct sockaddr_un addr;
  static const char unp_acpi[] = "/var/run/acpid.socket";

  if (_fd != -1)
    return true;

  fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (fd < 0)
    return false;

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, unp_acpi, sizeof(addr.sun_path) - 1);

  r = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
  if (r < 0) {
    close(fd);
    return false;
  }

  _fd = fd;
}
