#include <fstream>
  using std::flush;
  using std::ofstream;
#include <signal.h>

#include <wx/combo.h>

#include "util/BrowserHelp.H"
#include "util/ErrMsg.H"
#include "util/InvalidException.H"
#include "util/ResourceUtils.H"
#include "util/StringConverter.H"
#include "util/TempStorage.H"
#include "util/TypedFile.H"
#include "util/EditEvent.H"

#include "tdat/Fragment.H"
#include "tdat/GUIValues.H"

#include "dsm/GBSRules.H"
#include "dsm/TGBSConfig.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIGaussianBasisSetLibrary.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/ICalculation.H"
#include "dsm/ICalcUtils.H"
#include "dsm/JCode.H"
#include "dsm/ResourceTool.H"
#include "dsm/ResourceType.H"
#include "dsm/SummaryIterator.H"
#include "dsm/CodeFactory.H"

#include "wxgui/ThingToggle.H"
#include "wxgui/EcceTool.H"
#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxBoolClientData.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxConfig.H"
#include "wxgui/ewxComboBox.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxNonBoldLabel.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/WxEditSessionMgr.H"
#include "wxgui/WxFeedback.H"

#include "CalcEd.H"
#include "GeomConstraints.H"
#include "PartialCharge.H"

//#define DEBUG

IMPLEMENT_CLASS( CalcEd, CalcEdGUI )

BEGIN_EVENT_TABLE(CalcEd, CalcEdGUI)
  EVT_SOCKET(wxID_THEORY_CHANGE, CalcEd::OnTheoryIPC)
  EVT_SOCKET(wxID_RUNTYPE_CHANGE, CalcEd::OnRuntypeIPC)
END_EVENT_TABLE()

/** Basis set quick pick list.  */
const char* CalcEd::p_BASIS_QUICK_PICKS[] = {
  "STO-3G", "-", // len(2)
  "3-21G", "6-31G", "6-31++G", "6-31G*", "6-31+G*", "6-31++G**", "-", // len(7)
  "cc-pVDZ", "cc-pVTZ", "aug-cc-pVDZ", "aug-cc-pVTZ", "-", // len(5)
  "DZVP (DFT Orbital)", "DZVP2 (DFT Orbital)", "TZVP (DFT Orbital)" // len(3)
}; // len(17)


CalcEd::CalcEd( )
  : CalcEdGUI(),
    WxDavAuth(),
    JMSPublisher(CALCED),
    FeedbackSaveHandler(),
    CalcDropHandler(),
    EditListener(),
    p_feedback(NULL),
    p_builderTool(NULL),
    p_basisSetTool(NULL),
    p_codeName(""),
    p_code(NULL),
    p_GUIValues(NULL),
    p_iCalc(NULL),
    p_frag(NULL),
    p_fullFrag(NULL),
    p_basis(NULL),
    p_lastTheory(),
    p_lastRuntype(),
    p_geomConstraints(NULL),
    p_partialCharge(NULL),
    p_ESPCnstrnt(NULL),
    p_context(""),
    p_startUp(false),
    p_theoryPid(0),
    p_theoryInFilePath(""),
    p_theoryOutFile(NULL),
    p_theoryInSocket(NULL),
    p_theoryInPort(0),
    p_theoryInFlag(false),
    p_theoryHoldFlag(false),
    p_theoryInitFlag(false),
    p_runtypePid(0),
    p_runtypeInFilePath(""),
    p_runtypeOutFile(NULL),
    p_runtypeInSocket(NULL),
    p_runtypeInPort(0),
    p_runtypeInFlag(false),
    p_runtypeHoldFlag(false),
    p_runtypeInitFlag(false)
{

}


CalcEd::CalcEd( wxWindow* parent, wxWindowID id, const wxString& caption,
                const wxPoint& pos, const wxSize& size, long style)
  : CalcEdGUI(),
    WxDavAuth(),
    JMSPublisher(CALCED),
    FeedbackSaveHandler(),
    CalcDropHandler(),
    EditListener(),
    p_feedback(NULL),
    p_builderTool(NULL),
    p_basisSetTool(NULL),
    p_codeName(""),
    p_code(NULL),
    p_GUIValues(NULL),
    p_iCalc(NULL),
    p_frag(NULL),
    p_fullFrag(NULL),
    p_basis(NULL),
    p_lastTheory(),
    p_lastRuntype(),
    p_geomConstraints(NULL),
    p_partialCharge(NULL),
    p_ESPCnstrnt(NULL),
    p_context(""),
    p_startUp(false),
    p_theoryPid(0),
    p_theoryInFilePath(""),
    p_theoryOutFile(NULL),
    p_theoryInSocket(NULL),
    p_theoryInPort(0),
    p_theoryInFlag(false),
    p_theoryHoldFlag(false),
    p_theoryInitFlag(false),
    p_runtypePid(0),
    p_runtypeInFilePath(""),
    p_runtypeOutFile(NULL),
    p_runtypeInSocket(NULL),
    p_runtypeInPort(0),
    p_runtypeInFlag(false),
    p_runtypeHoldFlag(false),
    p_runtypeInitFlag(false)
{
  Create(parent, id, caption, pos, size, style);

  restoreSettings();
}


CalcEd::~CalcEd( )
{
  delete p_ESPCnstrnt;
  closeTheoryApp(true);
  closeRuntypeApp(true);
  ewxConfig::closeConfigs();
}


bool CalcEd::Create( wxWindow* parent, wxWindowID id, const wxString& caption,
                     const wxPoint& pos, const wxSize& size, long style)
{
  if (!CalcEdGUI::Create(parent, id, caption, pos, size, style)) {
    wxFAIL_MSG( wxT("CalcEd creation failed") );
    return false;
  }

  CreateControls();
  //GetSizer()->SetSizeHints(this);
  //GetSizer()->SetMinSize(SYMBOL_CALCEDGUI_SIZE);
  Centre();

  // set desktop icon
  SetIcon(wxIcon(ewxBitmap::pixmapFile(
          ResourceDescriptor::getResourceDescriptor().getTool(CALCED)
          ->getIcon()), wxBITMAP_TYPE_XPM));
  EDSIFactory::addAuthEventListener(this);

  return true;
}


void CalcEd::createCodeButtons()
{
  // create bitmap buttons for electronic structure codes
  bool rxnFlag = false;
  if (p_iCalc != (ICalculation*)0) {
    Resource *parent = EDSIFactory::getResource(p_iCalc->getURL().getParent());
    rxnFlag = parent->getApplicationType() ==
                         ResourceDescriptor::AT_REACTION_STUDY;
  }

  // remove any existing code buttons since this is called each time
  // the context is changed
  wxSizerItemList codeChildren = p_codeSizer->GetChildren();
  wxSizerItemList::compatibility_iterator codeNode = codeChildren.GetFirst();
  wxWindow *codeChild;

  while (codeNode) {
    codeChild = codeNode->GetData()->GetWindow();
    p_codeSizer->Detach(codeChild);
    p_codeSizer->Layout();
    (void)codeChild->Destroy();
    codeNode = codeNode->GetNext();
  }

  ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();
  ResourceType *rt;
  vector<string> codes = CodeFactory::getFullySupportedCodes();
  wxCustomButton *codeButton;
  wxSize margins(5,5);

  for (size_t it = 0; it < codes.size(); it++) {
    rt = rd.getResourceType("virtual_document", "ecceCalculation", codes[it]);
    if (rt!=(ResourceType*)0 && rt->getFactoryCategory()=="EsCalculation" &&
        !(rxnFlag && codes[it]=="Amica")) {
      // add the index into the list of supported codes as a way of
      // retrieving which code button was hit in the callback
      codeButton = new wxCustomButton(this, ID_BUTTON_CALCED_CODE,
                               ewxBitmap(rt->getIcon(), wxBITMAP_TYPE_XPM),
                               wxDefaultPosition, wxDefaultSize,
                               wxCUSTBUT_BUT_DCLICK_TOG);
      codeButton->SetClientData((void*)it);
      codeButton->SetMargins(margins);
      codeButton->SetToolTip(codes[it]);
      p_codeSizer->Add(codeButton, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);
    }
  }
}

void CalcEd::OnButtonCalcedCodeClick(wxCommandEvent &event)
{
  wxCustomButton *eventButton = (wxCustomButton*)event.GetEventObject();
  unsigned long icode = (unsigned long)eventButton->GetClientData();

  wxSizerItemList children = p_codeSizer->GetChildren();
  wxSizerItemList::compatibility_iterator node = children.GetFirst();

  wxSizerItem *child;
  wxCustomButton *codeButton;

  // enforce radio button behavior
  while (node) {
    child = node->GetData();
    codeButton = (wxCustomButton*)child->GetWindow();
    if (codeButton != eventButton) {
      codeButton->SetValue(false);
    } else {
      codeButton->SetValue(true);
    }
    node = node->GetNext();
  }

  vector<string> codes = CodeFactory::getFullySupportedCodes();
  if (codes[icode] != p_codeName) {
    p_startUp = true;
    doSetContext(codes[icode]);
    p_startUp = false;
  }
}


void CalcEd::setCurrentCodeButton(const string& currentCode)
{
  vector<string> codes = CodeFactory::getFullySupportedCodes();
  unsigned long icode;
  for (icode=0; icode<codes.size() && codes[icode]!=currentCode; icode++);
  if (icode < codes.size()) {
    wxSizerItemList children = p_codeSizer->GetChildren();
    wxSizerItemList::compatibility_iterator node = children.GetFirst();
    wxSizerItem *child;
    wxCustomButton *codeButton;
    while (node) {
      child = node->GetData();
      codeButton = (wxCustomButton*)child->GetWindow();
      if ((unsigned long)codeButton->GetClientData() == icode) {
        codeButton->SetValue(true);
      } else {
        codeButton->SetValue(false);
      }
      node = node->GetNext();
    }
  }
}

void CalcEd::setContext(const string& url, const string& codeName)
{
  // Make sure it is raised/uniconified for popup dialogs
  Raise();

  // A calculation with unsaved changes?
  if (p_iCalc && url != p_context
          && p_feedback->getEditStatus() == WxFeedback::MODIFIED) {
    ewxMessageDialog *dialog = new ewxMessageDialog(this,
            "The current calculation has unsaved changes!  Do you "
            "want to save changes before changing context?",
            "Ecce Change Context?", wxYES_NO|wxICON_QUESTION);
    wxWindowID answer = dialog->ShowModal();
    if (answer == wxID_YES) {
      doSave();
    }
  } else if (p_iCalc && url==p_context &&
             (codeName=="" || p_codeName==codeName)) {

    // bail because it's the same calculation
    return;
  }

  // decided to set context...
  // first let's make sure all is ok with the object we were handed
  bool doit = false;
  ICalculation *givenCalc =
          dynamic_cast<ICalculation*>(EDSIFactory::getResource(url));
  try {
    if (givenCalc) {
      doit = givenCalc->isValid();
    } else {
      p_feedback->setMessage("Could not retrieve calculation at "
              + url  + ".", WxFeedback::ERROR);
    }
  } catch (CancelException& ex) {
    p_feedback->setMessage("Cancelled validation during context switch.",
            WxFeedback::INFO);
  } catch (RetryException& ex) {
    ewxMessageDialog *dialog = new ewxMessageDialog(this,
            "The maximum number of authentication attempts has been "
            "exceeded.  Contact your ECCE administrator if you have "
            "forgotten your password.", "Authentication Failure");
    dialog->ShowModal();
  }

  if (doit) {
    freeContext();
    bool msgFlag = p_iCalc != (ICalculation*)0;
    p_iCalc = givenCalc;
    p_startUp = true;
    doSetContext(codeName);
    p_startUp = false;
    if (msgFlag) {
      p_feedback->setMessage("Calculation context set to " + p_iCalc->getName()
                             + ".", WxFeedback::INFO);
    }
  } else if (p_iCalc != (ICalculation*)0) {
    p_feedback->setMessage("Calculation context was not changed.",
                           WxFeedback::INFO);
  }
}


