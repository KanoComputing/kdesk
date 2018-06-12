#
#  Makefile - builds kdesk
#
#  Please read README.md file for differences on release and debug builds
#

all: kdesk

kdesk:
	mkdir -p build/release
	cd build/release && cmake -DCMAKE_BUILD_TYPE=Release ../../src && make

debug:
	mkdir -p build/debug
	cd build/debug && cmake -DCMAKE_BUILD_TYPE=Debug ../../src && make

kano-debber:
	mkdir -p /home/user/.kdesktop
	cp doc/config/.kdeskrc /home/user/
	cp doc/config/*.lnk /home/user/.kdesktop/

	sed -i.bak 's/@pcmanfm --desktop --profile LXDE/@kdesk/' /etc/xdg/lxsession/LXDE/autostart

	chown -R user:user /home/user/.kdesk*
	cd src && make all
	cd src/kdesk-blur && make all
	cd src/libkdesk-hourglass && make all
