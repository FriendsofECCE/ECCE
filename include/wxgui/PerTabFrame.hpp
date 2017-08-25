/**
 * @file
 *
 * Periodic table frame class header.
 *
 *
 */

#ifndef _PERTABFRAME_H_
#define _PERTABFRAME_H_

#include "wx/wxprec.h"
#include "wx/gbsizer.h"

// For compilers that support precompilation, includes "wx/wx.h".
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxFrame.H"
#include "wxgui/PerTabPanel.H"

#define PERTABFRAME_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxMINIMIZE_BOX|wxMAXIMIZE_BOX|wxCLOSE_BOX
#define PERTABFRAME_TITLE _("ECCE Periodic Table")
#define PERTABFRAME_SIZE wxSize(640, 430)
#define PERTABFRAME_POSITION wxDefaultPosition


// PerTabFrame Class
class PerTabFrame: public ewxFrame {

  DECLARE_DYNAMIC_CLASS( PerTabFrame )
  DECLARE_EVENT_TABLE()

public:

  PerTabFrame() {};
  PerTabFrame(wxWindow* parent,
              wxWindowID id = wxID_ANY,
              const wxString& caption = PERTABFRAME_TITLE,
              const wxPoint& pos = PERTABFRAME_POSITION,
              const wxSize& size = PERTABFRAME_SIZE,
              long style = PERTABFRAME_STYLE );

  void resetUserPreferences();

  void OnMenuClick(wxCommandEvent& event);
  void OnCloseWindow(wxCloseEvent& event);

  vector<string> selectedAtomSymbols();

protected:

  PerTabPanel * p_topPanel;
};

#endif
// _PERTABFRAME_H_