void CalcEd::doSetContext(const string& codeName)
{
  // hack to support two builder implementations
  fixBuilderButtonId();

  // make sure detail dialogs are closed
  closeTheoryApp(true);
  closeRuntypeApp(true);

  p_context = p_iCalc->getURL().toString();

  // initialize the feedback area
  p_feedback->clearMessage();
  p_feedback->setContextURL(p_context);
  if (!p_iCalc->messages().empty()) {
    // if the calculation contains messages, something is wrong
    p_feedback->setContextLabel("");
    p_feedback->setMessage(p_iCalc->messages(), WxFeedback::ERROR);
    setEditStatus(WxFeedback::READONLY);
    return;
  }
  p_feedback->setContextLabel(EDSIServerCentral().mapURLtoName(p_context));
  p_feedback->setRunState(p_iCalc->getState());
  if (p_feedback->getRunState() > ResourceDescriptor::STATE_READY) {
    setEditStatus(WxFeedback::READONLY);
  }
  enableSave(false);

  // check if they are switching codes
  const JCode* currApp = p_iCalc->application();
  string tmpCode;

  if (codeName=="" ||
      p_feedback->getRunState()>ResourceDescriptor::STATE_READY) {
    tmpCode = currApp->name();
  } else {
    if (currApp != (JCode*)0) {
      if (p_codeName == "") {
        p_codeName = currApp->name();
      }

      if (p_codeName!=codeName && p_iCalc->theory()!=(TTheory*)0) {
        long buttonFlags = wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION;
        ewxMessageDialog *dialog = new ewxMessageDialog(this,
                "The code for the selected calculation is different!  Do you "
                "want to change the code?",
                "Electronic Structure Editor Code Change", buttonFlags);
        wxWindowID answer = dialog->ShowModal();
        if (answer == wxID_YES) {
          tmpCode = codeName;
        } else {
          tmpCode = p_codeName;
        }
      } else {
        tmpCode = codeName;
      }
    } else {
      tmpCode = codeName;
    }
  }

  // initialize application code and its visible presence on the frame
  p_code = CodeFactory::lookup(tmpCode.c_str());
  if (!isValidCode(p_code)) {
    return;
  }

  SetTitle("ECCE " + p_code->name() + " Editor");
  p_detailsBox->SetLabel(p_code->name() + " Settings");

  createCodeButtons();
  setCurrentCodeButton(p_code->name());

  p_frag = p_iCalc->fragment();
  if (p_frag) {
    p_ESPCnstrnt = new ESPConstraintModel(*p_frag);
    p_fullFrag = new Fragment(*p_frag);
    if (p_frag->useSymmetry()) {
      p_fullFrag->generateFullMolecule();
    }

    // initialize spin multiplicities list and selection
    populateSpinMultiplicities();
    setSpinMult(p_iCalc->spinMultiplicity(), false);

    // initialize theory/runtype
    setContextTheoryRuntype();
  }

  // initialize basis set
  p_basis = p_iCalc->gbsConfig();

  // initialize GUIValues
  // depends on fragment, basis, code, and runstate

  if (currApp!=(JCode*)0 && p_codeName!="" && p_codeName!=p_code->name()) {
    if (p_iCalc->theory() != (TTheory*)0) {
      string msg = "Overriding code from ";
      msg += p_codeName;
      msg += " to ";
      msg += p_code->name();
      msg += ".  This will reset values on theory and runtype dialogs to "
             "defaults.";
      p_feedback->setMessage(msg, WxFeedback::WARNING);
    }

    p_GUIValues = new GUIValues();
    enableSave();
  } else if (!currApp) {
    p_GUIValues = new GUIValues();
    enableSave();
  } else if (isReady() && p_iCalc->isFragmentNew()) {
    p_feedback->setMessage("Chemical system has changed since the "
            "Electronic Structure Editor was last up.  Resetting theory and "
            "runtype details dialog values to defaults.", WxFeedback::WARNING);
    p_GUIValues = new GUIValues();
    enableSave();
  } else if (isReady() && !p_iCalc->isInputFileNew()) {
    // this can be either a new basis set or the input file was deleted
    // check for which case before issuing message
    if (p_iCalc->getDataFileNames(JCode::INPUT).size() > 0) {
      p_feedback->setMessage("Basis set has changed since the "
              "Electronic Structure Editor was last up.", WxFeedback::WARNING);
    } else {
      p_feedback->setMessage("Input file does not exist.  Hit save to "
              "generate input file.", WxFeedback::WARNING);
    }
    p_GUIValues = p_iCalc->guiparams();
    enableSave();

    // uncomment the next line to actually generate the input file 
    // also remove the feedback message above about hitting save
    //doSave();
  } else if (isReady() &&
             p_iCalc->getState()==ResourceDescriptor::STATE_CREATED) {
    // this covers the case of "duplicate setup with last geometry"
    p_feedback->setMessage("Existing input file is inconsistent.  Hit save to "
            "generate new input file.", WxFeedback::WARNING);
    p_GUIValues = new GUIValues();
    enableSave();
  } else {
    p_GUIValues = p_iCalc->guiparams();
  }

  p_codeName = p_code->name();

  updateAllFields();

  if (isDetailsReady() && p_GUIValues->size() == 0) {
    startTheoryApp(true);
    startRuntypeApp(true);
  }

  // GDB 2/19/13  Fix for dialog states getting cleared between app invocations
  // Always restore the underlying model regardless of whether dialog is invoked
  updateGeomModel();
  p_geomConstraints = new GeomConstraints(this, this, wxID_ANY, "Geometry Constraints");

  updateESPModel();
  p_partialCharge = new PartialCharge(this);
}


/**
 * Set the main window runtype/theory choices to the current calculation
 * values.
 */
void CalcEd::setContextTheoryRuntype()
{
  // initialize theories list and selection
  restrictTheoriesBySpin();
  TTheory *theory(p_iCalc->theory());
  if (!setTheory(theory, false)) {
    enableSave();
  }
  delete theory;
  // initialize runtypes list and selection
  populateRuntypes();           
  if (!setRuntype(p_iCalc->runtype())) {
    enableSave();
  }

  makeRuntypeNoSphericalConsistent();
}


void CalcEd::freeContext()
{
  if (p_frag) {
    delete p_frag;
    p_frag = 0;
  }

  if (p_fullFrag) {
    delete p_fullFrag;
    p_fullFrag = 0;
  }

  if (p_GUIValues) {
    delete p_GUIValues;
    p_GUIValues = 0;
  }

  if (p_basis) {
    delete p_basis;
    p_basis = 0;
  }

  if (p_ESPCnstrnt) {
    delete p_ESPCnstrnt;
    p_ESPCnstrnt = 0;
  }

  if (p_geomConstraints) {
    delete p_geomConstraints;
    p_geomConstraints = 0;
  } 

  if (p_partialCharge) {
    delete p_partialCharge;
    p_partialCharge = 0;
  } 

  p_code = 0;
  p_context = "";
  p_iCalc = 0;
 
  // clear the feedback area
  p_feedback->clearMessage();
  p_feedback->setContextURL("");
  p_feedback->setContextLabel("");
  p_feedback->setRunState(ResourceDescriptor::STATE_CREATED);
  setEditStatus(WxFeedback::EDIT);
  enableSave(false);

  populateSpinMultiplicities();
  populateTheories();
  populateRuntypes();

  updateAllFields();
}


string CalcEd::getContext() const
{
  return p_context;
}


void CalcEd::setEditStatus(WxFeedback::EditStatus status)
{
  p_feedback->setEditStatus(status);
}


Fragment *CalcEd::getFrag() const
{
  return p_frag;
}


WxFeedback *CalcEd::getFeedback()
{
  return p_feedback;
}


ESPConstraintModel *CalcEd::getESPConstraintModel()
{
  return p_ESPCnstrnt;
}


void CalcEd::processSave()
{
  doSave();
}


void CalcEd::processDrop(const string& url)
{
  setContext(url);
}


void CalcEd::processEditCompletion(const EditEvent& ee)
{
  ifstream ifs(ee.filename.c_str());

  string infile;
  TypedFile tinfile;
  p_iCalc->getDataFile(JCode::PRIMARY_INPUT, tinfile);
  infile = tinfile.name();
    
  if (!p_iCalc->putInputFile(infile, &ifs))
    p_feedback->setMessage("Input file could not be copied back to DAV",
                           WxFeedback::ERROR);
  ifs.close();
}


/**
 * The user clicked on one of the EcceTools (Builder, Basis, Launcher).
 */
void CalcEd::OnToolClick(wxCommandEvent &event)
{
  doSave();
  startApp(event.GetId(), 0, p_context);
}


void CalcEd::OnCloseWindow( wxCloseEvent& event )
{
  saveSettings();
  doClose(event.CanVeto());
}


void CalcEd::OnSaveClick( wxCommandEvent& event )
{
  doSave();
  event.Skip();
}


void CalcEd::OnMenuCalcedSavePrefClick( wxCommandEvent& event )
{
  ewxConfig *config = ewxConfig::getConfig("wxcalced.ini");
  config->Write("Theory", getTheoryName());
  config->Write("Category", getTheoryCategory());
  config->Write("Runtype", getRuntypeName());
  ewxConfig::closeConfigs();

  event.Skip();
}


void CalcEd::OnExitClick( wxCommandEvent& event )
{
  saveSettings();
  doClose();
}


void CalcEd::OnHelpClick( wxCommandEvent& event )
{
  BrowserHelp help;
  help.showPage(help.URL("CalculationEditor"));
}


void CalcEd::OnMenuFeedbackClick( wxCommandEvent& event )
{
  BrowserHelp help;
  help.showFeedbackPage();
}


void CalcEd::OnTextctrlCalcedNameEnter( wxCommandEvent& event )
{
  if (p_frag) {
    enableSave();
    p_frag->name(event.GetString().c_str());
    p_fullFrag->name(event.GetString().c_str());
  }

  event.Skip();
}


void CalcEd::OnComboboxCalcedChargeSelected( wxCommandEvent& event )
{
  if (p_frag) {
    int value;
    if (StringConverter::toInt(event.GetString().c_str(), value)) {
      enableSave();
      p_frag->charge(value);
      p_fullFrag->charge(value);
      updateChemSysFields();
      restrictSpinByCharge();
    }
  }

  event.Skip();
}


void CalcEd::OnComboboxCalcedChargeEnter( wxCommandEvent& event )
{
  OnComboboxCalcedChargeSelected(event);
}


void CalcEd::OnComboboxCalcedSpinMultSelected( wxCommandEvent& event )
{
  setSpinMult(SpinMult::toSpinMult(event.GetString().c_str()), true);
  setOpenShells(getSpinMult() - 1);
  restrictTheoriesBySpin();
  enableSave();

  event.Skip();
}


void CalcEd::OnComboboxCalcedSpinMultEnter( wxCommandEvent& event )
{
  string valstr = event.GetString().c_str();
  if (valstr != "") {
    setSpinMult(SpinMult::toSpinMult(valstr), true);
    setOpenShells(getSpinMult() - 1);
    restrictTheoriesBySpin();
    enableSave();
  }

  event.Skip();
}


void CalcEd::OnCheckboxCalcedIrreducibleClick( wxCommandEvent& event )
{
  if (p_frag) {
    if (p_fullFrag) delete p_fullFrag;
    p_fullFrag = new Fragment(*p_frag);
    if (event.IsChecked()) {
      p_fullFrag->generateFullMolecule();
    }
    p_frag->useSymmetry(event.IsChecked());

    updateChemSysFields();
    updateBasisSetFields();

    // parameter forces recalculation of frozen core default, etc. even if
    // the theory doesn't need to be updated for the modified chemical system
    restrictSpinByCharge(true);
  }

  event.Skip();
}


void CalcEd::OnButtonCalcedBasisQuickClick( wxCommandEvent& event )
{
  wxMenu menu;
  for (int i = 0; i < 17; i++) {
    if (strcmp(p_BASIS_QUICK_PICKS[i], "-") == 0) {
      menu.AppendSeparator();
    } else {
      menu.Append(100000 + i, p_BASIS_QUICK_PICKS[i]);
    }
  }
  PopupMenu(&menu, FindWindow(event.GetId())->GetPosition());
}


void CalcEd::OnChoiceCalcedTheorySelected( wxCommandEvent& event )
{
  if (p_lastTheory != getTheory()) {
    wxString lastTheoryName = p_lastTheory.name();
    if (!lastTheoryName.IsEmpty() && !lastTheoryName.IsSameAs("None")) {
      p_feedback->setMessage("Changing the theory level overrides all detail "
              "field changes.", WxFeedback::WARNING);
    }
    p_lastTheory = getTheory();
    enableSave();
    doTheoryChange();
  }

  event.Skip();
}


void CalcEd::OnChoiceCalcedRuntypeSelected( wxCommandEvent& event )
{
  if (p_lastRuntype != getRuntype()) {
    p_lastRuntype = getRuntype();
    enableSave();
    doRuntypeChange();
  }

  event.Skip();
}


