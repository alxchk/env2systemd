#pragma once

#ifndef SYSTEMD_OVERRIDE
#define SYSTEMD_OVERRIDE    "replace"
#endif

#ifndef UNIT_LOCK_SESSION
#define UNIT_LOCK_SESSION   "lock.target"
#endif

#ifndef UNIT_NETWORK_PLACE
#define UNIT_NETWORK_PLACE  "network@%i.target"
#endif

#ifndef UNIT_NETWORK_STATE
#define UNIT_NETWORK_STATE  "network.target"
#endif

#ifndef UNIT_ON_BATTERY
#define UNIT_ON_BATTERY     "battery.target"
#endif

#ifndef UNIT_ON_LOW_BATTERY
#define UNIT_ON_LOW_BATTERY "low-battery.target"
#endif

#ifndef UNIT_ACPI_LID
#define UNIT_ACPI_LID "acpi-lid.target"
#endif

#ifndef UNIT_ACPI_HKEY
#define UNIT_ACPI_HKEY "acpi-hkey"
#endif

#ifndef UNIT_ACPI_HKEY_TYPE
#define UNIT_ACPI_HKEY_TYPE "target"
#endif

#ifndef DEFAULT_ACTION_TIMEOUT
#define DEFAULT_ACTION_TIMEOUT 10000
#endif

#ifndef DEFAULT_ACPI_CONNECT_DELAY
#define DEFAULT_ACPI_CONNECT_DELAY 1000
#endif
