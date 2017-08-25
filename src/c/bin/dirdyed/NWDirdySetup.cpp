#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "NWDirdySetup.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <iostream>
  using std::cerr;
  using std::cout;
  using std::endl;
  using std::ends;
#include <fstream>
  using std::ofstream;
  using std::ifstream;
#include <sstream>
  using std::ostringstream;
#include <strstream>
  using std::istrstream;
  using std::ostrstream;
#include <unistd.h>
#include <values.h>
//#include <stdlib.h>  // abort

#include "util/Ecce.H"
#include "util/CommandWrapper.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/StringConverter.H"
#include "util/Preferences.H"
#include "util/SFile.H"
#include "util/TypedFile.H"
#include "util/SDirectory.H"
#include "util/TempStorage.H"
#include "util/EcceException.H"
#include "util/StringConverter.H"
#include "util/ResourceUtils.H"
#include "util/EditEvent.H"

#include "tdat/NWDirdyModel.H"

#include "dsm/JCode.H"
#include "dsm/CodeFactory.H"

#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxComboBox.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxRegexpValidator.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxUnitHelper.H"
#include "wxgui/ewxProgressDialog.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/WxState.H"
#include "wxgui/WxEditSessionMgr.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxSingleChoiceDialog.H"

#include "NWDirdySetup.H"
#include "NWDirdy.H"

static const string NWDirdyUnitFamily = "AU";
static const string WHOAMI = "NWDirdySetup";

/**
 * Constructor.
 * Create widgets. Initialize GUI state and set up validators and
 * unit helpers.
 */
NWDirdySetup::NWDirdySetup(NWDirdyModel *model, NWDirdy *nwdirdy,
                 wxWindow* parent, wxWindowID id, const wxString& caption,
                 const wxPoint& pos, const wxSize& size, long style) :
              NWDirdySetupGUI(parent, id, caption, pos, size, style)
{
   setNWDirdyModel(model);
   setParent(nwdirdy);
   initializeGUI();
}


NWDirdySetup::~NWDirdySetup()
{
}


/**
 * Sets up validators and unit helpers for input fields.
 */
void NWDirdySetup::initializeGUI()
{
   ewxTextCtrl *text = 0;
   ewxStaticText *label = 0;
   ewxChoice *choice = 0;
   ewxCheckBox *check = 0;
   NWDirdyModel *model = getNWDirdyModel();

   p_feedback = ((WxFeedback*)FindWindow(ID_FEEDBACK_NWDRDYSET));
   p_feedback->hideTopLine();
   p_feedback->hideDropSite();

   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_SYM));
   check->SetValue(model->AUTOSYM);
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_AUTOZ));
   check->SetValue(model->AUTOZ);
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_MEMORY));
   check->SetValue(model->MEMORYFLAG);
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_DISK));
   check->SetValue(model->DISKFLAG);
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_EXPCOEFF));
   check->SetValue(model->EXPCOEFF);
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_SPEXPCOEFF));
   check->SetValue(model->SPEXPCOEFF);

   restrictTheories(p_parent->tsSpinMultiplicity(), true);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_TOL));
   text->SetValidator(ewxNumericValidator(1.0e-10,100000.0,model->SYMTOL));
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_NWDRDYSET_TOL));
   text->setUnitHelper(new ewxUnitHelper(NWDirdyUnitFamily,"Length",label));

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SEP));
   text->SetValidator(ewxNumericValidator(0.0,100000.0,model->FRAGSEP));
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_NWDRDYSET_SEP));
   text->setUnitHelper(new ewxUnitHelper(NWDirdyUnitFamily,"Length",label));

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_MEMORY));
   text->SetValidator(ewxNumericValidator(0,1000000,model->MEMORY));

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_DISK));
   text->SetValidator(ewxNumericValidator(0,1000000,model->DISK));

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SSTEP));
   text->SetValidator(ewxNumericValidator(1.0e-10,100000.0,model->MEPINC));
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_NWDRDYSET_SSTEP));
   text->setUnitHelper(new ewxUnitHelper(NWDirdyUnitFamily,"Length",label));

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SSV));
   text->SetValidator(ewxNumericValidator(1.0e-10,100000.0,model->SAVEINC));
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_NWDRDYSET_SSV));
   text->setUnitHelper(new ewxUnitHelper(NWDirdyUnitFamily,"Length",label));

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SHSS));
   text->SetValidator(ewxNumericValidator(1.0e-10,100000.0,model->HESSINC));
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_NWDRDYSET_SHSS));
   text->setUnitHelper(new ewxUnitHelper(NWDirdyUnitFamily,"Length",label));

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SMSS));
   text->SetValidator(ewxNumericValidator(1.0e-10,100000.0,model->RMASS));
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_NWDRDYSET_SMSS));
   text->setUnitHelper(new ewxUnitHelper(NWDirdyUnitFamily,"Mass",label));

   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_TST));
   check->SetValue(model->TSTONLY);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SLM));
   text->SetValidator(ewxNumericValidator(-100000.0,100000.0,model->SLOW));
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_NWDRDYSET_SLM));
   text->setUnitHelper(new ewxUnitHelper(NWDirdyUnitFamily,"Length",label));

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SLP));
   text->SetValidator(ewxNumericValidator(-100000.0,100000.0,model->SHIGH));
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_NWDRDYSET_SLP));
   text->setUnitHelper(new ewxUnitHelper(NWDirdyUnitFamily,"Length",label));

   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_SGN));
   check->SetValue(model->SWITCH);

   choice = ((ewxChoice*)FindWindow(ID_CHOICE_NWDRDYSET_INTALG));
   choice->SetSelection(model->INTOPT);

   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_PRT));
   check->SetValue(model->PRINT);

   refreshGUI();
}


