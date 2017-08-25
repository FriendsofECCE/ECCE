#ifndef _DNA_H
#define _DNA_H

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "Dna.C"
#endif

#include "wxviz/WxVizTool.H"

#include "DnaGUI.H"

/**
 * DNA gui toolkit.
 * Design left unchanged during port.
 */
class Dna: public DnaGUI, public WxVizTool
{    
   DECLARE_DYNAMIC_CLASS( Dna)

   public:
      /// Constructors
      Dna( );
      Dna( wxWindow* parent, wxWindowID id = SYMBOL_DNAGUI_IDNAME, const wxPoint& pos = SYMBOL_DNAGUI_POSITION, const wxSize& size = SYMBOL_DNAGUI_SIZE, long style = SYMBOL_DNAGUI_STYLE );
      virtual ~Dna();

      virtual void OnButtonDnaAtClick( wxCommandEvent& event );
      virtual void OnButtonDnaTaClick( wxCommandEvent& event );
      virtual void OnButtonDnaGcClick( wxCommandEvent& event );
      virtual void OnButtonDnaCgClick( wxCommandEvent& event );
      virtual void OnButtonDnaGenerateClick( wxCommandEvent& event );
      virtual void OnTextctrlDnaSequenceUpdated( wxCommandEvent& event );
      virtual void OnTextctrlDnaSequenceEnter( wxCommandEvent& event );

   protected:
      bool is5Prime() const;

};

#endif
