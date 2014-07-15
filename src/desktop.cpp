//
// desktop.cpp
//
// Copyright (C) 2013-2014 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
// This module is the backbone of the desktop icons presentation.
// It dispatches all user interaction events to the icons, is responsible for the reload,
// redraws the icons when necessary, and also sends user defined signals.
//

#include <X11/Xatom.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

#include <string.h>
#include <errno.h>

#include "sn_callbacks.cpp"

//
// TODO: Always keep an eye on libsn upgrades.
// This API library is not officially conformed across different architectures
// See the file:  /usr/include/startup-notification-1.0/libsn/sn-common.h for details
//
#define SN_API_NOT_YET_FROZEN
#include <libsn/sn.h>

#include <unistd.h>
#include <map>

#include "icon.h"
#include "sound.h"
#include "background.h"
#include "desktop.h"
#include "logging.h"
#include "grid.h"

Desktop::Desktop(void)
{
  atom_finish = atom_reload = atom_icon_alert = 0L;
  wcontrol = 0L;
  numicons = 0;
  initialized = false;
  icon_grid = NULL;
}

void Desktop::initialize(Background *p)
{
  pbground = p;
  initialized = true;
}

Desktop::~Desktop(void)
{
  // Free all allocated icon handlers in the map
  std::map <Window, Icon *>::iterator it;
  for (it=iconHandlers.begin(); it != iconHandlers.end(); ++it)
    {
      delete it->second;
    }

  // Detach from Startup Notification library
  if (sn_display) {
    sn_display_unref (sn_display);
    sn_display = 0;
  }

  // FIXME: Atoms are not meant to be freed, correct me if I'm wrong.

  if (icon_grid) {
    delete icon_grid;
  }
}

bool Desktop::create_icons (Display *display)
{
  int nicon=0;

  icon_grid = new IconGrid(display, pconf);

  // Create and draw all icons, save a mapping of their window IDs -> handlers
  // so that we can dispatch events to each one in turn later on.
  //
  // The icons are created in two passes. The first one creates only grid
  // icons with hints, so they get a priority over the other ones.
  for (int pass = 0; pass < 2; pass++)
   {
    for (nicon=0; nicon < pconf->get_numicons(); nicon++)
      {
        string pos, x, y;
	pos = pconf->get_icon_string(nicon, "relative-to");
	x = pconf->get_icon_string(nicon, "x");
	y = pconf->get_icon_string(nicon, "y");
	if (pos == "grid" && x != "auto" && y != "auto") {
	  /* Has hints, skip in the second pass */
	  if (pass == 1)
	    continue;
	}
	else {
	  /* No hints, skip in the first pass */
	  if (pass == 0)
	    continue;
	}

        Icon *pico = new Icon(pconf, nicon);
        Window wicon = pico->create(display, icon_grid);
        if (wicon) {
	  XEvent emptyev;
	  iconHandlers[wicon] = pico;
	  pico->draw(display, emptyev, false);

	  // Invoke the icon hook so it is refreshed immediately
	  // right after Kdesk startup and refresh signals
	  string hookscript = pconf->get_config_string("iconhook");
	  if (hookscript.length() > 0) {
	    call_icon_hook (display, emptyev, pconf->get_config_string("iconhook"), pico);
	  }

	  numicons++;
        }
        else {
	  log1 ("Warning: error creating icon", pconf->get_icon_string(nicon, "filename"));
	  delete pico;
        }
      }
   }

  // Setup a connection with startup notification library
  // error_trap_push, error_trap_pop ???
  sn_display = sn_display_new (display, error_trap_push, error_trap_pop);
  sn_context = sn_launcher_context_new (sn_display, DefaultScreen (display));

  // tell the outside world how the icon creation has completed
  dump_metrics(display);

  // returns true if at least one icon is available on the desktop
  log1 ("Desktop icon classes have been allocated", nicon);
  return (bool) (nicon > 0);
}

Icon *Desktop::find_icon_name (char *icon_name)
{
  // Search through the icon dispatcher table for the icon filename
  std::map <Window, Icon *>::iterator it;
  for (it=iconHandlers.begin(); it != iconHandlers.end(); ++it)
    {
      if (it->second != NULL) {
	if (!strcasecmp (icon_name, it->second->get_icon_name().c_str())) {
	  log2 ("Icon name found (name, instance)", icon_name, it->second);
	  return it->second;
	}
      }
    }

  return NULL;
}

