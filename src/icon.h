//
//  icon.h
//
// Copyright (C) 2013-2014 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
// An app to show and bring life to Kano-Make Desktop Icons.
//

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <Imlib2.h>
#include <X11/cursorfont.h>

#include <string.h>

#include "configuration.h"

// Default icon cursor when mouse moves over the icon
// 
#define DEFAULT_ICON_CURSOR XC_hand1

class Icon
{
 private:
  Configuration *configuration;
  Display *icon_display;
  Window win;
  int iconx, icony, iconw, iconh;
  int shadowx, shadowy;
  int icontitlegap;
  int transparency_value;
  Cursor cursor;
  int cursor_id;
  Imlib_Updates updates;
  Imlib_Image image, image_stamp;
  Imlib_Image backsafe;
  Visual *vis;
  Colormap cmap;
  XftFont *font;
  XftFont *fontsmaller;
  XGlyphInfo fontInfoCaption, fontInfoMessage;
  XftDraw *xftdraw1, *xftdraw2;
  XftColor xftcolor, xftcolor_shadow;
  unsigned char *iconMapNone, *iconMapGlow, *iconMapTransparency;
  std::string ficon;
  std::string ficon_hover;
  std::string ficon_stamp;
  std::string caption;
  std::string message;

 public:
  int iconid;

  Icon (Configuration *loaded_conf, int iconidx);
  virtual ~Icon (void);

  int get_iconid(void);
  std::string get_icon_filename(void);
  int get_icon_horizontal_placement (int image_width);
  bool is_singleton_running (void);

  Window create(Display *display);
  void destroy(Display *display);

  void draw(Display *display, XEvent ev);
  void clear(Display *display, XEvent ev);
  bool blink_icon(Display *display, XEvent ev);
  bool unblink_icon(Display *display, XEvent ev);
  bool double_click(Display *display, XEvent ev);
  bool motion(Display *display, XEvent ev);

  void set_caption (char *new_caption);
  void set_message (char *new_message);
  void set_icon (char *new_icon);
  void set_icon_stamp (char *new_icon);

};
