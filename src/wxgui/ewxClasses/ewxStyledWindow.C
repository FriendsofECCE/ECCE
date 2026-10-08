/**
 * @file
 *
 *
 */

/*
#include <iostream>
 using std::cout;
 using std::endl;
*/

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/list.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>
#include <wx/treectrl.h>
#include <wx/listctrl.h>
#include <wx/settings.h>
#include <wx/ctrlsub.h>
#include <wx/grid.h>
#include <wx/dataview.h>
#include <wx/timer.h>
#include <wx/treelist.h>
#include <cstdio>
#include <cstdlib>

#include "util/Color.H"
#include "util/Preferences.H"
#include "util/PreferenceLabels.H"

#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxColor.H"
#include "wxgui/ewxComboBox.H"
#include "wxgui/ewxListBox.H"
#include "wxgui/ewxNonBoldLabel.H"
#include "wxgui/ewxSpinCtrl.H"
#include "wxgui/ewxSmallLabel.H"
#include "wxgui/ewxStyledWindow.H"
#include "wxgui/ewxThemeColours.H"


Preferences *ewxStyledWindow::p_prefs = 0;

/**
 * Constructor.
 */

// Debug aid, inert unless ECCE_FONT_DUMP names a file: every few seconds it
// appends the point size of each tree, list, grid and table control, with a
// button's size for reference, so a control outside the shared font shows up.
static void dumpFontsOf(wxWindow *win, FILE *out)
{
   const wxString name = win->GetClassInfo()->GetClassName();
   if (dynamic_cast<wxTreeCtrl*>(win) || dynamic_cast<wxListCtrl*>(win) ||
       dynamic_cast<wxGrid*>(win) || dynamic_cast<wxDataViewCtrl*>(win) ||
       dynamic_cast<wxTreeListCtrl*>(win) || dynamic_cast<wxListBox*>(win) ||
       dynamic_cast<wxButton*>(win)) {
      fprintf(out, "%s %s %d\n", (const char*)name.utf8_str(),
              win->GetName().utf8_str().data(), win->GetFont().GetPointSize());
      if (wxGrid *grid = dynamic_cast<wxGrid*>(win))
         fprintf(out, "  cell %d label %d\n",
                 grid->GetDefaultCellFont().GetPointSize(),
                 grid->GetLabelFont().GetPointSize());
   }
   wxWindowList kids = win->GetChildren();
   for (wxWindowList::compatibility_iterator n = kids.GetFirst(); n;
        n = n->GetNext())
      dumpFontsOf(n->GetData(), out);
}

namespace {
class FontDumpTimer : public wxTimer
{
public:
   void Notify() override
   {
      FILE *out = fopen(getenv("ECCE_FONT_DUMP"), "a");
      if (!out) return;
      fprintf(out, "--\n");
      for (wxWindowList::compatibility_iterator n = wxTopLevelWindows.GetFirst();
           n; n = n->GetNext())
         dumpFontsOf(n->GetData(), out);
      fclose(out);
   }
};
}

static void startFontDump()
{
   static FontDumpTimer *timer = 0;
   if (timer || !getenv("ECCE_FONT_DUMP") || !wxTheApp) return;
   timer = new FontDumpTimer;
   timer->Start(4000);
}

ewxStyledWindow::ewxStyledWindow()
{
   startFontDump();
   if (p_prefs == 0) p_prefs = new Preferences(PrefLabels::GLOBALPREFFILE);
}

ewxStyledWindow::ewxStyledWindow(wxWindow *win, bool recursive)
{
   startFontDump();
   if (p_prefs == 0) p_prefs = new Preferences(PrefLabels::GLOBALPREFFILE);
   setStyles(win, recursive);
}


/**
 * Destructor.
 */
ewxStyledWindow::~ewxStyledWindow()
{
}

// Colours come from the GTK theme (#210), so a dark theme and the
// desktop's own palette apply; fixed ECCE colours are kept only where a
// colour carries meaning (see ewxThemeColours.H).
wxColour ewxStyledWindow::getWindowColor()
{
  return wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
}

// Read-only fields take the window colour, so they stand apart from the
// view-coloured editable ones in light and dark themes alike.
wxColour ewxStyledWindow::getReadonlyColor()
{
  return wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
}

wxColour ewxStyledWindow::getButtonColor()
{
  return wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
}


wxColour ewxStyledWindow::getInputColor()
{
  return wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
}


wxColour ewxStyledWindow::getTextColor()
{
  return wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
}


wxColour ewxStyledWindow::getBtn3DDkShadowColor()
{
  return wxSystemSettings::GetColour(wxSYS_COLOUR_3DDKSHADOW);
}


wxColour ewxStyledWindow::getBtn3DDkLightColor()
{
  return wxSystemSettings::GetColour(wxSYS_COLOUR_BTNHILIGHT);
}


wxColour ewxStyledWindow::getFocusedSelectionColor()
{
  return wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
}


wxColour ewxStyledWindow::getUnfocusedSelectionColor()
{
  return wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
}


