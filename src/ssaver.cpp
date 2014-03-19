//
// ssaver.cpp  -  Class to detect user idle time and fire a screen saver.
//
// Copyright (C) 2013-2014 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
// An app to show and bring life to Kano-Make Desktop Icons.
//

#include "X11/extensions/scrnsaver.h"

#include <pthread.h>
#include <stdlib.h>

#include "logging.h"
#include "ssaver.h"

bool setup_ssaver (KSAVER_DATA *kdata)
{
  pthread_t *tssaver = new pthread_t();
  pthread_create (tssaver, NULL, &idle_time, (void *) kdata);
  log1 ("SSaver thread started tid", tssaver);
  return true;
}

Window create_top_window(Display *display)
{
  // This function will create a topmost window to hide dynamic windows on the desktop
  // FIXME: There is a bug in this function and the window is not brought to display.
  // Once fixed this will remove the flickering effect of the CPU plugin monitor

  int screen_num = DefaultScreen(display);
  int w = DisplayWidth(display, screen_num);
  int h = DisplayHeight(display, screen_num);
  int border=5;
  XSetWindowAttributes attr;

  attr.background_pixmap = ParentRelative;
  attr.backing_store = Always;
  attr.event_mask = ExposureMask | EnterWindowMask | LeaveWindowMask;
  attr.override_redirect = True;
  Window win = XCreateWindow (display, DefaultRootWindow(display), 0, 0, w, h, border,
  			      CopyFromParent, CopyFromParent, CopyFromParent,
  			      CWBackPixmap|CWBackingStore|CWOverrideRedirect|CWEventMask,
  			      &attr);

  XFillRectangle (display, win, DefaultGC (display, screen_num), 0, 0, w, h);
  XDrawString (display, win, DefaultGC (display, screen_num), 50, 300, "ALBERT", 6);

  XSelectInput(display, win, ButtonPressMask | ButtonReleaseMask | PointerMotionMask | ExposureMask | EnterWindowMask | LeaveWindowMask);
  XMapWindow (display, win);
  XRaiseWindow (display, win);

  log1 ("Top window created: win", win);
  return win;
}

void destroy_top_window(Display *display, Window win)
{
  XDestroyWindow (display, win);
}

void *idle_time (void *p)
{
  bool running=true;
  Status rc=0;
  unsigned long ms=1000 * POLL_INTERVAL;
  unsigned long ultimeout=0L;
  PKSAVER_DATA pdata=(PKSAVER_DATA) p;
  XScreenSaverInfo *info = XScreenSaverAllocInfo();

  Display *display = XOpenDisplay(pdata->display_name);
  if (!display) {
    log ("Ssaver cannot connect to X Display! No screen saver available");
    return NULL;
  }
  else {
    log2 ("Setting screen saver - T/O (secs) and program", pdata->idle_timeout, pdata->saver_program);
  }

  while (running)
    {
      log1 ("asking for system idle time on display", display);
      rc = XScreenSaverQueryInfo(display, DefaultRootWindow(display), info);
      if (rc)
	{
	  // If idle timeout expires, launch the screen saver
	  if (info->idle > (pdata->idle_timeout * 1000)) {
	    log2 ("Starting the Screen Saver (idle, timeout)", info->idle, pdata->idle_timeout * 1000);

	    Window win = create_top_window(display);
	    rc = system (pdata->saver_program);
	    destroy_top_window(display, win);

	    log2 ("Screen saver finished - calling xrefresh: rc, cmdline", rc, pdata->saver_program);
	    rc = system (XREFRESH);
	  }
	  else {
	    log2 ("XScreenSaverQueryInfo rc - idle ms, not screen saver yet...", rc, info->idle);
	  }
	}
      else {
	log1 ("XScreenSaverQueryInfo failed with rc", rc);
      }
      
      usleep(ms);
    }

  XFree(info);
  return NULL;
}
