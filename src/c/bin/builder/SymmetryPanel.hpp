#ifndef _SYMMETRYPANEL_H_
#define _SYMMETRYPANEL_H_

#include "util/Event.H"
  using ecce::Event;
#include "util/Listener.H"
  using ecce::Listener;

#include "wxviz/WxVizTool.H"

#include "SymmetryPanelGUI.H"

#define ID_COMBO_SYMMETRY_GROUP 9500

/**
 * Panel to implement and manage translations of system coordinates
 */
class SymmetryPanel : public SymmetryPanelGUI,
                      public WxVizTool,
                      public Listener
{
   public:
      SymmetryPanel();
      SymmetryPanel(wxWindow *parent, wxWindowID id=-1,
            const wxPoint &pos = wxDefaultPosition,
            const wxSize &size = wxDefaultSize,
            long style = wxSUNKEN_BORDER);
      virtual ~SymmetryPanel();

     // Listener virtuals
     virtual void eventMCB(const Event& event);

      void initializeGUI();

      // Event handlers
      virtual void OnButtonSymmFindClick( wxCommandEvent& event );
      virtual void OnTextctrlSymmAssignEnter( wxCommandEvent& event );
      virtual void OnButtonSymmGenerateGhostsClick( wxCommandEvent& event );
      virtual void OnButtonSymmFindIrreducibleClick( wxCommandEvent& event );
      virtual void OnButtonSymmGenerateMoleculeClick( wxCommandEvent& event );
      virtual void OnButtonSymmCleanClick( wxCommandEvent& event );
      virtual void OnButtonDelGhostsClick( wxCommandEvent& event );
      virtual void OnListChange( wxListEvent& event );

   private:
      wxControl *  getControl(int id);
      wxTextCtrl * getTextCtrl(int id);
      void updateUI(bool isC1);
      void assignSymmetry();
};
#endif