void CalcEd::OnMenuCalcedBasisSetSelected( wxCommandEvent& event )
{
  event.Skip();

  EDSIServerCentral central;
  EDSIGaussianBasisSetLibrary *gbsFactory = new EDSIGaussianBasisSetLibrary(
          central.getDefaultBasisSetLibrary());

  if (p_basis) {
    delete p_basis;
  }
  p_basis = gbsFactory->simpleLookup(p_BASIS_QUICK_PICKS[event.GetId()-100000],
                                     p_frag->uniqueTagStr().c_str());

  TTheory theory = getTheory();
  GBSRules::autoOptimize(p_basis, p_code, &theory);
  makeRuntypeNoSphericalConsistent();

  updateBasisSetFields();
  enableLaunch();

  enableSave();
}


void CalcEd::OnCheckboxCalcedUseExponentsClick( wxCommandEvent& event )
{
  enableSave();

  event.Skip();
}


void CalcEd::OnButtonCalcedTheoryClick( wxCommandEvent& event )
{
  startTheoryApp(false);

  event.Skip();
}


void CalcEd::OnButtonCalcedRuntypeClick( wxCommandEvent& event )
{
  startRuntypeApp(false);

  event.Skip();
}


void CalcEd::OnButtonCalcedPartialClick( wxCommandEvent& event )
{
  if (p_partialCharge == 0) {
      updateESPModel();
      p_partialCharge = new PartialCharge(this);
  }
  p_partialCharge->Show();

  event.Skip();
}


void CalcEd::OnButtonCalcedConstraintClick( wxCommandEvent& event )
{
  if (p_geomConstraints == 0) {
    updateGeomModel();
    p_geomConstraints = new GeomConstraints(this, this, wxID_ANY, "Geometry Constraints");
  }

  enableGeomConstraints();
  p_geomConstraints->Show(true);
  p_geomConstraints->Raise();

  event.Skip();
}

/**
 * Persistently delete the esp constraint model and clean up memory.
 */
void CalcEd::clearESPModel()
{
   ChemistryTask *task = dynamic_cast<ChemistryTask*>(p_iCalc);
   if (task) {
      task->setESPModel(0);
      if (p_ESPCnstrnt != 0) {
         delete p_ESPCnstrnt;
         p_ESPCnstrnt = 0;
      }
   }
}


/**
 * Persistently delete the geometry model and clean up memory.
 */
void CalcEd::clearGeomModel()
{
   ChemistryTask *task = dynamic_cast<ChemistryTask*>(p_iCalc);
   if (task) {
      task->setGeomConstraintModel(0);
      if (p_frag) {
         GeomConstraintModel *model = p_frag->getConstraints();
         delete model;
         p_frag->setConstraints(0);
      }
   }
}

void CalcEd::updateESPModel()
{
   if (p_frag) {
      if (p_ESPCnstrnt != 0) delete p_ESPCnstrnt;

      p_ESPCnstrnt = new ESPConstraintModel(*p_frag);
      p_ESPCnstrnt->setFragment(*p_frag);
      p_iCalc->getESPModel(*p_ESPCnstrnt);
   }
}

void CalcEd::updateGeomModel()
{
  if (p_frag) {
    GeomConstraintRules *geomRules = new GeomConstraintRules();
    GeomConstraintModel *geomModel = new GeomConstraintModel(*p_frag, *geomRules);
    p_frag->setConstraints(geomModel);
    if (p_iCalc) {
      ChemistryTask *calc = dynamic_cast<ChemistryTask*>(p_iCalc);
      if (calc) {
        calc->getGeomConstraintModel(*geomModel);
      }
    }
  }
}


void CalcEd::OnButtonCalcedFinalEditClick( wxCommandEvent& event )
{
  if (p_iCalc) {
    try {
      if (p_feedback->getEditStatus() == WxFeedback::MODIFIED) {
        doSave(); // also generates input file
      }
  
      istream* is = p_iCalc->getDataFile(JCode::PRIMARY_INPUT);
      if (is) {
        bool isReadOnly(p_feedback->getRunState() >
                        ResourceDescriptor::STATE_READY);
        if (isReadOnly) {
          p_feedback->setMessage("Input file is read only.  "
                  "Changes cannot be saved.", WxFeedback::INFO);
        } else {
          p_feedback->setMessage("In order for Final Edit changes to be "
                  "applied you must launch the task without making any "
                  "further changes.", WxFeedback::INFO);
        }
  
        string text;
        StringConverter::streamToText(*is, text);
        delete is;
  
        WxEditSessionMgr sessionMgr;
        sessionMgr.edit(text, "testing", this, isReadOnly,
                        p_codeName + " Input");
      } else {
        p_feedback->setMessage("Input file not found.", WxFeedback::WARNING);
      }
    } catch (EcceException& ex) {
      p_feedback->setMessage(ex.what(), WxFeedback::ERROR);
    }
  }

  event.Skip();
}


void CalcEd::OnButtonCalcedLaunchClick( wxCommandEvent& event )
{
  ResourceTool *tool =
          ResourceDescriptor::getResourceDescriptor().getTool(LAUNCHER);
  event.SetId(tool->getId());
  OnToolClick(event);
}


void CalcEd::moveMCB(wxCommandEvent& event)
{
  p_feedback->setContextURL(p_iCalc->getURL());
}


void CalcEd::deleteMCB(wxCommandEvent& event)
{
  freeContext();
}


void CalcEd::basisMCB(wxCommandEvent& event)
{
  enableSave();

  p_basis = p_iCalc->gbsConfig();

  makeRuntypeNoSphericalConsistent();

  updateBasisSetFields();
  enableLaunch();

  // auto-save
  doSave();
}


void CalcEd::codeMCB(wxCommandEvent& event)
{
  restrictSpinByCharge(true);
}


void CalcEd::detailsMCB(wxCommandEvent& event)
{
}


void CalcEd::launchMCB(wxCommandEvent& event)
{
  enableAllFields();
}


void CalcEd::runtypeMCB(wxCommandEvent& event)
{
}


void CalcEd::spinMCB(wxCommandEvent& event)
{
}


void CalcEd::stateMCB(wxCommandEvent& event)
{
  if (p_iCalc->getState() > ResourceDescriptor::STATE_READY) {
    setEditStatus(WxFeedback::READONLY);
  } else {
    setEditStatus(WxFeedback::EDIT);
  }
  enableAllFields();
}


void CalcEd::subjectMCB(wxCommandEvent& event)
{
  // reinitialize fragment(s)
  if (p_frag) {
    delete p_frag;
    p_frag = 0;
  }
  if (p_fullFrag) {
    delete p_fullFrag;
    p_fullFrag = 0;
  }

  // I believe the only correct action here can be to blow the models away
  clearESPModel();
  if (p_partialCharge) p_partialCharge->initializeGUI();

  clearGeomModel();
  if (p_geomConstraints) p_geomConstraints->initDisplay(false);

  p_frag = p_iCalc->fragment();
  if (p_frag) {

    p_fullFrag = new Fragment(*p_frag);
    if (p_frag->useSymmetry()) {
      p_fullFrag->generateFullMolecule();
    }
    updateGeomModel();
  }

  setSpinMult(p_iCalc->spinMultiplicity(), false);
  restrictSpinByCharge();
  //setContextTheoryRuntype();

  // GDB 7/24/13 runtypes are restricted for single atom systems so
  // make them consistent with the new chemical system
  populateRuntypes();           
  setRuntype(p_lastRuntype);

  updateAllFields();

  if (isDetailsReady() && p_GUIValues->size() == 0) {
    startTheoryApp(true);
    startRuntypeApp(true);
  }

  // auto-save
  enableSave();
  doSave();
}


void CalcEd::theoryMCB(wxCommandEvent& event)
{
}


void CalcEd::propertyMCB(wxCommandEvent& event)
{
  // this could be an annotation change
  if (p_iCalc->getState() <= ResourceDescriptor::STATE_READY) {
    // auto-save
    enableSave();
    doSave();
  }
}


void CalcEd::CreateControls()
{
  p_feedback = new WxFeedback(this);
  p_feedback->setDropHandler(this);
  p_feedback->setSaveHandler(this);
  GetSizer()->Add(p_feedback, 0, wxEXPAND, 0);

  replaceBuilderButton();
  replaceBasisSetButton();

  Connect( 100000, 100016, wxEVT_COMMAND_MENU_SELECTED,
           wxCommandEventHandler( CalcEd::OnMenuCalcedBasisSetSelected ) );
}


void CalcEd::replaceBuilderButton()
{
  // Hack to support duality of Builders (wx vs motif)
  // Create an EcceTool now to fill the space.  When setContext is called
  // with a valid calculation, we'll determine which Builder is to be 
  // invoked and reset the ID of the EcceTool.
  ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();
  ResourceTool *tool = rd.getTool(BUILDER);
  p_builderTool = new EcceTool(this, tool);
  Connect( tool->getId(), wxEVT_COMMAND_BUTTON_CLICKED,
           wxCommandEventHandler( CalcEd::OnToolClick ) );

  // find and replace the Builder button placeholder
  wxWindow *placeHolder = FindWindow(ID_BUTTON_CALCED_BUILDER);
  placeHolder->GetContainingSizer()->Replace(placeHolder, p_builderTool);
  placeHolder->Destroy();
}


void CalcEd::replaceBasisSetButton()
{
  // create our replacement
  ResourceTool *tool =
          ResourceDescriptor::getResourceDescriptor().getTool(BASISTOOL);
  p_basisSetTool = new EcceTool(this, tool);
  Connect( tool->getId(), wxEVT_COMMAND_BUTTON_CLICKED,
           wxCommandEventHandler( CalcEd::OnToolClick ) );

  // find and replace the Basis Set button placeholder
  wxWindow *placeholder = FindWindow(ID_BUTTON_CALCED_BASIS_SET);

  placeholder->GetContainingSizer()->Replace(placeholder, p_basisSetTool);
  placeholder->Destroy();
}


void CalcEd::fixBuilderButtonId()
{
  vector<ResourceTool*> tools = p_iCalc->getDescriptor()->getTools();
  vector<ResourceTool*>::iterator tool;
  for (tool = tools.begin(); tool != tools.end(); tool++) {
    string name = (*tool)->getName();
    if (name == BUILDER) {
      break;
    }
  }
  Disconnect( p_builderTool->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
              wxCommandEventHandler( CalcEd::OnToolClick ) );
  p_builderTool->SetId((*tool)->getId());
  Connect( p_builderTool->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
           wxCommandEventHandler( CalcEd::OnToolClick ) );
}


/**
 * Get currently selected theory category or empty string if none selected.
 */
wxString CalcEd::getTheoryCategory() const
{
  ewxChoice *choice = ((ewxChoice*) FindWindow(ID_CHOICE_CALCED_THEORY));
  int n = choice->GetSelection();
  if (n == wxNOT_FOUND) {
    return wxString();
  } else {
    return ((wxStringClientData*)choice->GetClientObject(n))->GetData();
  }
}


/**
 * Get currently selected theory name or empty string if none selected.
 */
wxString CalcEd::getTheoryName() const
{
  ewxChoice *choice = (ewxChoice*)FindWindow(ID_CHOICE_CALCED_THEORY);
  if (choice->GetStringSelection().IsSameAs(getTheoryCategory())) {
    return "None";
  } else {
    return choice->GetStringSelection();
  }
}


/**
 * Get currently selected theory.
 * If none selected, theory name and category fields will be empty.
 */
TTheory CalcEd::getTheory() const
{
  return TTheory(getTheoryCategory(), getTheoryName());
}


wxString CalcEd::getRuntypeName() const
{
  return ((ewxChoice*)FindWindow(ID_CHOICE_CALCED_RUNTYPE))
          ->GetStringSelection();
}


/**
 * Returns value of selected runtype's "NoSpherical" value.
 * Will retur false if no runtype is selected, so there is some
 * ambiguity if false is returned.
 */
bool CalcEd::getRuntypeNoSpherical() const
{
  ewxChoice *choice = (ewxChoice*)FindWindow(ID_CHOICE_CALCED_RUNTYPE);
  int n = choice->GetSelection();
  return (n != wxNOT_FOUND)
          && ((ewxBoolClientData*)choice->GetClientObject(n))->GetData();
}


TRunType CalcEd::getRuntype() const
{
  return TRunType(getRuntypeName(), getRuntypeNoSpherical());
}


int CalcEd::getOpenShells() const
{
  int ret(0);

  StringConverter::toInt(
          ((ewxStaticText*)FindWindow(ID_STATICTEXT_CALCED_OPEN_SHELLS))
                  ->GetLabel().c_str(), ret);

  return ret;
}


