//
// kdesk-hourglass.cpp  -  Provide a system wide library for a mouse hourglass
//
// Copyright (C) 2013-2014 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
// A shared library to provide apps with a desktop app loading mouse hourglass.
//

#define SN_API_NOT_YET_FROZEN
#include <libsn/sn.h>

#include "sn_callbacks.cpp"

#include <stdio.h>
#include <string.h>
#include "kdesk-hourglass.h"

// Global variables
SnDisplay *sn_display=NULL;
SnLauncherContext *sn_context=NULL;
Display *display=NULL;

// local private prototypes
void _start_launcher_(char *appname, char *cmdline);

void kdesk_hourglass_start(char *appname)
{
    // Use this API when you know the exact name of the app
    // in the X11 namespace (execute xwinfinfo and find the WINDOW_NAME tuple attributes)
    _start_launcher_(appname, NULL);
}

void kdesk_hourglass_start_appcmd(char *cmdline)
{
    // Use this API when you only have the command line of your app

    // We will strip off the cmdline full path,
    // beacause sn_notify wants to match against the program binary name only.
    char *jump_slash=strstr(cmdline, "/");
    while (jump_slash != NULL) {
        cmdline=jump_slash + sizeof(char);
        jump_slash=strstr(cmdline, "/");
    }
    
    _start_launcher_(NULL, cmdline);
}

void kdesk_hourglass_end()
{
    // unbind from startup notify
    if (sn_context) {
        sn_launcher_context_complete(sn_context);
    }
}

void _start_launcher_(char *appname, char *cmdline)
{
    Time xlib_time=0L;
  
    // sn_notify cannot cope with appname being NULL, but he likes it as an empty string.
    if (!appname) {
        appname=(char*) "";
    }

    // bind to the startup notify library
    if (!sn_display) {
        sn_display = sn_display_new (display, error_trap_push, error_trap_pop);
    }
  
    if (!sn_context) {
        sn_context = sn_launcher_context_new (sn_display, DefaultScreen (display));
    }
  
    sn_launcher_context_set_name (sn_context, appname);
    sn_launcher_context_set_description (sn_context, appname);
    sn_launcher_context_set_binary_name (sn_context, cmdline ? cmdline : appname);
    sn_launcher_context_set_icon_name(sn_context, appname);

    // We are not interested on the launcher-launchee relationship at this point
    sn_launcher_context_initiate (sn_context, "launcher", "launchee", xlib_time);
    sn_launcher_context_setup_child_process (sn_context);
}

// Library constructor
void __attribute__ ((constructor)) initialize(void)
{
    display=XOpenDisplay(NULL);
    if (!display) {
        return;
    }

    sn_display = sn_display_new (display, error_trap_push, error_trap_pop);
    sn_context = sn_launcher_context_new (sn_display, DefaultScreen (display));
}

// Library destructor
void __attribute__ ((destructor)) finalize(void)
{
    if (sn_context) {
        sn_launcher_context_unref(sn_context);
        sn_context=NULL;
    }
    
    if (display) {
        XCloseDisplay(display);
        display=0L;
        sn_display=0L;
    }
}
