#
#  Build kfbsaver - Kdesk Framebuffer screen saver
#

all: kfbsaver

kfbsaver: kfbsaver.o
	gcc kfbsaver.o -o kfbsaver

kfbsaver.o: kfbsaver.cpp
	gcc -c -O1 kfbsaver.cpp