bool CalcEd::getUseExpCoeff() const
{
  return ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CALCED_USE_EXPONENTS))
          ->IsChecked();
}


bool CalcEd::getUseIrreducible() const
{
  return ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CALCED_IRREDUCIBLE))
          ->IsChecked();
}


SpinMult::SpinMultEnum CalcEd::getSpinMult() const
{
  return p_spinMult;
}


string CalcEd::getCodeName() const
{
  return p_codeName;
}


/**
 * Persists the given theory choice.
 * If the given theory is not one of the available choices, we try to 
 * intelligently find one that is available.
 * 1) Try the user preference.
 * 2) Try to find the first theory category that matches.
 * 3) If choice list is empty, notify user.
 * 4) If all else fails, just pick the first one in the list.
 *
 * @param[in] value the theory to select
 * @param[in] overrideFlag indicates whether to reset unrestricted theories
 * @return true if the specified theory has been selected,
 *         false if it wasn't found
 */
bool CalcEd::setTheory(const TTheory* value, const bool& overrideFlag)
{
  bool ret = false;

  ewxChoice *choice = (ewxChoice*) FindWindow(ID_CHOICE_CALCED_THEORY);
  wxString lastTheoryName = p_lastTheory.name();
  wxString lastTheoryCategory = p_lastTheory.category();

  bool uTheoryFlag = false;
  if (value && overrideFlag) {
    uTheoryFlag = value->name().find('U') == 0;
  }

  if (!uTheoryFlag && value && choice->SetStringSelection(value->name())) {
    // theory was found based on name and correctly set
    // do nothing (this is Good)
    ret = true;
  }
  else if (!uTheoryFlag && value &&
           choice->SetStringSelection(value->category())) {
    // this is needed for the Amica code, which is a special case because
    // there are only categories and no underlying theory names

    // theory choice was found based on category and correctly set
    // do nothing (this is Good)
    ret = true;
  }
  else {
    // the previous theory is no longer valid
    // now try to auto-select

    wxString theory = wxEmptyString;
    bool rxnFlag = false;
    Resource *parent = EDSIFactory::getResource(p_iCalc->getURL().getParent());
    if (uTheoryFlag || lastTheoryName!="") {
      // no-op: leave theory set to empty string so it falls through to
      // selecting a theory from the current category
    } else if (parent->getApplicationType() ==
                       ResourceDescriptor::AT_REACTION_STUDY) {
      // special STTR logic to set the theory to RDFT (or UDFT if that fails)
      rxnFlag = true;
      theory = "RDFT";
      lastTheoryCategory = "DFT";
    } else {
      // otherwise, try the user preference
      ewxConfig *config = ewxConfig::getConfig("wxcalced.ini");
      config->Read("Theory", &theory, wxEmptyString);
      config->Read("Category", &lastTheoryCategory, wxEmptyString);
      ewxConfig::closeConfigs();
    }

    if (theory!=wxEmptyString && choice->SetStringSelection(theory)) {
      // value choice set from prefs or STTR logic
      if (!lastTheoryName.IsEmpty() && !lastTheoryName.IsSameAs("None") &&
          !rxnFlag) {
        p_feedback->setMessage(("Overriding theory " + lastTheoryName
                + " to user preferred theory " + choice->GetStringSelection()
                + ".").c_str(), WxFeedback::WARNING);
      }
    } else {
      // could not set theory from preferences, now try to find same category
      size_t n;
      wxString category;
      for (n = 0; n < choice->GetCount(); n++) {
        category = ((wxStringClientData*)choice->GetClientObject(n))->GetData();
        if (category.IsSameAs(lastTheoryCategory)) {
          break;
        }
      }
      if (n < choice->GetCount()) {
        // we found a theory in the same category
        choice->SetSelection(n);
        if (!lastTheoryName.IsEmpty() && !lastTheoryName.IsSameAs("None")) {
          // check if it's really the same theory as before
          if (lastTheoryName == choice->GetStringSelection()) {
            ret = true;
          } else {
            p_feedback->setMessage(("Overriding theory " + lastTheoryName
                    + " to " + choice->GetStringSelection() + ".").c_str(),
                    WxFeedback::WARNING);
          }
        }
      }
      else {
        // could not find theory in same category, either because we removed
        // all theories from the list or we just didn't find it
        if (choice->IsEmpty()) {
          // we removed all theories
          choice->SetSelection(wxNOT_FOUND);
          p_feedback->setMessage("The current spin/theory combination is "
                  "not supported.", WxFeedback::ERROR);
        } else {
          // we give up, just pick the first one in the list...
          choice->SetSelection(0);
          if (!lastTheoryName.IsEmpty() && !lastTheoryName.IsSameAs("None")) {
            p_feedback->setMessage(("Overriding theory " + lastTheoryName
                    + " to " + choice->GetStringSelection() + ".").c_str(),
                    WxFeedback::WARNING);
          }
        }
      }
    }
  }

  // now that we found a theory and set it, remember it for later
  p_lastTheory = getTheory();

  return ret;
}


bool CalcEd::setRuntype(const TRunType& value)
{
  bool ret = false;

  ewxChoice *choice = (ewxChoice*) FindWindow(ID_CHOICE_CALCED_RUNTYPE);
  wxString lastRuntypeName = p_lastRuntype.name();

  if (choice->SetStringSelection(value.name())) {
    // value choice was correctly set, do nothing
    ret = true;
  }
  else {
    // the previous runtype is no longer valid
    // now try to auto-select
 
    wxString runtype = wxEmptyString;
    bool rxnFlag = false;
    Resource *parent = EDSIFactory::getResource(p_iCalc->getURL().getParent());
    if (parent->getApplicationType()==ResourceDescriptor::AT_REACTION_STUDY) {
      // special STTR logic to set the runtype to GeoVib or Geometry for
      // codes not supporting GeoVib (GAMESS-UK)
      rxnFlag = true;
      runtype = "GeoVib";
      lastRuntypeName = "Geometry";
    } else {
      // otherwise, try the user preference first
      ewxConfig *config = ewxConfig::getConfig("wxcalced.ini");
      config->Read("Runtype", &runtype, wxEmptyString);
      ewxConfig::closeConfigs();
    }

    if (runtype!=wxEmptyString && choice->SetStringSelection(runtype)) {
      // value choice set from prefs or STTR logic
      if (!lastRuntypeName.IsEmpty() && !rxnFlag) {
        p_feedback->setMessage(("Overriding runtype " + lastRuntypeName
                + " to user preferred runtype " + choice->GetStringSelection()
                + ".").c_str(), WxFeedback::WARNING);
      }
    } else if (lastRuntypeName!=wxEmptyString &&
               choice->SetStringSelection(lastRuntypeName)) {
      // value choice set from STTR logic
      if (p_lastRuntype.name()!="" && !rxnFlag) {
        p_feedback->setMessage(("Overriding runtype " + p_lastRuntype.name()
                + " to user preferred runtype " + choice->GetStringSelection()
                + ".").c_str(), WxFeedback::WARNING);
      }
    } else {
      // could not set value from user preferences
      // is the list empty or could we just not find it?
      if (choice->IsEmpty()) {
        // list is empty
        choice->SetSelection(wxNOT_FOUND);
        p_feedback->setMessage("No runtypes are supported for the given "
                "code/theory combination.", WxFeedback::ERROR);
      } else {
        // we give up, just pick the first in the list...
        choice->SetSelection(0);
        if (!lastRuntypeName.IsEmpty()) {
          p_feedback->setMessage(("Overriding runtype " + lastRuntypeName
                  + " to " + choice->GetStringSelection() + ".").c_str(),
                  WxFeedback::WARNING);
        }
      }
    }
  }

  // now that we found a runtype and set it, remember it for later
  p_lastRuntype = getRuntype();

  return ret;
}


void CalcEd::setOpenShells(const int& value)
{
  string valueString = StringConverter::toString(value);
  ewxStaticText *text =
          (ewxStaticText*)FindWindow(ID_STATICTEXT_CALCED_OPEN_SHELLS);
  text->SetLabel(valueString);
  text->Show(value >= 0);
}


void CalcEd::setUseExpCoeff(const bool& value)
{
  ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CALCED_USE_EXPONENTS))
          ->SetValue(value);
}


void CalcEd::setUseIrreducible(const bool& value)
{
  ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CALCED_IRREDUCIBLE))
          ->SetValue(value);
}


/**
 * Persists the given spin choice as part of the GUI.
 * If the given spin is not one of the available choices, we try to 
 * intelligently find one that is available by:
 * 1) Just pick the first one in the list (singlet or doublet).
 *
 * @return true if the specified spin has been selected,
 *         false if it wasn't found
 */
bool CalcEd::setSpinMult(const SpinMult::SpinMultEnum& value,
                         const bool& reportFlag)
{
  bool ret = false;
  int spinval = (int)value;
  if (p_fullFrag) {
    if (spinval > 0) {
      if (spinval-1 <= p_fullFrag->numElectrons()) {
        if (p_fullFrag->numElectrons() % 2 == (spinval - 1) % 2) {
          ret = true;
        } else if (reportFlag) {
          if (p_fullFrag->numElectrons() % 2 == 0) { 
            p_feedback->setMessage("The selected spin multiplicity is not "
                  "allowed for an even number of electrons.", WxFeedback::ERROR);
          } else {
            p_feedback->setMessage("The selected spin multiplicity is not "
                  "allowed for an odd number of electrons.", WxFeedback::ERROR);
          }
        }
      } else if (reportFlag) {
        p_feedback->setMessage("The selected spin multiplicity exceeds the "
                    "total number of electrons.", WxFeedback::ERROR);
      }
    } else if (reportFlag) {
      p_feedback->setMessage("Invalid spin multiplicity entered.",
                             WxFeedback::ERROR);
    }
  }

  ewxComboBox *combo = (ewxComboBox*) FindWindow(ID_COMBOBOX_CALCED_SPIN_MULT);

  if (ret) {
    combo->SetValue(SpinMult::toString(value));
    p_spinMult = value;
  }
  else {
    // auto-select spin based on first available item
    combo->SetSelection(0);
    p_spinMult = SpinMult::toSpinMult(combo->GetString(0).c_str());
  }

  // footprints 1530, needs to be set regardless
  setOpenShells(getSpinMult() - 1);

  return ret;
}


/**
 * Set theory choices based on application code.
 * This function clears the current theory selection.
 * Will not perform without an application code set.
 */
void CalcEd::populateTheories()
{
  ewxChoice *choice = (ewxChoice*) FindWindow(ID_CHOICE_CALCED_THEORY);
  choice->SetSelection(wxNOT_FOUND);
  choice->Clear();
  if (p_code) {
    vector<TTheory> *theories = p_code->theories();
    vector<TTheory>::iterator it;
    for (it = theories->begin(); it != theories->end(); it++) {
      if (it->name() != "None") {
        choice->Append(it->name(), new wxStringClientData(it->category()));
      } else {
        choice->Append(it->category(), new wxStringClientData(it->category()));
      }
    }
    delete theories;
  }
}


/**
 * Set runtype choices based on the currently selected theory.
 * This function clears the current runtype selection.
 * Will not perform without a valid code and theory set.
 */
void CalcEd::populateRuntypes()
{
  ewxChoice *choice = (ewxChoice*) FindWindow(ID_CHOICE_CALCED_RUNTYPE);
  choice->SetSelection(wxNOT_FOUND);
  choice->Clear();
  if (p_code) {
    // as per Robert Shroll's and Bruce's comments
    // Geometry, Vibration, and GeoVib should be disabled if the 
    // chemical system contains a single atom
    TTheory theory = getTheory();
    vector<TRunType> *runTypes = p_code->runTypes(&theory);
    vector<TRunType>::iterator it;
    for (it = runTypes->begin(); it != runTypes->end(); it++) {
      if (p_frag && p_frag->numAtoms() > 1)
        choice->Append(it->name(), new ewxBoolClientData(it->noSpherical()));
      else if (it->name() != "Geometry"
            && it->name() != "Vibration"
            && it->name() != "GeoVib")
        choice->Append(it->name(), new ewxBoolClientData(it->noSpherical()));
    }
    delete runTypes;
  }
}


/**
 * Set spin choices based on number of electrons in full fragment.
 * This function clears the current spin selection.
 * Will not perform without a valid full fragment set.
 */
