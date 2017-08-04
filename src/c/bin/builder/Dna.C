#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "Dna.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "util/EventDispatcher.H"
#include "util/Event.H"

#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/WindowEvent.H"
  using namespace ecce;

#include "viz/GenerateDNACmd.H"
#include "viz/NewFragmentCmd.H"

#include "Dna.H"


IMPLEMENT_DYNAMIC_CLASS( Dna, DnaGUI )


Dna::Dna( )
{
}



Dna::Dna( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
  : DnaGUI(parent, id, pos, size, style)
{
   wxTextCtrl *txt = (wxTextCtrl*)FindWindow(ID_TEXTCTRL_DNA_SEQUENCE);
   wxTextValidator *v = new wxTextValidator(wxFILTER_INCLUDE_CHAR_LIST);
   wxArrayString valids;
   valids.Add("T");
   valids.Add("A");
   valids.Add("G");
   valids.Add("C");
   valids.Add("t");
   valids.Add("a");
   valids.Add("g");
   valids.Add("c");
   v->SetIncludes(valids);
   txt->SetValidator(*v);
}




Dna::~Dna()
{
}



/**
 * We decided it was always 5'
 */
bool Dna::is5Prime() const
{
   return true;
}


void Dna::OnButtonDnaAtClick( wxCommandEvent& event )
{
   wxTextCtrl *txt = (wxTextCtrl*)FindWindow(ID_TEXTCTRL_DNA_SEQUENCE);
   if (is5Prime()) {
      txt->WriteText("A");
   } else {
      txt->WriteText("T");
   }
}


void Dna::OnButtonDnaTaClick( wxCommandEvent& event )
{
   wxTextCtrl *txt = (wxTextCtrl*)FindWindow(ID_TEXTCTRL_DNA_SEQUENCE);
   if (is5Prime()) {
      txt->WriteText("T");
   } else {
      txt->WriteText("A");
   }
}


void Dna::OnButtonDnaGcClick( wxCommandEvent& event )
{
   wxTextCtrl *txt = (wxTextCtrl*)FindWindow(ID_TEXTCTRL_DNA_SEQUENCE);
   if (is5Prime()) {
      txt->WriteText("G");
   } else {
      txt->WriteText("C");
   }
}


void Dna::OnButtonDnaCgClick( wxCommandEvent& event )
{
   wxTextCtrl *txt = (wxTextCtrl*)FindWindow(ID_TEXTCTRL_DNA_SEQUENCE);
   if (is5Prime()) {
      txt->WriteText("C");
   } else {
      txt->WriteText("G");
   }
}


void Dna::OnButtonDnaGenerateClick( wxCommandEvent& event )
{
  WxVizToolFW &fw = getFW();
  SGFragment *frag = fw.getSceneGraph().getFragment();

  bool doit = true;
  if (frag->numAtoms() > 0) {
     ewxMessageDialog dlg(this, 
           "DNA strands can not be added to existing systems.  \n"
           "Do you want to clear the existing system?",
           "ECCE Clear System", wxOK|wxCANCEL|wxICON_QUESTION);
     doit = dlg.ShowModal() == wxID_OK;
     dlg.Destroy();
  }


  if (doit) {
     ewxTextCtrl *text;
     text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DNA_SEQUENCE));
     string sequence = text->GetValue().c_str();
      if (sequence.size() == 0) {
         // TODO this doesn't show up in status bar
         ::wxLogWarning("You must specify a sequence.");
      } else {

         Command *cmd = 0;
         if (frag->numAtoms() > 0) {
            cmd = new NewFragmentCmd("Clear", &fw.getSceneGraph());
            fw.execute(cmd);
         }

         cmd = new GenerateDNACmd("Generate DNA", &fw.getSceneGraph());
         cmd->getParameter("sequence")->setString(sequence);
         fw.execute(cmd);
      }
  }
  event.Skip();
}


void Dna::OnTextctrlDnaSequenceUpdated( wxCommandEvent& event )
{
  ewxTextCtrl * text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DNA_SEQUENCE);
  int pos = text->GetInsertionPoint();
  wxString sequence = event.GetString();
  wxString added = sequence[pos-1];
  if (added != added.Upper()) {
    text->Replace(pos-1, pos, added.Upper());
  }
}

void Dna::OnTextctrlDnaSequenceEnter( wxCommandEvent& event )
{
}
