//
// main.cpp
//
// Copyright (C) 2013-2014 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
// An app to show and bring life to Kano-Make Desktop Icons.
//

#include <stdlib.h>
#include <getopt.h>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <signal.h>
#include <fcntl.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

#include "X11/extensions/scrnsaver.h"

#include "version.h"
#include "main.h"
#include "icon.h"
#include "background.h"
#include "sound.h"
#include "desktop.h"
#include "logging.h"
#include "ssaver.h"

// The desktop object will be dynamically accessed to refresh the configuration
static Desktop dsk;

// local function prototypes
void signal_callback_handler(int signum);
void reload_configuration (Display *display);
void finish_kdesk (Display *display);

// Signal handler reached via a kill -USR
void signal_callback_handler(int signum)
{
  Display *display=XOpenDisplay(NULL);
  if (!display) {
    return;
  }

  log1 ("Received signal", signum);
  if (signum == SIGUSR1) {
    cout << "Received signal to reload configuration" << endl;
    reload_configuration(display);
  }
  else if (signum == SIGUSR2) {
    cout << "Received custom signal" << endl;
    finish_kdesk(display);
  }
  
  XCloseDisplay (display);
  return;
}

void reload_configuration (Display *display)
{
  log ("Reloading kdesk configuration");
  dsk.send_signal (display, KDESK_SIGNAL_RELOAD);
  return;
}

void finish_kdesk (Display *display)
{
  log ("Finishing Kdesk");
  dsk.send_signal (display, KDESK_SIGNAL_FINISH);
  return;
}