void CalcEd::populateSpinMultiplicities()
{
  ewxComboBox *combo = (ewxComboBox*) FindWindow(ID_COMBOBOX_CALCED_SPIN_MULT);
  combo->SetSelection(wxNOT_FOUND);
  combo->Clear();
  if (p_fullFrag) {
    size_t spin;
    if (p_fullFrag->numElectrons() % 2 == 0) {
      spin = SpinMult::singlet;
    } else {
      spin = SpinMult::doublet;
    }
    for (; spin <= SpinMult::nontet; spin += 2) {
      combo->Append(SpinMult::toString((SpinMult::SpinMultEnum)spin));
    }
  }
}


void CalcEd::populateSummaryFields()
{
  populateSummaryField("TheorySummary");
  populateSummaryField("RuntypeSummary");
}


void CalcEd::populateSummaryField(const string& summaryType)
{
  string topLabel("");
  bool toggleSet(false), noDefault(false);
  vector<string> keys, labels;

  GUIValue *guival(NULL);
  string label(""), value(""), unitstr("");
  //bool labFlag(false), togTrue(false);

  wxFlexGridSizer *sizer;
  if (summaryType == "TheorySummary") {
    sizer = p_theoryDetailsSizer;
  } else if (summaryType == "RuntypeSummary") {
    sizer = p_runtypeDetailsSizer;
  } else {
    return; // bad, so bail
  }
  sizer->Clear(true);
  sizer->Layout();
  sizer->SetSizeHints(this);

  if (p_code && p_GUIValues) {
    SummaryIterator *it = p_code->getSummaryIterator(summaryType);
    while (it->next(topLabel, toggleSet, noDefault, keys, labels)) {
      bool isNested = !topLabel.empty();
      int count = 0;
      // Go through all key/values and make sure at least one gets
      // displayed before outputting the top label
      for (size_t i = 0; i < keys.size(); i++) {
        if ((guival = p_GUIValues->get(keys[i]))
                && guival->m_sensitive
                && (!noDefault || (noDefault && guival->m_write))) {
          if (toggleSet) {
            if (guival->m_type == "toggle_input" && guival->getValueAsInt()) {
              count++;
            }
          } else {
            count++;
          }
        }
      }
      if (isNested && count > 0) {
        sizer->Add(new ewxNonBoldLabel(this, wxID_ANY,
                topLabel), 0, 0, 0);
        sizer->AddSpacer(1);
      }
      for (size_t i = 0; i < keys.size(); i++) {
        if ((guival = p_GUIValues->get(keys[i]))
                && guival->m_sensitive
                && (!noDefault || (noDefault && guival->m_write))) {
          if (guival->m_type == "toggle_input") {
            value = guival->getValueAsInt() ? "true" : "false";
          } else {
            value = guival->getValueAsString();
          }
          if (toggleSet) {
            if (value == "true") {
              sizer->Add(new ewxStaticText(this, wxID_ANY, labels[i]),
                         0, isNested ? wxALIGN_RIGHT : 0, 0);
              sizer->AddSpacer(0);
            }
          } else {
            sizer->Add(new ewxNonBoldLabel(this, wxID_ANY, labels[i]),
                       0, isNested ? wxALIGN_RIGHT : 0, 0);
            wxSizer *valSizer = new wxBoxSizer(wxHORIZONTAL);
            valSizer->Add(new ewxStaticText(this, wxID_ANY, value),
                          0, wxLEFT, 2);
            valSizer->Add(new ewxNonBoldLabel(this, wxID_ANY, guival->m_units),
                          0, wxLEFT, 2);
            sizer->Add(valSizer, 0, 0, 0);
          }
        }
      }
      
      keys.clear();
      labels.clear();
    }
    delete it;
  }

  p_theoryDetailsSizer->Layout();
  p_runtypeDetailsSizer->Layout();
  p_detailsBox->GetContainingSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}


/**
 * Clears both theory and runtype details changes.
 */
void CalcEd::resetTheoryDetails()
{
  if (p_GUIValues) {
    delete p_GUIValues;
  }
  p_GUIValues = new GUIValues();

  if (!p_startUp) {
    startTheoryApp(true);
    startRuntypeApp(true);
  }
}


/**
 * Clear runtype details changes.
 */
void CalcEd::resetRuntypeDetails()
{
  if (p_GUIValues) {
    p_GUIValues->deletePrefix("ES.Runtype");
  }

  if (!p_startUp) {
    p_feedback->setMessage("Changing the runtype overrides all runtype detail "
            "field changes.", WxFeedback::WARNING);
    startRuntypeApp(true);
  }
}


/**
 * Close details dialogs and give a warning about possible theory/runtype
 * inconsistencies.
 */
void CalcEd::resetMinorDetails()
{
  p_feedback->setMessage("Check detail fields since changing the chemical "
          "system, spin multiplicity, or theory can make the current values "
          "invalid.", WxFeedback::WARNING);

  closeTheoryApp(true);
  closeRuntypeApp(true);
}


/**
 * Repopulate spin choices and reselect current spin.
 * Spin is populated based on the number of electrons in the current fragment.
 * If our previously selected spin is not found in the new list of choices,
 * we pick a new spin intelligently and inform the user.
 *
 * @param[in] force ignored but passed through to restrictTheoriesBySpin
 */
void CalcEd::restrictSpinByCharge(const bool& force)
{
  SpinMult::SpinMultEnum spinMult = getSpinMult();
  populateSpinMultiplicities();
  if (!setSpinMult(spinMult, false)) {
    // spin overridden due to incompatibility
    if (spinMult != SpinMult::unknown) {
      p_feedback->setMessage(("Overriding spin "
              + SpinMult::toString(spinMult) + " to "
              + SpinMult::toString(getSpinMult()) + ".").c_str(),
              WxFeedback::WARNING);
    }
    restrictTheoriesBySpin(force);
  }
}


/**
 * Repopulate theory choices and remove those that are inappropriate for the
 * current spin/open shell selection.
 */
void CalcEd::restrictTheoriesBySpin(const bool& force)
{
  /* The following code really only applies to Amica calculations.
   * Truth is, it doesn't hurt anything to execute ::restrictTheoriesBySpin
   * even for Amica calcs, since it's just a long no-op.  Also, it turns out
   * the ::populateTheories() called below is needed during startup anyway.
   * The code is included here because similar logic was in the old calced.
  // This only applies to theories with names implying restricted/unrestricted.
  // Skip this method if there are only theory categories.
  if (p_code) {
    vector<TTheory> *theories = p_code->theories();
    vector<TTheory>::iterator it;
    for (it = theories->begin(); it != theories->end(); it++) {
      if (it->name() != "None") {
        break;
      }
    }
    bool hasOnlyTheoryCategories = (it == theories->end());
    delete theories;
    if (hasOnlyTheoryCategories) return;
  }
  */

  ewxChoice *choice = (ewxChoice*)FindWindow(ID_CHOICE_CALCED_THEORY);

  // we need to restore all theories for the code before removing them
  // otherwise we'd only get shorter and shorter lists of theories
  populateTheories();

  SpinMult::SpinMultEnum spinMult = getSpinMult();
  int openShells = getOpenShells();

  if (spinMult == SpinMult::singlet && openShells == 0) {
    // remove restricted open shell (RO) theories
    // remove UMP5* for closed shell systems
    for (size_t i = 0; i < choice->GetCount(); /*i++*/) {
      string name = choice->GetString(i).c_str();
      if (name.find("RO") == 0 || name.find("UMP5") == 0) {
        choice->Delete(i);
      } else {
        i++;
      }
    }
  } else if ((int)spinMult-1 == openShells) {
    // remove restricted spin theories
    for (size_t i = 0; i < choice->GetCount(); /*i++*/) {
      string name = choice->GetString(i).c_str();
      if (name.find('R') == 0 && name.find('O') != 1) {
        choice->Delete(i);
      } else {
        i++;
      }
    }
  } else { // below the diagonal
    // remove restricted, unrestricted, restricted open shell spin theories
    for (size_t i = 0; i < choice->GetCount(); /*i++*/) {
      string name = choice->GetString(i).c_str();
      // RO handled by first (R)
      if (name.find('R') == 0 || name.find('U') == 0) {
        choice->Delete(i);
      } else {
        i++;
      }
    }
  }

  // try to restore last theory
  wxString lastTheoryName = p_lastTheory.name();
  if ((!p_startUp && !setTheory(&p_lastTheory, true)) || force) {
    if (!lastTheoryName.IsEmpty() && !lastTheoryName.IsSameAs("None")) {
      p_feedback->setMessage("Changing the theory level overrides all detail "
              "field changes.", WxFeedback::WARNING);
    }
    doTheoryChange();
  }
}


/**
 * Check that the calculation has an associated and valid application code.
 * It's an error if there isn't one since the code is established at
 * the time of calculation creation.
 * Also check that we have appropriate parsers for the given application code.
 */
bool CalcEd::isValidCode(const JCode* code)
{
  bool ret = false;

  if (!code) {
    p_feedback->setMessage("Calculation does not have an application code "
            "associated.", WxFeedback::ERROR);
    ret = false;
  } else {
    string inputGeneratorString, templateString;
    code->get_string("InputGenerator", inputGeneratorString);
    code->get_string("Template", templateString);
    ret = (!inputGeneratorString.empty() && !templateString.empty());
    if (!ret) {
      p_feedback->setMessage("Code not supported by Electronic Structure "
              "Editor.  This could be due to an error in the codecap files.",
              WxFeedback::ERROR);
    }
  }

  return ret;
}


bool CalcEd::isDetailsReady() const
{
  return p_frag && !p_frag->containsNubs();
}

bool CalcEd::theoryNeedsBasis() const
{
  bool ret = true;

  if (p_code) {
    ret = p_code->theoryNeedsBasis(getTheory());
  }

  return ret;
}

bool CalcEd::isReady() const
{
  bool ret = false;

  if (p_iCalc->getState() < ResourceDescriptor::STATE_SUBMITTED
          && isDetailsReady()) {
    if (!theoryNeedsBasis()) {
      ret = true;
    } else {
      TTheory theory = getTheory();
      ret = p_basis && GBSRules::isConsistent(p_frag, p_basis, p_code, &theory)
                    && GBSRules::isComplete(p_frag, p_basis, p_code, &theory);
    }
  }

  return ret;
}


bool CalcEd::isGbsValid()
{
  bool ret = true;

  ErrMsg errors;
  errors.flush();

  TTheory theory = getTheory();

  if (p_frag && p_basis && p_code && !theory.name().empty()) {
    if (!ICalcUtils::makeConsistent(p_frag, p_basis, p_code, &theory)) {
      if (errors.count() > 0) {
        p_feedback->setMessage(errors.messageText(errors.last()),
                               WxFeedback::WARNING);
      }
    }
    errors.flush();

    if (!GBSRules::isComplete(p_frag, p_basis, p_code, &theory)) {
      p_feedback->setMessage("The selected configuration doesn't cover all "
              "elements in the chemical system.  Use the Basis Set Tool to "
              "complete the coverage.", WxFeedback::ERROR);
      ret = false;
    } else {
      if (errors.count() > 0) {
        p_feedback->setMessage(errors.messageText(errors.last()),
                               WxFeedback::WARNING);
      }
      errors.flush();
    }
    errors.flush();
  }

  return ret;
}


void CalcEd::updateAllFields()
{
  refreshAllFields();
  enableAllFields();
  showAllFields();
  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}


void CalcEd::updateChemSysFields()
{
  refreshChemSysFields();
  enableChemSysFields();
  showChemSysFields();
  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}


void CalcEd::updateBasisSetFields()
{
  refreshBasisSetFields();
  enableBasisSetFields();
  showBasisSetFields();
  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}


void CalcEd::updateDetailsFields()
{
  refreshDetailsFields();
  enableDetailsFields();
  showDetailsFields();
  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}


/**
 * Refreshes all values displayed on the GUI.
 */
void CalcEd::refreshAllFields()
{
  refreshChemSysFields();
  refreshBasisSetFields();
  refreshDetailsFields();
}


/**
 * Set text values associated with chemical system.
 */
