//
// desktop.h
//
// Copyright (C) 2013-2014 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
// An app to show and bring life to Kano-Make Desktop Icons.
//

//
// TODO: Always keep an eye on libsn upgrades.
// This API library is not officially conformed across different architectures
// See the file:  /usr/include/startup-notification-1.0/libsn/sn-common.h for details
//
#define SN_API_NOT_YET_FROZEN
#include <libsn/sn.h>

#define KDESK_CONTROL_WINDOW_NAME "KdeskControlWindow"
#define KDESK_SIGNAL_FINISH       "KSIG_FINISH"
#define KDESK_SIGNAL_RELOAD       "KSIG_RELOAD"
#define KDESK_SIGNAL_ICON_ALERT   "KSIG_ICON_ALERT"
#define KDESK_BLUR_DESKTOP        "KSIG_BLUR_DESKTOP"

class IconGrid;

class Desktop
{
 private:
  Window wcontrol;
  Background *pbground;
  bool initialized;
  std::map <Window, Icon *> iconHandlers;
  IconGrid *icon_grid;
  SnDisplay *sn_display;
  SnLauncherContext *sn_context;
  Configuration *pconf;
  Sound *psound;
  bool finish;
  static int error_trap_depth;
  int numicons;
  Atom atom_finish, atom_reload, atom_icon_alert, atom_blur;

 public:
  Desktop(void);
  virtual ~Desktop(void);

  void initialize(Background *p);
  bool create_icons (Display *display);
  Icon *find_icon_name (char *icon_name);
  bool destroy_icons (Display *display);

  bool notify_startup_load (Display *display, int iconid, Time time);
  bool notify_startup_ready (Display *display);
  void notify_startup_event (Display *display, XEvent *pev);

  bool initialize(Display *display, Configuration *loaded_conf, Sound *psound);
  bool is_kdesk_running (Display *display);
  Window find_kdesk_control_window (Display *display);
  bool process_and_dispatch(Display *display);
  bool send_signal (Display *display, const char *signalName, char *message);
  bool call_icon_hook (Display *display, XEvent ev, std::string hookscript, Icon *pico_hook);
  bool finalize(void);
};