bool NWDirdySetup::Show( bool show)
{

   if (show) {
      // restore settings - could optimize this to only do it the first time
      // but this is ok for now
      Preferences prefs(WHOAMI);
      restoreWindowSettings(WHOAMI, prefs, false);

      p_parent->setIgnoreFlag(true);
      wxCommandEvent event;
      OnCheckboxNwdrdysetSymClick(event);
      OnCheckboxNwdrdysetMemoryClick(event);
      OnCheckboxNwdrdysetDiskClick(event);
      OnCheckboxNwdrdysetTstClick(event);
      p_parent->setIgnoreFlag(false);
   } else {
      // save current settings
      Preferences prefs(WHOAMI);
      saveWindowSettings(WHOAMI, prefs, false);
      prefs.saveFile();
   }

   return NWDirdySetupGUI::Show(show);
}

void NWDirdySetup::setNWDirdyModel(NWDirdyModel *model)
{
  p_model = model;
}

NWDirdyModel* NWDirdySetup::getNWDirdyModel() const
{
  return p_model;
}

/**
 * Restore all GUI fields from the data model object.
 */
void NWDirdySetup::refreshGUI()
{
   // Change to default units since the model is always in these units
   ewxWindowUtils::setUnitFamily(this, NWDirdyUnitFamily);

   ewxTextCtrl *text = 0;
   ewxChoice *choice = 0;
   ewxCheckBox *check = 0;
   NWDirdyModel *model = getNWDirdyModel();

   // chemical system related parameters
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_SYM));
   check->SetValue(model->getAutoSym());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_TOL));
   text->setValueAsDouble(model->getSymmetryTol());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SEP));
   text->setValueAsDouble(model->getFragmentSep());
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_AUTOZ));
   check->SetValue(model->getAutoZ());

   // memory/disk parameters
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_MEMORY));
   check->SetValue(model->getMemoryFlag());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_MEMORY));
   text->setValueAsInt(model->getMemory());
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_DISK));
   check->SetValue(model->getDiskFlag());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_DISK));
   text->setValueAsInt(model->getDisk());

   // theory parameters
   choice = ((ewxChoice*)FindWindow(ID_CHOICE_NWDRDYSET_TH));
   choice->SetStringSelection(model->getTheory());
   choice = ((ewxChoice*)FindWindow(ID_CHOICE_NWDRDYSET_SPTH));
   string sptheory = model->getSPTheory();
   choice->SetStringSelection(sptheory);

   refreshBasisSetFields();

   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_EXPCOEFF));
   check->SetValue(model->getExpCoeff());
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_SPEXPCOEFF));
   check->SetValue(model->getSPExpCoeff());

   bool theFlag = sptheory != "None";
   FindWindow(ID_BUTTON_NWDRDYSET_SPBASIS)->Enable(theFlag);
   FindWindow(ID_CHECKBOX_NWDRDYSET_SPEXPCOEFF)->Enable(theFlag);
   FindWindow(ID_BUTTON_NWDRDYSET_SPDTLS)->Enable(theFlag);

   // path parameters
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SSTEP));
   text->setValueAsDouble(model->getMEPIncrement());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SSV));
   text->setValueAsDouble(model->getSaveIncrement());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SHSS));
   text->setValueAsDouble(model->getHessianIncrement());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SMSS));
   text->setValueAsDouble(model->getReducedMass());
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_TST));
   check->SetValue(model->getTSTOnly());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SLM));
   text->setValueAsDouble(model->getLowerSLimit());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SLP));
   text->setValueAsDouble(model->getHigherSLimit());
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_SGN));
   check->SetValue(model->getSwitchSign());
   choice = ((ewxChoice*)FindWindow(ID_CHOICE_NWDRDYSET_INTALG));
   choice->SetSelection(model->getIntegrationOpt());
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_PRT));
   check->SetValue(model->getPrintFlag());

   // Now change to user prefered units
   ewxWindowUtils::processGlobalPreferenceChange(this);

   // Now make sure all of the validators contain the forced values above
   TransferDataFromWindow();
}