void CalcEd::refreshChemSysFields()
{
  refreshChemSysThumb();

  if (p_frag && p_fullFrag) {
    setUseIrreducible(p_frag->useSymmetry());
    ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CALCED_NAME))
            ->SetValue(p_fullFrag->name());
    ((ewxComboBox*)FindWindow(ID_COMBOBOX_CALCED_CHARGE))
            ->SetValue(StringConverter::toString(p_fullFrag->charge()));
    ((ewxStaticText*)FindWindow(ID_STATICTEXT_CALCED_FORMULA))
            ->SetLabel(p_fullFrag->generateEmpiricalFormula());
    ((ewxStaticText*)FindWindow(ID_STATICTEXT_CALCED_SYMMETRY))
            ->SetLabel(p_fullFrag->pointGroup());
    ((ewxStaticText*)FindWindow(ID_STATICTEXT_CALCED_ATOMS))
            ->SetLabel(StringConverter::toString(p_fullFrag->numAtoms()));
    ((ewxStaticText*)FindWindow(ID_STATICTEXT_CALCED_ELECTRONS))
            ->SetLabel(StringConverter::toString(p_fullFrag->numElectrons()));
  } else {
    // clear local basis and GUIValues because they would be inconsistent
    if (p_basis) {
      delete p_basis;
      p_basis = 0;
    }

    if (p_GUIValues) {
      delete p_GUIValues;
      p_GUIValues = new GUIValues();
      updateDetailsFields();
    }

    ((ewxComboBox*)FindWindow(ID_COMBOBOX_CALCED_CHARGE))->SetValue("0");
    ((ewxComboBox*)FindWindow(ID_COMBOBOX_CALCED_SPIN_MULT))->SetValue("Singlet");
    ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CALCED_NAME))->SetValue("");
    ((ewxStaticText*)FindWindow(ID_STATICTEXT_CALCED_FORMULA))->SetLabel("");
    ((ewxStaticText*)FindWindow(ID_STATICTEXT_CALCED_SYMMETRY))->SetLabel("");
    ((ewxStaticText*)FindWindow(ID_STATICTEXT_CALCED_ATOMS))->SetLabel("");
    ((ewxStaticText*)FindWindow(ID_STATICTEXT_CALCED_ELECTRONS))->SetLabel("");
  }
}


/**
 * Set the bitmap for the chemical system or just the vanilla bitmap.
 */
void CalcEd::refreshChemSysThumb()
{
  bool restoreStandardBitmap = false;

  if (p_iCalc) {
    SFile *thumbnail = TempStorage::getTempFile();
    if (p_iCalc->getThumbnail(thumbnail)) {
      wxBitmap bitmap(_T(thumbnail->path()), wxBITMAP_TYPE_JPEG);
      if (bitmap.Ok()) 
        p_builderTool->setBitMap(bitmap);
      else
        restoreStandardBitmap = true;
    } else {
      restoreStandardBitmap = true;
    }
    thumbnail->remove();
    delete thumbnail;
  } else {
    restoreStandardBitmap = true;
  }

  if (restoreStandardBitmap) {
    ResourceTool *tool = ResourceDescriptor::getResourceDescriptor()
            .getTool(p_builderTool->GetId());
    p_builderTool->setBitMap(ewxBitmap(tool->getIcon(), wxBITMAP_TYPE_XPM));
  }
}


void CalcEd::refreshBasisSetFields()
{
  bool hasFullFragment(p_fullFrag);
  bool hasCalc(p_iCalc);

  if (!theoryNeedsBasis() && p_basis) {
    delete p_basis;
    p_basis = 0;
  }

  if (hasCalc) {
    setUseExpCoeff(p_iCalc->gbsUseExpCoeff());
  }

  if (p_basis && isGbsValid()) {
    setUseExpCoeff(p_basis->hasGeneralContractions() && !p_basis->optimize());

    ((ewxStaticText*)FindWindow(ID_STATICTEXT_CALCED_BASIS_NAME))
            ->SetLabel(p_basis->name());
    ((ewxStaticText*)FindWindow(ID_STATICTEXT_CALCED_POLARIZATION))
            ->SetLabel(p_basis->coordsys() == TGaussianBasisSet::Cartesian ?
                       "Cartesian" : "Spherical");

    if (hasFullFragment) {
      TagCountMap *tcmap = p_fullFrag->tagCountsSTL();
      ((ewxStaticText*)FindWindow(ID_STATICTEXT_CALCED_FUNCTIONS))
              ->SetLabel(StringConverter::toString(
                         p_basis->num_functions(*tcmap)));
      ((ewxStaticText*)FindWindow(ID_STATICTEXT_CALCED_PRIMITIVES))
              ->SetLabel(StringConverter::toString(
                         p_basis->num_primitives(*tcmap)));
      delete tcmap;
    }

    ((ewxStaticText*)FindWindow(ID_STATICTEXT_CALCED_ECP))
            ->SetLabel(p_basis->ecpName().empty() ? "None" :
                       p_basis->ecpName());
    ((ewxStaticText*)FindWindow(ID_STATICTEXT_CALCED_XC))
            ->SetLabel(p_basis->dftFittingName().empty() ? "None" :
                       p_basis->dftFittingName());
    ((ewxStaticText*)FindWindow(ID_STATICTEXT_CALCED_CD))
            ->SetLabel(p_basis->dftChargeFittingName().empty() ? "None" :
                       p_basis->dftChargeFittingName());
  } else {
    ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CALCED_USE_EXPONENTS))
            ->SetValue(false);
    ((ewxStaticText*)FindWindow(ID_STATICTEXT_CALCED_BASIS_NAME))
            ->SetLabel("");
    ((ewxStaticText*)FindWindow(ID_STATICTEXT_CALCED_POLARIZATION))
            ->SetLabel("");
    ((ewxStaticText*)FindWindow(ID_STATICTEXT_CALCED_FUNCTIONS))->SetLabel("");
    ((ewxStaticText*)FindWindow(ID_STATICTEXT_CALCED_PRIMITIVES))
            ->SetLabel("");
    ((ewxStaticText*)FindWindow(ID_STATICTEXT_CALCED_ECP))->SetLabel("");
    ((ewxStaticText*)FindWindow(ID_STATICTEXT_CALCED_XC))->SetLabel("");
    ((ewxStaticText*)FindWindow(ID_STATICTEXT_CALCED_CD))->SetLabel("");
  }
}


void CalcEd::refreshDetailsFields()
{
  populateSummaryFields();
}


void CalcEd::enableAllFields()
{
  enableChemSysFields();
  enableBasisSetFields();
  enableDetailsFields();
  enableLaunch();
  enableGeomConstraints();
}


/**
 * Enable/Disable fields based on whether a chemical system has been specified.
 */
void CalcEd::enableChemSysFields()
{
  bool hasFragment(p_frag);
  bool hasCalc(p_iCalc);
  bool isSubmitted(hasCalc
          && p_iCalc->getState() >= ResourceDescriptor::STATE_SUBMITTED);

  bool isReaction(false);
  if (hasCalc) {
    Resource *parent = EDSIFactory::getResource(p_iCalc->getURL().getParent());
    isReaction = parent->getApplicationType() ==
                         ResourceDescriptor::AT_REACTION_STUDY;
  }

  p_builderTool->Enable(hasCalc);
  FindWindow(ID_TEXTCTRL_CALCED_NAME)->Enable(!isSubmitted && hasFragment);
  FindWindow(ID_STATIC_CALCED_CHARGE)->Enable(!isSubmitted && hasFragment &&
                                              !isReaction);
  FindWindow(ID_COMBOBOX_CALCED_CHARGE)->Enable(!isSubmitted && hasFragment &&
                                                !isReaction);
  FindWindow(ID_STATIC_CALCED_SPIN_MULT)->Enable(!isSubmitted && hasFragment &&
                                                 !isReaction);
  FindWindow(ID_COMBOBOX_CALCED_SPIN_MULT)->Enable(!isSubmitted && hasFragment&&
                                                   !isReaction);
  FindWindow(ID_CHOICE_CALCED_THEORY)->Enable(!isSubmitted && hasFragment);
  FindWindow(ID_CHOICE_CALCED_RUNTYPE)->Enable(!isSubmitted && hasFragment);
  FindWindow(ID_CHECKBOX_CALCED_IRREDUCIBLE)->Enable(!isSubmitted && hasFragment);
}


void CalcEd::enableBasisSetFields()
{
  bool hasFragment(p_frag);
  bool needsBasis = theoryNeedsBasis();
  bool hasCalc(p_iCalc);
  bool isSubmitted(hasCalc
          && p_iCalc->getState() >= ResourceDescriptor::STATE_SUBMITTED);

  p_basisSetTool->Enable(hasFragment && needsBasis);
  FindWindow(ID_BUTTON_CALCED_BASIS_QUICK)->Enable(!isSubmitted
          && hasFragment && needsBasis);
  FindWindow(ID_CHECKBOX_CALCED_USE_EXPONENTS)->Enable(!isSubmitted
          && hasFragment && needsBasis && p_basis
          && !(p_basis->hasGeneralContractions() && !p_basis->optimize()));
}


void CalcEd::enableDetailsFields()
{
  bool hasFragment(p_frag);

  p_detailsBox->Enable(hasFragment);
  FindWindow(ID_BUTTON_CALCED_THEORY)->Enable(isDetailsReady()
          && p_GUIValues->containsKeyPrefix("ES.Theory"));
  FindWindow(ID_BUTTON_CALCED_RUNTYPE)->Enable(isDetailsReady()
          && p_GUIValues->containsKeyPrefix("ES.Runtype"));
}


void CalcEd::enableLaunch()
{
  if (p_iCalc) {
    bool ready = isReady();

    if (ready) {
      p_feedback->setRunState(ResourceDescriptor::STATE_READY);
    } else {
      if (p_iCalc->getState() == ResourceDescriptor::STATE_READY) {
        p_feedback->setRunState(ResourceDescriptor::STATE_CREATED);
      } else {
        p_feedback->setRunState(p_iCalc->getState());
      }
    }
  
    bool togo = p_feedback->getRunState() > ResourceDescriptor::STATE_CREATED
            && p_feedback->getRunState() != ResourceDescriptor::STATE_LOADED;
    FindWindow(ID_BUTTON_CALCED_FINAL_EDIT)->Enable(togo);
    FindWindow(ID_BUTTON_CALCED_LAUNCH)->Enable(togo);
  } else {
    FindWindow(ID_BUTTON_CALCED_FINAL_EDIT)->Enable(false);
    FindWindow(ID_BUTTON_CALCED_LAUNCH)->Enable(false);
  }
}


void CalcEd::enableGeomConstraints()
{
  showGeomEditor();
}


void CalcEd::enableSave(const bool& enable)
{
  if (p_iCalc && p_feedback->getEditStatus() != WxFeedback::READONLY) {
    setEditStatus(enable ? WxFeedback::MODIFIED : WxFeedback::EDIT);
    GetMenuBar()->GetMenu(GetMenuBar()->FindMenu("File"))
            ->Enable(wxID_SAVE, enable);
  } else {
    GetMenuBar()->GetMenu(GetMenuBar()->FindMenu("File"))
            ->Enable(wxID_SAVE, false);
  }
}


void CalcEd::showAllFields()
{
  showChemSysFields();
  showBasisSetFields();
  showDetailsFields();
}


void CalcEd::showChemSysFields()
{
  bool hasCode(p_code);
  bool isIrreducibleFragmentSupported(hasCode
          && p_code->getIrreducibleFragmentSupported());
  FindWindow(ID_CHECKBOX_CALCED_IRREDUCIBLE)->Show(
          isIrreducibleFragmentSupported);
}


void CalcEd::showBasisSetFields()
{
  if (p_code) {
    bool libraryNames = true;
    p_code->get_bool("LibraryNames", libraryNames);
    FindWindow(ID_CHECKBOX_CALCED_USE_EXPONENTS)->Show(libraryNames);
    FindWindow(ID_BUTTON_CALCED_BASIS_QUICK)
            ->Show(p_code->getBasisSetQuickListSupported());
  }

  int ids[] = { ID_STATICTEXT_CALCED_ECP, ID_LABEL_CALCED_ECP,
                ID_STATICTEXT_CALCED_XC,  ID_LABEL_CALCED_XC,
                ID_STATICTEXT_CALCED_CD,  ID_LABEL_CALCED_CD };
  for (int i = 0; i < 6; i += 2) {
    ewxStaticText *text = (ewxStaticText*) FindWindow(ids[i]);
    bool show = !(text->GetLabel().IsSameAs("None")
            || text->GetLabel().IsEmpty());
    text->Show(show);
    FindWindow(ids[i+1])->Show(show);
  }
}


/**
 * Determine whether we need to display the details buttons.
 */
void CalcEd::showDetailsFields()
{
  if (p_code) {
    string theoryCommand, runCommand;
    if (!p_code->getTheoryRunTypeEditorNames(theoryCommand, runCommand)) {
      INVALIDEXCEPTION(0,"Theory and Runtype dialog executables not defined.");
    }
    FindWindow(ID_BUTTON_CALCED_THEORY)->Show(!theoryCommand.empty());
    p_theoryDetailsSizer->Show(!theoryCommand.empty());
    FindWindow(ID_BUTTON_CALCED_RUNTYPE)->Show(!runCommand.empty());
    p_runtypeDetailsSizer->Show(!runCommand.empty());
    showGeomEditor();
    showPartialEditor();
  }
}


