#ifndef STYLEDROPDOWN_H
#define STYLEDROPDOWN_H

#include <wx/toolbar.h>

#include "util/Event.H"
#include "util/Listener.H"
  using namespace ecce;

#include "wxviz/WxVizTool.H"

class StyleDropDown : public wxToolBar, 
        public WxVizTool, // for access to fragment 
        public Listener   // for enable/disable based on context
{    
   DECLARE_DYNAMIC_CLASS( StyleDropDown )
   DECLARE_EVENT_TABLE()

   public:
      StyleDropDown( );
      StyleDropDown(wxWindow* parent, wxWindowID id=wxID_ANY,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize,
              long style = wxNO_BORDER|wxTB_VERTICAL|wxTB_TEXT|wxTB_NOICONS,
              const wxString& name = "Display Styles");
      virtual ~StyleDropDown();

      bool Create(wxWindow* parent, wxWindowID id=wxID_ANY,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize,
              long style = wxNO_BORDER|wxTB_VERTICAL|wxTB_TEXT|wxTB_NOICONS,
              const wxString& name = "Display Styles");

      // WxVizTool virtual
      virtual void connectToolKitFW(WxVizToolFW * fw);

      virtual void eventMCB(const Event& event);

   protected:
      void UICanUseResidueStyles(wxUpdateUIEvent& event);
};

#endif
