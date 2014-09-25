#ifndef _DBUS_PROPERTIES_PROXY_H_
#define _DBUS_PROPERTIES_PROXY_H_

#include <dbus-c++/dbus.h>
#include <cassert>

namespace org {
namespace freedesktop {
namespace DBus {

class Properties_proxy
: public ::DBus::InterfaceProxy
{
public:

    Properties_proxy()
    : ::DBus::InterfaceProxy("org.freedesktop.DBus.Properties")
    {
        connect_signal(Properties_proxy,
                       PropertiesChanged,
                       _PropertiesChanged_stub);
    }

public:

    /* signal handlers for this interface
     */
    virtual void PropertiesChanged() = 0;

private:

    /* unmarshalers (to unpack the DBus message before calling the actual signal handler)
     */
    void _PropertiesChanged_stub(const ::DBus::SignalMessage &sig)
    {
        PropertiesChanged();
    }
};

} } }
#endif
