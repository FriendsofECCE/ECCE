#ifndef _TRANSLATEPANEL_H_
#define _TRANSLATEPANEL_H_

#include "wxviz/WxVizTool.H"

#include "TranslatePanelGUI.H"

/**
 * Panel to implement and manage translations of system coordinates
 */
class TranslatePanel : public TranslatePanelGUI,
                       public WxVizTool
{
   public:
      TranslatePanel();
      TranslatePanel(wxWindow *parent, wxWindowID id=-1,
            const wxPoint &pos = wxDefaultPosition,
            const wxSize &size = wxDefaultSize,
            long style = wxSUNKEN_BORDER);
      virtual ~TranslatePanel();

      void initializeGUI();

      virtual void OnButtonTranslGoClick( wxCommandEvent& event );

};
#endif
