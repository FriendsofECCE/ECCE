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
ewxStyledWindow::ewxStyledWindow()
{
   if (p_prefs == 0) p_prefs = new Preferences(PrefLabels::GLOBALPREFFILE);
}

ewxStyledWindow::ewxStyledWindow(wxWindow *win, bool recursive)
{
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
 * Compute a font size.
 * The base sizes are from our original font selections and are considered
 * Large. 
 */
int ewxStyledWindow::getFontSize(int basesize)
{
  int size = basesize;
  // As per prefs dialog, 0=small, 1=medium, 2 = large
  int pref = 1;
  p_prefs->getInt("FONTSIZE",pref);
  if (pref == 0) 
     size-=3;
  else if (pref == 1) 
     size-=2;
  else if (pref == 3) 
     size+=3;
  return size;
}


wxFont ewxStyledWindow::getBoldFont()
{
   return wxFont(getFontSize(10), wxDEFAULT, 
         wxNORMAL, wxBOLD, FALSE, _T("helvetica"));
}


wxFont ewxStyledWindow::getNormalFont()
{
   return wxFont(getFontSize(10), wxDEFAULT, 
         wxNORMAL, wxNORMAL, FALSE, _T("helvetica"));
}


wxFont ewxStyledWindow::getUnitFont()
{
   return wxFont(getFontSize(9), wxDEFAULT, 
         wxNORMAL, wxLIGHT, FALSE, _T("helvetica"));
}


wxFont ewxStyledWindow::getMonoSpaceFont()
{
   return wxFont(getFontSize(10), wxDEFAULT, 
         wxNORMAL, wxNORMAL, FALSE, _T("courier"));
}


// The following three fonts are used by pertable
wxFont ewxStyledWindow::getAtomicNumFont()
{
   return wxFont(getFontSize(10), wxDEFAULT, 
         wxNORMAL, wxBOLD, FALSE, _T("helvetica"));
}


wxFont ewxStyledWindow::getBigAtomicSymbolFont()
{
   return wxFont(getFontSize(18), wxDEFAULT, 
         wxNORMAL, wxBOLD, FALSE, _T("helvetica"));
}


wxFont ewxStyledWindow::getSmallLabelFont()
{
   return wxFont(getFontSize(9), wxDEFAULT, 
         wxNORMAL, wxNORMAL, FALSE, _T("helvetica"));
}


void ewxStyledWindow::setStyles(wxWindow *win, bool recursive)
{
#ifdef __WXMAC__
  return;
#endif
   // Only fonts and the read-only marker are set here; every other colour
   // is left to the GTK theme (#210).  wxNullColour hands a field back to
   // the theme when it becomes editable again.
   if (wxTextCtrl *text = dynamic_cast<wxTextCtrl*>(win)) {
      text->SetFont(getBoldFont());
      setReadonlyMarker(win, !text->IsEditable());

   } else if (dynamic_cast<wxButton*>(win)) {
      win->SetFont(getBoldFont());

   } else if (dynamic_cast<wxCheckBox*>(win)) {
      win->SetFont(getBoldFont());
      // GTK3 paints a checkbox's background as a band across its label,
      // so a read-only checkbox gets no marker.
      win->SetBackgroundColour(wxNullColour);

   } else if (dynamic_cast<wxChoice*>(win)) {
      win->SetFont(getBoldFont());
      ewxChoice *choice = dynamic_cast<ewxChoice*>(win);
      if (choice) setReadonlyMarker(win, !choice->IsEditable());

   } else if (dynamic_cast<wxComboBox*>(win)) {
      win->SetFont(getBoldFont());
      ewxComboBox *box = dynamic_cast<ewxComboBox*>(win);
      if (box) setReadonlyMarker(win, !box->IsEditable());

   } else if (dynamic_cast<wxMenuBar*>(win)) {
      win->SetFont(getBoldFont());

   } else if (dynamic_cast<wxScrolledWindow*>(win)) {
      win->SetFont(getBoldFont());

   } else if (dynamic_cast<wxMenu*>(win)) {
      win->SetFont(getBoldFont());

   } else if (ewxSpinCtrl *spin = dynamic_cast<ewxSpinCtrl*>(win)) {
      win->SetFont(getBoldFont());
      setReadonlyMarker(win, !spin->IsEnabled());

   } else if (dynamic_cast<wxDialog*>(win)) {
      win->SetFont(getBoldFont());

   } else if (dynamic_cast<wxStaticLine*>(win)) {

   } else if (dynamic_cast<ewxNonBoldLabel*>(win)) {
      win->SetFont(getNormalFont());

   } else if (dynamic_cast<ewxSmallLabel*>(win)) {
      win->SetFont(getSmallLabelFont());

   } else if (dynamic_cast<wxStaticText*>(win)) {
      win->SetFont(getBoldFont());

   } else if (dynamic_cast<wxFrame*>(win)) {
      win->SetFont(getBoldFont());

   } else if (dynamic_cast<wxNotebook*>(win)) {
      win->SetFont(getBoldFont());

   } else if (dynamic_cast<wxListBox*>(win)) {
      win->SetFont(getBoldFont());
      ewxListBox *list = dynamic_cast<ewxListBox*>(win);
      if (list) setReadonlyMarker(win, !list->IsEditable());

   } else if (dynamic_cast<wxListCtrl*>(win)) {
      win->SetFont(getBoldFont());

   } else if (dynamic_cast<wxRadioBox*>(win)) {
      win->SetFont(getBoldFont());

   } else if (dynamic_cast<wxTreeCtrl*>(win)) {
      win->SetFont(getNormalFont());
   }

   //cout << ", " << recursive << ")" << endl;
   if (recursive) setChildStyles(win);
}


void ewxStyledWindow::setReadonlyMarker(wxWindow *win, bool readonly)
{
   win->SetBackgroundColour(readonly ? getReadonlyColor() : wxNullColour);
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
