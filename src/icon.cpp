//
// Icon.cpp  -  Class to draw an icon on the desktop
//
// Copyright (C) 2013-2014 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
// An app to show and bring life to Kano-Make Desktop Icons.
//

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

#include <Imlib2.h>

#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#include <iostream>
#include <algorithm>
#include <cctype>

#include "icon.h"
#include "logging.h"
#include "grid.h"

Icon::Icon (Configuration *loaded_conf, int iconidx)
{
  win = 0L;
  configuration = loaded_conf;
  iconid = iconidx;
  iconx=icony=iconw=iconh=0;
  pgrid = NULL;
  shadowx=shadowy=0;
  iconMapNone = iconMapGlow = iconMapTransparency = (unsigned char *) NULL;
  transparency_value=0;
  backsafe = NULL;
  font = fontsmaller = NULL;
  image = image_stamp = NULL;
  xftdraw1 = NULL;
  is_grid = false;
  gridx = gridy = 0;

  // save the lnk filename, icon, icon hover image files
  filename = configuration->get_icon_string (iconid, "filename");
  ficon = configuration->get_icon_string (iconid, "icon");
  ficon_hover = configuration->get_icon_string (iconid, "iconhover");
  ficon_stamp = configuration->get_icon_string (iconid, "iconstamp");

  // save the icon caption and message literals to be rendered around it
  caption = configuration->get_icon_string (iconid, "caption");
  message = configuration->get_icon_string (iconid, "message");

  // Initially we don't know yet which display we are bound to until create()
  icon_display = NULL;

  // default font details should be zero
  memset (&fontInfoCaption, 0x00, sizeof (XGlyphInfo));
  memset (&fontInfoMessage, 0x00, sizeof (XGlyphInfo));

  iconMapNone = (unsigned char *) calloc (sizeof(unsigned char), 256);
  if (iconMapNone) {
    for (int c=0; c < 256; c++) {
      iconMapNone[c] = (unsigned char) c;
    }
  }
  else {
    log ("Error allocating memory for iconMapNone");
  }

  iconMapGlow = (unsigned char *) calloc (sizeof(unsigned char), 256);
  if (!iconMapGlow) {
    log ("Error allocating memory for iconMapGlow");
  }

  // Icon transparency can be specified for all icons in the kdeskrc file.
  transparency_value = configuration->get_config_int("transparency");
  if (transparency_value > 0) {
    log1 ("Found icon transparency setting", transparency_value);
    iconMapTransparency = (unsigned char *) calloc (sizeof(unsigned char), 256);
    if (!iconMapTransparency) {
      log ("Error allocating memory for iconMapTransparency");
    }
  }

  // Define the default cursor for the mouse pointer
  // Or change to a custom one specified in the config file
  cursor_id = configuration->get_config_int("mousehovericon");
  if (cursor_id == 0) {
    cursor_id = DEFAULT_ICON_CURSOR;
  }

  cursor = 0L; // Set the initial cursor handle to nothing
}

Icon::~Icon (void)
{
}

int Icon::get_iconid(void)
{
  return iconid;
}

int Icon::set_iconid(int iconidx)
{
  iconid = iconidx;
}

string Icon::get_appid(void)
{
  string appid = configuration->get_icon_string (iconid, "appid");
  return appid;
}

string Icon::get_commandline(void)
{
  string commandline = configuration->get_icon_string (iconid, "command");
  return commandline;
}

std::string Icon::get_icon_filename(void)
{
  return filename;
}

std::string Icon::get_icon_name(void)
{
  // returns icon name without the LNK extension
  string filename = get_icon_filename();

  // FIXME: Upper/lowercase names for the extension
  return filename.erase (filename.rfind(".lnk"), std::string::npos);
}

void Icon::set_caption (char *new_caption)
{
  caption = new_caption;
}

void Icon::set_message (char *new_message)
{
  message = new_message;
}

void Icon::set_icon (char *new_icon)
{
  ficon = new_icon;
}

