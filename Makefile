PKGDEPS  := libsystemd-login libsystemd-daemon dbus-c++-1
PKGDEPS_EXISTS := checkdeps

OUTPUT   := env2systemd
SOURCES  := main.cpp \
		policy.cpp \
		login1-manager.cpp \
		upower1-manager.cpp \
		systemd1-manager.cpp \
		acpi-manager.cpp \
		network-manager.cpp
PROXIES  := systemd1-manager-proxy.hpp \
		systemd1-manager-unit-proxy.hpp \
		login1-proxy.hpp \
		login1-session-proxy.hpp \
		network-manager-proxy.hpp \
		network-manager-settings-proxy.hpp \
		network-manager-active-connection-proxy.hpp \
		upower1-proxy.hpp
CXXSTD   := -std=gnu++11
OPTIMIZE ?= -O2 -flto
CXXFLAGS += $(shell pkg-config --cflags $(PKGDEPS)) $(CXXSTD) $(OPTIMIZE)
LDFLAGS  += $(shell pkg-config --libs $(PKGDEPS)) -flto -Wl,-O1 -Wl,--as-needed

.PHONY: clean checkdeps

all: $(OUTPUT)

systemd1-manager-proxy.hpp: introspection/org.freedesktop.systemd1.Manager.xml
	dbusxx-xml2cpp $< --proxy=$@

systemd1-manager-unit-proxy.hpp: introspection/org.freedesktop.systemd1.Unit.xml
	dbusxx-xml2cpp $< --proxy=$@

login1-proxy.hpp: introspection/org.freedesktop.login1.xml
	dbusxx-xml2cpp $< --proxy=$@

login1-session-proxy.hpp: introspection/org.freedesktop.login1.Session.xml
	dbusxx-xml2cpp $< --proxy=$@

upower1-proxy.hpp: introspection/org.freedesktop.UPower.xml
	dbusxx-xml2cpp $< --proxy=$@

network-manager-proxy.hpp: introspection/org.freedesktop.NetworkManager.xml
	dbusxx-xml2cpp $< --proxy=$@

network-manager-settings-proxy.hpp: introspection/org.freedesktop.NetworkManager.Settings.xml
	dbusxx-xml2cpp $< --proxy=$@

network-manager-active-connection-proxy.hpp: introspection/org.freedesktop.NetworkManager.Active.Connection.xml
	dbusxx-xml2cpp $< --proxy=$@

$(OUTPUT): $(SOURCES:.cpp=.o) | $(PKGDEPS_EXISTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp | $(PKGDEPS_EXISTS) $(PROXIES)
	$(CXX) -c -o $@ $< $(CXXFLAGS)
	@$(CXX) -MM $< $(CXXFLAGS) | \
		sed -e "s@$$(basename $@):@$@:@" > $@.d

clean:
	rm -f $(OUTPUT) $(SOURCES:.cpp=.o) $(SOURCES:.cpp=.o.d) $(PROXIES)

checkdeps:
	pkg-config --exists $(PKGDEPS)

install: $(OUTPUT)
	install -m 0111 -D $(OUTPUT) $(DESTDIR)/usr/bin/$(OUTPUT)
	install -m 0444 -D contrib/env2systemd.service \
			$(DESTDIR)/usr/lib/systemd/system/env2systemd.service
	install -m 0444 -D contrib/env2systemd.service \
			$(DESTDIR)/usr/lib/systemd/user/env2systemd.service

-include $(SOURCES:.cpp=.o.d)
