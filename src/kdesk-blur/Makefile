#
# kdesk-blur Makefile
#

LIBS:=-lXft -lImlib2 -lstdc++ -lpthread -lXss
TARGET=kdesk-blur

all: $(TARGET)
debug: all

# the linkage
$(TARGET): $(TARGET).o
	g++ $(LIBS) $^ -o $(TARGET)

# the compilation
kdesk-blur.o: $(TARGET).cpp $(TARGET).h
	g++ -c $(TARGET).cpp
