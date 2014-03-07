#
#  Makefile - builds kdesk
#
#  Please read README.md file for differences on release and debug builds
#

all: kdesk

kdesk:
	cd src && make -B

debug:
	cd src && make debug -B