void CalcEd::showGeomEditor()
{
  bool hasFragment(p_frag);
  bool hasCalc(p_iCalc);
  bool isSubmitted(hasCalc
          && p_iCalc->getState() >= ResourceDescriptor::STATE_SUBMITTED);
  bool hasRuntypeGeom(getRuntypeName().IsSameAs("Geometry") ||
                      getRuntypeName().IsSameAs("GeoVib"));
  bool supportsConstraints = false;
  if (p_code) {
    p_code->get_bool("SupportsConstraints", supportsConstraints);
  }

  FindWindow(ID_BUTTON_CALCED_CONSTRAINT)->Show(!isSubmitted
          && hasFragment && hasRuntypeGeom && supportsConstraints);
  if (p_geomConstraints != 0) { 
    p_geomConstraints->initDisplay(!isSubmitted && 
                                   hasFragment && 
                                   hasRuntypeGeom);
  } 
  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}


void CalcEd::showPartialEditor()
{
  bool hasFragment(p_frag);
  bool hasCalc(p_iCalc);
  bool isSubmitted(hasCalc
          && p_iCalc->getState() >= ResourceDescriptor::STATE_SUBMITTED);
  bool hasRuntypeESP(getRuntypeName().IsSameAs("ESP"));
  bool supportsConstraints = false;
  if (p_code) {
    p_code->get_bool("SupportsConstraints", supportsConstraints);
  }

  FindWindow(ID_BUTTON_CALCED_PARTIAL)->Show(!isSubmitted
          && hasFragment && hasRuntypeESP && supportsConstraints);
}


void CalcEd::makeRuntypeNoSphericalConsistent()
{
  if (getRuntypeNoSpherical()
          && p_basis && p_basis->coordsys() == TGaussianBasisSet::Spherical) {
    p_basis->coordsys(TGaussianBasisSet::Cartesian);
  }
}


void CalcEd::urlChangeNotify(const string& topic) const
{
  JMSMessage *msg = newMessage();
  msg->addProperty("url", p_iCalc->getURL().toString());
  publish(topic, *msg);
  delete msg;
}


bool CalcEd::generateInput(const bool& paramFlag)
{
  bool success = false;

  if (isReady()) {
    string message;
    success = input_controller(paramFlag, getUseExpCoeff(), message);
    if (!message.empty()) {
      if (success) {
        //ewxMessageDialog *dialog = new ewxMessageDialog(this, message,
                //"Input File Generation");
      } else {
        p_feedback->setMessage(message, WxFeedback::ERROR);
      }
    }
  }

  return success;
}


wxWindowID CalcEd::doClose(const bool& allowCancel)
{
  wxWindowID answer = wxID_ANY;

  if (p_feedback->getEditStatus() == WxFeedback::MODIFIED) {
    long buttonFlags = wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION;
    buttonFlags |= allowCancel ? wxCANCEL : 0;
    ewxMessageDialog *dialog = new ewxMessageDialog(this,
            "The current calculation has unsaved changes!  Do you "
            "want to save changes before quitting?",
            "Save " + p_codeName + " Editor Changes?", buttonFlags);
    answer = dialog->ShowModal();
    if (answer != wxID_CANCEL) {
      if (answer == wxID_YES) {
        doSave();
      }
      Destroy();
    }
  } else {
    Destroy();
  }

  return answer;
}


void CalcEd::doSave()
{
  // check that there's really been something changed
  if (p_feedback->getEditStatus() == WxFeedback::MODIFIED)
  {
    // save code
    bool saveCode = false;
    const JCode *currentCode = p_iCalc->application();
    if (!currentCode || currentCode->name() != p_code->name()) {
      p_iCalc->application(p_code);
      saveCode = true;
    }
    if (currentCode) currentCode = 0;

    // save frag and spin and open shells
    // name, charge, and use irreducible are part of the frag
    bool saveFrag = false;
    bool saveSpin = false;
    if (p_frag) {
      Fragment *currentFragment = p_iCalc->fragment();
      if (!currentFragment) {
        p_iCalc->fragment(p_frag);
        saveFrag = true;
      } else if (currentFragment->charge() != p_frag->charge() ||
                 currentFragment->name() != p_frag->name() ||
                 currentFragment->useSymmetry() != p_frag->useSymmetry()) {
        // update the old fragment with new values because we may have
        // either an irreducible fragment or full generated molecule in the
        // calced with a different number of coordinates than what we need
        // to be saved
        currentFragment->charge(p_frag->charge());
        currentFragment->name(p_frag->name());
        currentFragment->useSymmetry(p_frag->useSymmetry());
        p_iCalc->fragment(currentFragment);
        saveFrag = true;
      }
      if (currentFragment) delete currentFragment;

      // save spin multiplicity and open shells
      if (p_iCalc->spinMultiplicity() != getSpinMult()
              || p_iCalc->openShells() != getOpenShells()) {
        p_iCalc->spinMultiplicity(getSpinMult());
        p_iCalc->openShells(getOpenShells());
        saveSpin = true;
      }

    }

    // Save geometry and ESP constraint model
    ChemistryTask *task = dynamic_cast<ChemistryTask*>(p_iCalc);
    if (p_frag) {
      task->setGeomConstraintModel(p_frag->getConstraints());
      task->setESPModel(p_ESPCnstrnt);
    }
    else {
      // make sure to remove model if there was one
      task->setGeomConstraintModel(0);
      task->setESPModel(0);
    }

    // save basis
    bool saveBasis = false;
    TGBSConfig *currentBasis = p_iCalc->gbsConfig();
    if ((p_basis && (!currentBasis || !currentBasis->isEqual(p_basis)))
            || !p_basis) {
      p_iCalc->gbsConfig(p_basis);
      saveBasis = true;
    }
    if (currentBasis) delete currentBasis;

    // save "use exp and coeffs" value
    p_iCalc->gbsUseExpCoeff(getUseExpCoeff());

    // save theory
    bool saveTheory = false;
    TTheory *currentTheory = p_iCalc->theory();
    if (!currentTheory
            || getTheoryCategory() != currentTheory->category()
            || getTheoryName() != currentTheory->name()) {
      TTheory theory = getTheory();
      p_iCalc->theory(&theory);
      saveTheory = true;
    }
    if (currentTheory) delete currentTheory;

    // save runtype
    bool saveRuntype = false;
    TRunType currentRuntype = p_iCalc->runtype();
    if (currentRuntype.name() != getRuntypeName()) {
      TRunType runtype = getRuntype();
      p_iCalc->runtype(runtype);
      saveRuntype = true;
    }

    // save GUIValues
    p_iCalc->guiparams(p_GUIValues);

    // determine whether to save intermediate files for debugging
    // save if shift key is held down
    bool save_param = ::wxGetKeyState(WXK_SHIFT);

    // input file
    // must generate a valid input file before changing state so that
    // other apps (launcher) are guaranteed the calculation is launchable
    if (generateInput(save_param)) {
      p_feedback->setMessage("New input file generated.", WxFeedback::INFO);
      if (p_iCalc->getState() != p_feedback->getRunState()) {
        p_iCalc->setState(p_feedback->getRunState());
      }
    }

    // reset the save status indicator
    enableSave(false);

    p_feedback->setMessage("Calculation saved as " + p_iCalc->getName() + ".",
                           WxFeedback::INFO);

    if (saveCode)    urlChangeNotify("ecce_url_code");
    if (saveFrag)    urlChangeNotify("ecce_url_subject");
    if (saveSpin)    urlChangeNotify("ecce_url_spin");
    if (saveTheory)  urlChangeNotify("ecce_url_theory");
    if (saveRuntype) urlChangeNotify("ecce_url_runtype");
    if (saveBasis)   urlChangeNotify("ecce_url_basis");
    
    // Details always change because any calced change
    // triggers them to be reset
    urlChangeNotify("ecce_url_details");
  }
}


/**
 * Support method for changing the theory category/name.
 */
void CalcEd::doTheoryChange()
{
  populateRuntypes();
  setRuntype(p_lastRuntype);

  enableSave();
  makeRuntypeNoSphericalConsistent();
  resetTheoryDetails();
  updateBasisSetFields();
  updateDetailsFields();
  enableLaunch();
}


void CalcEd::doRuntypeChange()
{
  enableSave();
  makeRuntypeNoSphericalConsistent();
  resetRuntypeDetails();
  updateBasisSetFields();
  updateDetailsFields();
  enableLaunch();
}


void CalcEd::saveSettings()
{
  //ewxConfig *config = ewxConfig::getConfig("wxcalced.ini");
  ewxConfig::closeConfigs();
}


void CalcEd::restoreSettings()
{
  //ewxConfig *config = ewxConfig::getConfig("wxcalced.ini");
  ewxConfig::closeConfigs();
}


void CalcEd::startApp(int id, int force, const string& url) const
{
  startApp(ResourceDescriptor::getResourceDescriptor().getTool(id)->getName(),
           force, url);
}


void CalcEd::startApp(const string& app, int force, const string& url) const
{
  JMSMessage *msg = newMessage();
  msg->addProperty("action", "start");
  publish("ecce_activity", *msg);
  delete msg;

  Target gateway(GATEWAY, "");
  msg = newMessage(gateway);
  msg->addProperty("appname", app);
  msg->addIntProperty("forcenew", force);
  msg->addProperty("calcurl", url);
  publish("ecce_get_app", *msg);
  delete msg;
}


string CalcEd::buildTheoryRuntypeArgs(const bool& isTheory) const
{
  string args;

  if (p_iCalc->getState() > ResourceDescriptor::STATE_READY) {
    args = " ReadOnly";
  } else {
    args = " Writable";
  }

#ifdef DEBUG
  args += " DebugOn \"";
#else
  args += " DebugOff \"";
#endif

  // quote the theory and runtype args because they may contain special
  // characterse such as parentheses that the shell will complain about
  args += getTheoryCategory() + "\"";
  args += " \"" + getTheoryName() + "\"";
  args += " \"" + getRuntypeName() + "\"";
  args += " \"" + p_iCalc->getName() + "\"";

  Resource *parent = EDSIFactory::getResource(p_iCalc->getURL().getParent());
  if (parent->getApplicationType() == ResourceDescriptor::AT_REACTION_STUDY) {
    args += " 1";
  } else {
    args += " 0";
  }

  string symmetry = "C1";
  if (p_fullFrag) {
    string symmetryTmp = p_fullFrag->pointGroup();
    StringConverter::capitalize(symmetryTmp, symmetry);
  }
  args += " \"" + symmetry + "\"";

  int electrons, spin, frozenOrbs, occupiedOrbs, virtualOrbs, normalModes;
  getOrbitalParams(electrons, spin, frozenOrbs, occupiedOrbs,
                   virtualOrbs, normalModes);
  char buf[80];
  sprintf(buf, " %d %d %d %d %d %d", electrons, spin, frozenOrbs, occupiedOrbs,
          virtualOrbs, normalModes);
  args += buf;

  return args;
}


void CalcEd::getOrbitalParams(int &electrons, int &spin, int &frozenOrbs, 
                           int &occupiedOrbs, int &virtualOrbs,
                           int &normalModes) const
{
  // Note: Our definition for total number of orbitals is:
  // #TotalOrbs = #Occupied + #Unoccupied + #OpenShells
  // where #Unoccupied is equivalent to #Virtual

  // -1 is a flag to the theory dialogs that we can't determine a value
  // because it is dependent upon the basis set which hasn't been set yet.
  // Otherwise we don't let them invoke the details dialogs until the
  // basis set has been set and Theresa did not like that idea at all.
  electrons = spin = occupiedOrbs = normalModes = 0;
  frozenOrbs = virtualOrbs = -1;

  if (p_fullFrag) {
    electrons = p_fullFrag->nuclearCharge() - p_fullFrag->charge();

    spin = getSpinMult();

    normalModes = 3 * p_frag->numAtoms();

    int coreElectrons = 0;
    for (size_t i=0; i < p_frag->numAtoms(); i++) {
      coreElectrons += getCoreElectrons(p_frag->atomRef(i)->atomicNumber());
    }

    string theory = getTheoryName().c_str();
    int openShells = 0;
    if (theory.find("RO") == 0 || theory.find('U') == 0) {
      openShells = getOpenShells();
    }

    occupiedOrbs = (electrons - openShells)/2;

    if (p_basis) {
      TagCountMap *tcmap = p_fullFrag->tagCountsSTL();
      // subtract off ECP electron contributions
      frozenOrbs = (coreElectrons - p_basis->num_ecpCoreElectrons(*tcmap))/2;
      virtualOrbs = p_basis->num_functions(*tcmap) - occupiedOrbs - openShells;
    }
  }
}


