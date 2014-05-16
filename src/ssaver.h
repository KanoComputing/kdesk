//
// ssaver.h -  Class to detect user idle time and fire a screen saver.
//
// Copyright (C) 2013-2014 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
// An app to show and bring life to Kano-Make Desktop Icons.
//

#define POLL_INTERVAL      15000                // milliseconds between each system idle query
#define XREFRESH           "xrefresh"           // called after the screen saver to redraw the desktop
#define TTY_QUERY          "/dev/tty1"          // name of the tty device to use as a trampoline to know who has the focus
#define SSAVER_HOOK_START  "ScreenSaverStart"   // First parameter name sent to hooks when the screen saver is about to start
#define SSAVER_HOOK_FINISH "ScreenSaverFinish"  // First parameter name sent to hooks when screen saver terminates

#define GUI_TTY_DEVICE   7               // tty device number where the GUI XServer is running, normally tty7

typedef struct _ksaver_data {

  const char *display_name;           // Usually this can be set to NULL to attach to first available display
  unsigned long idle_timeout;         // seconds to idle before starting the screen saver
  const char *saver_program;          // path to binary program that paints the screen saver
  const char *saver_hooks;            // path to a hook script that will be executed to alert on screen saver transitions (start, finish)

} KSAVER_DATA;

typedef KSAVER_DATA* PKSAVER_DATA;

bool setup_ssaver (KSAVER_DATA *kdata);
void *idle_time (void *p);
int hook_ssaver_start(const char *hook_script);
int hook_ssaver_finish(const char *hook_script);
void fake_user_input (void);