wxColour ewxStyledWindow::getHightLightRowColor()
{
  return ewxThemeColours::alternateRow();
}


/**
 * Points to add to the theme's font size for the Font Size preference
 * (0 small, 1 medium, 2 large, 3 extra large).  Medium is the theme's
 * own size, so a default install draws exactly what GTK draws.
 */
int ewxStyledWindow::getFontSizeStep()
{
  int pref = 1;
  p_prefs->getInt("FONTSIZE", pref);
  switch (pref) {
    case 0:  return -1;
    case 2:  return 2;
    case 3:  return 4;
    default: return 0;
  }
}


// Fonts derive from the theme's GUI font (#210); the base sizes are the
// old ones relative to its 10pt normal size.
int ewxStyledWindow::getFontSize(int basesize)
{
  return getBaseFontSize() + (basesize - 10) + getFontSizeStep();
}


// ECCE's windows are dense: the theme's family at 10 pt, unless the
// "Use the system font size" preference asks for the desktop's size.
int ewxStyledWindow::getBaseFontSize()
{
  bool useSystem = false;
  p_prefs->getBool(PrefLabels::USESYSTEMFONT, useSystem);
  if (useSystem)
    return wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetPointSize();
  return 10;
}


static wxFont themeFont(int size, wxFontWeight weight)
{
  wxFont font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
  font.SetPointSize(size);
  font.SetWeight(weight);
  return font;
}


// Bold only for callers that mark a heading or a label role.
wxFont ewxStyledWindow::getBoldFont()
{
   return themeFont(getFontSize(10), wxFONTWEIGHT_BOLD);
}


wxFont ewxStyledWindow::getNormalFont()
{
   return themeFont(getFontSize(10), wxFONTWEIGHT_NORMAL);
}


wxFont ewxStyledWindow::getUnitFont()
{
   return themeFont(getFontSize(9), wxFONTWEIGHT_NORMAL);
}


wxFont ewxStyledWindow::getMonoSpaceFont()
{
   return wxFont(wxFontInfo(getFontSize(10)).Family(wxFONTFAMILY_TELETYPE));
}


// The following three fonts are used by pertable
wxFont ewxStyledWindow::getAtomicNumFont()
{
   return themeFont(getFontSize(9), wxFONTWEIGHT_NORMAL);
}


wxFont ewxStyledWindow::getBigAtomicSymbolFont()
{
   return themeFont(getFontSize(18), wxFONTWEIGHT_BOLD);
}


wxFont ewxStyledWindow::getSmallLabelFont()
{
   return themeFont(getFontSize(9), wxFONTWEIGHT_NORMAL);
}


void ewxStyledWindow::setStyles(wxWindow *win, bool recursive)
{
#ifdef __WXMAC__
  return;
#endif
   // Only the font size preference and the read-only marker apply here; every other colour
   // is left to the GTK theme (#210).  wxNullColour hands a field back to
   // the theme when it becomes editable again.
   if (wxTextCtrl *text = dynamic_cast<wxTextCtrl*>(win)) {
      applyFont(text);
      setReadonlyMarker(win, !text->IsEditable());

   } else if (dynamic_cast<wxButton*>(win)) {
      applyFont(win);

   } else if (dynamic_cast<wxCheckBox*>(win)) {
      applyFont(win);
      // GTK3 paints a checkbox's background as a band across its label,
      // so a read-only checkbox gets no marker.
      win->SetBackgroundColour(wxNullColour);

   } else if (dynamic_cast<wxChoice*>(win)) {
      applyFont(win);
      ewxChoice *choice = dynamic_cast<ewxChoice*>(win);
      if (choice) setReadonlyMarker(win, !choice->IsEditable());

   } else if (dynamic_cast<wxComboBox*>(win)) {
      applyFont(win);
      ewxComboBox *box = dynamic_cast<ewxComboBox*>(win);
      if (box) setReadonlyMarker(win, !box->IsEditable());

   } else if (dynamic_cast<wxMenuBar*>(win)) {
      applyFont(win);

   } else if (dynamic_cast<wxScrolledWindow*>(win)) {
      applyFont(win);

   } else if (dynamic_cast<wxMenu*>(win)) {
      applyFont(win);

   } else if (ewxSpinCtrl *spin = dynamic_cast<ewxSpinCtrl*>(win)) {
      applyFont(win);
      setReadonlyMarker(win, !spin->IsEnabled());

   } else if (dynamic_cast<wxDialog*>(win)) {
      applyFont(win);

   } else if (dynamic_cast<wxStaticLine*>(win)) {
      // Many section-heading lines are stretched in both directions, and
      // GTK3 fills a separator's whole box with its colour; keep the box
      // clear, as the fixed window colour used to.
      win->SetBackgroundColour(wxTransparentColour);

   } else if (dynamic_cast<ewxNonBoldLabel*>(win)) {
      applyFont(win);

   } else if (dynamic_cast<ewxSmallLabel*>(win)) {
      win->SetFont(getSmallLabelFont());

   } else if (dynamic_cast<wxStaticText*>(win)) {
      applyFont(win);

   } else if (dynamic_cast<wxFrame*>(win)) {
      applyFont(win);
#ifdef __WXMSW__
      // wxMSW gives a frame the APPWORKSPACE grey; the editors place their
      // controls directly on the frame (no wxPanel), so it shows.
      if (win->GetBackgroundColour() ==
          wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE))
         win->SetBackgroundColour(
               wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
#endif

   } else if (dynamic_cast<wxNotebook*>(win)) {
      applyFont(win);

   } else if (dynamic_cast<wxListBox*>(win)) {
      applyFont(win);
      ewxListBox *list = dynamic_cast<ewxListBox*>(win);
      if (list) setReadonlyMarker(win, !list->IsEditable());

   } else if (dynamic_cast<wxListCtrl*>(win)) {
      applyFont(win);

   } else if (dynamic_cast<wxRadioBox*>(win)) {
      applyFont(win);

   } else if (dynamic_cast<wxTreeCtrl*>(win)) {
      applyFont(win);

   // Controls that are not wxScrolledWindows and would keep the system size.
   } else if (wxGrid *grid = dynamic_cast<wxGrid*>(win)) {
      applyFont(win);
      wxFont font = win->GetFont();
      grid->SetDefaultCellFont(font);
      grid->SetLabelFont(font);

   } else if (dynamic_cast<wxDataViewCtrl*>(win) ||
              dynamic_cast<wxTreeListCtrl*>(win)) {
      applyFont(win);
   }

   //cout << ", " << recursive << ")" << endl;
   if (recursive) setChildStyles(win);
}


