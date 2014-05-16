//
// background.cpp  -  Class to draw an icon on the desktop
//
// Copyright (C) 2013-2014 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
// An app to show and bring life to Kano-Make Desktop Icons.
//

#include <X11/Xlib.h>
#include <Imlib2.h>

#include <cmath>
#include <memory.h>

#include "configuration.h"
#include "background.h"
#include "logging.h"

Background::Background (Configuration *loaded_conf)
{
  running = false;
  pconf = loaded_conf;
  winblur = 0L;
}

Background::~Background (void)
{
}

bool Background::setup(Display *display)
{
  int i, screen;

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
	  XSetWindowBackgroundPixmap(display, root, pmap);

	  // Apply the background to the root window
	  // so it stays permanently even if kdesk quits (-w parameter)
	  XClearWindow (display, root);
	  XFlush (display);

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

bool Background::blur(Display *display)
{
  bool success = false; // always assume the worst, unless our tiny steps reach to a success state
  Pixmap pmap_blur = 0L;
  Imlib_Image img_blurred;
  Imlib_Color_Modifier colorTrans=NULL;

  // FIXME: Below variables will be accessed from the class instance via a signal
  int screen = DefaultScreen (display);
  root = RootWindow (display, screen);
  deskw = DisplayWidth(display, screen);
  deskh = DisplayHeight(display, screen);

  // Blur is a toggle effect method. If desktop is blurred, remove it. Otherwise blur it.
  if (winblur != 0L) {
    // Removing blur - In order to be as visually fast as possible, first hide the blurring window
    // revealing the real desktop image, then destroy the XWindow resource.
    XUnmapWindow (display, winblur);
    XDestroyWindow (display, winblur);
    winblur = 0L;
    success = true;
  }
  else {
    // Creating a desktop blur effect.
    pmap_blur = XCreatePixmap (display, root, deskw, deskh, DefaultDepth (display, DefaultScreen (display)));
    img_blurred = imlib_create_image(deskw, deskh);
    if (!pmap_blur || !img_blurred) {
      cout << "no resources to create a blurred pixmap or imlib image buffer" << endl;
      success = false;
    }
    else {
      // Take a screenshot from the desktop and save it in a Imblib2 object
      // So we can use it's API to blur the image
      imlib_context_set_image(img_blurred);
      imlib_context_set_display(display);
      imlib_context_set_visual(DefaultVisual(display, 0));
      imlib_context_set_drawable(root);
      imlib_copy_drawable_to_image(0, 0, 0, deskw, deskh, 0, 0, 1);

      // Create RGB tables to blur the colors
      unsigned char iconR[256], iconG[256], iconB[256], iconTR[256];
      colorTrans = imlib_create_color_modifier();
      imlib_context_set_color_modifier(colorTrans);    
      imlib_get_color_modifier_tables(iconR, iconG, iconB, iconTR);
      imlib_reset_color_modifier();

      // Blur the RGB channels to a third of their intensities
      for (int n=0; n < 256; n++) {
	iconR[n] = iconR[n] / 3;
	iconG[n] = iconG[n] / 3;
	iconB[n] = iconB[n] / 3;
      }

      // At this point img_blurred contains a blurred copy of the desktop image
      imlib_set_color_modifier_tables (iconR, iconG, iconB, iconR);

      // create a top level window which will draw the blurred desktop on top
      XSetWindowAttributes attr;
      memset (&attr, 0x00, sizeof (attr));
      attr.background_pixmap = ParentRelative;

      // this will ensure the blurred image is repainted
      // should the top level unblurred window move along the desktop.
      attr.backing_store = Always;

      attr.event_mask = 0; // no relevant events we want to care about, XServer will do it for us
      attr.override_redirect = True;
      winblur = XCreateWindow (display, DefaultRootWindow(display), 0, 0,
			       //
			       // FIXME: 
			       // int 41 number needs to be fit with the amount of over-space used by the decorations.
			       //
			       deskw, deskh - 41, 0,
			       CopyFromParent, CopyFromParent, CopyFromParent,
			       CWEventMask, &attr);
      if (!winblur) {
	// TODO: Too bad... free resources and leave
	log ("Could not create a blurred Xwindow, cannot blur");
	success = false;
      }
      else {
	// The "Hints" code below is needed to remove the window decorations
	// We don't want a title or border frames.
	// TODO: Remove the taskbar icon associated to the window.
	typedef struct Hints
	{
	  unsigned long   flags;
	  unsigned long   functions;
	  unsigned long   decorations;
	  long            inputMode;
	  unsigned long   status;
	} Hints;
	
	Hints hints;
	Atom property_hints, property_icon;
	hints.flags = 2;
	hints.decorations = 0;
	property_hints = XInternAtom(display, "_MOTIF_WM_HINTS", true);
	property_icon = XInternAtom(display, "_NET_WM_ICON", true);
	XChangeProperty (display, winblur, property_hints, property_hints, 32, PropModeReplace,(unsigned char *)&hints, 5);

	// Give the blurred window a meaningful name and show it
	XStoreName (display, winblur, "KdeskBlur");
	XMapWindow(display, winblur);

	// Draw the blurred image into the pixmap, then apply the pixmap to the window
	// We do it this way so the XServer can have a copy for the backing store to repaint on Exposure events
	imlib_context_set_drawable(pmap_blur);
	imlib_render_image_on_drawable (0, 0);
	XSetWindowBackgroundPixmap(display, winblur, pmap_blur);
	XFlush (display);
	imlib_free_image();
	success = true;
      }
    }

    return (success);
  }
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
	  XSendEvent(display,children_return[i],False,ExposureMask,&exppp);
	  XFlush(display);
	}
    }

  return nchildren_return;
}

bool Background::is_blurred(Display *display)
{
  int screen = DefaultScreen (display);
  Window root = RootWindow (display, screen);
  Window root_return, parent_return, *children_return=NULL, *subchildren_return=NULL;
  unsigned int nchildren_return=0, nsubchildren_return=0;
  bool found = false;

  // Enumerate all top level windows in search for the Kdesk's blurred window
  if (XQueryTree(display, root, &root_return, &parent_return, &children_return, &nchildren_return))
    {
      char *windowname=NULL;
      for (int i=0; i < nchildren_return; i++)
	{
	  if (XFetchName (display, children_return[i], &windowname)) {
	    if (!strncmp (windowname, "KdeskBlur", strlen ("KdeskBlur"))) {
	      found = true;
	      XFree (windowname);
	      break;
	    }
	  }

	  XQueryTree (display, children_return[i], &root_return, &parent_return, &subchildren_return, &nsubchildren_return);

	  for (int k=nsubchildren_return-1; k>=0; k--) {
	    if (XFetchName (display, subchildren_return[k], &windowname)) {
	      if (!strncmp (windowname, "KdeskBlur", strlen ("KdeskBlur"))) {
		found=true;
		XFree (windowname);
		break;
	      }
	    }
	  }

	}
    }

  if (children_return) {
    XFree(children_return);
  }

  if (subchildren_return) {
    XFree(subchildren_return);
  }

  return found;
}