void Icon::set_icon_stamp (char *new_icon)
{
  ficon_stamp = new_icon;
}

bool Icon::is_singleton_running (Display *display)
{
  bool bAppRunning=false;
  string appid = configuration->get_icon_string (iconid, "appid");
  string singleton = configuration->get_icon_string (iconid, "singleton");

  if (singleton == "true" && appid.size())
    {
      // Return wether we can find the icon app window on the desktop
      bAppRunning = (find_icon_window (display, appid) ? true : false);
    }

  log2 ("Is kdesk icon application running? (AppID, bool)", appid, bAppRunning);
  return bAppRunning;
}

Window Icon::create (Display *display, IconGrid *icon_grid)
{
  unsigned int rc=0;
  int border;

  // save the display variable and grid for later cleanup
  icon_display = display;
  pgrid = icon_grid;
  vis = DefaultVisual(display, DefaultScreen(display));
  cmap = DefaultColormap(display, DefaultScreen(display));

  // If there is an icon caption or message defined, allocate a font for it
  if (caption.length() > 0 || message.length() > 0) {
    log ("allocating font resources for icon title");
    int fontsize = configuration->get_config_int ("fontsize");
    string fontname = configuration->get_config_string ("fontname");
    string fontbold = configuration->get_config_string ("bold");

    // Collect font details: shadow offsets and caption screen space occupied, used for centering
    int shadowx = configuration->get_icon_int (iconid, "shadowx");
    int shadowy = configuration->get_icon_int (iconid, "shadowy");

    if (fontbold.size()) {
      fontname += " bold";
    }

    log2 ("opening font name and point size", fontname, fontsize);
    font = XftFontOpen (display, DefaultScreen(display),
			XFT_FAMILY, XftTypeString, fontname.c_str(),
			XFT_SIZE, XftTypeDouble, (float) fontsize,
			NULL);
    if (!font) {
      log("Could not create font!");
    }
    else{
      log("font loaded");
      rc = XftColorAllocName(display, DefaultVisual(display,0), DefaultColormap(display,0), "white", &xftcolor);
      rc = XftColorAllocName(display, DefaultVisual(display,0), DefaultColormap(display,0), "black", &xftcolor_shadow);
      log1 ("XftColorAllocName bool", rc);

      int subtitle_fontsize = configuration->get_config_int ("subtitlefontsize");
      if (!subtitle_fontsize) {
        // Assign a smaller font size
        subtitle_fontsize = fontsize - DEFAULT_SUBTITLE_FONT_POINT_DECREASE;
      }

      fontsmaller = XftFontOpen (display, DefaultScreen(display),
				 XFT_FAMILY, XftTypeString, fontname.c_str(),
				 XFT_SIZE, XftTypeDouble, (float) subtitle_fontsize,
				 NULL);
      log1 ("creating a smaller font for messages", fontsmaller);

      // Find out the extend of icon caption and message on the rendering surface
      // The window containing the icon will be enlarged vertically to accomodate this space
      if (caption.length() > 0) {
	XftTextExtentsUtf8 (display, font, (XftChar8*) caption.c_str(), caption.length(), &fontInfoCaption);
      }
    }
  }

  XSetWindowAttributes attr;

  attr.background_pixmap = ParentRelative;
  attr.backing_store = Always;
  attr.event_mask = ExposureMask | EnterWindowMask | LeaveWindowMask;
  attr.override_redirect = True;

  int screen_num = DefaultScreen(display);
  int w = DisplayWidth(display, screen_num);
  int h = DisplayHeight(display, screen_num);

  // Using this parameter we can control the space
  // between the icon and name rendered just below
  icontitlegap = configuration->get_config_int("icontitlegap");
  log1 ("Icon gap for font title rendering", icontitlegap);

  string icon_placement = configuration->get_icon_string(iconid, "relative-to");
  if (icon_placement == "grid") {
    // save grid icon mode
    is_grid = true;

    // Grid icons have fixed size
    iconw = icon_grid->ICON_W;
    iconh = icon_grid->ICON_H;

    string iconx_tmp, icony_tmp;

    iconx_tmp = configuration->get_icon_string (iconid, "x");
    if (iconx_tmp == "auto") {
      iconx = -1;
    } else {
      iconx = configuration->get_icon_int (iconid, "x");
    }

    icony_tmp = configuration->get_icon_string (iconid, "y");
    if (icony_tmp == "auto") {
      icony = -1;
    } else {
      icony = configuration->get_icon_int (iconid, "y");
    }

    if (!icon_grid->request_position(iconx, icony, &iconx, &icony, &gridx, &gridy)) {
      /* Error! No more space available! */
      log("No spaces available in the grid!");
      return None;
    }

  } else {
    iconx = configuration->get_icon_int (iconid, "x");
    icony = configuration->get_icon_int (iconid, "y");
    iconw = configuration->get_icon_int (iconid, "width");
    iconh = configuration->get_icon_int (iconid, "height");

    // Decide which icon positioning to use on the desktop
    if (icon_placement == "bottom-centre") {
      iconx = w / 2 + iconx;
      icony = h + icony;
    }
    else if (icon_placement == "top-centre") {
      iconx = w / 2 + iconx;
    }
    else if (icon_placement == "top-left") {
      // no coordinate transformation necessary. 0,0 is already top-left
      ;
    }
    else if (icon_placement == "top-right") {
      // icon horizontal position decreases from the right to the left
      iconx = w - (iconx + iconw);
    }
  }

  // In debug version, icons are drawn with a black frame
  #ifdef DEBUG
  border = 1;
  #else
  border = 0;
  #endif

  log4 ("icon placement (x,y,w,h): @", iconx, icony, iconw, iconh);
  win = XCreateWindow (display, DefaultRootWindow(display), iconx, icony, 
		       iconw, iconh + fontInfoCaption.height + icontitlegap, border,
		       CopyFromParent, CopyFromParent, CopyFromParent,
		       CWBackPixmap|CWBackingStore|CWOverrideRedirect|CWEventMask,
		       &attr );

  xftdraw1 = XftDrawCreate(display, win, DefaultVisual(display,0),DefaultColormap(display,0));
  log1("xftdraw1 is", xftdraw1);
  if( win == None ) {
    log ("error creating windows");
  }
  else {
    XSelectInput(display, win, ButtonPressMask | ButtonReleaseMask | PointerMotionMask | ExposureMask | EnterWindowMask | LeaveWindowMask);
    XMapWindow(display, win);
    XLowerWindow(display, win);
  }

  // Set mouse cursor to "hand" when the mouse moves over the icon
  // These are the standard icons defined here:
  // http://tronche.com/gui/x/xlib/appendix/b/
  // which can be replaced by system "themes".

  // TODO: Extract this iconid into .kdeskrc
  cursor = XCreateFontCursor (display, cursor_id);
  XDefineCursor(display, win, cursor);

  // this will hold a copy of the current icon rendered space
  backsafe = imlib_create_image (iconw, iconh);
  return win;
}