void NWDirdySetup::refreshBasisSetFields()
{
  string basisName = "None";
  TGBSConfig *genGbsConfig = 0;
  if (p_parent->getDirDyVTSTTask() &&
      (genGbsConfig = p_parent->getDirDyVTSTTask()->
                                DavCalculation::gbsConfig("general"))) {
    basisName = genGbsConfig->name();
  }
  ((ewxStaticText*)FindWindow(ID_STATIC_NWDRDYSET_BASIS_NAME))
            ->SetLabel(basisName);

  NWDirdyModel *model = getNWDirdyModel();
  if (model->getSPTheory() != "None") {
    ((ewxStaticText*)FindWindow(wxID_STATIC_NWDRDYSET_SPBASIS_LABEL))
              ->Enable(true);
    ((ewxStaticText*)FindWindow(ID_STATIC_NWDRDYSET_SPBASIS_NAME))
              ->Enable(true);
    TGBSConfig *spGbsConfig = p_parent->getDirDyVTSTTask()->
                                        DavCalculation::gbsConfig("single");
    basisName = "None";
    if (spGbsConfig != (TGBSConfig*)0) {
      basisName = spGbsConfig->name();
    }
    ((ewxStaticText*)FindWindow(ID_STATIC_NWDRDYSET_SPBASIS_NAME))
              ->SetLabel(basisName);
  } else {
    ((ewxStaticText*)FindWindow(wxID_STATIC_NWDRDYSET_SPBASIS_LABEL))
              ->Enable(false);
    ((ewxStaticText*)FindWindow(ID_STATIC_NWDRDYSET_SPBASIS_NAME))
              ->Enable(false);
    ((ewxStaticText*)FindWindow(ID_STATIC_NWDRDYSET_SPBASIS_NAME))
              ->SetLabel("None");
  }
}


/**
 * Implementation of TaskApp::setUnitFamily.
 * All unitted fields are updated to reflect the new unit family.
 */
void NWDirdySetup::setUnitFamily(const string& family)
{
   ewxWindowUtils::setUnitFamily(this, family);
   Fit();
}


