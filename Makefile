#
#  Makefile - builds kdesk
#
#  Please read README.md file for differences on release and debug builds
#

all: kdesk

kdesk:
	cd src/libkdesk-hourglass && make all

	cd src && make -B
	cd src && make debug -B

	cd src/kdesk-eglsaver && make all
	cd src/kdesk-blur && make all

debug:
	cd src/libkdesk-hourglass && make all
	cd src && make debug -B
	cd src/kdesk-eglsaver && make debug
	cd src/kdesk-blur && make debug

kano-debber:
	mkdir -p /home/user/.kdesktop
	cp doc/config/.kdeskrc /home/user/
	cp doc/config/*.lnk /home/user/.kdesktop/

	sed -i.bak 's/@pcmanfm --desktop --profile LXDE/@kdesk/' /etc/xdg/lxsession/LXDE/autostart

	chown -R user:user /home/user/.kdesk*
	cd src && make all
	cd src/kdesk-blur && make all
	cd src/libkdesk-hourglass && make all
