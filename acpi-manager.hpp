#pragma once

#include <functional>
#include <string>
#include <vector>

namespace Acpi
{
  class Manager
  {
  public:
    Manager(std::function<void (const std::vector<std::string> &)>);
    virtual ~Manager();

    bool dispatch();
    void stop();

  private:
    bool _connect();

  private:
    int _fd;
    bool _active;
    std::function<void (const std::vector<std::string> &)> _dispatch;
  };
}