void NWDirdySetup::restrictTheories(const SpinMult::SpinMultEnum& mult,
                                    const bool& initializeFlag)
{
  // restore full list of theories before cutting down to those applicable
  ewxChoice* choice = ((ewxChoice*)FindWindow(ID_CHOICE_NWDRDYSET_TH));
  ewxChoice* spchoice = ((ewxChoice*)FindWindow(ID_CHOICE_NWDRDYSET_SPTH));

  choice->SetSelection(wxNOT_FOUND);
  choice->Clear();

  spchoice->SetSelection(wxNOT_FOUND);
  spchoice->Clear();

  spchoice->Append("None", new wxStringClientData("None"));

  // Hardwire code to NWChem because that's all DirDyVTST supports
  // Don't delete the JCode* returned
  const JCode* code = CodeFactory::lookup("NWChem");

  // populate the list of theories to be the same as all the registered
  // NWChem theories
  vector<TTheory> *theories = code->theories();
  int numTheories = theories->size();
  for (int ic=0; ic<numTheories; ic++) {
    // adds the category as "client data" so it can be a hidden attribute
    // of each item and then accessed when needed to set the full theory
    choice->Append((*theories)[ic].name(),
                   new wxStringClientData((*theories)[ic].category()));
    spchoice->Append((*theories)[ic].name(),
                      new wxStringClientData((*theories)[ic].category()));
  }
  delete theories;

  NWDirdyModel *model = getNWDirdyModel();
  wxString lastTheory = model->getTheory();
  wxString lastCategory = model->getCategory();
  wxString lastSPTheory = model->getSPTheory();
  wxString lastSPCategory = model->getSPCategory();
  bool overrideTheoryFlag = false;
  bool overrideSPTheoryFlag = false;

  string name;
  int it;
  // restrict theories based on open shells/spin multiplicity
  if (mult==SpinMult::singlet || mult==SpinMult::unknown) {
    // remove restricted open shell (RO) theories
    // remove UMP5* for closed shell systems
    for (it = 0; it < (int)choice->GetCount(); ) {
      name = choice->GetString(it).c_str();
      if (name.find("RO") == 0 || name.find("UMP5") == 0) {
        choice->Delete(it);
        spchoice->Delete(it+1);

        if (lastTheory == name)
          overrideTheoryFlag = true;
        if (lastSPTheory == name)
          overrideSPTheoryFlag = true;
      } else {
        it++;
      }
    }
  } else {
    // remove restricted spin theories
    for (it = 0; it < (int)choice->GetCount(); ) {
      name = choice->GetString(it).c_str();
      if (name.find('R') == 0 && name.find('O') != 1) {
        choice->Delete(it);
        spchoice->Delete(it+1);

        if (lastTheory == name)
          overrideTheoryFlag = true;
        if (lastSPTheory == name)
          overrideSPTheoryFlag = true;
      } else {
        it++;
      }
    }
  }

  wxString category;
  // restore the previously set theories if possible or otherwise
  // override to use the first one listed
  // Check if either the previous theory is still supported or if it was
  // an unrestricted theory
  bool uTheoryFlag = !initializeFlag && lastTheory.Find('U')==0;
  if (!overrideTheoryFlag && !uTheoryFlag)
    choice->SetStringSelection(lastTheory);
  else {
    // first try to find a theory in the same category
    for (it = 0; it < (int)choice->GetCount(); it++) {
      category = ((wxStringClientData*)choice->GetClientObject(it))->GetData();
      if (category.IsSameAs(lastCategory)) {
        break;
      }
    }
    if (it < (int)choice->GetCount()) {
      // we found a theory in the same category
      choice->SetSelection(it);
      if (!lastTheory.IsEmpty() && !lastTheory.IsSameAs("None") &&
          choice->GetStringSelection()!=lastTheory) {
        p_parent->getFeedback()->setMessage(("Overriding general theory " +
              lastTheory + " to " + choice->GetStringSelection() + ".").c_str(),
              WxFeedback::WARNING);
      }
    } else {
      if (choice->IsEmpty()) {
        // we removed all theories
        choice->SetSelection(wxNOT_FOUND);
        p_parent->getFeedback()->setMessage("No general theory selections are "
                               "supported.", WxFeedback::ERROR);
      } else {
        // we give up, just pick the first one in the list...
        choice->SetSelection(0);
        if (!lastTheory.IsEmpty() && !lastTheory.IsSameAs("None")) {
          p_parent->getFeedback()->setMessage(("Overriding general theory " +
              lastTheory + " to " + choice->GetStringSelection() + ".").c_str(),
              WxFeedback::WARNING);
        }
      }
    }

    model->setTheory(choice->GetStringSelection().c_str());
    category = ((wxStringClientData*)choice->GetClientObject(choice->GetSelection()))->GetData();
    model->setCategory((char*)category.c_str());
    p_parent->startGeneralTheoryApp(true);
  }

  uTheoryFlag = !initializeFlag && lastSPTheory.Find('U')==0;
  if (!overrideSPTheoryFlag && !uTheoryFlag)
    spchoice->SetStringSelection(lastSPTheory);
  else {
    // first try to find a theory in the same category
    for (it = 0; it < (int)spchoice->GetCount(); it++) {
      category = ((wxStringClientData*)spchoice->GetClientObject(it))->GetData();
      if (category.IsSameAs(lastSPCategory)) {
        break;
      }
    }
    if (it < (int)spchoice->GetCount()) {
      // we found a theory in the same category
      spchoice->SetSelection(it);
      if (!lastSPTheory.IsEmpty() && !lastSPTheory.IsSameAs("None") &&
          spchoice->GetStringSelection()!=lastSPTheory) {
        p_parent->getFeedback()->setMessage(("Overriding single point theory " +
          lastSPTheory + " to " + spchoice->GetStringSelection() + ".").c_str(),
          WxFeedback::WARNING);
      }
    } else {
      if (spchoice->IsEmpty()) {
        // we removed all theories
        spchoice->SetSelection(wxNOT_FOUND);
        p_parent->getFeedback()->setMessage("No single point theory selections "
                               "are supported.", WxFeedback::ERROR);
      } else {
        // we give up, just pick the first one in the list...
        spchoice->SetSelection(0);
        if (!lastSPTheory.IsEmpty() && !lastSPTheory.IsSameAs("None")) {
          p_parent->getFeedback()->setMessage("Clearing single point theory "
                                 "settings.", WxFeedback::WARNING);
        }
      }
    }

    model->setSPTheory(spchoice->GetStringSelection().c_str());
    category = ((wxStringClientData*)spchoice->GetClientObject(
                                       spchoice->GetSelection()))->GetData();
    model->setSPCategory((char*)category.c_str());

    if (spchoice->GetSelection() == 0) {
      getNWDirdyModel()->setUseSP(false);
      FindWindow(ID_BUTTON_NWDRDYSET_SPBASIS)->Enable(false);
      FindWindow(ID_CHECKBOX_NWDRDYSET_SPEXPCOEFF)->Enable(false);
      FindWindow(ID_BUTTON_NWDRDYSET_SPDTLS)->Enable(false);
    } else {
      p_parent->startSingleTheoryApp(true);
    }
  }
}

