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

#include <X11/extensions/XShm.h>

#include "version.h"
#include "main.h"
#include "icon.h"
#include "background.h"
#include "sound.h"
#include "desktop.h"
#include "logging.h"
#include "ssaver.h"


// A printf macro sensitive to the -v (verbose) flag
// Use kprintf for regular stdout messages instead of printf or cout
bool verbose=false; // Kdesk is mute by default, no stdout messages unless in Debug build
#define kprintf(fmt, ...) ( (verbose==false ? : printf(fmt, ##__VA_ARGS__) ))

// The desktop object will be dynamically accessed to refresh the configuration
static Desktop dsk;

// Flag to enable imlib2 to use the MIT-SHM X11 extension, see command line options.
// Disabled by default to fix black areas on Debian Stretch
bool enable_shm=false;

// local function prototypes
void signal_callback_handler(int signum);
void reload_configuration (Display *display);
void reload_icons (Display *display);
void trigger_icon_hook (Display *display, char *message);
void finish_kdesk (Display *display);

Display *get_current_display(void);
void close_display(Display *display);



// Signal handler reached via a kill -USR
void signal_callback_handler(int signum)
{
  Display *display=get_current_display();
  if (!display) {
    return;
  }

  log1 ("Received signal", signum);
  if (signum == SIGHUP) {
    log ("Received signal to reload configuration");
    reload_configuration(display);
  }
  else if (signum == SIGUSR1) {
    log ("Received graceful kdesk quit signal");
    finish_kdesk(display);
  }
  
  close_display(display);
  return;
}

void reload_configuration (Display *display)
{
  log ("Reloading kdesk configuration");
  dsk.send_signal (display, KDESK_SIGNAL_RELOAD, NULL);
  return;
}

void reload_icons (Display *display)
{
  log ("Reloading kdesk configuration");
  dsk.send_signal (display, KDESK_SIGNAL_RELOAD_ICONS, NULL);
  return;
}

// Prints a json string with icon coordinate details. returns false if not found
bool print_json_icon_placement(Display *display, char *icon_name)
{
  bool found=false;
  Window returnedroot=0L, returnedparent=0L, *children=0L, root=DefaultRootWindow(display);
  char *windowname=NULL;
  unsigned int numchildren=0;

  if (!icon_name) {
    return false;
  }

  XQueryTree (display, root, &returnedroot, &returnedparent, &children, &numchildren);
  for (int i=numchildren-1; i>=0; i--)
    {
      windowname = NULL;
      if (XFetchName (display, children[i], &windowname)) {

	// kdesk icon names have a prefix so they are unique
	string search_name=string("kdesk-");
	search_name += icon_name;
	if (!strcmp (windowname, search_name.c_str())) {
	  XWindowAttributes xwa;
	  memset(&xwa, 0x00, sizeof(xwa));
	  if (XGetWindowAttributes(display, children[i], &xwa)) {
	    printf ("{ \"icon_name\": \"%s\", \"x\": %d, \"y\": %d, "
		    "\"width\": %d, \"height\": %d }\n",
		    icon_name, xwa.x, xwa.y, xwa.width, xwa.height);
	    XFree(windowname);
	    found=true;
	    break; // stop searching
	  }
	}

	XFree(windowname);
      }
    }

  if(numchildren) {
    XFree(children);
  }

  return found;
}

void trigger_icon_hook (Display *display, char *message)
{
  log ("Trigger an icon hook signal");
  dsk.send_signal (display, KDESK_SIGNAL_ICON_ALERT, message);
  return;
}

void finish_kdesk (Display *display)
{
  log ("Finishing Kdesk");
  dsk.send_signal (display, KDESK_SIGNAL_FINISH, NULL);
  return;
}

Display *get_current_display(void)
{
    // Get a handle to the display bound to the current user session
    return XOpenDisplay(NULL);
}

void close_display(Display *display)
{
    XCloseDisplay(display);
}


/*
 * XShmQueryExtension
 *
 * This is an instrumented function that replaces the official API
 * from the libXext library - X11 extensions. It allows to disable this extension for kdesk.
 *
 * On Debian Stretch, using the MIT-SHM extension from imlib2 produces unfortunate
 * graphics artifacts that break the rendering with black areas.
 *
 */
int XShmQueryExtension(Display *d)
{
    return (enable_shm);
}


