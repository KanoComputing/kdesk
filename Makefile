#
#  Makefile - builds kdesk
#
#  Please read README.md file for differences on release and debug builds
#

all: kdesk

kdesk:
	cd src && make all
	cd src/kdesk-eglsaver && make all

debug:
	cd src && make debug
	cd src/kdesk-eglsaver && make debug

kano-debber:
	mkdir -p /home/user/.kdesktop
	cp doc/config/.kdeskrc /home/user/
	cp doc/config/XEyes.lnk /home/user/.kdesktop/
	cp doc/config/XCalc.lnk /home/user/.kdesktop/
	sed -i.bak '/^@pcman/d' /etc/xdg/lxsession/LXDE/autostart
	echo '@kdesk' >> /etc/xdg/lxsession/LXDE/autostart
	chown -R user:user /home/user/.kdesk*
	cd src && make all

