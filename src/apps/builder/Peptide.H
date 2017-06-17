#ifndef _PEPTIDE_H
#define _PEPTIDE_H

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "Peptide.C"
#endif

#include "wxviz/WxVizTool.H"

#include "PeptideGUI.H"

/**
 * Peptide gui toolkit.
 * Design left unchanged during port.
 */
class Peptide: public PeptideGUI, public WxVizTool
{    
   DECLARE_DYNAMIC_CLASS( Peptide)
   DECLARE_EVENT_TABLE()

   public:
      /// Constructors
      Peptide( );
      Peptide( wxWindow* parent, wxWindowID id = SYMBOL_PEPTIDEGUI_IDNAME,
              const wxPoint& pos = SYMBOL_PEPTIDEGUI_POSITION,
              const wxSize& size = SYMBOL_PEPTIDEGUI_SIZE,
              long style = SYMBOL_PEPTIDEGUI_STYLE );
      bool Create( wxWindow* parent, wxWindowID id = SYMBOL_PEPTIDEGUI_IDNAME,
              const wxPoint& pos = SYMBOL_PEPTIDEGUI_POSITION,
              const wxSize& size = SYMBOL_PEPTIDEGUI_SIZE,
              long style = SYMBOL_PEPTIDEGUI_STYLE );
      virtual ~Peptide();

      virtual void OnTextctrlPeptideSequenceUpdated( wxCommandEvent& event );
      virtual void OnTextctrlPeptideSequenceEnter( wxCommandEvent& event );
      virtual void OnButtonPeptideGenerateClick(wxCommandEvent&);
      virtual void OnButtonPeptideClearClick( wxCommandEvent& event );
      virtual void OnButtonAAClick( wxCommandEvent& event );

   protected:
      void CreateControls();
};

#endif