/**
 * Event handlers
 */
void NWDirdySetup::OnCloseWindow( wxCloseEvent& event )
{
  OnButtonNwdrdysetCloseClick((wxCommandEvent&)event);
}

void NWDirdySetup::OnChoiceNwdrdysetThSelected( wxCommandEvent& event )
{
  ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_NWDRDYSET_TH));
  getNWDirdyModel()->setTheory(choice->GetStringSelection().c_str());
  wxString category = ((wxStringClientData*)choice->GetClientObject(choice->GetSelection()))->GetData();
  getNWDirdyModel()->setCategory((char*)category.c_str());
  p_parent->startGeneralTheoryApp(true);
  p_parent->setSaveState(true);
  event.Skip();
}

void NWDirdySetup::OnChoiceNwdrdysetSpthSelected( wxCommandEvent& event )
{
  ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_NWDRDYSET_SPTH));
  string sptheory = choice->GetStringSelection().c_str();
  getNWDirdyModel()->setSPTheory(sptheory);
  wxString category = ((wxStringClientData*)choice->GetClientObject(choice->GetSelection()))->GetData();
  getNWDirdyModel()->setSPCategory((char*)category.c_str());

  refreshBasisSetFields();

  bool theFlag = sptheory != "None";
  getNWDirdyModel()->setUseSP(theFlag);
  FindWindow(ID_BUTTON_NWDRDYSET_SPBASIS)->Enable(theFlag);
  FindWindow(ID_CHECKBOX_NWDRDYSET_SPEXPCOEFF)->Enable(theFlag);
  FindWindow(ID_BUTTON_NWDRDYSET_SPDTLS)->Enable(theFlag);

  if (theFlag)
    p_parent->startSingleTheoryApp(true);

  p_parent->setSaveState(true);
  event.Skip();
}

