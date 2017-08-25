#ifndef _PBC_H_
#define _PBC_H_

#include "util/Event.H"
  using ecce::Event;
#include "util/Listener.H"
  using ecce::Listener;

#include "wxviz/WxVizTool.H"

#include "PBCGUI.H"

class PBCCmd;

/**
 * 
 */

class PBC : public PBCGUI,
            public WxVizTool, 
            public Listener
{    
   DECLARE_DYNAMIC_CLASS( PBC)

   public:
      /// Constructors
      PBC( );
      PBC( wxWindow* parent, wxWindowID id = SYMBOL_PBCGUI_IDNAME, const wxPoint& pos = SYMBOL_PBCGUI_POSITION, const wxSize& size = SYMBOL_PBCGUI_SIZE, long style = SYMBOL_PBCGUI_STYLE );
      virtual ~PBC( );
      bool Create( wxWindow* parent, wxWindowID id = SYMBOL_PBCGUI_IDNAME, const wxPoint& pos = SYMBOL_PBCGUI_POSITION, const wxSize& size = SYMBOL_PBCGUI_SIZE, long style = SYMBOL_PBCGUI_STYLE );
      bool Enable(bool enable);
      void indicateHaveLattice(bool havelattice);
      void updateReadOnly();
      void updateSpaceGroup();

      // Listener virtuals
      virtual void eventMCB(const Event& event);
      Command *restoreUnitCell();
      void refresh();
      void showReplication();
      void showLattice(bool showit);
      void replicate();
      void replicate(int a, int b, int c);
      void execute(PBCCmd *cmd);
      void makeSuperLattice();


      virtual    void OnChoicePbcTypeSelected( wxCommandEvent& event );
      virtual    void OnTextctrlPbcA1xEnter( wxCommandEvent& event );
      virtual    void OnTextctrlPbcA1yEnter( wxCommandEvent& event );
      virtual    void OnTextctrlPbcA1zEnter( wxCommandEvent& event );
      virtual    void OnTextctrlPbcA2xEnter( wxCommandEvent& event );
      virtual    void OnTextctrlPbcA2yEnter( wxCommandEvent& event );
      virtual    void OnTextctrlPbcA2zEnter( wxCommandEvent& event );
      virtual    void OnTextctrlPbcA3xEnter( wxCommandEvent& event );
      virtual    void OnTextctrlPbcA3yEnter( wxCommandEvent& event );
      virtual    void OnTextctrlPbcA3zEnter( wxCommandEvent& event );
      virtual    void OnTextctrlPbcAEnter( wxCommandEvent& event );
      virtual    void OnTextctrlPbcAlphaEnter( wxCommandEvent& event );
      virtual    void OnTextctrlPbcBEnter( wxCommandEvent& event );
      virtual    void OnTextctrlPbcBetaEnter( wxCommandEvent& event );
      virtual    void OnTextctrlPbcCEnter( wxCommandEvent& event );
      virtual    void OnTextctrlPbcGammaEnter( wxCommandEvent& event );
      virtual    void OnSpinctrlPbcXrepUpdated( wxSpinEvent& event );
      virtual    void OnSpinctrlPbcYrepUpdated( wxSpinEvent& event );
      virtual    void OnSpinctrlPbcZrepUpdated( wxSpinEvent& event );
      virtual    void OnSpinctrlPbcXrepTextUpdated( wxCommandEvent& event );
      virtual    void OnSpinctrlPbcYrepTextUpdated( wxCommandEvent& event );
      virtual    void OnSpinctrlPbcZrepTextUpdated( wxCommandEvent& event );
      virtual    void OnCheckboxPbcShowlatticeClick( wxCommandEvent& event );
      virtual    void OnCheckboxPbcCenterClick( wxCommandEvent& event );
      virtual    void OnButtonPbcReplicateClick( wxCommandEvent& event );
      virtual    void OnButtonPbcFoldClick( wxCommandEvent& event );
      virtual    void OnButtonPbcSuperClick( wxCommandEvent& event );
      virtual    void OnButtonPbcCreateClick( wxCommandEvent& event );
      virtual    void OnButtonPbcDeleteClick( wxCommandEvent& event );
      virtual    void OnButtonPbcRestoreClick( wxCommandEvent& event );
      virtual    void OnPbcGenerateClick( wxCommandEvent& event );
      virtual    void OnListChange(wxListEvent& event);
      virtual    void OnCheckboxPbcUniformClick( wxCommandEvent& event );
      virtual    void OnButtonPbcEquivrectClick( wxCommandEvent& event );

      static const wxWindowID ID_COMBO_SPACE_GROUP;
   protected:
      void center(bool docenter);
      void vectorChanged();
      void unitCellChanged();
      void stashFragment();
      void indicateHaveLattice();
      void updateReplicateBtn(int x, int y, int z);
      void enforceUniform(int value);


      /* flag to indicate a new fragment should not be stashed. 
         occurs when replicating. 
       */
      bool p_noStashFlag;

      SGFragment *p_baseFrag;

};

#endif // _PBC_H_
