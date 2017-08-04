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

wxColour ewxStyledWindow::getWindowColor()
{
  return ewxColor(Color::WINDOW);
}

wxColour ewxStyledWindow::getReadonlyColor()
{
  return ewxColor(Color::READONLY);
}

wxColour ewxStyledWindow::getButtonColor()
{
  return ewxColor(Color::WINDOW);
}


wxColour ewxStyledWindow::getInputColor()
{
  return ewxColor(Color::INPUT);
}


wxColour ewxStyledWindow::getTextColor()
{
  return ewxColor(Color::TEXT);
}


wxColour ewxStyledWindow::getBtn3DDkShadowColor()
{
  //  return wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
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
  return ewxColor(146, 146, 146);
}


wxColour ewxStyledWindow::getHightLightRowColor()
{
  return ewxColor(200, 200, 200);
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
   //cout << "ewxStyledWindow::setStyles(";
   
   win->SetForegroundColour(getTextColor());
   win->SetBackgroundColour(getWindowColor());

   if (wxTextCtrl *text = dynamic_cast<wxTextCtrl*>(win)) {
      //cout << "wxTextCtrl";
      text->SetFont(getBoldFont());
      win->SetBackgroundColour(getInputColor());
      if (!text->IsEditable()) {
         win->SetBackgroundColour(getReadonlyColor());
      }

   } else if (dynamic_cast<wxButton*>(win)) {
      //cout << "wxButton";
      win->SetBackgroundColour(getButtonColor());
      win->SetFont(getBoldFont());

   } else if (dynamic_cast<wxCheckBox*>(win)) {
      //cout << "wxCheckBox";
      win->SetFont(getBoldFont());
      win->SetBackgroundColour(getInputColor());
      ewxCheckBox *ebox = dynamic_cast<ewxCheckBox*>(win);
      if (ebox && !ebox->IsEditable()) {
         //cout << "\tewxCheckBox";
         win->SetBackgroundColour(getReadonlyColor());
      }

   } else if (dynamic_cast<wxChoice*>(win)) {
      //cout << "wxChoice";
      win->SetFont(getBoldFont());
      win->SetBackgroundColour(getInputColor());
      ewxChoice *choice = dynamic_cast<ewxChoice*>(win);
      if (choice && !choice->IsEditable()) {
         //cout << "\tewxChoice";
         win->SetBackgroundColour(getReadonlyColor());
      }

   } else if (dynamic_cast<wxComboBox*>(win)) {
      //cout << "wxComboBox";
      win->SetFont(getBoldFont());
      win->SetBackgroundColour(getInputColor());
      ewxComboBox *box = dynamic_cast<ewxComboBox*>(win);
      if (box) {
         //cout << "\tewxComboBox";
         if (!box->IsEditable()) {
            win->SetBackgroundColour(getReadonlyColor());
         }
      }

   } else if (dynamic_cast<wxMenuBar*>(win)) {
      //cout << "wxMenuBar";
      win->SetBackgroundColour(getReadonlyColor());
      win->SetFont(getBoldFont());

   } else if (dynamic_cast<wxScrolledWindow*>(win)) {
      //cout << "wxScrolledWindow";
      // scroll bars not set by this
      win->SetBackgroundColour(getWindowColor());
      win->SetFont(getBoldFont());

   } else if (dynamic_cast<wxMenu*>(win)) {
      //cout << "wxMenu";
      // This doesn't work but do it anyway
      win->SetBackgroundColour(getReadonlyColor());
      win->SetFont(getBoldFont());

   } else if (ewxSpinCtrl *spin = dynamic_cast<ewxSpinCtrl*>(win)) {
      //cout << "ewxSpinCtrl";
      win->SetFont(getBoldFont());
      win->SetBackgroundColour(getInputColor());
      if (!spin->IsEnabled()) {
         win->SetBackgroundColour(getReadonlyColor());
      }

   } else if (dynamic_cast<wxDialog*>(win)) {
      //cout << "wxDialog";
      win->SetBackgroundColour(getWindowColor());
      win->SetFont(getBoldFont());
      //Version 2.5.3
      //win->SetBackgroundStyle(wxBG_STYLE_COLOUR);
      //win->SetThemeEnabled(false);

   } else if (dynamic_cast<wxStaticLine*>(win)) {
      //cout << "wxStaticLine";
      win->SetBackgroundColour(getButtonColor());

   } else if (dynamic_cast<ewxNonBoldLabel*>(win)) {
      //cout << "ewxNonBoldLabel";
      win->SetFont(getNormalFont());

   } else if (dynamic_cast<ewxSmallLabel*>(win)) {
      //cout << "ewxSmallLabel";
      win->SetFont(getSmallLabelFont());

   } else if (dynamic_cast<wxStaticText*>(win)) {
      //cout << "wxStaticText";
      win->SetFont(getBoldFont());

   } else if (dynamic_cast<wxFrame*>(win)) {
      //cout << "wxFrame";
      win->SetBackgroundColour(getWindowColor());
      win->SetFont(getBoldFont());
      //Version 2.5.3
      //SetBackgroundStyle(wxBG_STYLE_COLOUR);

   } else if (dynamic_cast<wxNotebook*>(win)) {
      //cout << "wxNotebook";
      // Setting background has side effect of eliminating
      // faded unselected tabs.
      win->SetBackgroundColour(getWindowColor());
      win->SetFont(getBoldFont());

   } else if (dynamic_cast<wxListBox*>(win)) {
      //cout << "wxListBox";
      win->SetFont(getBoldFont());
      // this is ignored for some reason 
      win->SetBackgroundColour(getInputColor());
      ewxListBox *list = dynamic_cast<ewxListBox*>(win);
      if (list && !list->IsEditable()) {
         //cout << "\tewxListBox";
         win->SetBackgroundColour(getReadonlyColor());
      }

   } else if (dynamic_cast<wxListCtrl*>(win)) {
      win->SetFont(getBoldFont());
      win->SetBackgroundColour(getInputColor());

   } else if (dynamic_cast<wxRadioBox*>(win)) {
      //cout << "wxRadioBox";
      win->SetFont(getBoldFont());
      win->SetBackgroundColour(getInputColor());

   } else if (dynamic_cast<wxTreeCtrl*>(win)) {
      //cout << "wxTreeCtrl";
      win->SetFont(getNormalFont());

   } else {
      //cout << "unknown window type";
   }

   //cout << ", " << recursive << ")" << endl;
   if (recursive) setChildStyles(win);
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
