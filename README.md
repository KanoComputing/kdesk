== Kano Desktop manager - kDesk

Kano desktop manager is an app specifically designed to work on KanuxOS.
It is responsible for presenting the Kano desktop background image and Kano Make icons.

=== Introduction

It is based on idesk and vdesk but it is rewritten from scratch.

 * https://github.com/neagix/idesk
 * http://xvnkb.sourceforge.net/?menu=vdesk&lang=en

It also provides a connection with the notifiy library for Openbox window manager
and others, so the hourglass start-up feature works seamlessly.

It comes with the following additional features:

 * icon sounds on desktop startup, icon startup, and disabled icon double click events
 * cost effective screen saver mechanism
 * a screen saver based on native RaspberryPI openGL in the GPU

==== Configuration

Configuration is handled by the file /usr/share/kano-desktop/kdesk/kdeskrc.
The syntax is compatible with that of kdesk, but it also offers additional
keywords which are detailed below.

Icons are defined in independent files located under /usr/share/kano-desktop/kdesk/kdesktop
directory, and also under homedir/.kdesktop. The syntax of the icon files format
is also compatible with kdesk but also provides for additional features.

==== Kdesk Configuration file parameters

The following keywords allow for extra functionalities in kDesk itself.

 * ScreenSaverTimeout - Time in seconds of user idle time before the screen saver should start. The value 0 disables the screen saver.
 * ScreenSaverProgram - The path to a program to be started as the screen saver. The program should itself watch for further user input and exit.

 * EnableSound - If this flag is set to true, sounds will be enabled
 * SoundWelcome - Path to an mp3 sound file to be played on kdesk startup
 * SoundLLaunchApp - Path to an mp3 sound file to be played when the user double clicks on a disabled icon
 * SoundDisabledIcon - Path to an mp3 sound file to be played when the user double clicks and starts an app

==== Icons Configuration file parameters

The following keywords allow for additional functionalities to each icon on the desktop

 * Singleton - When this flag is set to true, kdesk will only allow a single running instance of this app
 * AppID - Unique pattern to the icon program command line used to decide if the application is running for Singleton icons

==== Kdesk-eglsaver Screen saver

This app is Kdesk's default screen saver. It renders three bitmaps on a black background screen which are rotated
on a 3D effect in slow motion. It uses the native RaspberryPI Opengl GPU features thus it looks very comfortable to
the user, consumes low CPU power and completely hides applications which would draw on the framebuffer, as is the
case with Minecraft and omxplayer used in Kano-Video.

If you want to develop your own screen saver, please read the file src/kdesk-eglsaver/README.md

==== kfbsaver

Kfbsaver is a framebuffer based screen saver showing a fixed color on top of the desktop.
It was developed as a first approach for integrating into Kanus but it is no optimally fitted as it causes
flickering effects with framebuffer based apps like Minecraft and omxplayer. It is not being built / packaged.

You should use kdesk-eglsaver instead.

=== Building kdesk and Kdesk-eglsaver

From the base directory you can invoke make to build a release version:

 $ make

Alternatively, if you prefer to work on further development you should build a debug version:

 $ make debug

The debug version will inline debug statements across all major operations to easily follow up the code.
The debug statements are defined as inlined code, which means they are completely removed on release versions,
hence compacting the binary and speeding up the work.

You can easily tell if you are running a debug build because the desktop icons will show a narrow black border.
To build  the debian package just execute the following:

 $ debuild -us -uc

The binaries are self-contained in 2 files: kdesk and kdesk-eglsaver. Their runtime and build dependencies
are defined in the file debian/control. KanuxOS already comes with the run time dependencies for you.

KDesk plays well under Kanux and also on Intel. You can run it on top of a VNC session using Fluxbox or other
window managers during testing / development.
