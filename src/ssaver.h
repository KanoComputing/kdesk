//
// ssaver.h -  Class to detect user idle time and fire a screen saver.
//
// Copyright (C) 2013-2014 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
// An app to show and bring life to Kano-Make Desktop Icons.
//

#define POLL_INTERVAL    15000           // milliseconds between each system idle query
#define XREFRESH         "xrefresh"      // called after the screen saver to redraw the desktop
#define TTY_QUERY        "/dev/tty1"     // name of the tty device to use as a trampoline to know who has the focus
#define GUI_TTY_DEVICE   7               // tty device number where the GUI XServer is running, normally tty7

typedef struct _ksaver_data {

  const char *display_name;           // Usually this can be set to NULL to attach to first available display
  unsigned long idle_timeout;         // seconds to idle before starting the screen saver
  const char *saver_program;          // path to binary program that paints the screen saver

} KSAVER_DATA;

typedef KSAVER_DATA* PKSAVER_DATA;

bool setup_ssaver (KSAVER_DATA *kdata);
void *idle_time (void *p);
