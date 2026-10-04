/**
 * @file
 *
 * Colours that carry meaning, chosen to suit the GTK theme in use (#210).
 *
 * Everything else takes the theme's own colours.  Each colour here has a
 * light and a dark variant; tests/look/contrast.py reads the tables in
 * ewxThemeColours.C and fails if either variant drops below 4.5:1.
 */
#ifndef _EWXTHEMECOLOURS_H
#define _EWXTHEMECOLOURS_H

#include "wx/colour.h"

namespace ewxThemeColours
{
  enum Status { GOOD = 0, UNSURE = 1, BAD = 2 };

  /** True when the theme draws light text on a dark background. */
  bool isDark();

  /** Text in a status colour, on the theme's window or view background. */
  wxColour statusText(Status status);

  /** A background tint for a status, under the theme's own text colour. */
  wxColour statusTint(Status status);

  /** Alternate-row background for lists and grids. */
  wxColour alternateRow();
}

#endif
