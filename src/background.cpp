//
// background.cpp  -  Class to draw an icon on the desktop
//
// Copyright (C) 2013-2014 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
// An app to show and bring life to Kano-Make Desktop Icons.
//

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <Imlib2.h>

#include <cmath>
#include <memory.h>
#include <stdlib.h>

#include "configuration.h"
#include "background.h"
#include "logging.h"



int Background::setRootAtoms (Display *display, Pixmap pixmap)
{
    //
    // This function taken from hsetroot,
    // which in turn inherits from fluxbox bsetroot.
    //
    // The use of Atoms makes the background play nice with xcompmgr
    // compositor, which otherwise creates faulty redraw areas on the screen.
    // xsetroot seems to suffer from this same issue.
    //

    Atom atom_root, atom_eroot, type;
    unsigned char *data_root, *data_eroot;
    int format;
    unsigned long length, after;

    atom_root = XInternAtom (display, "_XROOTMAP_ID", True);
    atom_eroot = XInternAtom (display, "ESETROOT_PMAP_ID", True);

    // doing this to clean up after old background
    if (atom_root != None && atom_eroot != None)
        {
            XGetWindowProperty (display, RootWindow (display, screen),
                                atom_root, 0L, 1L, False, AnyPropertyType,
                                &type, &format, &length, &after, &data_root);
            if (type == XA_PIXMAP)
                {
                    XGetWindowProperty (display, RootWindow (display, screen),
                                        atom_eroot, 0L, 1L, False, AnyPropertyType,
                                        &type, &format, &length, &after, &data_eroot);
                    if (data_root && data_eroot && type == XA_PIXMAP &&
                        *((Pixmap *) data_root) == *((Pixmap *) data_eroot))
                        {
                            XKillClient (display, *((Pixmap *) data_root));
                        }
                }
        }

    atom_root = XInternAtom (display, "_XROOTPMAP_ID", False);
    atom_eroot = XInternAtom (display, "ESETROOT_PMAP_ID", False);

    if (atom_root == None || atom_eroot == None)
        return 0;

    // setting new background atoms
    XChangeProperty (display, RootWindow (display, screen),
                     atom_root, XA_PIXMAP, 32, PropModeReplace,
                     (unsigned char *) &pixmap, 1);
    XChangeProperty (display, RootWindow (display, screen), atom_eroot,
                     XA_PIXMAP, 32, PropModeReplace, (unsigned char *) &pixmap,
                     1);

    return 1;
}

Background::Background (Configuration *loaded_conf)
{
  running = false;
  pconf = loaded_conf;
}

Background::~Background (void)
{
}

bool Background::setup(Display *display)
{
  // Setup X resources
  screen = DefaultScreen (display);
  vis = DefaultVisual (display, screen);
  cm = DefaultColormap (display, screen);
  root = RootWindow (display, screen);
  XSelectInput (display, root, StructureNotifyMask);

  deskw = DisplayWidth(display, screen);
  deskh = DisplayHeight(display, screen);

  // Bind resources to Imlib2
  imlib_context_set_display(display);
  imlib_context_set_visual(vis);
  imlib_context_set_colormap(cm);

  pmap = XCreatePixmap (display, root, deskw, deskh, DefaultDepth (display, DefaultScreen (display)));
  if (!pmap) {
    log ("error creating pixmap for desktop background");
    return false;
  }

  return true;
}

