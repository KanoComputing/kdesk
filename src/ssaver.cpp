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

#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/vt.h>
#include <unistd.h>

// Returns the currently active tty on the local system
// If 0 is returned this info could not be obtained
int get_current_console (void)
{
  int current_tty = 0;
  struct vt_stat vtstat = {0,0,0};

  int fd = open (TTY_QUERY, O_RDONLY);
  int rc = ioctl (fd, VT_GETSTATE, &vtstat);
  close(fd);
  if (rc >= 0) {
    current_tty = vtstat.v_active;
  }
  return current_tty;
}

bool setup_ssaver (KSAVER_DATA *kdata)
{
  pthread_t *tssaver = new pthread_t();
  pthread_create (tssaver, NULL, &idle_time, (void *) kdata);
  log1 ("SSaver thread started tid", tssaver);
  return true;
}

void *idle_time (void *p)
{
  bool running=true;
  Status rc=0;
  unsigned long ms=1000 * POLL_INTERVAL;
  unsigned long ultimeout=0L;
  PKSAVER_DATA pdata=(PKSAVER_DATA) p;

  // Initial X11 connection delay
  usleep(ms);

  Display *display = XOpenDisplay(pdata->display_name);
  if (!display) {
    log ("Ssaver cannot connect to X Display! No screen saver available");
    return NULL;
  }
  else {
    log2 ("Setting screen saver - T/O (secs) and program", pdata->idle_timeout, pdata->saver_program);
  }

  XScreenSaverInfo *info = XScreenSaverAllocInfo();
  while (running)
    {
      rc = XScreenSaverQueryInfo(display, DefaultRootWindow(display), info);
      log3 ("asking for system idle time - rcsuccess, T/O, and idle time in secs", rc, info->idle / 1000, pdata->idle_timeout);
      if (rc)
	{
	  // If idle timeout expires, and focus is on the GUI tty device, then launch the screen saver
	  if (info->idle > (pdata->idle_timeout * 1000) && get_current_console() == GUI_TTY_DEVICE) {
	    log2 ("Starting the Screen Saver (idle, timeout in secs)", info->idle / 1000, pdata->idle_timeout);
	    rc = system (pdata->saver_program);
	    log1 ("Screen saver finished with rc", rc);
	    if (rc == 0) {
	      log1 ("Calling xrefresh: ", XREFRESH);
	      rc = system (XREFRESH);
	    }
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
