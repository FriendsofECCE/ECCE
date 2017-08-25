/**
 * @file
 * @author Lisong Sun
 *
 * A subclass derived from wxTreeCtrl
 *
 * This class will apply all the ECCE default styles including fonts and colors
 * to the wxTreeCtrl. It will be subclassed by EDSITreeCtrl.
 *
 */

#if 000
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "ewxTreeCtrl.C"
#endif
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/treectrl.h"

#include "wxgui/ewxTreeCtrl.H"

ewxTreeCtrl::ewxTreeCtrl()
  : wxTreeCtrl(),
    ewxStyledWindow()
{

}

ewxTreeCtrl::ewxTreeCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos,
			                   const wxSize& size, long style,
			                   const wxValidator& validator, const wxString& name)
  : wxTreeCtrl(),
    ewxStyledWindow()
{
  Create(parent, id, pos, size, style, validator, name);
}


bool ewxTreeCtrl::Create(wxWindow* parent, wxWindowID id, const wxPoint& pos,
			                   const wxSize& size, long style,
			                   const wxValidator& validator, const wxString& name)
{
  if (!wxTreeCtrl::Create(parent, id, pos, size, style, validator, name)) {
    wxFAIL_MSG( wxT("ewxTreeCtrl creation failed") );
    return false;
  }

  setStyles(this, false);

  return true;
}


ewxTreeCtrl::~ewxTreeCtrl()
{

}