bool Background::load (Display *display)
{
  int i, w, h, nx, ny, nh, nw, tmp;
  float ratio, dist43, dist169;
  double factor;
  string background_file;
  Imlib_Image tmpimg, buffer;
  bool bsuccess=false;

  buffer = imlib_create_image (deskw, deskh);
  if (!buffer)
    {
      log ("error creating an image surface for the background");
    }
  else
    {
      ratio = (deskw *1.0) / (deskh);
      dist43 = std::abs (ratio - 4.0/3.0);
      dist169 = std::abs (ratio - 16.0/9);

      // Decide which image to load depending on screen resolution and aspect/ratio
      unsigned int midreswidth=pconf->get_config_int ("screenmedreswidth");
      if (midreswidth > 0 && deskw < midreswidth) {
	// If a minimal medium resolution is specified, and
	// the screen resolution falls below this setting, then
	// Take the middle resolution wallpaper image
	background_file = pconf->get_config_string ("background.file-medium");
	log1 ("loading medium resolution wallpaper image", background_file);
      }
      else {
	//
	// Otherwise we are on a high resolution screen,
	// display the appropiate wallpaper based on the screen aspect/ratio.
	//
	if (dist43 < dist169) {
	  background_file = pconf->get_config_string ("background.file-4-3");
	  log1 ("loading 4:3 wallpaper image", background_file);
	}
	else {
	  background_file = pconf->get_config_string ("background.file-16-9");
	  log1 ("loading 16:9 wallpaper image", background_file);
	}
      }

      image = imlib_load_image_without_cache(background_file.c_str());
      if (!image) {
	log1 ("error loading background", background_file);
      }
      else {
	// Prepare imlib2 drawing spaces
	imlib_context_set_image(buffer);
	imlib_context_set_color(0,0,0,0);
	imlib_image_fill_rectangle(0, 0, deskw, deskh);
	imlib_context_set_blend(1);
      
	imlib_context_set_image(image);
	w = imlib_image_get_width();
	h = imlib_image_get_height();
	if (!(tmpimg = imlib_clone_image())) {
	  log ("error cloning the desktop background image");
	}
	else {
	  imlib_context_set_image(tmpimg);
	  imlib_context_set_image(buffer);
	  
	  int screen_num = DefaultScreen(display);
	  int nw = DisplayWidth(display, screen_num);
	  int nh = DisplayHeight(display, screen_num);

	  imlib_blend_image_onto_image(tmpimg, 0, 0, 0, w, h, 0, 0, nw, nh);
	  imlib_context_set_image(tmpimg);
	  imlib_free_image();
	  
	  imlib_context_set_blend(0);
	  imlib_context_set_image(buffer);
	  imlib_context_set_drawable(root);
	  imlib_render_image_on_drawable(0, 0);
	  imlib_context_set_drawable(pmap);
	  imlib_render_image_on_drawable(0, 0);

	  // Apply the background to the root window
	  // so it stays permanently even if kdesk quits (-w parameter)

          setRootAtoms(display, pmap);

          XKillClient(display, AllTemporary);
          XSetCloseDownMode(display, RetainTemporary);

          XSetWindowBackgroundPixmap(display, root, pmap);

	  XClearWindow (display, root);
	  XFlush (display);
          XSync(display, False);

	  // Free imlib and Xlib image resources
	  imlib_context_set_image(buffer);
	  imlib_free_image();
	  imlib_context_set_image(image);
	  imlib_free_image();

          XFreePixmap(display, pmap);

	  bsuccess = true;
	  log1 ("desktop background created successfully", image);
	}
      }
    }

  return bsuccess;
}

int Background::refresh_background(Display *display)
{
  Window root_return, parent_return, *children_return;
  unsigned int nchildren_return=0;

  // Enumerate all top level windows and request a repaint
  // to refresh transparent areas with the new background.
  if (XQueryTree(display, root, &root_return, &parent_return, &children_return, &nchildren_return))
    {
      for (int i=0; i < nchildren_return; i++)
	{
	  // clear the window area
	  XClearWindow (display, children_return[i]);

	  // repaint
	  XEvent exppp;
	  memset(&exppp, 0x00, sizeof(exppp));
	  exppp.type = Expose;
	  exppp.xexpose.window = children_return[i];
	  XSendEvent (display, children_return[i], False, ExposureMask, &exppp);
	  XFlush(display);
	}
    }

  return nchildren_return;
}