void NWDirdySetup::OnButtonNwdrdysetBasisClick( wxCommandEvent& event )
{
  if (p_parent != 0) {
    p_parent->processSave();
    p_parent->requestBasisTool("general");
  }
  event.Skip();
}

void NWDirdySetup::OnButtonNwdrdysetSpbasisClick( wxCommandEvent& event )
{
  if (p_parent != 0) {
    p_parent->processSave();
    p_parent->requestBasisTool("single");
  }
  event.Skip();
}

void NWDirdySetup::OnButtonNwdrdysetDtlsClick( wxCommandEvent& event )
{
  if (p_parent != 0) {
    p_parent->startGeneralTheoryApp();
  }
  event.Skip();
}

void NWDirdySetup::OnButtonNwdrdysetSpdtlsClick( wxCommandEvent& event )
{
  if (p_parent != 0) {
    p_parent->startSingleTheoryApp();
  }
  event.Skip();
}

void NWDirdySetup::OnCheckboxNwdrdysetSymClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_SYM));
  bool set = check->GetValue();

  ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_TOL))->Enable(set);
  ((ewxStaticText*)FindWindow(wxID_STATIC_NWDRDYSET_TOL))->Enable(set);

  getNWDirdyModel()->setAutoSym(set);
  p_parent->setSaveState(true);
  event.Skip();
}

void NWDirdySetup::OnTextctrlNwdrdysetTolEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_TOL));
  getNWDirdyModel()->setSymmetryTol(text->getBaseValueAsDouble());
  p_parent->setSaveState(true);
  event.Skip();
}

void NWDirdySetup::OnTextctrlNwdrdysetSepEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SEP));
  getNWDirdyModel()->setFragmentSep(text->getBaseValueAsDouble());
  p_parent->setSaveState(true);
  event.Skip();
}

void NWDirdySetup::OnTextctrlNwdrdysetSstepEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SSTEP));
  getNWDirdyModel()->setMEPIncrement(text->getBaseValueAsDouble());
  p_parent->setSaveState(true);
  event.Skip();
}

void NWDirdySetup::OnTextctrlNwdrdysetSsvEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SSV));
  getNWDirdyModel()->setSaveIncrement(text->getBaseValueAsDouble());
  p_parent->setSaveState(true);
  event.Skip();
}

void NWDirdySetup::OnTextctrlNwdrdysetShssEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SHSS));
  getNWDirdyModel()->setHessianIncrement(text->getBaseValueAsDouble());
  p_parent->setSaveState(true);
  event.Skip();
}

void NWDirdySetup::OnTextctrlNwdrdysetSmssEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SMSS));
  getNWDirdyModel()->setReducedMass(text->getBaseValueAsDouble());
  p_parent->setSaveState(true);
  event.Skip();
}

void NWDirdySetup::OnCheckboxNwdrdysetTstClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_TST));
  bool set = check->GetValue();
  getNWDirdyModel()->setTSTOnly(set);

  ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SSTEP))->Enable(!set);
  ((ewxStaticText*)FindWindow(wxID_STATIC_NWDRDYSET_SSTEP))->Enable(!set);
  ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SSV))->Enable(!set);
  ((ewxStaticText*)FindWindow(wxID_STATIC_NWDRDYSET_SSV))->Enable(!set);
  ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SHSS))->Enable(!set);
  ((ewxStaticText*)FindWindow(wxID_STATIC_NWDRDYSET_SHSS))->Enable(!set);
  ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SMSS))->Enable(!set);
  ((ewxStaticText*)FindWindow(wxID_STATIC_NWDRDYSET_SMSS))->Enable(!set);
  ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SLM))->Enable(!set);
  ((ewxStaticText*)FindWindow(wxID_STATIC_NWDRDYSET_SLM))->Enable(!set);
  ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SLP))->Enable(!set);
  ((ewxStaticText*)FindWindow(wxID_STATIC_NWDRDYSET_SLP))->Enable(!set);
  ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_SGN))->Enable(!set);
  ((ewxChoice*)FindWindow(ID_CHOICE_NWDRDYSET_INTALG))->Enable(!set);
  ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_PRT))->Enable(!set);

  p_parent->setSaveState(true);
  event.Skip();
}

