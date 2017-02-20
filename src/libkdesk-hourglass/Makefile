#
# libkdesk-hourglass - Makefile
#
#  A library API to provide for a loading progress mouse cursor accross your apps
#

libstartup:=libstartup-notification-1.0

INCS:=-I../ `pkg-config --cflags-only-I $(libstartup)`
LIBS:=`pkg-config --libs $(libstartup)`
TARGET:=kdesk-hourglass

CPPTESTDYNLIB:=testdynlib
CPPTESTSHARED:=testshared
CPPTESTSHARED:=testshared

KDESK_HOURGLASS_APP:=kdesk-hourglass-app

all: $(TARGET) $(CPPTESTDYNLIB) $(CPPTESTSHARED) $(KDESK_HOURGLASS_APP)

debug:
	make all DEBUGGING="-ggdb -O3 -DDEBUG"

clean:
	-rm *o
	-rm lib$(TARGET).so
	-rm $(CPPTESTDYNLIB) $(CPPTESTSHARED) $(KDESK_HOURGLASS_APP)

# the linkage
$(TARGET): $(TARGET).o
	$(CXX) -shared -rdynamic -Wl,-soname,lib$(TARGET).so -o lib$(TARGET).so $(LIBS) $^ $(DEBUGGING)

# the compilation
$(TARGET).o: $(TARGET).cpp $(TARGET).h
	$(CXX) -c -fPIC $(INCS) $(DEBUGGING) $(TARGET).cpp

# C++ sample on how to use the kdesk hourglass library dynamically
$(CPPTESTDYNLIB): $(CPPTESTDYNLIB).o
	$(CXX) $(CPPTESTDYNLIB).o -ldl -o $(CPPTESTDYNLIB)

$(CPPTESTDYNLIB).o: $(CPPTESTDYNLIB).cpp
	$(CXX) -c $(CPPTESTDYNLIB).cpp

# C++ sample on how to use the kdesk hourglass library at build time
$(CPPTESTSHARED): $(CPPTESTSHARED).o
	$(CXX) $(CPPTESTSHARED).o -L. -l$(TARGET) -o $(CPPTESTSHARED)

$(CPPTESTSHARED).o: $(CPPTESTSHARED).cpp
	$(CXX) -c -g $(CPPTESTSHARED).cpp

# A command line tool to bring up the hourglass to a loading app
$(KDESK_HOURGLASS_APP): $(KDESK_HOURGLASS_APP).o
	$(CXX) $(KDESK_HOURGLASS_APP).o -L. -l$(TARGET) -o $(KDESK_HOURGLASS_APP)

$(KDESK_HOURGLASS_APP).o: $(KDESK_HOURGLASS_APP).cpp
	$(CXX) -c -g $(KDESK_HOURGLASS_APP).cpp

