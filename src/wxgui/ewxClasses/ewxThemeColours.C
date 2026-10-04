/**
 * @file
 *
 * Light and dark variants of the colours that carry meaning (#210).
 */

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/settings.h>

#include "wxgui/ewxThemeColours.H"

// [status][light, dark] -- checked by tests/look/contrast.py.
static const char* STATUS_TEXT[3][2] = {
  {"#00731f", "#6fd16f"},   // GOOD
  {"#8a5a00", "#ffb84d"},   // UNSURE
  {"#b00000", "#ff8080"},   // BAD
};

static const char* STATUS_TINT[3][2] = {
  {"#dff5df", "#1e4620"},   // GOOD
  {"#fff1cc", "#4d3a10"},   // UNSURE
  {"#ffdddd", "#5c1f1f"},   // BAD
};


bool ewxThemeColours::isDark()
{
  return wxSystemSettings::GetAppearance().IsDark();
}


wxColour ewxThemeColours::statusText(Status status)
{
  return wxColour(STATUS_TEXT[status][isDark() ? 1 : 0]);
}


wxColour ewxThemeColours::statusTint(Status status)
{
  return wxColour(STATUS_TINT[status][isDark() ? 1 : 0]);
}


wxColour ewxThemeColours::alternateRow()
{
  // A step from the view's base colour toward its text, so it reads as a
  // band in either theme without a fixed grey.
  wxColour base = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX);
  return base.ChangeLightness(isDark() ? 112 : 95);
}
