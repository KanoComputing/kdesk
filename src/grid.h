//
// grid.h
//
// Copyright (C) 2013-2014 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
//

#include <vector>
#include <string>

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

class IconGrid
{
  private:
    static const int ICON_W = 128;
    static const int ICON_H = 128;

    static const int VERT_SPC = 10;
    static const int HORZ_SPC = 10;

    static const int MARGIN_BOTTOM = 84;
    static const int MARGIN_TOP = 50;

    static const int MAX_FIELDS_X = 7;

    typedef std::vector<std::pair<int, int> > coord_list_t;

    int width;
    int height;
    coord_list_t used_fields;

    int start_x;
    int start_y;

    bool is_place_used(int x, int y);
    void get_real_position(int field_x, int field_y, int *real_x, int *real_y);

  public:
    IconGrid(Display *display);
    ~IconGrid(void);

    bool request_position(int field_hint_x, int field_hint_y, int *x, int *y);
};
