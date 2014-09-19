#
# libkdesk-hourglass Makefile
#
#  gcc -shared  -rdynamic -Wl,-soname,testlib -o testlib.so -fPIC testlib.cpp
#

libstartup:=libstartup-notification-1.0

INCS:=-I../ `pkg-config --cflags-only-I $(libstartup)`
LIBS:=`pkg-config --libs $(libstartup)`
TARGET:=kdesk-hourglass

CPPTEST:=testdynlib
CPPTESTSHARED:=testshared

all: $(TARGET) $(CPPTEST) $(CPPTESTSHARED)

debug:
	make all DEBUGGING="-ggdb -O3 -DDEBUG"

# the linkage
$(TARGET): $(TARGET).o
	g++ -shared -rdynamic -Wl,-soname,lib$(TARGET).so -o lib$(TARGET).so $(LIBS) $^ $(DEBUGGING)

# the compilation
$(TARGET).o: $(TARGET).cpp $(TARGET).h
	g++ -c -fPIC $(INCS) $(DEBUGGING) $(TARGET).cpp

# c++ dynamic loading test application
$(CPPTEST): $(CPPTEST).o
	g++ $(CPPTEST).o -ldl -o $(CPPTEST)

$(CPPTEST).o: $(CPPTEST).cpp
	g++ -c $(CPPTEST).cpp

# c++ dynamic loading test application
$(CPPTESTSHARED): $(CPPTESTSHARED).o
	g++ $(CPPTESTSHARED).o -L. -l$(TARGET) -o $(CPPTESTSHARED)

$(CPPTESTSHARED).o: $(CPPTESTSHARED).cpp
	g++ -c -g $(CPPTESTSHARED).cpp