bool Desktop::redraw_icons (Display *display, bool forceClear)
{
  // Search through the icon dispatcher table for the icon filename
  XEvent ev;
  std::map <Window, Icon *>::iterator it;
  int redraws=0;

  memset (&ev, 0x00, sizeof (ev));

  for (it=iconHandlers.begin(); it != iconHandlers.end(); ++it)
    {
      if (it->second != NULL) {
	it->second->draw (display, ev, forceClear);
	redraws++;
      }
    }

  return (redraws > 0);
}

bool Desktop::destroy_icons (Display *display)
{
  std::map <Window, Icon *>::iterator it;

  // Ask every icon to close, deallocate, and disappear from the desktop
  for (it=iconHandlers.begin(); it != iconHandlers.end(); ++it)
    {
      // The reason we check for icon handler validity here is because
      // during close time, the previous icons can still get notification
      // events which we are not dealing with anymore, they are defunct.
      // This is the case with hover effects when the mouse is over them during refresh.
      if (it->second) {
	it->second->destroy(display);
	delete it->second;
      }
      iconHandlers.erase(it);
    }

  // Then empty the list of icon handlers
  iconHandlers.clear();
  numicons = 0;
}

bool Desktop::notify_startup_load (Display *display, int iconid, Time time)
{
  bool bsuccess=false;
  string name = pconf->get_icon_string (iconid, "command");
  string command = pconf->get_icon_string (iconid, "command");

  if (!sn_context || sn_launcher_context_get_initiated (sn_context)) {
    sn_launcher_context_unref (sn_context);
    sn_context = sn_launcher_context_new (sn_display, DefaultScreen (display));
  }

  if (!sn_context) {
    log ("could not acquire a startup notification context - hourglass not working");
  }
  else {
    log1 ("startup notification for app", command);

    sn_launcher_context_set_name (sn_context, name.c_str());
    sn_launcher_context_set_description (sn_context, command.c_str());
    sn_launcher_context_set_binary_name (sn_context, command.c_str());
    sn_launcher_context_set_icon_name(sn_context, name.c_str());
    sn_launcher_context_initiate (sn_context, name.c_str(), command.c_str(), time);
    sn_launcher_context_setup_child_process (sn_context);
    bsuccess = true;
  }

  return bsuccess;
}

void Desktop::notify_startup_event (Display *display, XEvent *pev)
{
  if (sn_context) {
    sn_display_process_event (sn_display, pev);
  }
}

bool Desktop::notify_startup_ready (Display *display)
{
  if (sn_context) {
    sn_launcher_context_unref (sn_context);
    sn_context = 0;
  }
}