int main(int argc, char *argv[])
{
  Status rc;
  Display *display;
  Configuration conf;
  char *display_name = NULL;
  string strKdeskRC, strHomeKdeskRC, strKdeskDir, strKdeskUser;
  bool test_mode = false, wallpaper_mode = false;
  int c;

  cout << "Kano-Desktop - A desktop Icon Manager" << endl;
  cout << "Version v" << VERSION << endl << endl;

  // We don't allow kdesk to run as the superuser
  uid_t userid = getuid();
  if (userid == 0) {
    cout << "kdesk cannot run as root, please use sudo or login is a regular user" << endl;
    exit(1);
  }

  // Collect command-line parameters
  while ((c = getopt(argc, argv, "?htwr")) != EOF)
    {
      switch (c)
        {
	case '?':
	case 'h':
	  cout << "kano-desktop [ -h | -t | -w | -r ]" << endl;
	  cout << " -h help, or -? this screen" << endl;
	  cout << " -t test mode, read configuration files and exit"<< endl;
	  cout << " -w set desktop wallpaper and exit" << endl;
	  cout << " -r refresh configuration and exit" << endl << endl;
	  exit (1);

	case 't':
	  cout << "testing configuration" << endl;
	  test_mode = true;
	  break;

	case 'w':
	  wallpaper_mode = true;
	  break;

	case 'r':
	  display = XOpenDisplay(display_name);
	  if (!display) {
	    cout << "Could not connect to the XServer" << endl;
	    cout << "Is the DISPLAY variable correctly set?" << endl << endl;
	    exit (1);
	  }

	  cout << "Sending a refresh signal to Kdesk" << endl;
	  reload_configuration(display);
	  XCloseDisplay(display);
	  exit (0);
        }
    }

  // Load configuration settings from user's home director
  cout << "initializing..." << endl;
  struct passwd *pw = getpwuid(getuid());
  const char *homedir = pw->pw_dir;
  bool bgeneric, buser = false;
  strKdeskRC     = FILE_KDESKRC;
  strHomeKdeskRC = homedir + string("/") + string(FILE_HOME_KDESKRC);
  strKdeskDir    = DIR_KDESKTOP;
  strKdeskUser   = homedir + string(DIR_KDESKTOP_USER);

  // Load configuration file from world settings (/usr/share)
  // And override any settings provided by the user's home dir configuration
  cout << "loading generic configuration file " << strKdeskRC.c_str() << endl;
  bgeneric = conf.load_conf(strKdeskRC.c_str());
  cout << "overriding settings with home configuration file " << strHomeKdeskRC.c_str() << endl;
  buser = conf.load_conf(strHomeKdeskRC.c_str());
  if (!bgeneric && !buser) {
    cout << "could not read generic or user configuration settings" << endl;
    exit(1);
  }

  log1 ("loading icons from directory", strKdeskDir.c_str());
  conf.load_icons(strKdeskDir.c_str());

  if (test_mode == true) {
    cout << "test mode - exiting" << endl;
    exit(0);
  }

  if (conf.get_numicons() == 0) {
    log ("Warning: no icons have been loaded");
  }

  // Kdesk is a multithreaded X app
  rc = XInitThreads();
  log1 ("XInitThreads rc", rc);

  // Connect to the X Server
  display = XOpenDisplay(display_name);
  if (!display) {
    char *env_display = getenv ("DISPLAY");
    cout << "could not connect to X display" << endl;
    cout << "DISPLAY=" << (env_display ? env_display : "null") << endl << endl;
    exit (1);
  }
  else {
    cout << "Connected to display: " << DisplayString(display) << endl;
  }

  // Create and draw the desktop background
  Background bg(&conf);
  bg.setup(display);
  bg.load(display);

  // If wallpaper mode requested, exit now.
  if (wallpaper_mode == true) {
    cout << "refreshing background and exiting" << endl;    
    bg.refresh_background(display);
    exit (0);
  }

  // Play sound once the background is displayed
  // Only if we are running on the first available display,
  // Otherwise disable sound - VNC remote sessions
  Sound ksound(&conf);
  if (!(strncmp (DisplayString(display), DEFAULT_DISPLAY, strlen (DEFAULT_DISPLAY)))) {
    ksound.init();
    ksound.play_sound("soundwelcome");
  }
  else {
    cout << "This allocated display is not primary, disabling sound" << DEFAULT_DISPLAY << endl;
  }      

  // Delay desktop startup if requested
  unsigned long startup_delay=0L;
  startup_delay = conf.get_config_int ("background.delay");
  if (startup_delay > 0) {
    log1 ("Delaying desktop startup for milliseconds", startup_delay);
    unsigned long ms=1000 * startup_delay;
    usleep(ms);   // 1000 microseconds in a millisecond.
  }

  // Create and draw desktop icons, then attend user interaction  
  dsk.initialize(display, &conf, &ksound);
  bool bicons = dsk.create_icons(display);
  log1 ("desktop icons created", (bicons == true ? "successfully" : "errors found"));

  // Starting screen saver thread
  if (conf.get_config_int("screensavertimeout") > 0) {

    int rc=0, event_base=0, error_base=0;
    rc = XScreenSaverQueryExtension (display, &event_base, &error_base);
    if (rc == 0) {
      cout << "This XServer does not provide Screen Saver extensions - disabling" << endl;
    }
    else {
      KSAVER_DATA ksaver_data;
      ksaver_data.display_name  = display_name;
      ksaver_data.idle_timeout  = conf.get_config_int("screensavertimeout");
      ksaver_data.saver_program = conf.get_config_string("screensaverprogram").c_str();
      setup_ssaver (&ksaver_data);
    }
  }

  // Register signal handlers to provide for external wake-ups
  signal (SIGUSR1, signal_callback_handler);
  signal (SIGUSR2, signal_callback_handler);

  bool reload = false, running=true;
  do {
    cout << "processing events..." << endl;
    reload = dsk.process_and_dispatch(display);
    if (reload == true) {
      // Discard configuration and reload everything again
      conf.reset();      

      cout << "loading generic configuration file " << strKdeskRC.c_str() << endl;
      conf.load_conf(strKdeskRC.c_str());
      cout << "overriding home configuration file " << strHomeKdeskRC.c_str() << endl;
      conf.load_conf(strHomeKdeskRC.c_str());

      log1 ("loading icons from directory", strKdeskDir.c_str());
      conf.load_icons(strKdeskDir.c_str());
      conf.dump();

      // Destroy all icons and recreate them from the new reloaded configuration
      dsk.destroy_icons(display);

      // Reload the desktop wallpaper
      bg.setup(display);
      bg.load(display);

      // Regenerate new icons
      bool bicons = dsk.create_icons(display);
    }
    else {
      cout << "Custom Signal - not yet implemented" << endl;
    }

  } while (running == true);

  cout << "kdesk is finishing..." << endl;
  exit (0);
}
