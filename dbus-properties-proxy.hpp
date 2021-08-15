#ifndef _DBUS_PROPERTIES_PROXY_H_
#define _DBUS_PROPERTIES_PROXY_H_

#include <dbus-c++/dbus.h>
#include <cassert>
#include <string>
#include <map>
#include <vector>

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
    virtual void PropertiesChanged(
        const std::string& interface,
        const std::map< std::string, ::DBus::Variant >& changed_properties,
        const std::vector< std::string >& invalidated_properties
    ) = 0;

private:

    /* unmarshalers (to unpack the DBus message before calling the actual signal handler)
     */
    void _PropertiesChanged_stub(const ::DBus::SignalMessage &sig)
    {
        ::DBus::MessageIter ri = sig.reader();

        std::string interface;
        std::map< std::string, ::DBus::Variant > changed_properties;
        std::vector< std::string > invalidated_properties;

        ri >> interface;
        ri >> changed_properties;
        ri >> invalidated_properties;

        PropertiesChanged(interface, changed_properties, invalidated_properties);
    }
};

} } }
#endif