bool Desktop::process_and_dispatch(Display *display)
{
  // Process X11 events and dispatch them to each icon handler for processing
  static unsigned long last_click=0, last_dblclick=0;
  static bool bstarted = false;
  int iGrace = 500;
  XEvent ev;
  Window wtarget = 0;

  do
    {
      // This is the main X11 event processing loop
      XNextEvent(display, &ev);
      wtarget = ev.xany.window;

      // The libnotify library likes to know what's happening
      if (sn_display != NULL) {
	sn_display_process_event (sn_display, &ev);
      }

      // If the event is sent to Kdesk's object control window,
      // this means it's a special signal sent from external processes via kill SIG or an XSendEvent.
      // It will allow us to give UIX visual feedback on-the-fly, reload configuration, or other useful async use cases.
      if (wtarget == wcontrol)
	{
	  if (ev.type == ClientMessage) {
	    log2 ("Kdesk client message arriving to control window with atom", ev.type, ev.xclient.data.l[0]);
	    if ((Atom) ev.xclient.data.l[0] == atom_reload) {
	      log ("Kdesk object control window receives a RELOAD event");
	      return true; // true means do whatever you need to, and come back to process_and_dispatch, we are ready.
	    }
	    else if ((Atom) ev.xclient.data.l[0] == atom_finish) {
	      log ("Kdesk object control window receives a FINISH event");
	      return false; // false means quit kdesk
	    }
	    else if ((Atom) ev.xclient.data.l[0] == atom_icon_alert) {

	      // Kdesk Icon Hooks alert is managed here.
	      // This event comes with a message string (the icon name)
	      // The icon name can be 16 bytes maximum. This comes from 20 bytes for X11 CientMessage data buffer minus 4 bytes
	      // the actual Atom message ID.
	      // An Icon hook is fired so the icon look&feel can be dynamically changed.

	      // Is there a Kdesk Icon hook defined?
	      string iconhook_script = pconf->get_config_string("iconhook");
	      if (iconhook_script.length() == 0) {
		log ("No kdesk icon hook defined - ignoring alert");
	      }
	      else {
		// Collect the icon name from the X11 event to which the hook alert needs to be sent
		char alert_iconname[17];
		memset (alert_iconname, 0x00, sizeof (alert_iconname));
		memcpy (alert_iconname, &ev.xclient.data.l[1], 16);
		alert_iconname[16] = 0x00; // Truncate it - this is not a nullified string
		log1 ("Icon Hook signal received for icon", alert_iconname);

		// Is the icon name on the desktop? Can we send him a signal?
		Icon *pico_hook = find_icon_name (alert_iconname);
		if (!pico_hook) {
		  log ("Could not find this icon on the desktop, ignoring");
		}
		else {
		  log2 ("Calling icon hook script (script, icon name)", iconhook_script, alert_iconname);
		  call_icon_hook (display, ev, iconhook_script, pico_hook);
		}
	      }
	    }
	  }

	  XFlush (display);
	  continue;
	}

      // During Kdesk configuration refresh we might get events for now defunct icon windows
      if (iconHandlers[wtarget] == NULL) {
	XFlush (display);
	continue;
      }

      // All events directed to the desktop icons we have created are processed here.
      switch (ev.type)
	{
	case ButtonPress:

	  // Stop processing a.s.a.p. if the mouse button being pressed is not left or right.
	  // Button1=Left, Button2=Middle, Button3=Right, higher than those are mapped to mouse scroll wheel
	  //
	  //  http://tronche.com/gui/x/xlib/events/keyboard-pointer/keyboard-pointer.html
	  //
	  if (ev.xbutton.button != Button1 && ev.xbutton.button != Button3) {
	    log1 ("ButtonPress for unsupported button number - ignoring", ev.xbutton.button);
	    break;
	  }

	  // A double click event is defined by the time elapsed between 2 clicks: "clickdelay"
	  // And a grace time period to protect nested double clicks: "clickgrace"
	  // Note we are listening for both left and right mouse click events.
	  log3 ("ButtonPress event: window, time, last click", wtarget, ev.xbutton.time, last_click);
	  if (ev.xbutton.time - last_click < pconf->get_config_int("clickdelay") || pconf->get_config_string("oneclick") == "true")
	    {
	      // Protect the UI experience by disallowing a new app startup if one is in progress
	      if (bstarted == true && (ev.xbutton.time - last_dblclick < pconf->get_config_int("iconstartdelay"))) {
		log1 ("icon start request too fast (iconstartdelay)", pconf->get_config_int("iconstartdelay"));
		psound->play_sound("sounddisabledicon");
	      }
	      else {
		log ("Processing mouse click startup event");
		last_dblclick = ev.xbutton.time;
		bstarted = false;

		// Save to request an app startup: tell the icon a mouse double click needs processing
		Window winapp = iconHandlers[wtarget]->find_icon_window (display, iconHandlers[wtarget]->get_appid());
		if (!winapp) {
		  // Notify system we are about to load a new app (hourglass)
		  psound->play_sound("soundlaunchapp");
		  notify_startup_load (display, iconHandlers[wtarget]->iconid, ev.xbutton.time);
		  bstarted = iconHandlers[wtarget]->double_click (display, ev);
		}
		else {
		  // The app is already running, icon is disabled, unless MaximizeSingleton flag is set
		  if (pconf->get_config_string("maximizesingleton") == "true") {
		    log1 ("Maximizing AppID which is a running singleton", iconHandlers[wtarget]->get_appid());
		    iconHandlers[wtarget]->maximize (display, winapp);
		  }
		  else {
		    log1 ("AppID running, disabling icon with alert sound", iconHandlers[wtarget]->get_appid());
		    psound->play_sound("sounddisabledicon");
		  }
		}
	      }
	    }
	  break;

	case ButtonRelease:
	  if (ev.xbutton.button != Button1 && ev.xbutton.button != Button3) {
	    log1 ("ButtonRelease for unsupported button number - ignoring", ev.xbutton.button);
	  }
	  else {
	    log2 ("ButtonRelease event to window and time ms", wtarget, ev.xbutton.time);
	    last_click = ev.xbutton.time;
	  }
	  break;

	case MotionNotify:
	  iconHandlers[wtarget]->motion(display, ev);
	  break;

	case EnterNotify:
	  log1 ("EnterNotify event to window", wtarget);
	  iconHandlers[wtarget]->blink_icon(display, ev);
	  break;

	case LeaveNotify:
	  log1 ("LeaveNotify event to window", wtarget);
	  iconHandlers[wtarget]->unblink_icon(display, ev);
	  break;

	case Expose:
	  iconHandlers[wtarget]->draw(display, ev, false);
	  break;
	  
	default:
	  break;
	}

      // Pass on rest of unhandled events to the startup notification library
      notify_startup_event (display, &ev);

    } while (!finish);

  return true;
}

