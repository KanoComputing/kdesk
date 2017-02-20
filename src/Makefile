#
# Build kano-desktop
#
#  $ make - Builds the release version
#  $ make debug - Debug version (kdesk-dbg binary)
#
#

DEBUGGING:=

LIBS:=-lXft -lImlib2 -lstdc++ -lpthread -lX11 -lXss -L`pwd`/libkdesk-hourglass -lkdesk-hourglass
XFTINC:=-I/usr/include/freetype2
HOURGLASSINCS= -I`pwd`/libkdesk-hourglass

CFLAGS=-std=c++11

TARGET=kdesk

.PHONY: clean

all: $(TARGET)

debug:
	make all DEBUGGING="-ggdb -DDEBUG" TARGET=kdesk-dbg

# the linkage
$(TARGET): main.o icon.o grid.o background.o configuration.o desktop.o sound.o ssaver.o
	$(CXX) $(LIBS) $^ -o $(TARGET)

# the compilation
icon.o: icon.cpp icon.h logging.h configuration.h grid.h
	$(CXX) -c $(CFLAGS) $(DEBUGGING) $(XFTINC) icon.cpp

grid.o: grid.cpp grid.h
	$(CXX) -c $(CFLAGS) $(DEBUGGING) $(XFTINC) grid.cpp

main.o: main.cpp main.h configuration.h logging.h version.h ssaver.h
	$(CXX) -c $(CFLAGS) $(DEBUGGING) $(XFTINC) main.cpp

background.o: background.cpp logging.h sound.h
	$(CXX) -c $(CFLAGS) $(DEBUGGING) background.cpp

configuration.o: configuration.cpp configuration.h logging.h main.h
	$(CXX) -c $(CFLAGS) $(DEBUGGING) configuration.cpp

desktop.o: desktop.cpp desktop.h logging.h configuration.h sound.h grid.h
	$(CXX) -c $(CFLAGS) $(DEBUGGING) $(XFTINC) $(HOURGLASSINCS) desktop.cpp

sound.o: sound.cpp sound.h
	$(CXX) -c $(CFLAGS) $(DEBUGGING) sound.cpp

ssaver.o: ssaver.cpp ssaver.h
	$(CXX) -c $(CFLAGS) $(DEBUGGING) ssaver.cpp

clean:
	-rm *o kdesk kdesk-dbg
