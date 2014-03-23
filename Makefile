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