bool Desktop::initialize(Display *display, Configuration *loaded_conf, Sound *ksound)
{
  pconf = loaded_conf;
  psound = ksound;
  finish = false;

#ifdef DEBUG
  int x=10,y=10,cw=100,ch=100,width=5;
#else
  int x=0,y=0,cw=1,ch=1,width=0;
#endif

  // Allocate Atoms used for signaling Kdesk's object window
  atom_finish = XInternAtom(display, KDESK_SIGNAL_FINISH, False);
  atom_reload = XInternAtom(display, KDESK_SIGNAL_RELOAD, False);
  atom_icon_alert = XInternAtom(display, KDESK_SIGNAL_ICON_ALERT, False);

  // Create a hidden Object Control window which will receive Kdesk external events
  XSetWindowAttributes attr;
  attr.background_pixmap = ParentRelative;
  attr.backing_store = Always;
  attr.event_mask = ExposureMask | EnterWindowMask | LeaveWindowMask;
  attr.override_redirect = True;
  wcontrol = XCreateWindow (display, DefaultRootWindow(display), x, y, cw, ch, width,
			    CopyFromParent, CopyFromParent, CopyFromParent,
			    CWEventMask,
			    &attr);

  XClearWindow(display, wcontrol);

  // We'll give this window a meaningful name
  XStoreName(display, wcontrol, KDESK_CONTROL_WINDOW_NAME);

#ifdef DEBUG
  // Show kdesk control window on the bottom right corner of the screen
  XMapWindow (display, wcontrol);
  int deskw = DisplayWidth(display, DefaultScreen (display));
  int deskh = DisplayHeight(display, DefaultScreen (display));
  XMoveWindow (display, wcontrol, deskw - cw - width, deskh - ch - width);
#endif

  log1 ("Creating Kdesk control window, handle", wcontrol);
  return (wcontrol ? true : false);
}

bool Desktop::finalize(void)
{
  finish = true;
}

Window Desktop::find_kdesk_control_window (Display *display)
{
  Window kdesk_control_window=0L;
  Window returnedroot, returnedparent, root = DefaultRootWindow(display);
  char *windowname=NULL;
  Window *children=NULL, *subchildren=NULL;
  unsigned int numchildren, numsubchildren;
  
  // Enumarate top-level windows in search for Kdesk control window
  XQueryTree (display, root, &returnedroot, &returnedparent, &children, &numchildren);
  XWindowAttributes w;
  int i;
  for (int i=numchildren-1; i>=0 && !kdesk_control_window; i--) {
    
    if (XFetchName (display, children[i], &windowname)) {
      if (!strncmp (windowname, KDESK_CONTROL_WINDOW_NAME, strlen (KDESK_CONTROL_WINDOW_NAME))) {
	kdesk_control_window = children[i];
	log1 ("Kdesk control window found", kdesk_control_window);
	XFree (windowname);
	break;
      }
    }
    
    // Kdesk might sit a little deeper in the tree hierarchy, let's step in
    XQueryTree (display, children[i], &returnedroot, &returnedparent, &subchildren, &numsubchildren);
    
    for (int k=numsubchildren-1; k>=0; k--) {
      if (XFetchName (display, subchildren[k], &windowname)) {
	if (!strncmp (windowname, KDESK_CONTROL_WINDOW_NAME, strlen (KDESK_CONTROL_WINDOW_NAME))) {
	  kdesk_control_window = subchildren[k];
	  log1 ("Kdesk control window found", kdesk_control_window);
	  XFree (windowname);
	  break;
	}
      }
    }
  }
  
  if(children) {
    XFree(children);
  }
  
  if(subchildren) {
    XFree(subchildren);
  }

  return kdesk_control_window;
}