void Icon::destroy(Display *display)
{
  // Deallocate resources to terminate the icon
  if (iconMapNone) {
    free (iconMapNone);
    iconMapNone = NULL;
  }

  if (iconMapGlow) {
    free (iconMapGlow);
    iconMapGlow = NULL;
  }

  if (iconMapTransparency) {
    free (iconMapTransparency);
    iconMapTransparency = NULL;
  }

  if (cursor) {
    XFreeCursor (icon_display, cursor);
    cursor = 0L;
  }

  if (xftdraw1) {
    XftDrawDestroy (xftdraw1);
    xftdraw1 = NULL;
  }

  if (font) {
    XftFontClose(display, font);
    font = NULL;
  }

  if (fontsmaller) {
    XftFontClose(display, fontsmaller);
    fontsmaller = NULL;
  }

  if (image != NULL) {
    imlib_context_set_image(image);
    imlib_free_image_and_decache();
  }

  if (image_stamp != NULL) {
    imlib_context_set_image(image_stamp);
    imlib_free_image_and_decache();
  }

  if (backsafe != NULL) {
    imlib_context_set_image(backsafe);
    imlib_free_image_and_decache();
  }

  // free the grid position just occupied if necessary
  if (is_grid == true) {
    pgrid->free_space_used (gridx, gridy);
  }

  XDestroyWindow (display, win);
}

