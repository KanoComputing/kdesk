#
# libkdesk-hourglass Makefile
#
#  gcc -shared  -rdynamic -Wl,-soname,testlib -o testlib.so -fPIC testlib.cpp
#

libstartup:=libstartup-notification-1.0

INCS:=-I../ `pkg-config --cflags-only-I $(libstartup)`
LIBS:=`pkg-config --libs $(libstartup)`
TARGET:=kdesk-hourglass

CPPTEST:=testlib

all: $(TARGET) $(CPPTEST)

debug:
	make all DEBUGGING="-ggdb -O3 -DDEBUG"

# the linkage
$(TARGET): $(TARGET).o
	g++ -shared -rdynamic -fPIC $(LIBS) $^ $(DEBUGGING) -o lib$(TARGET).so

# the compilation
$(TARGET).o: $(TARGET).cpp $(TARGET).h
	g++ -c $(INCS) $(DEBUGGING) $(TARGET).cpp


# c++ test application
$(CPPTEST): $(CPPTEST).o
	g++ $(CPPTEST).o -ldl -o $(CPPTEST)

$(CPPTEST).o: $(CPPTEST).cpp
	g++ -c $(CPPTEST).cpp
