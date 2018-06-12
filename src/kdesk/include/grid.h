//
// grid.h
//
// Copyright (C) 2013-2014-2015 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
//

#include <vector>
#include <string>

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#define DEFAULT_GRID_WIDTH   128
#define DEFAULT_GRID_HEIGHT  128

#define DEFAULT_ICON_HORZ_SPACE   50
#define DEFAULT_ICON_VERT_SPACE   25

class IconGrid
{
  private:
    int VERT_SPC;
    int HORZ_SPC;

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
    bool get_real_position(int field_x, int field_y, int *real_x, int *real_y, int *gridx, int *gridy);

  public:
    IconGrid(Display *display, Configuration *pconf);
    ~IconGrid(void);

    static int ICON_W;
    static int ICON_H;

    bool grid_full;

    bool request_position(int field_hint_x, int field_hint_y, int *x, int *y, int *gridx, int *gridy);
    bool free_space_used(int x, int y);
};