int main(int argc, char *argv[])
{
  Status rc;
  Display *display=get_current_display();
  Configuration conf;
  KSAVER_DATA ksaver_data;
  string strKdeskRC, strHomeKdeskRC, strKdeskDir, strKdeskUser;
  string configuration_file;
  bool test_mode = false, wallpaper_mode = false, screen_saver_mode = false;
  bool reload = false, running=true;
  int c;


  // Collect command-line parameters
  while ((c = getopt(argc, argv, "?htwsc:ria:vqmj:")) != EOF)
    {
      switch (c)
        {
	case '?':
	case 'h':
	  cout << "kano-desktop [ -h | -t | -w | -r | -a <icon name> | -q ]" << endl;
	  cout << " -h help, or -? this screen" << endl;
	  cout << " -v verbose mode with minimal progress messages" << endl;
	  cout << " -t test mode, read configuration files and exit"<< endl;
	  cout << " -w set desktop wallpaper and exit" << endl;
	  cout << " -s screen saver mode - sets wallpaper, no icons, screen saver hooks" << endl;
	  cout << " -c <kdeskrc filename> use a custom configuration file"<< endl;
	  cout << " -r refresh configuration and exit" << endl;
	  cout << " -i refresh desktop icons only and exit" << endl;
	  cout << " -q query if kdesk is running on the current desktop (rc 0 running, nonzero otherwise)" << endl;
	  cout << " -a <icon name> send an icon hook alert" << endl;
	  cout << " -m enable the use of MIT-SHM XServer extension (default=" << enable_shm << ")" << endl;
	  cout << " -j <icon name> get a json dump of the icon position on the desktop" << endl << endl;
	  exit (1);

	case 't':
	  kprintf ("testing configuration\n");
	  test_mode = true;
	  break;

	case 'v':
	  verbose = true;
	  break;

	case 'w':
	  wallpaper_mode = true;
	  break;

        case 's':
          screen_saver_mode = true;
          break;

	case 'c':
	  configuration_file = optarg;
          if (access(configuration_file.c_str(), R_OK)) {
              kprintf("Could not find configuration file: %s\n", configuration_file.c_str());
              exit(1);
          }
	  break;

	case 'r':
	  if (!display) {
	    kprintf ("Could not connect to the XServer\n");
	    kprintf ("Is the DISPLAY variable correctly set?\n\n");
	    exit (1);
	  }

	  kprintf ("Sending a refresh signal to Kdesk\n");
	  reload_configuration(display);
	  close_display(display);
	  exit (0);

	case 'i':
	  if (!display) {
	    kprintf ("Could not connect to the XServer\n");
	    kprintf ("Is the DISPLAY variable correctly set?\n\n");
	    exit (1);
	  }

	  kprintf ("Sending an icon refresh signal to Kdesk\n");
	  reload_icons(display);
	  close_display(display);
	  exit (0);

	case 'a':
	  if (!display) {
	    kprintf ("Could not connect to the XServer\n");
	    kprintf ("Is the DISPLAY variable correctly set?\n\n");
	    exit (1);
	  }

	  kprintf ("Triggering icon hook with message: %s\n\n", optarg);
	  trigger_icon_hook(display, optarg);
	  close_display(display);
	  exit (0);

	case 'j':
	  if (!display) {
	    kprintf ("Could not connect to the XServer\n");
	    kprintf ("Is the DISPLAY variable correctly set?\n\n");
	    exit (1);
	  }

	  kprintf ("Querying information for icon name: %s\n\n", optarg);
	  if(print_json_icon_placement(display, optarg) == true) {
	    exit(0);
	  }
	  exit(1);

        case 'm':
            kprintf ("Enabling use of the MIT-SHM Xserver extension\n");
            enable_shm=true;
            break;

	case 'q':
	  if (!display) {
	    kprintf ("Could not connect to the XServer\n");
	    kprintf ("Is the DISPLAY variable correctly set?\n\n");
	    exit (1);
	  }
	  else {
	    //dsk.initialize (&bg);
	    if (dsk.find_kdesk_control_window (display)) {
	      kprintf ("Kdesk is running on this Display\n");
              close_display(display);
	      exit (0);
	    }
	    else {
	      kprintf ("Kdesk is not running on this Display\n");
              close_display(display);
	      exit (-1);
	    }
	  }
	}
    }
      
  kprintf ("Kano-Desktop - A desktop Icon Manager\n");
  kprintf ("Version v%s\n", VERSION);
  
  // We don't allow kdesk to run as the superuser
  uid_t userid = getuid();
  if (userid == 0) {
    kprintf ("kdesk cannot run as root, please use sudo or login as a regular user\n");
    exit(1);
  }

  // Load configuration settings from user's home directory
  kprintf ("initializing...\n");
  struct passwd *pw = getpwuid(getuid());
  const char *homedir = pw->pw_dir;
  bool bconf, buser = false;
  strKdeskRC     = FILE_KDESKRC;
  strHomeKdeskRC = homedir + string("/") + string(FILE_HOME_KDESKRC);
  strKdeskDir    = DIR_KDESKTOP;
  strKdeskUser   = homedir + string(DIR_KDESKTOP_USER);

  // Load a custom configuration
  if (configuration_file.length()) {
      kprintf ("loading custom configuration file: %s\n", configuration_file.c_str());
      bconf = conf.load_conf(configuration_file.c_str());
      if (!bconf) {
          kprintf ("could not read custom configuration settings\n");
          exit(1);
      }
  }
  else {
      // Load system wide configuration file from /usr/share
      // And override any settings provided by the user's home dir configuration
      kprintf ("loading generic configuration file: %s\n", strKdeskRC.c_str());
      bconf = conf.load_conf(strKdeskRC.c_str());
  }

  // combine loaded configuration with custom settings located in the users HOME dir
  kprintf ("overriding settings with home configuration file: %s\n", strHomeKdeskRC.c_str());
  buser = conf.load_conf(strHomeKdeskRC.c_str());
  if (bconf == false && buser == false) {
      kprintf ("could not read generic or user configuration settings\n");
      exit(1);
  }

  log1 ("loading icons from directory", strKdeskDir.c_str());
  conf.load_icons(strKdeskDir.c_str());
  if (test_mode == true) {
    kprintf ("configuration test mode - exiting\n");
    exit(0);
  }

  if (conf.get_numicons() == 0) {
    log ("Warning: no icons have been loaded");
  }

  // Kdesk is a multithreaded X app
  rc = XInitThreads();
  log1 ("XInitThreads rc", rc);

  // Connect to the X Server
  if (!display) {
    char *env_display = getenv ("DISPLAY");
    kprintf ("could not connect to X display\n");
    kprintf ("DISPLAY=%s\n", (env_display ? env_display : "null"));
    exit (1);
  }
  else {
    kprintf ("Connected to display %s\n", DisplayString(display));
  }

  // Create and draw the desktop background
  Background bg(&conf);
  bg.setup(display);
  bg.load(display);

  // If wallpaper mode requested, exit now.
  if (wallpaper_mode == true) {
    kprintf ("refreshing background and exiting\n");
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
    kprintf ("This allocated display is not primary, disabling sound (%s)\n", DEFAULT_DISPLAY);
  }

  // Initialize the desktop management class,
  // stop here if there is already a Kdesk running on this display
  dsk.initialize (&bg);
  if (dsk.find_kdesk_control_window (display)) {
    kprintf ("Kdesk is already running on this Desktop - exiting\n");
    exit (1);
  }
  else {
    dsk.initialize(display, &conf, &ksound);
  }

  // Register signal handlers to provide for external wake-ups
  signal (SIGUSR1, signal_callback_handler);
  signal (SIGUSR2, signal_callback_handler);

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
      kprintf ("This XServer does not provide Screen Saver extensions - disabling\n");
    }
    else {
      memset(&ksaver_data, 0, sizeof(KSAVER_DATA));

      ksaver_data.display_name  = NULL;   // NULL means the currently attached display
      ksaver_data.idle_timeout  = conf.get_config_int("screensavertimeout");
      ksaver_data.saver_program = strdup(conf.get_config_string("screensaverprogram").c_str());
      ksaver_data.saver_hooks   = strdup(conf.get_config_string("iconhook").c_str());
      setup_ssaver (&ksaver_data);
    }

    // in screen saver mode, we only process the thread events, no icons loaded.
    if (screen_saver_mode == true) {
        kprintf ("starting kdesk in screen saver mode\n");
        do {
            reload = dsk.process_and_dispatch(display);
            // TODO: Reload configuration settings if ever needed

        }  while (reload == true);

        // terminate gracefully via user signal SIGUSR1
        kprintf ("terminating screensaver mode gracefully\n");
        free(ksaver_data.saver_program);
        free(ksaver_data.saver_hooks);
        exit(0);
    }
  }

  // Create and draw desktop icons, then attend user interaction
  bool bicons = dsk.create_icons(display);
  log1 ("desktop icons created", (bicons == true ? "successfully" : "errors found"));

  kprintf ("processing X11 events...\n");
  do {
    reload = dsk.process_and_dispatch(display);
    if (reload == true) {
      // Discard configuration and reload everything again
      conf.reset();      

      kprintf ("loading generic configuration file: %s\n", strKdeskRC.c_str());
      conf.load_conf(strKdeskRC.c_str());
      kprintf ("overriding home configuration file: %s\n", strHomeKdeskRC.c_str());
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
      // This means we don't want to refresh settings
      // TODO: We need to change this semantics the day we want a graceful kdesk stop
      running = true;
    }

  } while (running == true);

  // Free strings used to start screen saver
  if (ksaver_data.saver_program)
      free(ksaver_data.saver_program);
  if (ksaver_data.saver_hooks)
      free(ksaver_data.saver_hooks);

  kprintf ("kdesk is finishing...\n");
  exit (0);
}
