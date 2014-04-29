//
// desktop.cpp
//
// Copyright (C) 2013-2014 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
// An app to show and bring life to Kano-Make Desktop Icons.
//

#include <X11/Xatom.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

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
#include "desktop.h"
#include "logging.h"

Desktop::Desktop(void)
{
  atom_finish = atom_reload = 0L;
  wcontrol = 0L;
  numicons = 0;
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
}

bool Desktop::create_icons (Display *display)
{
  int nicon=0;

  // Create and draw all icons, save a mapping of their window IDs -> handlers
  // so that we can dispatch events to each one in turn later on.
  for (nicon=0; nicon < pconf->get_numicons(); nicon++)
    {
      Icon *pico = new Icon(pconf, nicon);
      Window wicon = pico->create(display);
      if (wicon) {
	XEvent emptyev;
	iconHandlers[wicon] = pico;
	pico->draw(display, emptyev);
	numicons++;
      }
      else {
	log1 ("Warning: error creating icon", pconf->get_icon_string(nicon, "filename"));
	delete pico;
      }
    }

  // Setup a connection with startup notification library
  // error_trap_push, error_trap_pop ???
  sn_display = sn_display_new (display, error_trap_push, error_trap_pop);
  sn_context = sn_launcher_context_new (sn_display, DefaultScreen (display));

  // returns true if at least one icon is available on the desktop
  log1 ("Desktop icon classes have been allocated", nicon);
  return (bool) (nicon > 0);
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
	iconHandlers.erase(it);
      }
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
		if (iconHandlers[wtarget]->is_singleton_running () == false) {
		  // Notify system we are about to load a new app (hourglass)
		  psound->play_sound("soundlaunchapp");
		  notify_startup_load (display, iconHandlers[wtarget]->iconid, ev.xbutton.time);
		  bstarted = iconHandlers[wtarget]->double_click (display, ev);
		}
		else {
		  // The app is already running, icon is disabled
		  psound->play_sound("sounddisabledicon");
		}
	      }
	    }
	  break;

	case ButtonRelease:
	  log2 ("ButtonRelease event to window and time ms", wtarget, ev.xbutton.time);
	  last_click = ev.xbutton.time;
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
	  iconHandlers[wtarget]->draw(display, ev);
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
  int x=10,y=10,cw=100,ch=100,width=5;
  #endif

  // Allocate Atoms used for signaling Kdesk's object window
  atom_finish = XInternAtom(display, KDESK_SIGNAL_FINISH, False);
  atom_reload = XInternAtom(display, KDESK_SIGNAL_RELOAD, False);

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
  XMapWindow(display, wcontrol);
  #endif

  log1 ("Creating Kdesk control window, handle", wcontrol);
  return (wcontrol ? true : false);
}

bool Desktop::finalize(void)
{
  finish = true;
}

bool Desktop::send_signal (Display *display, const char *signalName)
{
  Window wsig = wcontrol; // Kdesk control window, either found by instantiation or enumarated.

  // The signal name has been allocated by kdesk, if it's not there, kdesk is not running.
  Atom atom_signal = XInternAtom(display, signalName, True);
  if (!atom_signal) {
    log1 ("Atom signal name not defined. Perhaps kdesk is not running?", signalName);
    return false;
  }

  if (!wsig) {
    // Highly suggests kdesk is instantiated from a new process,
    // we need to get Kdesk's control window by querying the XServer hierarchy
    // or find out that it is not running

    Window returnedroot, returnedparent, root = DefaultRootWindow(display);
    char *windowname=NULL;
    Window *children=NULL, *subchildren=NULL;
    unsigned int numchildren, numsubchildren;

    // Tell us the top-most level windows
    XQueryTree (display, root, &returnedroot, &returnedparent, &children, &numchildren);
    XWindowAttributes w;
    int i;
    for (int i=numchildren-1; i>=0 && !wsig; i--) {
      
      if (XFetchName (display, children[i], &windowname)) {
	if (!strncmp (windowname, KDESK_CONTROL_WINDOW_NAME, strlen (KDESK_CONTROL_WINDOW_NAME))) {
	  wsig = children[i];
	  log1 ("Kdesk control window found", wsig);
	  XFree (windowname);
	  break;
	}
      }

      // Kdesk might sit a little deeper in the tree hierarchy, let's step in
      XQueryTree (display, children[i], &returnedroot, &returnedparent, &subchildren, &numsubchildren);

      for (int k=numsubchildren-1; k>=0; k--) {
	if (XFetchName (display, subchildren[k], &windowname)) {
	  if (!strncmp (windowname, KDESK_CONTROL_WINDOW_NAME, strlen (KDESK_CONTROL_WINDOW_NAME))) {
	    wsig = subchildren[k];
	    log1 ("Kdesk control window found", wsig);
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

    if (!wsig) {
      cout << "Could not find Kdesk control window - perhaps it is not running on this Display?" << endl;
      return false;
    }
  }

  // Finally send the event to the control window to unfold the job
  XEvent xev;
  memset (&xev, 0x00, sizeof(xev));
  xev.type                 = ClientMessage;
  xev.xclient.window       = wsig;
  xev.xclient.format       = 32;
  xev.xclient.data.l[0]    = atom_signal;
  xev.xclient.data.l[1]    = atom_signal;

  int rc = XSendEvent (display, wsig, 1, NoEventMask, (XEvent *) &xev);
  XFlush (display);
  log2 ("Sending a client message event to kdesk control window (win, rc)", wsig, rc);
  return (rc == Success ? true : false);
}