int Icon::get_icon_horizontal_placement (int image_width)
{
  // The default is to render the icon to the left,
  // But the HAlign attribute may override this with the "right" setting.
  // This is useful so the "message" attribute is rendered to the left of the icon
  //
  int subx=0;
  if (configuration->get_icon_string (iconid, "halign") == "right") {
    subx = iconw - image_width;
  }

  return subx;
}

void Icon::clear(Display *display, XEvent ev)
{
  XClearWindow (display, win);
}

void Icon::draw(Display *display, XEvent ev, bool fClear)
{
  Imlib_Color_Modifier colorTrans=NULL;   // used if general icon transparency is requested
  Imlib_Image resized = NULL;
  int h=0, w=0, subx=0;
  int stamp_w=0, stamp_h=0;
  int iconxmove=0, iconymove=0;

  imlib_context_set_display(display);
  imlib_context_set_visual(vis);
  imlib_context_set_colormap(cmap);
  imlib_context_set_drawable(win);

  log5 ("drawing icon (name @coords)", ficon, iconx, icony, iconw, iconh);

  // Reinforcing the window to stay at the bottom of all windows. From the docs on XLowerWindow...
  // "Lowering a mapped window will generate Expose events on any windows it formerly obscured."
  //
  XMapWindow(display, win);
  XLowerWindow(display, win);

  if (fClear == true) {
    // Clear the icon area completely
    // This is needed when for example the transparent background has changed due to blur effect
    XClearWindow (display, win);
  }

  // Is there a s Stamp icon? If so, load it now
  if (ficon_stamp.length() > 0) {
    log3 ("loading stamp icon (icon, width, height)", ficon_stamp, stamp_w, stamp_h);
    image_stamp = imlib_load_image (ficon_stamp.c_str());
    if (image_stamp) {
      imlib_context_set_image(image_stamp);
      stamp_w = imlib_image_get_width();
      stamp_h = imlib_image_get_height();
    }
  }

  image = imlib_load_image(ficon.c_str());
  if (image != NULL) {

    imlib_context_set_image(image);

    // imlib2 stores each image buffer as a sequence of ARGB objects (32 bits per pixel)
    // this log will emit this size to better profile kdesk's cache size setting.
    log4 ("Image filename, width, height, and RGBA buffer size",
          ficon.c_str(),
          imlib_image_get_width(), imlib_image_get_height(),
          (imlib_image_get_width() * imlib_image_get_height() * 32) / 8);

    w = imlib_image_get_width();
    h = imlib_image_get_height();
    subx = get_icon_horizontal_placement(w);

    // Icons contained in a grid are uniformly resized
    int neww = configuration->get_config_int ("gridiconwidth");
    int newh = configuration->get_config_int ("gridiconheight");
    if ((neww && newh) && (w != neww || h != newh) && configuration->get_icon_string(iconid, "relative-to") == "grid")
      {
	// Create a new image with the new uniformed size
	log5 ("Resizing grid icon (name, original-size, new-size", ficon.c_str(), w, h, neww, newh);
	imlib_context_set_anti_alias(1);
	resized = imlib_create_cropped_scaled_image (0, 0, w, h, neww, newh);
	imlib_context_set_image(resized);

	// swap the original image with the resized one, discard the original one.
	w = neww;
	h = newh;
	imlib_context_set_image(image);
	imlib_free_image();
	image = resized;
	imlib_context_set_image(image);
      }

    // Prepare the icon image for rendering
    imlib_context_set_drawable(win);
    Imlib_Color_Modifier cmHighlight;
    cmHighlight = imlib_create_color_modifier();
    imlib_context_set_color_modifier(cmHighlight);
    imlib_modify_color_modifier_brightness(0);
    imlib_context_set_anti_alias(1);
    imlib_context_set_blend(1);

    // If Icon transparency is provided, apply the mapping now, before rendering the image
    if (iconMapTransparency && transparency_value) {
      colorTrans = imlib_create_color_modifier();
      imlib_context_set_color_modifier(colorTrans);    
      imlib_get_color_modifier_tables(iconMapNone, iconMapNone, iconMapNone, iconMapTransparency);
      imlib_reset_color_modifier();

      // Create a transparency mapping based on kdeskrc setting
      for (int n=0; n < 256; n++) {
	if (iconMapTransparency[n]) {
	  iconMapTransparency[n] = transparency_value;
	}
      }

      imlib_set_color_modifier_tables (iconMapNone, iconMapNone, iconMapNone, iconMapTransparency);
    }

    // If we have a stamp icon, draw it on top of the icon
    if (image_stamp != NULL) {
      imlib_blend_image_onto_image (image_stamp, 1, 0, 0, stamp_w, stamp_h,
				    (w - stamp_w) / 2,
				    (h - stamp_h) / 2,
				    stamp_w, stamp_h);
    }

    // Draw the icon on the surface window, default is top-left.
    if (configuration->get_icon_string(iconid, "relative-to") == "grid") {
      // If it's inside a grid we will position it horizontally centered, and to the bottom.
        int gridx = (configuration->get_config_int ("gridwidth") > w ? 
                     (configuration->get_config_int ("gridwidth") - w) / 2 : 0);

        int gridy = (configuration->get_config_int ("gridheight") > h ?
                     configuration->get_config_int ("gridheight") - h : 0);

      log2 ("XXX and YYY", gridx, gridy);

      imlib_render_image_on_drawable (gridx, gridy);
    } 
    else {   
      imlib_render_image_on_drawable (subx + iconxmove, iconymove);
    }

    // Set context to stamped image so we can free it.
    if (image_stamp != NULL) {
      imlib_context_set_image(image_stamp);
      imlib_free_image();
    }

    // Free the color transformation if used
    if (colorTrans) {
      imlib_free_color_modifier();
    }

  } // if icon image could be loaded

  // Render the icon name below it, twice to create a shadow effect
  if (caption.length() > 0) {
    log1 ("Drawing icon caption", caption);
    int fx = (iconw - fontInfoCaption.width) / 2;
    int fy = iconh;
    if (configuration->get_config_string("shadow") == "true")
      {
	XftDrawStringUtf8( xftdraw1, &xftcolor_shadow, font, 
			   fx + shadowx, fy + shadowy + icontitlegap, 
			   (XftChar8 *) caption.c_str(), caption.size());
      }
    
    XftDrawStringUtf8 (xftdraw1, &xftcolor, font, 
		       fx, fy + icontitlegap,
		       (XftChar8 *) caption.c_str(), caption.size());
  }

  // Render the message information area, default is on the right side of the icon
  if (message.length() > 0 && font && fontsmaller) {

    log1 ("Drawing icon message", message);

    // If the message is dual-line (i.e. some\ntext), then split it in 2 messages
    // and use a smaller font to render the text below the first line.
    char *msg1 = strdup (message.c_str());
    char *msg2 = NULL;
    char *newline = NULL;

    // If we find the newline magic mark ( OR sign ), this is a dual-line message. Split it.
    newline = (char *) strstr (msg1, "|");
    if (newline != NULL) {
      *newline = 0x00;
      msg2 = newline + sizeof (char);
      log2 ("Message is dual-line (first, second)", msg1, msg2);
    }

    // Compute the message positioning
    XftTextExtentsUtf8 (display, font, (XftChar8*) msg1, strlen (msg1), &fontInfoMessage);
    int fx=0, fy=0;
    int xgap=5;      // used to avoid the text from blending with the icon when halign=right
    fy = h / 2;      // FIXME: This is not pixel-accurate
    if (subx > 0) {
      // Icon is aligned to the right - Align the message to the left of the icon
      fx = subx - fontInfoMessage.width - xgap;
    }
    else {
      fx = w + icontitlegap;
    }

    // Render the first line
    XftDrawStringUtf8 (xftdraw1, &xftcolor, font, fx, fy, (XftChar8 *) msg1, strlen (msg1));

    // Render the second line - try using a smaller font
    XGlyphInfo fiSmaller;
    if (msg2 != NULL) {
      XftTextExtentsUtf8 (display, fontsmaller, (XftChar8*) msg2, strlen(msg2), &fiSmaller);
      if (subx) {
	// The icon sits to the right, draw the text to the left.
	fx = subx - fiSmaller.width - xgap;
      }

      XftDrawStringUtf8 (xftdraw1, &xftcolor, fontsmaller ? fontsmaller : font, 
			 fx, fy + fontInfoMessage.height + 5,
			 (XftChar8 *) msg2, strlen (msg2));
    }

    free (msg1);

    // Automatically expand icon's width if message text
    // is aligned to the left, so that the "message" attribute is not cut.
    if (configuration->get_icon_string (iconid, "halign") == "left")
      {
	XWindowChanges xwc;
	memset (&xwc, 0x00, sizeof (xwc));
	int longest_text = fiSmaller.width > fontInfoMessage.width ? fiSmaller.width : fontInfoMessage.width;

	xwc.width = w + longest_text + 5;
	log2 ("Adjusting icon box width (icon, new width)", get_icon_name(), xwc.width);
	XConfigureWindow (display, win, CWWidth, &xwc);
      }
  }

  // save the current icon render so we can restore when mouse hovers out
  imlib_context_set_image (backsafe);
  imlib_context_set_drawable (win);
  imlib_copy_drawable_to_image (0, 0, 0, iconw, iconh, 0, 0, 1);
}

