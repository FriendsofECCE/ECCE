/**
 * @file
 *
 *
 */
// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <math.h>

#include "util/STLUtil.H"

#include "wxgui/ewxChoice.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxSpinCtrl.H"
#include "wxgui/ewxStaticText.H"

#include "wxgui/WxPDBPrompt.H"

/**
 * constructors
 */
WxPDBPrompt::WxPDBPrompt( ) 
{
   //setStyles(this,false);
}

WxPDBPrompt::WxPDBPrompt( wxWindow* parent, wxWindowID id,
      const wxString& caption, const wxPoint& pos,
      const wxSize& size, long style )
: WxPDBPromptGUI(parent, id, caption, pos, size, style)
{
   //setStyles(this,false);
}



int WxPDBPrompt::getPDBModel() const
{
   ewxSpinCtrl *spin = ((ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PDBMODEL));
   return spin->GetValue();
}

string WxPDBPrompt::getAltLocation() const
{
   string ret = " ";

   ewxChoice *win =((ewxChoice*)FindWindow(ID_ALT_LOC_CHOICE));
   ret = win->GetStringSelection().c_str();
   STLUtil::stripLeadingAndTrailingWhiteSpace(ret);
   if (ret == "None") ret = " ";
   return ret;
}

string WxPDBPrompt::getChainID() const
{
   string ret = " ";
   ewxChoice *win =((ewxChoice*)FindWindow(ID_CHAIN_CHOICE));
   ret = win->GetStringSelection().c_str();
   STLUtil::stripLeadingAndTrailingWhiteSpace(ret);
   if (ret == "All") ret = " ";
   return ret;
}

/**
 * Setup string containing displaying number of models available in
 * PDB file
 */
void WxPDBPrompt::setModelRange(int models)
{
   string range;
   range = "[";
   if (models <= 1) {
      range.append("1]");
   } else {
      char buf[120];
      range.append("1..");
      sprintf(buf,"%d]",models);
      range.append(buf);
   }
   ewxStaticText *stext = (ewxStaticText*)FindWindow(ID_PROMPT_MODEL_OPTIONS_TEXT);
   stext->SetLabel(range.c_str());

   // Add a validator
   ewxSpinCtrl *spin = ((ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PDBMODEL));
   spin->SetRange(1,models);
   spin->Enable(models > 1);
}

/**
 * Set option list of available alternate locations
 */
void WxPDBPrompt::setAltLocOptions(vector<string> opt)
{
   int i;
   ewxChoice *choice = (ewxChoice*)FindWindow(ID_ALT_LOC_CHOICE);
   ewxSpinCtrl *spin = ((ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PDBMODEL));
   int numModels = spin->GetMax();
   choice->Clear();
   if (numModels > 0 && opt[numModels-1].length() > 0) {
      for (i=0; i<opt[numModels-1].length(); i++) {
         choice->Append((opt[numModels-1]).substr(i,1).c_str());
      }
      choice->Select(0);
      choice->Enable(true);
   } else {
      choice->Append("None");
      choice->Select(0);
      choice->Enable(false);
   }
}

/**
 * Set option list of available chains
 */
void WxPDBPrompt::setChainOptions(vector<string> opt)
{
   ewxSpinCtrl *spin = ((ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PDBMODEL));
   int numModels = spin->GetMax();
   int i;
   ewxChoice *choice = (ewxChoice*)FindWindow(ID_CHAIN_CHOICE);
   choice->Clear();
   choice->Append("All");
   if (numModels > 0 && opt[numModels-1].length() > 0) {
      for (i=0; i<opt[numModels-1].length(); i++) {
         choice->Append((opt[numModels-1]).substr(i,1).c_str());
      }
   }
   choice->Select(0);
}


/**
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_OK
 */
void WxPDBPrompt::OnButtonOkClick( wxCommandEvent& event )
{
   EndModal(wxID_OK);
   Close(TRUE);
   event.Skip();
}

/**
 * Process cancel button.
 * Canceling will cause a -1 to be the return code for modal dialog.
 */
void WxPDBPrompt::OnButtonCancelClick( wxCommandEvent& event )
{
   EndModal(wxID_CANCEL);
   Destroy();
   event.Skip();
}

/**
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_HELP
 */

void WxPDBPrompt::OnButtonHelpClick( wxCommandEvent& event )
{
   // Insert custom code here
   event.Skip();
}
