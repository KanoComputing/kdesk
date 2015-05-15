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
#include <stdio.h>
#include <stdlib.h>

#include "logging.h"
#include "ssaver.h"

#include <sys/wait.h>

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

int execute_hook(const char *hook_script, const char *params)
{
  int rc=-1;
  size_t cmdline_bytes=1024 * sizeof(char);
  char *chcmdline = (char *) calloc (1, cmdline_bytes);

  // Set environment variable so other programs called from script
  // cannot accidentally create an infinite loop.
  const char *norec = "export KDESK_NO_RECURSE=1";

  if (chcmdline != NULL && hook_script != NULL) {
    snprintf (chcmdline, cmdline_bytes, "/bin/bash -c \"%s; %s %s\"",
	      norec,
	      hook_script,
	      (params ? params : ""));
    log1 ("Executing screen saver hook:", chcmdline);

    signal(SIGCHLD, SIG_DFL);
    rc = system (chcmdline);
    signal(SIGCHLD, SIG_IGN);

    log2 ("Screen saver hook returns with RC, WEXITSTATUS", rc, WEXITSTATUS(rc));
    if (rc != -1) {
      rc = WEXITSTATUS(rc);
    }
  }

  if (chcmdline) {
      free (chcmdline);
  }

  return rc;
}

int hook_ssaver_start(const char *hook_script)
{
  int rc = execute_hook (hook_script, SSAVER_HOOK_START);
  if (rc == -1) {
    // If the hook script cannot be executed, assume success, go forward
    log ("Warning: Screen saver start hook returns -1, assuming 0=success");
    rc = 0;
  }

  return rc;
}

int hook_ssaver_finish(const char *hook_script, time_t time_ssaver_run)
{
  char hook_params[256];

  sprintf (hook_params, "%s %ld", SSAVER_HOOK_FINISH, time_ssaver_run);
  int rc = execute_hook (hook_script, hook_params);
  return 0;
}

bool setup_ssaver (KSAVER_DATA *kdata)
{
  pthread_t *tssaver = new pthread_t();
  pthread_create (tssaver, NULL, &idle_time, (void *) kdata);
  log1 ("SSaver thread started tid", tssaver);
  return true;
}

void fake_user_input (Display *display)
{
  // Send a fake mouse movement so that the XServer restarts the inactivity timer
  Window root = DefaultRootWindow(display);

  // From the docs: If dest_w is None, XWarpPointer() moves the pointer by the offsets
  // (dest_x, dest_y) relative to the current position of the pointer
  XWarpPointer(display, root, 0, // Display, source window, destination window
               0, 0, 0, 0,       // source x,y and width, height
               1, 1);            // destination x,y

  XFlush(display);
}

void *idle_time (void *p)
{
  bool running=true;
  Status rc=0, rchook=0;
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
  if (!info) {
    log ("Error! Could not allocate screen saver information structure, screen saver disabled");
    return NULL;
  }

  while (running)
    {
      rc = XScreenSaverQueryInfo(display, DefaultRootWindow(display), info);
      log3 ("asking for system idle time - rcsuccess, T/O, and idle time in secs", rc, info->idle / 1000, pdata->idle_timeout);
      if (rc)
	{
	  // If idle timeout expires, and focus is on the GUI tty device, then launch the screen saver
	  if (info->idle > (pdata->idle_timeout * 1000) && get_current_console() == GUI_TTY_DEVICE) {
	    rchook = hook_ssaver_start(pdata->saver_hooks);
	    if (rchook == 0) {
	      log2 ("Starting the Screen Saver (idle, timeout in secs)", info->idle / 1000, pdata->idle_timeout);

              time_t ssaver_time_start = time (NULL);
              signal(SIGCHLD, SIG_DFL);
              rc = system (pdata->saver_program);
              signal(SIGCHLD, SIG_IGN);
              time_t ssaver_time_end = time (NULL);

              log1 ("Screen saver finished with rc", rc);
	      if (rc == 0) {
		log1 ("Calling xrefresh: ", XREFRESH);
		rc = system (XREFRESH);

		// Tell kdesk hooks that the screen saver has finished
		rchook = hook_ssaver_finish(pdata->saver_hooks, ssaver_time_end - ssaver_time_start);

		// some bluetooth keyboard devices need an explicit activity event,
		// otherwise the inactivity timer stops working (returning 0 which means activity is being received).
		fake_user_input(display);
	      }
	    }
	    else {
	      log1 ("Screen saver start hook not returning 0, cancelling the screen saver, rc=", rchook);
	      fake_user_input(display);
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