bool Icon::blink_icon(Display *display, XEvent ev)
{
  bool bsuccess = false;
  Imlib_Image original = imlib_load_image(ficon.c_str());
  Imlib_Color_Modifier colorMod=NULL;

  // If a second texture is provided, create a visual effect when mouse moves over the icon (hover effect)
  if (ficon_hover.length() > 0) {

    // start by laoding the second texture icon
    log1 ("drawing second texture icon", ficon_hover);
    Imlib_Image imghover = imlib_load_image (ficon_hover.c_str());
    
    // if blending is also requested (HoverTransparent) mix original icon with the second texture
    // with a transparency percentage specified by this same flag (0 will blend with desktop, 255 full opaque blend)
    if (configuration->get_icon_int (iconid, "hovertransparent") > 0) {

      // Set drawing operations using the original icon
      imlib_context_set_drawable(win);
      imlib_context_set_image(original);
      imlib_context_set_anti_alias(1);
      imlib_context_set_blend(1);
      
      // Create a color modifier which we'll use to blend both images
      colorMod = imlib_create_color_modifier();
      imlib_context_set_color_modifier(colorMod);

      if (iconMapNone && iconMapGlow) {
	imlib_get_color_modifier_tables(iconMapNone, iconMapNone, iconMapNone, iconMapGlow);
	imlib_reset_color_modifier();

	for (int n=0; n < 256; n++) {
	  if (iconMapGlow[n] > 127) {
	    // The value 127 shows me it smoothly blends both images without distorting their alphas
	    // The higher the value (hovertransparent), the more the textures blend.
	    iconMapGlow[n] = configuration->get_icon_int (iconid, "hovertransparent");
	  }
	}
      }
      else {
	log ("iconMapNone / iconMapGlow have not been allocated, no blending possible");
      }

      // Use the new modified color mapping, and blend the second texture on top of the original icon
      imlib_set_color_modifier_tables (iconMapNone, iconMapNone, iconMapNone, iconMapGlow);
      imlib_blend_image_onto_image (imghover, 1, 0, 0, iconw, iconh, 0, 0, iconw, iconh);
      
    } // if HoverTransparent
    else {

      // If there is no blending requested, just draw the second texture as the icon representation
      imlib_context_set_drawable(win);
      imlib_context_set_image(imghover);
    }
    
    int xoffset = configuration->get_icon_int (iconid, "hoverxoffset");
    int yoffset = configuration->get_icon_int (iconid, "hoveryoffset");

    // Account for icons with HAlign=right
    int image_width = imlib_image_get_width();
    int subx = get_icon_horizontal_placement(image_width);

    // If the icon is in a grid, the hover icon will be on top, horizontally centered
    if (configuration->get_icon_string(iconid, "relative-to") == "grid") {
      int horzx = (configuration->get_config_int ("gridwidth") - image_width) / 2;
      if (horzx > configuration->get_config_int ("gridwidth")) {
	horzx = 0; // rectify possible wrong hover icons that are wider than the grid
      }
      imlib_render_image_on_drawable (horzx, 0);
    }
    else {
      imlib_render_image_on_drawable (xoffset + subx, yoffset);
    }

    if (colorMod) {
      // The color modifier might have been used during texture blending
      imlib_free_color_modifier();
    }

    imlib_free_image();
    bsuccess = true;
  }

  return bsuccess;
}

