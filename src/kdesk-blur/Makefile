#
# kdesk-blur Makefile
#

INCS:=-I../
LIBS:=-lXft -lImlib2 -lstdc++ -lpthread -lXss -lX11
TARGET=kdesk-blur

all: $(TARGET)

debug:
	make all DEBUGGING="-ggdb -O3 -DDEBUG"

# the linkage
$(TARGET): $(TARGET).o
	g++ $(LIBS) $^ $(DEBUGGING) -o $(TARGET)

# the compilation
kdesk-blur.o: $(TARGET).cpp $(TARGET).h
	g++ -c $(INCS) $(DEBUGGING) $(TARGET).cpp