bool Desktop::send_signal (Display *display, const char *signalName, char *message)
{
  Window wsig = wcontrol; // Kdesk control window, either found by instantiation or enumarated.

  // The signal name has been allocated by kdesk, if it's not there, kdesk is not running.
  Atom atom_signal = XInternAtom(display, signalName, True);
  if (!atom_signal) {
    log1 ("Atom signal name not defined. Perhaps kdesk is not running?", signalName);
    return false;
  }

  if (!wsig && !(wsig=find_kdesk_control_window(display))) {
    // Kdesk control window cannot be found, most likely Kdesk is not running.
    return false;
  }

  // Finally send the event to the control window to unfold the job
  XEvent xev;
  memset (&xev, 0x00, sizeof(xev));
  xev.type                 = ClientMessage;
  xev.xclient.window       = wsig;
  xev.xclient.format       = 32;   // 32 means data is interpreted as consecutive set of unordered LONGs

  // A Kdesk message data is encoded in the following way:
  // 4-byte atom ID name + an optional 16 byte char icon name - the case for KDESK_SIGNAL_ICON_ALERT.
  xev.xclient.data.l[0]    = atom_signal;

  if (message != NULL) {
    // Append a message string to the event - This is a custom fixed lenght literal.
    memcpy (&xev.xclient.data.l[1], message, (strlen(message) > 15 ? 16 : strlen(message)));
  }

  int rc = XSendEvent (display, wsig, 1, NoEventMask, (XEvent *) &xev);
  XFlush (display);
  log2 ("Sending a client message event to kdesk control window (win, rc)", wsig, rc);
  return (rc == Success ? true : false);
}

bool Desktop::call_icon_hook (Display *display, XEvent ev, string hookscript, Icon *pico_hook)
{
  FILE *fp_iconhooks=NULL;
  char chcmdline[1024];
  char chline[1024], key[64], value[900], word[256];
  int updates=0;

  // Given a hook script name and an icon instance, call the script
  // to update its attributes, then ask the icon instance to refresh them
  if (!pico_hook) {
    log ("Icon handler is empty");
    return false;
  }

  // Execute the Icon Hook, parse the stdout, and communicate with the icon to refresh attributes
  sprintf (chcmdline, "/bin/bash -c \"%s %s\"", hookscript.c_str(), pico_hook->get_icon_name().c_str());
  log1 ("Executing hook script:", chcmdline);
  fp_iconhooks = popen (chcmdline, "r");
  while (fgets (chline, sizeof (chline), fp_iconhooks) != NULL)
    {
      char *toks=chline;
      memset (key, 0x00, sizeof(key));
      memset (value, 0x00, sizeof(value));
      int n=0;
      while (sscanf (toks, "%s%n", word, &n) == 1 ) {
	if (word[strlen(word)-1] == ':') {
	  strcpy (key, word);
	}
	else {
	  strcat (value, word);
	  strcat (value, " ");
	}
	toks += n;
      }
	
      // Parse the keys (attributes) that can be applied to the icon, pass them to the icon instance
      value[strlen(value)-1]=0x00;
      if (!strcmp (key, "Message:")) {
	pico_hook->set_message (value);
	updates++;
      }
      else if (!strcmp (key, "Caption:")) {
	pico_hook->set_caption (value);
	updates++;
      }
      else if (!strcmp (key, "Icon:")) {
	pico_hook->set_icon (value);
	updates++;
      }
      else if (!strcmp (key, "IconStamp:")) {
	pico_hook->set_icon_stamp (value);
	updates++;
      }
    }
    
  // Redraw the icon if attributes have been modified
  fclose (fp_iconhooks);
  if (updates) {
    log1 ("Populating hook updates to icon (#updates)", updates);
    pico_hook->clear(display, ev);
    pico_hook->draw(display, ev, false);
  }
  return true;
}

bool Desktop::dump_metrics (Display *display)
{
  //
  //  Save Kdesk counters in a json-like file prepended with current display number.
  //

  char chmetrics_filename[80];
  sprintf (chmetrics_filename, "/tmp/kdesk-metrics%s.dump", XDisplayString(display));

  FILE *fp = fopen (chmetrics_filename, "w");
  if (fp) {
    fprintf (fp, "{\n \"icons-found\": %d,\n", pconf->get_numicons());
    fprintf (fp, " \"icons-rendered\": %d,\n", numicons);
    fprintf (fp, " \"grid-full\": %s\n}\n", icon_grid->grid_full == true ? "true" : "false");
    log1 ("Metrics file saved", chmetrics_filename);
    fclose (fp);
    return true;
  }
  else {
    log2 ("Error creating metrics file (errno, file)", errno, chmetrics_filename);
    return false;
  }
}