bool Icon::unblink_icon(Display *display, XEvent ev)
{
  // Mouse is moving out of the icon
  // smoothly restore the original rendered icon.
  log ("smoothly restoring original rendered icon");
  imlib_context_set_image(backsafe);
  imlib_context_set_drawable(win);
  imlib_render_image_on_drawable(0, 0);
  return true;
}

bool Icon::motion(Display *display, XEvent ev)
{
  return true;
}

Window Icon::find_icon_window (Display *display, std::string appid)
{
  Window wmax=0L;
  Window returnedroot, returnedparent, root = DefaultRootWindow(display);
  char *windowname=NULL;
  Window *children=NULL, *subchildren=NULL;
  unsigned int numchildren, numsubchildren;
  XClassHint classHint;

  // sanity check
  if (!appid.length()) {
    return 0L;
  }

  // Remove the AppID delimiters
  // TODO: Remove this code eventually, as it's obsolete legacy icon syntax 
  // for when we used the "pgrep" strategy, for example, 
  // AppID: pcmanf[m] was used to avoid pgrep finding himself.
  //
  appid.erase (std::remove(appid.begin(), appid.end(), '['), appid.end());
  appid.erase (std::remove(appid.begin(), appid.end(), ']'), appid.end());

  log1 ("Searching for Icon Window from Appid string match", appid);

  // Enumarate top-level windows in search for Kdesk control window
  XQueryTree (display, root, &returnedroot, &returnedparent, &children, &numchildren);
  for (int i=numchildren-1; i>=0 && !wmax; i--)
    {
      // enumerate child windows from each top-level
      XQueryTree (display, children[i], &returnedroot, &returnedparent, &subchildren, &numsubchildren);
      for (int k=numsubchildren-1; k>=0 && !wmax; k--) 
	{
	  // Get Class Hint, window title, along with _NET_WM_ICON_GEOMETRY
	  // If the hint's class name or window title matches AppID...
	  windowname = NULL;
	  classHint.res_name = classHint.res_class = NULL;
	  XFetchName (display, subchildren[k], &windowname);
	  XGetClassHint (display, subchildren[k], &classHint);
	  
	  if ( (classHint.res_name && !strncasecmp (classHint.res_name, appid.c_str(), strlen (appid.c_str()))) ||
	       (windowname && !strncasecmp (windowname, appid.c_str(), strlen (appid.c_str()))) )
	    {
	      // And the window's Icon Geometry returns 4 leftover LONGs,
	      // This means that's the window associated with AppID and it's
	      // represented with an icon on the desktop, which also means it accepts movement events (restore, maximize, ...)
	      unsigned long nitems=0L;
	      unsigned long leftover=0L;
	      Atom xa_IconGeometry, actual_type;
	      int actual_format;
	      int status=-1;
	      unsigned char *p = NULL;
	      xa_IconGeometry = XInternAtom(display, "_NET_WM_ICON_GEOMETRY", false);
	      status = XGetWindowProperty (display, subchildren[k],
					   xa_IconGeometry, 0L, sizeof(unsigned long) * 64,
					   false, xa_IconGeometry, &actual_type, &actual_format,
					   &nitems, &leftover, &p);
	      if (status == Success) {
		if (leftover == 4) {
		  log2 ("Icon app window was found (Appid, WindowID)", appid, subchildren[k]);
		  wmax = subchildren[k];
		}

		if (p) {
		  XFree (p);
		}
	      }
	    }

	  if (windowname) {
	    XFree (windowname);
	  }

	  if (classHint.res_name) {
	    XFree (classHint.res_name);
	  }

	  if (classHint.res_class) {
	    XFree (classHint.res_class);
	  }
	}
    }

  if (children) {
    XFree (children);
  }
  
  if (subchildren) {
    XFree (subchildren);
  }

  return wmax;
}

