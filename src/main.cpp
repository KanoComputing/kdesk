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

int main(int argc, char *argv[])
{
  Status rc;
  Display *display;
  Configuration conf;
  char *display_name = NULL;
  string strKdeskRC, strHomeKdeskRC, strKdeskDir, strKdeskUser;
  bool test_mode = false, wallpaper_mode = false;

  cout << "Kano-Desktop - A desktop Icon Manager" << endl;
  cout << "Version v" << VERSION << endl << endl;

  int c;
  while ((c = getopt(argc, argv, "htw")) != EOF)
    {
      switch (c)
        {
	case 'h':
	  cout << "kano-desktop [ -t | -h | -w ]" << endl;
	  cout << " -t test mode, read configuration files and exit"<< endl;
	  cout << " -w set desktop wallpaper and exit"<< endl;
	  cout << " -h help, this screen" << endl << endl;
	  exit (1);

	case 't':
	  cout << "testing configuration" << endl;
	  test_mode = true;
	  break;

	case 'w':
	  cout << "setting wallpaper mode" << endl;
	  wallpaper_mode = true;
	  break;

	case '?':
	  exit(1);
        }
    }

  // Load configuration settings from user's home director
  cout << "initializing..." << endl;
  struct passwd *pw = getpwuid(getuid());
  const char *homedir = pw->pw_dir;
  strKdeskRC     = FILE_KDESKRC;
  strHomeKdeskRC = homedir + string("/") + string(FILE_HOME_KDESKRC);
  strKdeskDir    = DIR_KDESKTOP;
  strKdeskUser   = homedir + string(DIR_KDESKTOP_USER);

  log1 ("loading home configuration file", strHomeKdeskRC.c_str());
  if (!conf.load_conf(strHomeKdeskRC.c_str())) {
    log1 ("loading generic configuration file", strKdeskRC.c_str());
    if (!conf.load_conf(strKdeskRC.c_str())) {
      cout << "error reading configuration file" << endl;
      exit(1);
    }
  }

  log1 ("loading icons from directory", strKdeskDir.c_str());
  conf.load_icons(strKdeskDir.c_str());
  conf.dump();

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

  // Create and draw desktop icons, then attend user interaction  
  Desktop dsk(&conf, &ksound);
  bool bicons = dsk.create_icons(display);
  log1 ("desktop icons created", (bicons == true ? "successfully" : "errors found"));

  cout << "processing events..." << endl;
  dsk.process_and_dispatch(display);
  
  cout << "finishing..." << endl;
  exit (0);
}
