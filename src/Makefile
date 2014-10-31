#
# Build kano-desktop
#
#  $ make - builds the release version
#  $ make debug - with debug logs to console
#
# Requirements:
#
#  libx11-dev, libxft-dev, libstartup-notification0-dev, libao-dev, libmpg123-0-dev
#

DEBUGGING:=

LIBS:=-lXft -lImlib2 -lstdc++ `pkg-config --libs libstartup-notification-1.0` -lpthread -lX11 -lXss
XFTINC:=-I/usr/include/freetype2
SNINC=-I/usr/include/startup-notification-1.0

TARGET=kdesk

.PHONY: clean

all: $(TARGET)

debug:
	make all DEBUGGING="-ggdb -O3 -DDEBUG" TARGET=kdesk-dbg

# the linkage
$(TARGET): main.o icon.o grid.o background.o configuration.o desktop.o sound.o ssaver.o
	g++ $(LIBS) $^ -o $(TARGET)

# the compilation
icon.o: icon.cpp icon.h logging.h configuration.h grid.h
	g++ -c $(DEBUGGING) $(XFTINC) icon.cpp

grid.o: grid.cpp grid.h
	g++ -c $(DEBUGGING) $(XFTINC) grid.cpp

main.o: main.cpp main.h configuration.h logging.h version.h ssaver.h
	g++ -c $(DEBUGGING) $(XFTINC) $(SNINC) main.cpp

background.o: background.cpp logging.h sound.h
	g++ -c $(DEBUGGING) background.cpp

configuration.o: configuration.cpp configuration.h logging.h main.h
	g++ -c $(DEBUGGING) configuration.cpp

desktop.o: desktop.cpp desktop.h logging.h configuration.h sn_callbacks.cpp sound.h grid.h
	g++ -c $(DEBUGGING) $(XFTINC) $(SNINC) desktop.cpp

sound.o: sound.cpp sound.h
	g++ -c $(DEBUGGING) $(XFTINC) $(SNINC) sound.cpp

ssaver.o: ssaver.cpp ssaver.h
	g++ -c $(DEBUGGING) $(XFTINC) $(SNINC) ssaver.cpp

clean:
	-rm *o kdesk kdesk-dbg