// The theme's family at ECCE's base size plus the Font Size step.
void ewxStyledWindow::applyFont(wxWindow *win)
{
   wxFont font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
   font.SetPointSize(getBaseFontSize() + getFontSizeStep());
   win->SetFont(font);
}


void ewxStyledWindow::setReadonlyMarker(wxWindow *win, bool readonly)
{
   win->SetBackgroundColour(readonly ? getReadonlyColor() : wxNullColour);
}


/**
 * Every drop-down is at least as wide as its widest entry, in the current
 * font, so no entry is cut off; a larger width given at construction is
 * kept.  See docs/claude/wx-viewer/drop-down-min-width.md.
 */
void ewxStyledWindow::fitDropDown(wxWindow *ctrl,
                                  wxItemContainerImmutable *items,
                                  int explicitWidth, const wxString& value)
{
   int widest = value.empty() ? 0 : ctrl->GetTextExtent(value).x;
   for (unsigned int i = 0; i < items->GetCount(); i++) {
      widest = wxMax(widest, ctrl->GetTextExtent(items->GetString(i)).x);
   }
   if (widest == 0) return;
   // Room for the entry's padding and the drop-down arrow.  Not
   // GetSizeFromTextSize(): wxGTK returns garbage for a combo box there.
   int width = widest + 3*ctrl->GetCharHeight();
   wxSize min = ctrl->GetMinSize();
   if (width < explicitWidth) width = explicitWidth;
   if (width != min.x) {
      ctrl->SetMinSize(wxSize(width, min.y));
      ctrl->InvalidateBestSize();
   }
}


void ewxStyledWindow::setChildStyles(wxWindow *win)
{
   wxWindowList children = win->GetChildren();
   wxWindowList::compatibility_iterator node = children.GetFirst();
   while (node) {
      wxWindow *child = (wxWindow*)node->GetData();
   //cout << "setChild " << win->GetLabel() << " " << win->GetClassInfo()->GetClassName() << endl;
      setStyles(child, true);
      node = node->GetNext();
   }
}


/**
 * Set widget to our custom disabled style.
 * @param enabled true to enable, false to disable
 * Default implementation is to do nothing.
 */
void ewxStyledWindow::setCustomDisabledStyle(bool enabled)
{
}


/**
 * Draw a button border around the dc.
 * Based on the state, decide the color scheme.
 * Only support width = 2 or 1. Default 2
 */
void ewxStyledWindow::drawButtonBorder(wxPaintDC &dc, bool isSunken, int width)
{
  wxCoord w, h;
  dc.GetSize(&w, &h);
  wxPoint upperLeft[3] = {wxPoint(0, h),
                          wxPoint(0, 0),
                          wxPoint(w, 0)};
  wxPoint lowerRight1[3] = {wxPoint(1, h-1),
                            wxPoint(w-1, h-1),
                            wxPoint(w-1, 0)};
  wxPoint lowerRight2[3] = {wxPoint(2, h-2),
                            wxPoint(w-2, h-2),
                            wxPoint(w-2, 1)};

  dc.SetPen(isSunken? getBtn3DDkShadowColor(): getBtn3DDkLightColor());
  dc.DrawLines(3, upperLeft);
  if (width == 2)
    dc.DrawLines(3, upperLeft, 1, 1);
  
  dc.SetPen(!isSunken? getBtn3DDkShadowColor(): getBtn3DDkLightColor());
  dc.DrawLines(3, lowerRight1);
  if (width == 2)
    dc.DrawLines(3, lowerRight2);
}