bool Icon::maximize(Display *display, Window win)
{
  if (!win) {
    return false;
  }

  // Map window on the desktop
  XMapWindow(display, win);
  XRaiseWindow (display, win);

  // Ask window to become in "normal" state, this causes an un-minimize if it currently is
  XClientMessageEvent ev;
  Atom prop;
  prop = XInternAtom (display, "WM_CHANGE_STATE", False);
  if (prop != None) {
    ev.type = ClientMessage;
    ev.window = win;
    ev.message_type = prop;
    ev.format = 32;
    ev.data.l[0] = NormalState;
    XSendEvent (display, DefaultRootWindow (display), False, SubstructureRedirectMask | SubstructureNotifyMask, (XEvent *) &ev);
  }
  
  XFlush (display);
  return true;
}

bool Icon::maximize(Display *display)
{
  // Find the main Window ID of the icon's application ID,
  // AppID, found in the icon LNK file
  // then give focus to it.
  static bool fmaximizing=false;
  bool fdone=false;
  
  if (fmaximizing == true) {
    return fdone;
  }
  else {
    string appid = configuration->get_icon_string (iconid, "appid");
    Window wmaximize = find_icon_window (display, appid);
    if (wmaximize) {
      log2 ("found window to maximize (appid, window)", appid, wmaximize);
      fdone = maximize (display, wmaximize);
    }
    else {
      log1 ("Appid to Maximize was not found", appid);
    }
  }

  fmaximizing = false;
  return fdone;
}

