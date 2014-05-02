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

#include "icon.h"
#include "logging.h"

Icon::Icon (Configuration *loaded_conf, int iconidx)
{
  configuration = loaded_conf;
  iconid = iconidx;
  iconx=icony=iconw=iconh=0;
  shadowx=shadowy=0;
  iconMapNone = iconMapGlow = (unsigned char *) NULL;
  backsafe = NULL;
  font = fontsmaller = NULL;
  win = 0;

  // save the icon, icon hover image files
  ficon = configuration->get_icon_string (iconid, "icon");
  ficon_hover = configuration->get_icon_string (iconid, "iconhover");  
  
  // save the icon caption and message literals to be rendered around it
  caption = configuration->get_icon_string (iconid, "caption");
  message = configuration->get_icon_string (iconid, "message");

  // Initially we don't know yet which display we are bound to until create()
  icon_display = NULL;

  // default font details should be zero
  fontInfoCaption.height = fontInfoCaption.width = 0;
  fontInfoMessage.height = fontInfoMessage.width = 0;

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

  // FIXME: We are not using transparency at all, superseeded by icon blending?
  // I will be conservative and keep it here.
  unsigned int transparency = configuration->get_config_int("transparency");
  if (!transparency) {
    // Setting transparency to 1 has no visual effect
    transparency = 1;
  }

  // Define the default cursor for the mouse pointer
  // Or change to a custom one specified in the config file
  cursor_id = configuration->get_config_int("mousehovericon");
  if (cursor_id == 0) {
    cursor_id = DEFAULT_ICON_CURSOR;
  }

  cursor = 0; // Set the initial cursor handle to nothing
}

Icon::~Icon (void)
{
}

int Icon::get_iconid(void)
{
  return iconid;
}

std::string Icon::get_icon_filename(void)
{
  return configuration->get_icon_string (iconid, "filename");
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

bool Icon::is_singleton_running (void)
{
  bool bAppRunning=false;
  string appid = configuration->get_icon_string (iconid, "appid");
  string singleton = configuration->get_icon_string (iconid, "singleton");

  if (singleton == "true" && appid.size())
    {
      unsigned int rc=0, exitstatus=0;
      char cmdpgrep[512];

      log1 ("searching for singleton app", appid);
      memset(cmdpgrep, 0, sizeof(cmdpgrep));

      sprintf (cmdpgrep, "pgrep -fl '%s'", appid.c_str());
      rc = system (cmdpgrep);
      exitstatus = WEXITSTATUS(rc);
      if (exitstatus == 0) {
	bAppRunning = true;
	log1 ("Application is running", appid);
      }
      else {
	bAppRunning = false;
	log1 ("Application is NOT running", appid);
      }
    }
  return bAppRunning;
}

Window Icon::create (Display *display)
{
  unsigned int rc=0;
  int border;

  // save the display variable for later cleanup
  icon_display = display;
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
      
      fontsmaller = XftFontOpen (display, DefaultScreen(display),
				 XFT_FAMILY, XftTypeString, fontname.c_str(),
				 XFT_SIZE, XftTypeDouble, (float) fontsize - 2,
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

  iconx = configuration->get_icon_int (iconid, "x");
  icony = configuration->get_icon_int (iconid, "y");
  iconw = configuration->get_icon_int (iconid, "width");
  iconh = configuration->get_icon_int (iconid, "height");

  // Using this parameter we can control the space
  // between the icon and name rendered just below
  icontitlegap = configuration->get_config_int("icontitlegap");
  log1 ("Icon gap for font title rendering", icontitlegap);

  // Decide which icon positioning to use on the desktop
  string icon_placement = configuration->get_icon_string(iconid, "relative-to");
  if (icon_placement == "bottom-centre") {
    iconx = w / 2 + iconx;
    icony = h + icony;
  }
  else if (icon_placement == "top-left") {
    // no coordinate transformation necessary. 0,0 is already top-left
    ;
  }
  else if (icon_placement == "top-right") {
    // icon horizontal position decreases from the right to the left
    iconx = w - (iconx + iconw);
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
  }

  if (iconMapGlow) {
    free (iconMapGlow);
  }

  if (cursor) {
    XFreeCursor (icon_display, cursor);
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

void Icon::draw(Display *display, XEvent ev)
{
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

  imlib_context_set_drawable(win);
  image = imlib_load_image(ficon.c_str());

  Imlib_Color_Modifier cmHighlight;
  cmHighlight = imlib_create_color_modifier();
  imlib_context_set_color_modifier(cmHighlight);
  imlib_modify_color_modifier_brightness(0);

  imlib_context_set_anti_alias(1);
  imlib_context_set_blend(1);
  imlib_context_set_image(image);
  int w = imlib_image_get_width();
  int h = imlib_image_get_height();

  // Draw the icon on the surface window, default is top-left.
  int subx = get_icon_horizontal_placement(w);
  imlib_render_image_on_drawable (subx, 0);
  updates = imlib_update_append_rect (updates, subx, 0, w, h);
  imlib_free_image();

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
  if (message.length() > 0) {

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
    fy = h / 2;     // FIXME: This is not pixel-accurate
    if (subx > 0) {
      // Icon is aligned to the right - Align the message to the left of the icon
      fx = subx - fontInfoMessage.width;
    }
    else {
      fx = w + icontitlegap;
    }

    // Render the first line
    XftDrawStringUtf8 (xftdraw1, &xftcolor, font, fx, fy, (XftChar8 *) msg1, strlen (msg1));

    // Render the second line - try using a smaller font
    if (msg2 != NULL) {
      XGlyphInfo fiSmaller;
      XftTextExtentsUtf8 (display, fontsmaller, (XftChar8*) msg2, strlen(msg2), &fiSmaller);
      if (subx) {
	// The icon sits to the right, draw the text to the left.
	fx = subx - fiSmaller.width;
      }

      XftDrawStringUtf8 (xftdraw1, &xftcolor, fontsmaller ? fontsmaller : font, 
			 fx, fy + fontInfoMessage.height + 5,
			 (XftChar8 *) msg2, strlen (msg2));
    }

    free (msg1);
  }

  // save the current icon render so we can restore when mouse hovers out
  imlib_context_set_image(backsafe);
  imlib_context_set_drawable(win);
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
    int subx = get_icon_horizontal_placement(imlib_image_get_width());
    imlib_render_image_on_drawable (xoffset + subx, yoffset);

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

bool Icon::double_click(Display *display, XEvent ev)
{
  bool success = false;
  string filename = configuration->get_icon_string (iconid, "filename");
  string command  = configuration->get_icon_string (iconid, "command");
  
  bool isrunning = is_singleton_running();
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
	  {
	    log2 ("error starting app (rc, command)", rc, command.c_str());
	    exit(1);
	  }
	
	log1 ("app has finished", filename);
	exit(0);
      }
      else if (pid == -1) {
	log1 ("fork call failed, could not start app (errno)", errno);
      }
      else {
	// we are in the parent process, wait for it to 
	success = true;
	log2 ("app has been started (pid, icon)", pid, filename);
      }
    }

  return success;
}
