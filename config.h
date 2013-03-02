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