bool Icon::double_click(Display *display, XEvent ev)
{
  bool success = false;
  string filename = configuration->get_icon_string (iconid, "filename");
  string command  = get_commandline();
  
  bool isrunning = is_singleton_running (display);
  if (isrunning == true) {
    log1 ("not starting app - it's a running singleton", filename);
  }
  else
    {
      // Launch the icon's appplication asynchronously,
      // Set the status to starting, so that icons get disabled
      // Until the app is up and running
      pid_t pid = fork();
      if (pid == 0) {
	// we are in the new forked process
	setsid ();
	int rc = execl ("/bin/sh", "/bin/sh", "-c", command.c_str(), 0);
	if (rc == -1)
	  // We are in the child process
	  {
	    log2 ("error starting app (rc, command)", rc, command.c_str());
	    _exit (1); // rather than exit() because the latter could interfere with parent's atexit()
	  }
	
	log1 ("app has finished", filename);
	_exit (0);
      }
      else if (pid == -1) {
	log1 ("fork call failed, could not start app (errno)", errno);
      }
      else {
	// we are in the parent process, tell the kernel we don't bother about its termination
	signal(SIGCHLD, SIG_IGN);

	success = true;
	log2 ("app has been started (pid, icon)", pid, filename);
      }
    }

  return success;
}