unsigned long CalcEd::getCoreElectrons(const unsigned long atomicNumber) const
{
  static unsigned long electrons[] = {0, 2, 10, 18, 36, 54, 86, 999};

  int ie;
  for (ie=1; atomicNumber > electrons[ie]; ie++);

  return electrons[ie-1];
}


void CalcEd::startTheoryApp(const bool& localInitFlag)
{
  string cmd, otherCmd;
  if (!p_code->getTheoryRunTypeEditorNames(cmd, otherCmd)) {
    EE_RT_ASSERT(0, EE_FATAL,
                "Theory and Runtype dialog executables not defined.");
  }

  if (!cmd.empty()) {
    // Close existing details dialog
    closeTheoryApp(true);

    p_theoryOutFile = TempStorage::getTempFile();
    ofstream outStream(p_theoryOutFile->path().c_str());

    // Create a file just to guarantee a unique name
    SFile *tmpFile = TempStorage::getTempFile();
    p_theoryInFilePath = tmpFile->path();
    tmpFile->remove();
    delete tmpFile;

    // Setup socket based server for wxPython detail dialog IPC
    // Create datagram socket, Let Wx select a port for us - (Service(0))
    wxIPV4address bindAddress;
    bindAddress.Service(0);
    p_theoryInSocket = new wxDatagramSocket(bindAddress);

    // Check state of new socket, if its OK:
    //   Configure socket as non-blocking
    //   Set socket event handler and enable event notification
    //   Save port number for future reference
    if (p_theoryInSocket->Ok()) {
      p_theoryInSocket->SetFlags(wxSOCKET_NOWAIT);
      p_theoryInSocket->SetEventHandler(*this, wxID_THEORY_CHANGE);
      p_theoryInSocket->SetNotify(wxSOCKET_INPUT_FLAG);
      p_theoryInSocket->Notify(true);
      p_theoryInSocket->GetLocal(bindAddress);
      p_theoryInPort = bindAddress.Service();
    }
    else {
      cerr << "Theory dialog IPC socket creation failed." << endl;
    }

    p_theoryInFlag = false;
    p_theoryInitFlag = false;

    cmd += " ";
    cmd += p_theoryOutFile->path() + " ";

    char buf[32];
    sprintf(buf, "%d", p_theoryInPort);
    cmd += buf;

    if (localInitFlag) {
      p_theoryHoldFlag = true;
      cmd += " NO_GUIValues";
    } else {
      p_theoryHoldFlag = false;
      cmd += " GUIValues";
      p_GUIValues->dump(outStream);
      outStream << "END_GUIValues" << endl << flush;
    }

    outStream.close();

    cmd += buildTheoryRuntypeArgs(true);

#ifdef DEBUG
    cout << "Starting wxPython dialog with (" << cmd << ")" << endl;
#endif

    string err;
    if (system((cmd+"&").c_str()) < 0) {
      p_feedback->setMessage("Unable to invoke theory details dialog",
                             WxFeedback::ERROR);
    } else {
      // TODO Add wxTimer derived class to add a timeout capability to
      // starting the details dialog.  See wxgui/wxtools/WxEditSessionMgr.C
      // and search for WxEditTimer for an example of how to do this with
      // a "helper" class.  For now, don't use a timeout and hope that
      // nothing hangs when bringing up the details dialog.
      p_theoryHoldFlag = false;
    }
  }
}

void CalcEd::startRuntypeApp(const bool& localInitFlag)
{
  string cmd, otherCmd;
  if (!p_code->getTheoryRunTypeEditorNames(otherCmd, cmd)) {
    EE_RT_ASSERT(0, EE_FATAL,
                "Theory and Runtype dialog executables not defined.");
  }

  if (!cmd.empty()) {
    // Close existing details dialog
    closeRuntypeApp(true);

    p_runtypeOutFile = TempStorage::getTempFile();
    ofstream outStream(p_runtypeOutFile->path().c_str());

    // Create a file just to guarantee a unique name
    SFile *tmpFile = TempStorage::getTempFile();
    p_runtypeInFilePath = tmpFile->path();
    tmpFile->remove();
    delete tmpFile;

    // Setup socket based server for wxPython detail dialog IPC
    // Create datagram socket, Let Wx select a port for us - (Service(0))
    wxIPV4address bindAddress;
    bindAddress.Service(0);
    p_runtypeInSocket = new wxDatagramSocket(bindAddress);

    // Check state of new socket, if its OK:
    //   Configure socket as non-blocking
    //   Set socket event handler and enable event notification
    //   Save port number for future reference
    if (p_runtypeInSocket->Ok()) {
      p_runtypeInSocket->SetFlags(wxSOCKET_NOWAIT);
      p_runtypeInSocket->SetEventHandler(*this, wxID_RUNTYPE_CHANGE);
      p_runtypeInSocket->SetNotify(wxSOCKET_INPUT_FLAG);
      p_runtypeInSocket->Notify(true);
      p_runtypeInSocket->GetLocal(bindAddress);
      p_runtypeInPort = bindAddress.Service();
    }
    else {
      cerr << "Runtype dialog IPC socket creation failed." << endl;
    }

    p_runtypeInFlag = false;
    p_runtypeInitFlag = false;

    cmd += " ";
    cmd += p_runtypeOutFile->path() + " ";

    char buf[32];
    sprintf(buf, "%d", p_runtypeInPort);
    cmd += buf;

    if (localInitFlag) {
      p_runtypeHoldFlag = true;
      cmd += " NO_GUIValues";
    } else {
      p_runtypeHoldFlag = false;
      cmd += " GUIValues";
      p_GUIValues->dump(outStream);
      outStream << "END_GUIValues" << endl << flush;
    }

    outStream.close();

    cmd += buildTheoryRuntypeArgs(false);

#ifdef DEBUG
    cout << "Starting wxPython dialog with (" << cmd << ")" << endl;
#endif

    string err;
    if (system((cmd+"&").c_str()) < 0) {
      p_feedback->setMessage("Unable to invoke runtype details dialog",
                             WxFeedback::ERROR);
    } else {
      // TODO Add wxTimer derived class to add a timeout capability to
      // starting the details dialog.  See wxgui/wxtools/WxEditSessionMgr.C
      // and search for WxEditTimer for an example of how to do this with
      // a "helper" class.  For now, don't use a timeout and hope that
      // nothing hangs when bringing up the details dialog.
      p_runtypeHoldFlag = false;
    }
  }
}


void CalcEd::OnTheoryIPC(wxSocketEvent& event)
{
#ifdef DEBUG
  cout << "OnTheoryIPC" << endl;
#endif

  static char databuf[8192];

  p_theoryInSocket->Read(databuf, 8191);
  int nbytes = p_theoryInSocket->LastCount();
  if ((!p_theoryInSocket->Error() > 0) && (nbytes > 0)) {
    databuf[nbytes] = '\0';
    processTheoryInput(databuf);
  }
}

void CalcEd::OnRuntypeIPC(wxSocketEvent& event)
{
#ifdef DEBUG
  cout << "OnRuntypeIPC" << endl;
#endif

  static char databuf[8192];

  p_runtypeInSocket->Read(databuf, 8191);
  int nbytes = p_runtypeInSocket->LastCount();
  if ((!p_runtypeInSocket->Error() > 0) && (nbytes > 0)) {
    databuf[nbytes] = '\0';
    processRuntypeInput(databuf);
  }
}

void CalcEd::processTheoryInput(const char* databuf)
{
#ifdef DEBUG
  cout << "processTheoryInput (" << databuf << ")" << endl;
#endif

  if (p_GUIValues->append(databuf)) {
    p_theoryInFlag = true;
    if (!p_theoryHoldFlag) {
      populateSummaryField("TheorySummary");
      if (p_theoryInitFlag && p_feedback->getEditStatus() !=
                                          WxFeedback::READONLY) {
        enableSave();
#ifdef DEBUG
        cout << "**** theory set modified" << endl;
#endif
      }
    }
  } else if (strstr(databuf, "#READONLY") != NULL) {
    p_theoryInFlag = true;
  }

  // These comparisons are not mutually exclusive (else if...) because a single
  // getTheoryInput invocation may contain multiple messages
  char *start;
  if ((start = (char*)strstr(databuf, "#STARTED")) != NULL) {
    // TODO stop wx timeout?
    // Grab process id as the second argument
    start = strchr(start, ' ');
    if (start != NULL) {
      p_theoryPid = (int)strtol(start, NULL, 10);
    } else {
      p_theoryPid = 0;
    }
  }

  if (strstr(databuf, "#INITIALIZED") != NULL) {
    p_theoryOutFile->remove();
    delete p_theoryOutFile;

    p_theoryInitFlag = true;

    FindWindow(ID_BUTTON_CALCED_THEORY)->Enable(isDetailsReady() &&
                                                p_theoryInFlag);

    if (p_theoryHoldFlag) {
      p_theoryHoldFlag = false;
      // This logic assumes theory and runtype dialogs are always
      // initialized as a pair since it waits for both to complete
      if (!p_runtypeHoldFlag) {
        populateSummaryField("TheorySummary");
      }
      if (p_feedback->getEditStatus() != WxFeedback::READONLY) {
        enableSave();
      }
    }
  }

  if (strstr(databuf, "#CLOSING") != NULL) {
    closeTheoryApp(false);
  }
}

void CalcEd::processRuntypeInput(const char* databuf)
{
#ifdef DEBUG
  cout << "processRuntypeInput (" << databuf << ")" << endl;
#endif

  if (p_GUIValues->append(databuf)) {
    p_runtypeInFlag = true;
    if (!p_runtypeHoldFlag) {
      populateSummaryField("RuntypeSummary");
      if (p_runtypeInitFlag && p_feedback->getEditStatus() !=
                                          WxFeedback::READONLY) {
        enableSave();
#ifdef DEBUG
        cout << "**** runtype set modified" << endl;
#endif
      }
    }
  } else if (strstr(databuf, "#READONLY") != NULL) {
    p_runtypeInFlag = true;
  }

  // These comparisons are not mutually exclusive (else if...) because a single
  // getRuntypeInput invocation may contain multiple messages
  char *start;
  if ((start = (char*)strstr(databuf, "#STARTED")) != NULL) {
    // TODO stop wx timeout?
    // Grab process id as the second argument
    start = strchr(start, ' ');
    if (start != NULL) {
      p_runtypePid = (int)strtol(start, NULL, 10);
    } else {
      p_runtypePid = 0;
    }
  }

  if (strstr(databuf, "#INITIALIZED") != NULL) {
    p_runtypeOutFile->remove();
    delete p_runtypeOutFile;

    p_runtypeInitFlag = true;

    FindWindow(ID_BUTTON_CALCED_RUNTYPE)->Enable(isDetailsReady() &&
                                                 p_runtypeInFlag);

    if (p_runtypeHoldFlag) {
      p_runtypeHoldFlag = false;
      // This logic assumes theory and runtype dialogs are always
      // initialized as a pair since it waits for both to complete
      if (!p_theoryHoldFlag) {
        populateSummaryField("RuntypeSummary");
      }
      if (p_feedback->getEditStatus() != WxFeedback::READONLY) {
        enableSave();
      }
    }
  }

  if (strstr(databuf, "#CLOSING") != NULL) {
    closeRuntypeApp(false);
  }
}

void CalcEd::closeTheoryApp(const bool& sendTerm)
{
#ifdef DEBUG
  cout << "closeTheoryApp(" << sendTerm << ")" << endl;
#endif

  if (sendTerm && (p_theoryPid)) {
    (void)kill((p_theoryPid), SIGTERM);
  }

  if (p_theoryInPort != 0) {
    p_theoryInSocket->Notify(false);
    p_theoryInSocket->Destroy();
    p_theoryInPort = 0;
  }
}

void CalcEd::closeRuntypeApp(const bool& sendTerm)
{
#ifdef DEBUG
  cout << "closeRuntypeApp(" << sendTerm << ")" << endl;
#endif

  if (sendTerm && (p_runtypePid)) {
    (void)kill((p_runtypePid), SIGTERM);
  }

  if (p_runtypeInPort != 0) {
    p_runtypeInSocket->Notify(false);
    p_runtypeInSocket->Destroy();
    p_runtypeInPort = 0;
  }
}

