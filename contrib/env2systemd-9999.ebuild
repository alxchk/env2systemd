EAPI=5
inherit eutils mercurial
DESCRIPTION="Package translate various dbus events to systemd units states"
HOMEPAGE="https://bitbucket.org/alxchk/env2systemd/src"
EHG_REPO_URI="https://bitbucket.org/alxchk/env2systemd"
LICENSE="MIT"
SLOT="0"
KEYWORDS="~x86 ~amd64"
DEPEND="dev-libs/dbus-c++[glib] dev-cpp/glibmm virtual/pkgconfig"
RDEPEND="
    dev-libs/dbus-c++[glib]
    dev-cpp/glibmm
    >sys-apps/systemd-195 
    sys-power/acpi 
    sys-power/upower
    >net-misc/networkmanager-0.9"

