#pragma once

#include <ext/stdio_filebuf.h>

#include <functional>
#include <string>
#include <vector>

#include <glibmm.h>

namespace Acpi
{
  class Manager
  {
  public:
    Manager(std::function<void (const std::vector<std::string> &)>);
    virtual ~Manager();

  private:
    bool _reconnect();
    void _reconnect_multiple();

  private:
    __gnu_cxx::stdio_filebuf<char> * _stream;
    std::function<void (const std::vector<std::string> &)> _dispatch;
  };
}
