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

#ifndef EWXTREECTRL_HH
#define EWXTREECTRL_HH

#if 000
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "ewxTreeCtrl.C"
#endif
#endif

#include "wx/treectrl.h"

#include "wxgui/ewxGenericValidator.H" // for ewxDefaultValidator
#include "wxgui/ewxStyledWindow.H"

class ewxTreeCtrl : public wxTreeCtrl, public ewxStyledWindow
{
public:
  ewxTreeCtrl();
  ewxTreeCtrl(wxWindow* parent, wxWindowID id = wxID_ANY,
	            const wxPoint& pos = wxDefaultPosition,
	            const wxSize& size = wxDefaultSize,
	            long style = wxTR_HAS_BUTTONS | wxTR_HIDE_ROOT | wxTR_NO_LINES,
	            const wxValidator& validator = ewxDefaultValidator,
	            const wxString& name = "treeCtrl");
  bool Create(wxWindow* parent, wxWindowID id = wxID_ANY,
	            const wxPoint& pos = wxDefaultPosition,
	            const wxSize& size = wxDefaultSize,
	            long style = wxTR_HAS_BUTTONS | wxTR_HIDE_ROOT | wxTR_NO_LINES,
	            const wxValidator& validator = ewxDefaultValidator,
	            const wxString& name = "treeCtrl");
  virtual ~ewxTreeCtrl();
};

#endif