void NWDirdySetup::OnTextctrlNwdrdysetSlmEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SLM));
  getNWDirdyModel()->setLowerSLimit(text->getBaseValueAsDouble());
  p_parent->setSaveState(true);
  event.Skip();
}

void NWDirdySetup::OnTextctrlNwdrdysetSlpEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_SLP));
  getNWDirdyModel()->setHigherSLimit(text->getBaseValueAsDouble());
  p_parent->setSaveState(true);
  event.Skip();
}

void NWDirdySetup::OnCheckboxNwdrdysetSgnClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_SGN));
  getNWDirdyModel()->setSwitchSign(check->GetValue());
  p_parent->setSaveState(true);
  event.Skip();
}

void NWDirdySetup::OnChoiceNwdrdysetIntalgSelected( wxCommandEvent& event )
{
  ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_NWDRDYSET_INTALG));
  getNWDirdyModel()->setIntegrationOpt(choice->GetSelection());
  p_parent->setSaveState(true);
  event.Skip();
}

void NWDirdySetup::OnCheckboxNwdrdysetPrtClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_PRT));
  getNWDirdyModel()->setPrintFlag(check->GetValue());
  p_parent->setSaveState(true);
  event.Skip();
}

void NWDirdySetup::OnCheckboxNwdrdysetAutozClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_AUTOZ));
  getNWDirdyModel()->setAutoZ(check->GetValue());
  p_parent->setSaveState(true);
  event.Skip();
}

void NWDirdySetup::OnCheckboxNwdrdysetMemoryClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_MEMORY));
  bool set = check->GetValue();

  ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_MEMORY))->Enable(set);
  ((ewxStaticText*)FindWindow(wxID_STATICTEXT_NWDRDYSET_MEMORY))->Enable(set);

  getNWDirdyModel()->setMemoryFlag(set);
  p_parent->setSaveState(true);
  event.Skip();
}

void NWDirdySetup::OnTextctrlNwdrdysetMemoryEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_MEMORY));
  getNWDirdyModel()->setMemory(text->getBaseValueAsInt());
  p_parent->setSaveState(true);
  event.Skip();
}

void NWDirdySetup::OnCheckboxNwdrdysetDiskClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_DISK));
  bool set = check->GetValue();

  ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_DISK))->Enable(set);
  ((ewxStaticText*)FindWindow(wxID_STATICTEXT_NWDRDYSET_DISK))->Enable(set);

  getNWDirdyModel()->setDiskFlag(set);
  p_parent->setSaveState(true);
  event.Skip();
}

void NWDirdySetup::OnTextctrlNwdrdysetDiskEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NWDRDYSET_DISK));
  getNWDirdyModel()->setDisk(text->getBaseValueAsInt());
  p_parent->setSaveState(true);
  event.Skip();
}

void NWDirdySetup::OnCheckboxNwdrdysetExpcoeffClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_EXPCOEFF));
  getNWDirdyModel()->setExpCoeff(check->GetValue());
  p_parent->setSaveState(true);
  event.Skip();
}

void NWDirdySetup::OnCheckboxNwdrdysetSpexpcoeffClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_NWDRDYSET_SPEXPCOEFF));
  getNWDirdyModel()->setSPExpCoeff(check->GetValue());
  p_parent->setSaveState(true);
  event.Skip();
}

void NWDirdySetup::OnButtonNwdrdysetCloseClick( wxCommandEvent& event )
{
  p_parent->closeGeneralTheoryApp(true);
  p_parent->closeSingleTheoryApp(true);

  Show(false);

  //Don't call Skip because it will cause the app to crash when the window
  //manager close is hit
  //event.Skip();
}

void NWDirdySetup::OnButtonNwdrdysetResetClick( wxCommandEvent& event )
{
  getNWDirdyModel()->reset();
  refreshGUI();
  p_parent->setSaveState(true);
  event.Skip();
}

/**
 *  Set a pointer from the parent application so that you can enable
 *  setSaveState from the dialog
 */
void NWDirdySetup::setParent(NWDirdy *parent)
{
  p_parent = parent;
}
