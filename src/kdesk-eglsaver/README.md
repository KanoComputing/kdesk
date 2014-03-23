== Kdesk - OpenGL screen saver

This is kdesk openGL screen saver. To build it just execute:

 $ make

You will need to change the variable OPTDIR in the Makefile to point to your Raspberry /opt
include files and libraries. Usually this is provided by the Raspbian package "raspberrypi-firmware".

=== Icon RAW format

Icons are in raw byte format, no encoding, to make for faster load times.

In order to convert the icons for the cube surfaces, the must be
128x128, 3bpp and no alpha channel. To obtain the raw formats
use of imagemagick and following these simple steps

 * convert source.png -resize 128x128 -alpha off clean.png
 * convert clean.png -size 128x128 -endian LSB -flip rgb:source.raw

=== Building your own screen saver

You can provide your own screen saver program which performs whatever action needed. In such case please keep in mind the following
rules to integrate nicely into kdesk:

 * Provide an initial wait delay of approximately 1 second for seamless user experience.
 * Periodically listen for user input events from keyboard/mouse (/dev/input is a good source)
 * Upon reception of user input, just terminate the program and kdesk will take over control
 * If you'd like kdesk to refresh the graphical desktop upon termination, set your return code to 0, any other value will not refresh the X screen.

