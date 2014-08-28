//
// grid.cpp
//
// Copyright (C) 2013-2014 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//

#include "configuration.h"
#include "grid.h"
#include "logging.h"


// Static non-const member variable need be defined at file scope
int IconGrid::ICON_W;
int IconGrid::ICON_H;

/* IconGrid */
IconGrid::IconGrid(Display *display, Configuration *pconf)
{
  int screen_num = DefaultScreen(display);
  int w = DisplayWidth(display, screen_num);
  int h = DisplayHeight(display, screen_num);

  grid_full = false;

  // Get the grid dimensions from kdeskrc file
  if (pconf) {
    ICON_W = pconf->get_config_int("gridwidth");
    ICON_H = pconf->get_config_int("gridheight");
  }

  // Or set default values if they're not defined
  if (!ICON_W) {
    ICON_W = DEFAULT_GRID_WIDTH;
  }

  if (!ICON_H) {
    ICON_H = DEFAULT_GRID_HEIGHT;
  }

  log2 ("Icon grid width and height", ICON_W, ICON_H);

  width = w / ICON_W;
  if (width > MAX_FIELDS_X)
    width = MAX_FIELDS_X;

  height = (h - MARGIN_TOP - MARGIN_BOTTOM) / ICON_H;

  start_x = w/2 - (width * (ICON_W + HORZ_SPC))/2;
  start_y = h - MARGIN_BOTTOM;
}

IconGrid::~IconGrid()
{
  /* Nothing here yet */
}


bool IconGrid::is_place_used(int x, int y)
{
  for (coord_list_t::iterator it = used_fields.begin();
      it != used_fields.end(); it++) {
    if (it->first == x && it->second == y) {
      return true;
    }
  }
  return false;
}

bool IconGrid::free_space_used(int x, int y)
{
  for (coord_list_t::iterator it = used_fields.begin();
      it != used_fields.end(); it++) {

    if (it->first == x && it->second == y) {
      used_fields.erase(it);
      return true;
    }
  }
  return false;
}

bool IconGrid::get_real_position(int field_x, int field_y,
                                 int *real_x, int *real_y, int *gridx, int *gridy)
{
  *real_x = start_x + field_x * (ICON_W + HORZ_SPC);
  *real_y = start_y - (1 + field_y) * (ICON_H + VERT_SPC);

  // Because the grid grows from bottom to top (0,0 being top left corner of screen)
  // if the real icon position represented on the screen falls beyond limits, reject the icon
  if (*real_y <= MARGIN_TOP) {
    grid_full = true;
    return false;
  }
  else {
    if (gridx) *gridx = field_x;
    if (gridy) *gridy = field_y;
    used_fields.push_back(std::pair<int, int>(field_x, field_y));
    return true;
  }
}

bool IconGrid::request_position(int field_hint_x, int field_hint_y,
                                int *x, int *y, int *gridx, int *gridy)
{
  if (field_hint_x >= 0 && field_hint_x < width &&
      field_hint_y >= 0 && field_hint_y < height) {
    if (!is_place_used(field_hint_x, field_hint_y)) {
      return (get_real_position(field_hint_x, field_hint_y, x, y, gridx, gridy));
    }
  }

  /* find a free spot */
  for (int iy = 0; iy < height; iy++) {
    for (int ix = 0; ix < width; ix++) {
      if (!is_place_used(ix, iy)) {
        return (get_real_position(ix, iy, x, y, gridx, gridy));
      }
    }
  }

  grid_full = true;
  return false;
}
