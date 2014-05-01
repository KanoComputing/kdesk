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

class Desktop
{
 private:
  Window wcontrol;
  std::map <Window, Icon *> iconHandlers;
  SnDisplay *sn_display;
  SnLauncherContext *sn_context;
  Configuration *pconf;
  Sound *psound;
  bool finish;
  static int error_trap_depth;
  int numicons;
  Atom atom_finish, atom_reload, atom_icon_alert;

 public:
  Desktop(void);
  virtual ~Desktop(void);

  bool create_icons (Display *display);
  bool destroy_icons (Display *display);

  bool notify_startup_load (Display *display, int iconid, Time time);
  bool notify_startup_ready (Display *display);
  void notify_startup_event (Display *display, XEvent *pev);

  bool initialize(Display *display, Configuration *loaded_conf, Sound *psound);
  bool process_and_dispatch(Display *display);
  bool send_signal (Display *display, const char *signalName, char *message);
  bool finalize(void);
};
