#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "Peptide.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "util/Event.H"
#include "util/EventDispatcher.H"
#include "util/STLUtil.H"

#include "tdat/TResTab.H"

#include "viz/GeneratePeptideCmd.H"
#include "viz/NewFragmentCmd.H"

#include "wxgui/ewxButton.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/WindowEvent.H"
  using namespace ecce;

#include "Peptide.H"

static const int AA_FIRST_ID = 9550 + 10;  // 10 is to account for ids in pjd


IMPLEMENT_DYNAMIC_CLASS( Peptide, PeptideGUI )

BEGIN_EVENT_TABLE( Peptide, PeptideGUI )
   EVT_COMMAND_RANGE(AA_FIRST_ID, AA_FIRST_ID+90, wxEVT_COMMAND_BUTTON_CLICKED,Peptide::OnButtonAAClick)
END_EVENT_TABLE()



Peptide::Peptide( )
  : PeptideGUI(),
    WxVizTool()
{
}


Peptide::Peptide( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                  const wxSize& size, long style )
  : PeptideGUI(),
    WxVizTool()
{
  Create(parent, id, pos, size, style);
}


bool Peptide::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                      const wxSize& size, long style )
{
   if (!PeptideGUI::Create(parent, id, pos, size, style)) {
     wxFAIL_MSG( wxT("Peptide creation failed") );
     return false;
   }

   CreateControls();
   GetSizer()->Fit(this);
   GetSizer()->SetSizeHints(this);
   Centre();

   return true;
}




Peptide::~Peptide()
{
}



void Peptide::OnButtonAAClick( wxCommandEvent& event )
{
   wxButton *btn = (wxButton*)event.GetEventObject();
   string aa = btn->GetLabel().c_str();
   STLUtil::stripLeadingAndTrailingWhiteSpace(aa);

   ewxTextCtrl * text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PEPTIDE_SEQUENCE);
   string sequence = text->GetValue().c_str();
   if (sequence.size() > 0) sequence.append("-"); 
   sequence.append(aa.c_str());
   text->SetValue(sequence.c_str());

   event.Skip();


}

void Peptide::OnButtonPeptideGenerateClick( wxCommandEvent& event )
{
  WxVizToolFW &fw = getFW();
  SGFragment *frag = fw.getSceneGraph().getFragment();

  bool doit = true;
  if (frag->numAtoms() > 0) {
     ewxMessageDialog dlg(this, 
           "Peptides can not be added to existing systems.  \n"
           "Do you want to clear the existing system?",
           "ECCE Clear System", wxOK|wxCANCEL|wxICON_QUESTION);
     doit = dlg.ShowModal() == wxID_OK;
     dlg.Destroy();
  }

   if (doit) {
      ewxTextCtrl *text;

      text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PEPTIDE_SEQUENCE));
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

         cmd = new GeneratePeptideCmd("Generate Peptide", &fw.getSceneGraph());
         cmd->getParameter("sequence")->setString(sequence);
         fw.execute(cmd);
      }
   }
  event.Skip();
}


void Peptide::OnTextctrlPeptideSequenceUpdated( wxCommandEvent& event )
{
  /*
  ewxTextCtrl * text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PEPTIDE_SEQUENCE);
  int pos = text->GetInsertionPoint();
  wxString sequence = text->GetValue().Upper();
  text->ChangeValue(sequence);
  text->SetInsertionPoint(pos);
  */
   event.Skip();
}

void Peptide::OnTextctrlPeptideSequenceEnter( wxCommandEvent& event )
{
}

void Peptide::OnButtonPeptideClearClick( wxCommandEvent& event )
{
   ewxTextCtrl * text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PEPTIDE_SEQUENCE);
   text->SetValue("");
   event.Skip();
}


void Peptide::CreateControls()
{
   /* Not allowing text entry for now
   wxTextCtrl *txt = (wxTextCtrl*)FindWindow(ID_TEXTCTRL_PEPTIDE_SEQUENCE);
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
   */

   vector<string> aanames = TResTab::getAminoAcids();
   ewxButton * btn;
   for (size_t idx=0; idx<aanames.size(); idx++) {
     if (aanames[idx] != "CYM ") {
       btn = new ewxButton(this, AA_FIRST_ID+idx,aanames[idx].c_str(),
           wxDefaultPosition, wxSize(36,24));
       btn->SetFont(ewxStyledWindow::getSmallLabelFont());
       p_buttonSizer->Add(btn, 1, wxCENTER|wxALL, 1);
     }
   }
}


