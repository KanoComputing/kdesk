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
#include "kdesk-hourglass.h"

// TODO: make sure the CDATA attributes guarantee a per-process instance
SnDisplay *sn_display=NULL;
SnLauncherContext *sn_context=NULL;
Display *display=NULL;

void kdesk_hourglass_start()
{
  Time xlib_time=0L;

  display=XOpenDisplay(NULL);
  if (!display) {
    return;
  }

  // bind to the startup notify library
  sn_display = sn_display_new (display, error_trap_push, error_trap_pop);
  sn_context = sn_launcher_context_new (sn_display, DefaultScreen (display));

  char *appname=(char *)"helloWorld";
  sn_launcher_context_set_name (sn_context, appname);
  sn_launcher_context_set_description (sn_context, appname);
  sn_launcher_context_set_binary_name (sn_context, appname);
  sn_launcher_context_set_icon_name(sn_context, appname);
  sn_launcher_context_initiate (sn_context, appname, appname, xlib_time);
  sn_launcher_context_setup_child_process (sn_context);
}

void kdesk_hourglass_end()
{
  // unbind from startup notify
  sn_launcher_context_complete (sn_context);
  sn_launcher_context_unref (sn_context);
}
