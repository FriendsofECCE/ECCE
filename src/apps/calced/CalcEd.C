#ifdef _WIN32
#include <windows.h>
#endif
#include <cstdint>
#include <fstream>
#include <memory>
  using std::flush;
  using std::ofstream;
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <wx/combo.h>

#include "util/Ecce.H"
#include "util/BrowserHelp.H"
#include "wxgui/WxHelpViewer.H"
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
#include "dsm/TGBSGroup.H"
#include "dsm/TGaussianBasisSet.H"
#include <set>
#include "dsm/EDSIFactory.H"
#include "dsm/TaskJob.H"
#include "dsm/EDSIGaussianBasisSetLibrary.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/ICalculation.H"
#include "dsm/ICalcUtils.H"
#include "dsm/JCode.H"
#include "dsm/ResourceTool.H"
#include "dsm/ResourceType.H"
#include "dsm/SummaryIterator.H"
#include "dsm/CodeFactory.H"

#include "wxgui/ewxThemeColours.H"
#include "wxgui/ThingToggle.H"
#include "wxgui/ewxWindowUtils.H"
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
#include <wx/timer.h>
#include "wxgui/WxFeedback.H"

#include "CalcEd.H"
#include "VerifyReportDialog.H"
#include "GeomConstraints.H"
#include "PartialCharge.H"

//#define DEBUG

namespace {

//  ai.<code>'s own diagnostic is worth showing; the shell session it
//  ran in is not.  RCommand::execout() can leak its prompt/status
//  bookkeeping into the captured output (most reliably on a non-zero
//  exit code it doesn't specifically recognise -- see RCommand.C's
//  CMDSTAT matching), and ESInputController.C appends the reproduction
//  command after it.  Strip all of that before anything reaches a
//  user-visible message.
string cleanGeneratorMessage(const string& raw)
{
  string out;
  size_t pos = 0;
  while (pos <= raw.size()) {
    size_t eol = raw.find('\n', pos);
    string line = raw.substr(pos, eol == string::npos ? string::npos
                                                       : eol - pos);
    pos = (eol == string::npos) ? raw.size() + 1 : eol + 1;

    size_t start = line.find_first_not_of(" \t\r");
    if (start == string::npos) continue;
    line = line.substr(start, line.find_last_not_of(" \t\r") - start + 1);

    if (line == "Input files could not be generated." ||
        line.rfind("CMDSTAT=", 0) == 0 ||
        line.rfind("+go+", 0) == 0 ||
        line.rfind("(command:", 0) == 0)
      continue;

    if (!out.empty()) out += "  ";
    out += line;
  }
  return out;
}

//  The status area has room for one line, not a generator's whole
//  explanation -- take the first sentence (or the first line, if that
//  comes first) and point at Verify for the rest.
string firstSentence(const string& text)
{
  size_t cut = text.find(". ");
  if (cut != string::npos) return text.substr(0, cut + 1);
  return text;
}

}  // namespace


IMPLEMENT_CLASS( CalcEd, CalcEdGUI )

BEGIN_EVENT_TABLE(CalcEd, CalcEdGUI)
  EVT_SOCKET(wxID_THEORY_CHANGE, CalcEd::OnTheoryIPC)
  EVT_SOCKET(wxID_RUNTYPE_CHANGE, CalcEd::OnRuntypeIPC)
END_EVENT_TABLE()

/** Basis set quick pick list.  */
const char* CalcEd::p_BASIS_QUICK_PICKS[] = {
  "6-31G", "6-31++G", "6-31G*", "6-31+G*", "6-31++G**", "-", // len(6)
  "cc-pVDZ", "cc-pVTZ", "aug-cc-pVDZ", "aug-cc-pVTZ", "-", // len(5)
  "def2-svp", "def2-svpd", "def2-tzvp","-", // len(4)
  "DZVP (DFT Orbital)", "DZVP2 (DFT Orbital)", "TZVP (DFT Orbital)" // len(3)
}; // len(18)

// Both the menu and its event range follow the list, so an entry added to it
// can't be left out of one of them (TZVP was, with a hardcoded 17).
#define BASIS_QUICK_COUNT \
    ((int)(sizeof(p_BASIS_QUICK_PICKS) / sizeof(p_BASIS_QUICK_PICKS[0])))


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
    p_handEdited(false),
    p_inputGenFailed(false),
    p_inputGenError(""),
    p_hasInputFile(false),
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
    p_handEdited(false),
    p_inputGenFailed(false),
    p_inputGenError(""),
    p_hasInputFile(false),
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

  //  ECCE_TEST_CALCED=<file>: lines appended to <file> are run as commands
  //  (runTestCommand), each answered on stderr as
  //  "ECCE_TEST_CALCED: <command>: <outcome>".  Inert unless set; for
  //  tests/apps/session_end.py, which drives an editor with no clicks.
  if (const char *cmdPath = getenv("ECCE_TEST_CALCED")) {
    string path = cmdPath;
    auto done = std::make_shared<size_t>(0);
    wxTimer *timer = new wxTimer();   // lives until the process exits
    timer->Bind(wxEVT_TIMER, [this, path, done](wxTimerEvent&) {
      std::ifstream in(path.c_str());
      string line;
      size_t n = 0;
      while (std::getline(in, line)) {
        if (n++ < *done) continue;
        *done = n;
        if (!line.empty() && line.back() == '\r')   // written on Windows
          line.pop_back();
        runTestCommand(line);
      }
    });
    timer->Start(500);
  }
}


/**
 * One command of the ECCE_TEST_CALCED hook:
 *   state             the stored ES.Theory.UseSymmetry, the box, the theory
 *   theory <label>    choose a theory as the Theory menu does
 *   box 0|1           untick/tick "Use symmetry" as a click does
 *   opentheory        Theory Details..., closetheory closes it
 *   save              Save, keeping the generator's files (as Shift+Save)
 *   quit              exit without asking
 *   builder           the Builder button (startApp of the Builder)
 */
void CalcEd::runTestCommand(const string& line)
{
  string outcome;
  if (line == "state") {
    GUIValue *v = p_GUIValues ? p_GUIValues->get("ES.Theory.UseSymmetry") : 0;
    wxWindow *box = FindWindow(ID_CHECKBOX_CALCED_USE_SYMMETRY);
    outcome = "stored=" + (v ? v->getValueAsString() : string("absent")) +
        " box=" + (box && ((ewxCheckBox*)box)->IsChecked() ? "1" : "0") +
        " theory=" + getTheoryCategory().ToStdString() + "/" +
        getTheoryName().ToStdString() +
        " keys=" + StringConverter::toString(
                       p_GUIValues ? (int)p_GUIValues->size() : -1);
  } else if (line.compare(0, 7, "theory ") == 0) {
    ewxChoice *choice = (ewxChoice*)FindWindow(ID_CHOICE_CALCED_THEORY);
    if (!choice->SetStringSelection(line.substr(7))) {
      outcome = "no such theory";
    } else {
      wxCommandEvent event(wxEVT_COMMAND_CHOICE_SELECTED,
                           ID_CHOICE_CALCED_THEORY);
      OnChoiceCalcedTheorySelected(event);
      outcome = "ok";
    }
  } else if (line == "box 0" || line == "box 1") {
    ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CALCED_USE_SYMMETRY))
        ->SetValue(line == "box 1");
    wxCommandEvent event(wxEVT_COMMAND_CHECKBOX_CLICKED,
                         ID_CHECKBOX_CALCED_USE_SYMMETRY);
    OnCheckboxCalcedUseSymmetryClick(event);
    outcome = "ok";
  } else if (line == "opentheory") {
    startTheoryApp(false);
    outcome = "ok";
  } else if (line == "closetheory") {
    closeTheoryApp(true);
    outcome = "ok";
  } else if (line == "save") {
    p_keptGeneratorDir = "";
    p_testKeepParams = true;
    enableSave();
    doSave();
    p_testKeepParams = false;
    outcome = p_keptGeneratorDir.empty() ? "no input generated"
                                         : "kept " + p_keptGeneratorDir;
  } else if (line == "builder") {
    startApp("Builder", 0, p_context);
    outcome = "requested";
  } else if (line == "quit") {
    cerr << "ECCE_TEST_CALCED: quit: ok" << endl;
    closeTheoryApp(true);
    closeRuntypeApp(true);
    _exit(0);
  } else {
    outcome = "unknown command";
  }
  cerr << "ECCE_TEST_CALCED: " << line << ": " << outcome << endl;
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

  //  A charge outside the list (+3 for [Co(NH3)6]3+) is typed, and was
  //  only ever applied by the Enter event -- which the box never sent,
  //  lacking wxTE_PROCESS_ENTER, so the spin choices stayed those of the
  //  neutral molecule.  Enter now works; leaving the box applies it too.
  wxWindow *chargeBox = FindWindow(ID_COMBOBOX_CALCED_CHARGE);
  if (chargeBox) {
    chargeBox->Bind(wxEVT_KILL_FOCUS, [this, chargeBox](wxFocusEvent& ev) {
      int typed;
      const string text = ((wxComboBox*)chargeBox)->GetValue().ToStdString();
      if (p_frag && StringConverter::toInt(text, typed) &&
          typed != p_frag->charge()) {       // only a real change marks it dirty
        wxCommandEvent apply(wxEVT_TEXT_ENTER, ID_COMBOBOX_CALCED_CHARGE);
        apply.SetString(text);
        OnComboboxCalcedChargeSelected(apply);
      }
      ev.Skip();
    });
  }
  //GetSizer()->SetSizeHints(this);
  //GetSizer()->SetMinSize(SYMBOL_CALCEDGUI_SIZE);
  Centre();

  // set desktop icon
  ewxWindowUtils::setToolIcon(this, CALCED);
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
  unsigned long icode = (uintptr_t)eventButton->GetClientData();

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
      if ((uintptr_t)codeButton->GetClientData() == icode) {
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

    //  Check whatever input file this calculation already has, so the
    //  lamp means something on a calculation that is merely opened
    //  and not re-saved (#148).  Once per open, not per edit.  It sets
    //  the lamp blank, not green, when there is no input file yet.
    //  It also refreshes p_hasInputFile, so re-run enableLaunch() to
    //  put the Launch button in step with whatever that turned out to
    //  be -- doSetContext() above already ran it once, against
    //  whatever the previous calculation (or nothing) had left there.
    loadInputGenWarnings();
    verifyInput();
    enableLaunch();
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
      if (!p_fullFrag->generateFullMolecule())
        p_feedback->setMessage("The full molecule could not be generated from "
                "its symmetry-unique atoms (point group " +
                p_fullFrag->pointGroup() + "); see the terminal for why.",
                WxFeedback::WARNING);
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

  // The stored deck may have been generated from the unconverted number,
  // and Launch reuses a deck unless the calc is modified -- so mark it.
  if (p_GUIValues->convertedLegacyUnits() &&
      p_feedback->getEditStatus() != WxFeedback::READONLY) {
    p_feedback->setMessage("The memory setting was saved in an older unit "
            "and has been converted to gigabytes.  Check it in Theory "
            "Details, then save to regenerate the input file.",
            WxFeedback::WARNING);
    enableSave();
  }

  p_codeName = p_code->name();

  updateAllFields();

  if (isDetailsReady() && !hasDetailsValues()) {
    startTheoryApp(true);
    startRuntypeApp(true);
  }

  // GDB 2/19/13  Fix for dialog states getting cleared between app invocations
  // Always restore the underlying model regardless of whether dialog is invoked
  updateGeomModel();
  p_geomConstraints = new GeomConstraints(this, this, wxID_ANY, "Geometry Constraints");

  updateESPModel();
  p_partialCharge = new PartialCharge(this);

  static bool scriptStarted = false;
  if (!scriptStarted && getenv("ECCE_CALCED_SCRIPT")) {
    scriptStarted = true;
    runCalcEdScript(getenv("ECCE_CALCED_SCRIPT"));
  }
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
  p_inputGenFailed = false;
  p_inputGenError = "";
  p_inputGenWarnings.clear();

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
  else {
    // The file on disk is now a hand edit, not something generateInput()
    // wrote -- regenerateIfStructureChanged() must ask before replacing
    // it, rather than silently regenerating over it.
    p_handEdited = true;

    // A hand edit is the user's call: it replaces whatever the last
    // generateInput() attempt did or didn't produce, so a stale
    // "generator refused these settings" no longer describes the file
    // that is now on disk.
    p_inputGenFailed = false;
    p_inputGenWarnings.clear();
    storeInputGenWarnings();
  }
  ifs.close();

  //  A hand edit is the ONE case where the deck can become broken
  //  without ECCE having written it, so it is the case the checker
  //  most needs to see -- and it was the one case that never reached
  //  it (#148, reported live 2026-09-25: "I corrupted a file and it
  //  doesn't trigger the check").  Nothing here regenerates the
  //  input, so the user's own edit is what gets checked.
  vector<VerifyFinding> findings;
  if (verifyInput(&findings) &&
      InputVerifier::worst(findings) == VerifyFinding::BAD) {
    //  Say so here rather than only colouring the lamp.  The user has
    //  just typed into this file and is still looking at the editor;
    //  a silent lamp two panels away is not where their attention is.
    p_feedback->setMessage("Your edit leaves a problem in the input "
                           "file: " + VerifyReportDialog::summary(findings) +
                           ". Click Verify to see where.",
                           WxFeedback::WARNING);
  }
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


/**
 * Rebuild the input file from the calculation's current settings.
 *
 * Save already does this, but File > Save is greyed out unless something
 * has been edited (enableSave()), so there was no way to rebuild a deck
 * whose settings had not changed.  That matters more than it sounds:
 * the input file is generated once and then kept, so a calculation
 * copied from an older one, or simply created before an ai.<code>
 * generator was improved, goes on using the deck it was born with
 * forever.  A fix to a generator never reaches any existing
 * calculation.
 *
 * Hit live: MOPAC gained a "print molecular orbitals" option, and
 * duplicating an older calculation kept producing decks without it, on
 * four separate attempts, with nothing in the UI to say why or what to
 * do about it.
 *
 * Deliberately destructive of hand edits, and says so first -- the
 * generated file is exactly what Save would have written, so anything
 * typed into the input by hand is lost.  That is the whole point of the
 * action, but it should not be a surprise.
 */
void CalcEd::OnMenuCalcedRegenInputClick( wxCommandEvent& event )
{
  if (!p_iCalc) {
    event.Skip();
    return;
  }

  //  A calculation that has already run needs resetting first.
  //  generateInput() goes through isReady(), which requires the state to
  //  be below SUBMITTED -- rightly, since rewriting the deck of a job
  //  that has run would leave its stored results describing a different
  //  input.  Without this the menu item simply did nothing on a
  //  completed calculation, which is exactly when someone wants it.
  bool hasRun = (p_iCalc->getState() >= ResourceDescriptor::STATE_SUBMITTED);
  if (hasRun) {
    ewxMessageDialog ask(this,
          "This calculation has already run, so its results were produced "
          "by the current input file.\n\n"
          "Reset it for rerun and rebuild the input file?  The previous "
          "results and output are discarded, and any edits made to the "
          "input file by hand are lost.",
          "Regenerate Input File", wxYES_NO | wxICON_QUESTION);
    if (ask.ShowModal() != wxID_YES) {
      event.Skip();
      return;
    }

    TaskJob *task = dynamic_cast<TaskJob*>(
            EDSIFactory::getResource(p_iCalc->getURL()));
    if (task == 0 || !task->resetForRerun()) {
      p_feedback->setMessage("Could not reset the calculation for rerun, "
                             "so the input file was left alone.",
                             WxFeedback::ERROR);
      event.Skip();
      return;
    }
    //  Pick the new state up, so isReady() below sees it.
    p_feedback->setRunState(p_iCalc->getState());
  } else {
    ewxMessageDialog confirm(this,
          "Rebuild the input file from this calculation's current settings?"
          "\n\nAny edits made to the input file by hand will be lost.",
          "Regenerate Input File", wxYES_NO | wxICON_QUESTION);
    if (confirm.ShowModal() != wxID_YES) {
      event.Skip();
      return;
    }
  }

  if (generateInput(false)) {
    if (p_inputGenWarnings.empty())
      p_feedback->setMessage("Input file regenerated.", WxFeedback::INFO);
    else
      p_feedback->setMessage("Input file regenerated with warnings. "
                             "See Verify.", WxFeedback::WARNING);
  } else {
    //  generateInput() puts the generator's own diagnostic on screen
    //  when it fails, so do not paper over it with a generic message.
    p_feedback->setMessage("Input file could not be regenerated.",
                           WxFeedback::ERROR);
  }

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
  WxHelpViewer::showKey("CalculationEditor");
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
    p_frag->name(event.GetString().ToStdString());
    p_fullFrag->name(event.GetString().ToStdString());
  }

  event.Skip();
}


void CalcEd::OnComboboxCalcedChargeSelected( wxCommandEvent& event )
{
  if (p_frag) {
    int value;
    if (StringConverter::toInt(event.GetString().ToStdString(), value)) {
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
  setSpinMult(SpinMult::toSpinMult(event.GetString().ToStdString()), true);
  setOpenShells(getSpinMult() - 1);
  restrictTheoriesBySpin();
  enableSave();

  event.Skip();
}


void CalcEd::OnComboboxCalcedSpinMultEnter( wxCommandEvent& event )
{
  string valstr = event.GetString().ToStdString();
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
      if (!p_fullFrag->generateFullMolecule())
        p_feedback->setMessage("The full molecule could not be generated from "
                "its symmetry-unique atoms (point group " +
                p_fullFrag->pointGroup() + "); see the terminal for why.",
                WxFeedback::WARNING);
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


/**
 * The basis sets the quick menu offers: the code's own short list when its
 * .edml gives one (ECCE-QM), otherwise the built-in list.  "-" is a separator.
 */
vector<string> CalcEd::quickPicks() const
{
  vector<string> picks;
  if (p_code) picks = p_code->getBasisSetPicks();
  if (picks.empty()) {
    for (int i = 0; i < BASIS_QUICK_COUNT; i++) picks.push_back(p_BASIS_QUICK_PICKS[i]);
  }
  return picks;
}


void CalcEd::OnButtonCalcedBasisQuickClick( wxCommandEvent& event )
{
  wxMenu menu;
  vector<string> picks = quickPicks();
  for (size_t i = 0; i < picks.size(); i++) {
    if (picks[i] == "-") {
      menu.AppendSeparator();
    } else {
      menu.Append(ID_BASIS_PICK0 + (int)i, picks[i]);
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

  vector<string> picks = quickPicks();
  size_t pick = event.GetId() - ID_BASIS_PICK0;
  if (pick >= picks.size()) {
    delete gbsFactory;
    return;
  }
  if (p_basis) {
    delete p_basis;
  }
  p_basis = gbsFactory->simpleLookup(picks[pick].c_str(),
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
      if (!regenerateIfStructureChanged()) {
        event.Skip();
        return;
      }

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
        if (p_inputGenFailed) {
          p_feedback->setMessage("The current settings could not be "
                  "generated, so this is the last input file that was: it "
                  "does not reflect the settings the generator refused. "
                  "Saving an edit makes it launchable as you leave it.",
                  WxFeedback::WARNING);
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


//  ------------------------------------------------ input checking (#148)
//
//  A deck can be structurally broken in ways nothing in this editor
//  would notice: the basis block emerging as six bytes of binary
//  (#146), a route card promising /GEN with nothing following it.  Both
//  happened on 2026-09-25, both looked perfectly normal in the editor,
//  and both were discovered when the job died in a queue.  The checker
//  is a script so that what it knows can grow from decks that failed
//  without waiting for a release; see scripts/parsers/verifyinput.

bool CalcEd::verifyInput(vector<VerifyFinding>* out)
{
  if (out) out->clear();
  if (!p_iCalc) return false;

  istream* is = p_iCalc->getDataFile(JCode::PRIMARY_INPUT);
  p_hasInputFile = (is != 0);

  //  A failed generation is a fact about the deck that the checker
  //  script never sees -- it only ever reads what's on disk.  Fold it
  //  in here, as a finding, so the SAME path colours the lamp and
  //  builds the Verify dialog, rather than a second mechanism that
  //  could disagree with this one.
  vector<VerifyFinding> findings;
  if (p_inputGenFailed) {
    VerifyFinding bad;
    bad.level = VerifyFinding::BAD;
    bad.check = "generatorFailed";
    bad.message = "The input generator refused the current settings: " +
                  p_inputGenError + (p_hasInputFile ?
                    "  The deck below is from the last successful "
                    "generation and does not reflect these settings." :
                    "  There is no input file: nothing has ever been "
                    "generated for this calculation.");
    findings.push_back(bad);
  }

  if (!is) {
    //  Unchanged for the plain case: a checker that never ran says
    //  nothing about the deck, so leave the lamp as it was.  But a
    //  known generator failure IS something to say, even with no deck
    //  to check.
    if (findings.empty()) return false;

    if (out) *out = findings;
    setVerifyLight(true, VerifyFinding::BAD,
                   VerifyReportDialog::summary(findings) +
                   ". Click Verify for the detail.");
    return true;
  }

  string text;
  StringConverter::streamToText(*is, text);
  delete is;

  //  How many atoms the calculation thinks it has, so the checker can
  //  compare.  Pass 0 rather than a guess when there is no fragment:
  //  a wrong count here produces a confident and false complaint,
  //  which is the one thing this feature must not do.
  int atoms = 0;
  if (p_frag) atoms = p_frag->numAtoms();

  //  Combination problems the generator reported while still writing
  //  the deck.  Placed on the first deck line holding the anchor, so
  //  the dialog can mark it; 0 (whole file) when it is not there.
  for (size_t w = 0; w < p_inputGenWarnings.size(); w++) {
    VerifyFinding warn;
    warn.level = VerifyFinding::BAD;
    warn.check = "generatorWarning";
    warn.message = p_inputGenWarnings[w].second;
    int lineNo = 1;
    size_t start = 0;
    while (start <= text.size()) {
      size_t eol = text.find('\n', start);
      if (eol == string::npos) eol = text.size();
      if (text.substr(start, eol - start).find(
              p_inputGenWarnings[w].first) != string::npos) {
        warn.line = warn.lineEnd = lineNo;
        break;
      }
      start = eol + 1;
      lineNo++;
    }
    findings.push_back(warn);
  }

  vector<VerifyFinding> checked;
  string error;
  if (InputVerifier::run(text, p_codeName, atoms, checked, error)) {
    findings.insert(findings.end(), checked.begin(), checked.end());
  } else if (findings.empty()) {
    setVerifyLight(false, VerifyFinding::GOOD, error);
    return false;
  }

  if (out) *out = findings;

  const VerifyFinding::Level worst = InputVerifier::worst(findings);
  setVerifyLight(true, worst, VerifyReportDialog::summary(findings) +
                              ". Click Verify for the detail.");
  return true;
}


void CalcEd::setVerifyLight(bool checked, VerifyFinding::Level level,
                            const string& tip)
{
  wxWindow* window = FindWindow(ID_STATICTEXT_CALCED_VERIFY_LIGHT);
  if (!window) return;
  wxStaticText* lamp = wxDynamicCast(window, wxStaticText);
  if (!lamp) return;

  if (!checked) {
    //  Nothing was checked.  Show nothing -- not a green light, and
    //  not a red one either, since a checker that failed to run says
    //  nothing whatever about the deck.
    lamp->SetLabel(wxEmptyString);
    lamp->SetToolTip(wxString(tip.c_str(), wxConvUTF8));
    return;
  }

  //  The glyph differs per level as well as the colour, so the lamp
  //  still carries its meaning to a reader who does not see the
  //  difference between the reds and greens.
  const char* glyph = "\xe2\x97\x8f";          // filled circle
  wxColour colour = ewxThemeColours::statusText(ewxThemeColours::GOOD);
  if (level == VerifyFinding::BAD) {
    glyph = "\xe2\x9c\x95";                     // cross
    colour = ewxThemeColours::statusText(ewxThemeColours::BAD);
  } else if (level == VerifyFinding::UNSURE) {
    glyph = "\xe2\x9a\xa0";                     // warning sign
    colour = ewxThemeColours::statusText(ewxThemeColours::UNSURE);
  }

  lamp->SetLabel(wxString::FromUTF8(glyph));
  lamp->SetForegroundColour(colour);
  lamp->SetToolTip(wxString(tip.c_str(), wxConvUTF8));
  lamp->Refresh();
}


void CalcEd::OnButtonCalcedVerifyClick( wxCommandEvent& event )
{
  if (p_iCalc) {
    try {
      //  DO NOT save first.  Saving regenerates the input file from
      //  the editor's fields, which throws away whatever the user did
      //  in Final Edit -- so a Verify press after a hand edit would
      //  silently destroy the edit and then cheerfully report that
      //  the regenerated deck is fine.  Check the file that is
      //  actually there, and say plainly when the editor has changes
      //  that are not in it.
      vector<VerifyFinding> findings;
      if (!verifyInput(&findings)) {
        p_feedback->setMessage("The input file could not be checked. "
                               "Generate it first, or see the Verify "
                               "light for why.", WxFeedback::WARNING);
      } else {
        istream* is = p_iCalc->getDataFile(JCode::PRIMARY_INPUT);
        string text;
        if (is) {
          StringConverter::streamToText(*is, text);
          delete is;
        }
        //  A stale file is not a wrong file, and must not be reported
        //  as one -- but the user has to know which file they are
        //  looking at.
        if (p_feedback->getEditStatus() == WxFeedback::MODIFIED) {
          VerifyFinding stale;
          stale.level = VerifyFinding::UNSURE;
          stale.line = 0;
          stale.check = "unsaved";
          stale.message = "The editor has changes that are not in this "
                          "file yet. This is the input file as it was "
                          "last generated; save to regenerate it.";
          //  Insert after a generator-failure finding rather than
          //  before it -- that one must stay first, since it is the
          //  more pressing of the two ("at the top" per #187 followup).
          size_t at = 0;
          while (at < findings.size() &&
                 findings[at].check == "generatorFailed")
            at++;
          findings.insert(findings.begin() + at, stale);
        }

        VerifyReportDialog dialog(this, p_codeName, text, findings);
        dialog.ShowModal();
      }
    } catch (EcceException& ex) {
      p_feedback->setMessage(ex.what(), WxFeedback::ERROR);
    }
  }

  event.Skip();
}


void CalcEd::OnButtonCalcedLaunchClick( wxCommandEvent& event )
{
  if (!regenerateIfStructureChanged())
    return;

  ResourceTool *tool =
          ResourceDescriptor::getResourceDescriptor().getTool(LAUNCHER);
  event.SetId(tool->getId());
  OnToolClick(event);
}


/**
 * See CalcEd.H for the full rationale (#GitHub, 2026-09-27).  Asks the
 * calculation itself -- a real WebDAV timestamp comparison, independent
 * of whatever this window's own edit-status bookkeeping believes --
 * whether the structure is newer than the input file, and regenerates
 * when it is, so a stale deck (edited in the Builder viewer while this
 * window stayed open, or simply never regenerated) cannot reach
 * Launch or Final Edit.
 */
bool CalcEd::regenerateIfStructureChanged()
{
  if (!p_iCalc || !p_iCalc->isFragmentNew())
    return true;

  if (p_handEdited) {
    long buttons = wxYES_NO | wxCANCEL | wxICON_QUESTION | wxNO_DEFAULT;
    ewxMessageDialog dialog(this,
        "The chemical structure has changed since this input file was "
        "last generated, but the current input file is a hand edit "
        "(Final Edit) that regenerating it would lose.\n\n"
        "Regenerate the input file from the current structure now?\n"
        "Yes: regenerate it, losing the hand edit.\n"
        "No: launch the hand-edited file unchanged.\n"
        "Cancel: do nothing.",
        "Structure Changed Since Hand Edit", buttons);
    int answer = dialog.ShowModal();
    if (answer == wxID_CANCEL) return false;
    if (answer == wxID_NO) return true;
    // wxID_YES falls through to regenerate, below.
  } else {
    p_feedback->setMessage("The chemical structure has changed since "
            "this input file was last generated.  Regenerating the "
            "input file.", WxFeedback::INFO);
  }

  //  Same refresh-and-regenerate path the JMS "ecce_url_subject"
  //  notification already runs (subjectMCB()) when the Builder is open
  //  on the same calc at the moment the edit is saved.  Calling it
  //  again here is deliberately redundant with that: it is the one
  //  path guaranteed to pick up the new structure even if that
  //  notification was never delivered (CalcEd opened in a separate
  //  session, message missed, this window's context not matched, ...).
  wxCommandEvent dummy;
  subjectMCB(dummy);

  return true;
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
      if (!p_fullFrag->generateFullMolecule())
        p_feedback->setMessage("The full molecule could not be generated from "
                "its symmetry-unique atoms (point group " +
                p_fullFrag->pointGroup() + "); see the terminal for why.",
                WxFeedback::WARNING);
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

  if (isDetailsReady() && !hasDetailsValues()) {
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

  //  Save sits in the button row, a standard button just before Launch.
  wxButton *save = p_feedback->adoptSaveButton(this);
  p_buttonRow->Insert(p_buttonRow->GetItemCount() - 1, save, 0,
                      wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

  replaceBuilderButton();
  replaceBasisSetButton();

  //  tests/apps/clip_test.py: show Save, which appears with unsaved edits.
  if (getenv("ECCE_CLIP_AUDIT") != 0) {
    wxTimer *timer = new wxTimer();   // lives until the process exits
    timer->Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
      p_feedback->setEditStatus(WxFeedback::MODIFIED);
      Layout();
    });
    timer->StartOnce(2000);
  }

  //  Up to 256 ids: a code may give its own quick-pick list (ECCE-QM).
  Connect( ID_BASIS_PICK0, ID_BASIS_PICK0 + 255, wxEVT_COMMAND_MENU_SELECTED,
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
  wxString label = choice->GetStringSelection();

  //  Reverse of populateTheories(), which shows name() for every theory
  //  EXCEPT one whose name is literally "None" -- for that it shows
  //  category() instead.
  //
  //  This used to detect that case by comparing the label against the
  //  category, which is a guess, and it is wrong for any code whose
  //  theory NAME EQUALS ITS CATEGORY. Quantum ESPRESSO's is
  //  category="PW" name="PW", so selecting it yielded ("PW", "None") --
  //  a theory that exists in no .edml. Both lookups keyed on it then
  //  fell through to their defaults, which is why QE showed two
  //  unrelated-looking faults at once (reported live 2026-09-22):
  //
  //    * JCode::theoryNeedsBasis() returns true when the theory is not
  //      found, so the Basis Set Tool stayed enabled for a PLANE-WAVE
  //      code, despite needsBasis="false" being set correctly.
  //    * populateRuntypes() found no runtypes for it, so the list came
  //      up empty and CalcEd reported "No runtypes are supported for
  //      the given code/theory combination" -- leaving nothing to edit
  //      and nothing to launch.
  //
  //  Ask the code whether a "None"-named theory in this category
  //  actually exists, rather than inferring it from the label.
  if (p_code) {
    wxString category = getTheoryCategory();
    if (label.IsSameAs(category)) {
      bool hasNoneTheory = false;
      vector<TTheory> *theories = p_code->theories();
      if (theories) {
        vector<TTheory>::iterator it;
        for (it = theories->begin(); it != theories->end(); it++) {
          if (it->name() == "None" && it->category() == category.ToStdString()) {
            hasNoneTheory = true;
            break;
          }
        }
        delete theories;
      }
      if (hasNoneTheory) {
        return "None";
      }
    }
  }

  return label;
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
                  ->GetLabel().ToStdString(), ret);

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
                + ".").ToStdString(), WxFeedback::WARNING);
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
                    + " to " + choice->GetStringSelection() + ".").ToStdString(),
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
                    + " to " + choice->GetStringSelection() + ".").ToStdString(),
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
                + ".").ToStdString(), WxFeedback::WARNING);
      }
    } else if (lastRuntypeName!=wxEmptyString &&
               choice->SetStringSelection(lastRuntypeName)) {
      // value choice set from STTR logic
      if (p_lastRuntype.name()!="" && !rxnFlag) {
        p_feedback->setMessage(("Overriding runtype " + p_lastRuntype.name()
                + " to user preferred runtype " + choice->GetStringSelection()
                + ".").ToStdString(), WxFeedback::WARNING);
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
                  + " to " + choice->GetStringSelection() + ".").ToStdString(),
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


bool CalcEd::getUseSymmetry() const
{
  if (p_GUIValues != (GUIValues*)0) {
    GUIValue *value = p_GUIValues->get("ES.Theory.UseSymmetry");
    if (value != (GUIValue*)0) {
      return value->getValueAsInt() != 0;
    }
  }
  //  No stored choice: on.
  return true;
}


void CalcEd::setUseSymmetry(const bool& value)
{
  wxWindow *box = FindWindow(ID_CHECKBOX_CALCED_USE_SYMMETRY);
  if (box != 0) ((ewxCheckBox*)box)->SetValue(value);
  storeUseSymmetry(value);
}


void CalcEd::storeUseSymmetry(const bool& value)
{
  if (p_GUIValues == (GUIValues*)0) return;

  //  WRITTEN WHETHER OR NOT IT WAS TOUCHED.
  //
  //  ai.gauss16 tests "$useSymmetry == 0", and an absent key is undef,
  //  which is 0 -- so a calculation that never stored the choice got
  //  NoSymm no matter what any dialog's default said.  That is why
  //  flipping the dialog default alone was not enough: the theory
  //  dialog has to be opened before it writes anything.
  GUIValue *entry = p_GUIValues->get("ES.Theory.UseSymmetry");
  if (entry == (GUIValue*)0) {
    entry = new GUIValue();
    entry->m_key = "ES.Theory.UseSymmetry";
    entry->m_type = "toggle_input";
    entry->m_sensitive = true;
    entry->m_write = true;
    p_GUIValues->set("ES.Theory.UseSymmetry", entry);
  }
  entry->setValue(value ? 1 : 0);
}


bool CalcEd::useSymmetryBox() const
{
  wxWindow *box = FindWindow(ID_CHECKBOX_CALCED_USE_SYMMETRY);
  return box != 0 ? ((ewxCheckBox*)box)->IsChecked() : getUseSymmetry();
}


//  Whether the details dialogs have supplied values under prefix.  The
//  "Use symmetry" key is this page's own and is stored whenever a molecule
//  is shown, so it does not count.
bool CalcEd::hasDetailsValues(const string& prefix) const
{
  if (p_GUIValues == (GUIValues*)0) return false;
  for (GUIValues::const_iterator it = p_GUIValues->begin();
       it != p_GUIValues->end(); ++it) {
    if (it->first != "ES.Theory.UseSymmetry" &&
        it->first.compare(0, prefix.size(), prefix) == 0)
      return true;
  }
  return false;
}


void CalcEd::OnCheckboxCalcedUseSymmetryClick( wxCommandEvent& event )
{
  wxWindow *box = FindWindow(ID_CHECKBOX_CALCED_USE_SYMMETRY);
  if (box != 0) storeUseSymmetry(((ewxCheckBox*)box)->IsChecked());

  //  IT IS A CHANGE TO THE CALCULATION, so say so.
  //
  //  Without this the calculation was never marked modified, so no
  //  Save appeared and there was no way to write the new input:
  //  ticking the box changed the stored value and nothing else, and
  //  "Regenerate input file" does not help because the editor does
  //  not believe anything has changed. Every other control on this
  //  panel does the same thing -- see the exponents checkbox.
  enableSave();

  event.Skip();
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
    p_spinMult = SpinMult::toSpinMult(combo->GetString(0).ToStdString());
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
  fitToScreen();
}


/**
 * Clears both theory and runtype details changes.
 */
void CalcEd::resetTheoryDetails()
{
  //  The details dialogs go back to their defaults; the "Use symmetry"
  //  tick on this page does not.  Without its key the generators read
  //  "off" and write NoSymm/noautosym under a ticked box.
  bool useSymmetry = useSymmetryBox();
  if (p_GUIValues) {
    delete p_GUIValues;
  }
  p_GUIValues = new GUIValues();
  storeUseSymmetry(useSymmetry);

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
      string name = choice->GetString(i).ToStdString();
      if (name.find("RO") == 0 || name.find("UMP5") == 0) {
        choice->Delete(i);
      } else {
        i++;
      }
    }
  } else if ((int)spinMult-1 == openShells) {
    // remove restricted spin theories
    for (size_t i = 0; i < choice->GetCount(); /*i++*/) {
      string name = choice->GetString(i).ToStdString();
      if (name.find('R') == 0 && name.find('O') != 1) {
        choice->Delete(i);
      } else {
        i++;
      }
    }
  } else { // below the diagonal
    // remove restricted, unrestricted, restricted open shell spin theories
    for (size_t i = 0; i < choice->GetCount(); /*i++*/) {
      string name = choice->GetString(i).ToStdString();
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

/**
 * True when this code cannot run without a unit cell and the structure
 * does not have one.
 *
 * Declared per code as <RequiresPeriodic>true</RequiresPeriodic> in its
 * .edml. JCode::getValue() is a generic getElementsByTagName lookup, so
 * this needed no parser change -- and being declarative keeps the
 * knowledge next to the code it describes rather than hardcoded here.
 *
 * Quantum ESPRESSO is the case that motivated it: pw.x is a plane-wave
 * code and cannot run on a non-periodic structure at all. ai.qe already
 * refused loudly, naming the Builder's "Periodic Builder" panel -- but
 * only at input-generation time, which is after the user has set up the
 * whole calculation. Reported live as "drawing a molecule doesn't do
 * anything when it comes to creating a periodic system" and "Launch
 * doesn't do anything".
 */
bool CalcEd::requiresMissingCell() const
{
  if (p_code == 0 || p_frag == 0) {
    return false;
  }

  string flag;
  if (!p_code->get_string("RequiresPeriodic", flag)) {
    return false;
  }
  if (flag != "true" && flag != "TRUE" && flag != "yes" && flag != "1") {
    return false;
  }

  return (p_frag->getLattice() == (LatticeDef*)0);
}


bool CalcEd::isReady() const
{
  bool ret = false;

  //  A plane-wave code with no cell can never produce a valid deck, so
  //  it is not ready however complete the rest of the setup is.
  if (requiresMissingCell()) {
    return false;
  }

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


/**
 * A code with a basisSetDefault in its .edml starts every calculation with
 * that basis set, so a student has nothing to choose to get a working setup.
 */
void CalcEd::applyDefaultBasis()
{
  if (p_basis || !p_code || !p_frag || !p_iCalc || !theoryNeedsBasis()) return;
  if (p_iCalc->getState() >= ResourceDescriptor::STATE_SUBMITTED) return;
  string name = p_code->getBasisSetDefault();
  if (name.empty()) return;

  EDSIServerCentral central;
  EDSIGaussianBasisSetLibrary gbsFactory(central.getDefaultBasisSetLibrary());
  p_basis = gbsFactory.simpleLookup(name.c_str(), p_frag->uniqueTagStr().c_str());
  TTheory theory = getTheory();
  GBSRules::autoOptimize(p_basis, p_code, &theory);
  makeRuntypeNoSphericalConsistent();
  enableSave();
}


/**
 * For a code that only offers its quick-pick basis sets (ECCE-QM): which
 * elements of the molecule the chosen basis has no functions for, or needs a
 * core potential for, in words a student can act on.
 */
string CalcEd::uncoveredElementsMessage() const
{
  if (!p_basis || !p_code || p_code->getBasisSetPicks().empty()) return "";

  set<string> missing, ecp;
  for (TGBSConfig::iterator it = p_basis->begin(); it != p_basis->end(); ++it) {
    const TGBSGroup *group = it->second;
    if (group == NULL) continue;
    TGBSConfigTags tags(it->first.c_str());
    vector<const TGaussianBasisSet*> *sets = group->getOrbitalGBSList();
    const TGaussianBasisSet *ecpSet = group->ecp();
    for (size_t i = 0; i < tags.size(); i++) {
      if (ecpSet && ecpSet->p_contractions.find(tags[i]) != ecpSet->p_contractions.end()) {
        ecp.insert(tags[i]);
        continue;
      }
      bool covered = (sets != 0 && !sets->empty());
      if (sets) {
        for (size_t j = 0; j < sets->size(); j++) {
          if ((*sets)[j]->p_contractions.find(tags[i]) == (*sets)[j]->p_contractions.end()) {
            covered = false;
          }
        }
      }
      if (!covered) missing.insert(tags[i]);
    }
    delete sets;
  }

  string msg;
  string name = p_basis->name();
  if (!missing.empty()) {
    msg = "The basis set " + name + " has no functions for ";
    for (set<string>::const_iterator e = missing.begin(); e != missing.end(); ++e)
      msg += (e == missing.begin() ? "" : ", ") + *e;
    msg += ", which is in this molecule.  Choose another basis set from the "
           "Basis Set menu, or another molecule.";
  } else if (!ecp.empty()) {
    msg = "The basis set " + name + " uses a core potential for ";
    for (set<string>::const_iterator e = ecp.begin(); e != ecp.end(); ++e)
      msg += (e == ecp.begin() ? "" : ", ") + *e;
    msg += ", which ECCE-QM does not support.  Choose another basis set from "
           "the Basis Set menu, or another molecule.";
  }
  return msg;
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

    string uncovered = uncoveredElementsMessage();
    if (!uncovered.empty()) {
      p_feedback->setMessage(uncovered, WxFeedback::ERROR);
      ret = false;
    } else if (!GBSRules::isComplete(p_frag, p_basis, p_code, &theory)) {
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
  fitToScreen();
}


void CalcEd::updateChemSysFields()
{
  refreshChemSysFields();
  enableChemSysFields();
  showChemSysFields();
  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
  fitToScreen();
}


void CalcEd::updateBasisSetFields()
{
  refreshBasisSetFields();
  enableBasisSetFields();
  showBasisSetFields();
  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
  fitToScreen();
}


void CalcEd::updateDetailsFields()
{
  refreshDetailsFields();
  enableDetailsFields();
  showDetailsFields();
  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
  fitToScreen();
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

    //  THE CHOICE IS THE USER'S, AND THE DEFAULT IS ON.
    //
    //  This is a different thing from "Use As Irreducible Fragment"
    //  just above, which asks whether the geometry is written out as
    //  its symmetry-unique atoms.  This one asks whether the CODE is
    //  told to exploit the point group -- Gaussian's NoSymm, NWChem's
    //  noautosym.  They were conflated once; they are not the same
    //  question and must not share a flag.
    //
    //  Read from the calculation where it has one, and defaulted on
    //  where it does not: a job run without symmetry reports no
    //  orbital symmetry labels at all, which is why no Gaussian
    //  calculation had ever produced any (#145).
    setUseSymmetry(getUseSymmetry());
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
      bool useSymmetry = useSymmetryBox();
      delete p_GUIValues;
      p_GUIValues = new GUIValues();
      storeUseSymmetry(useSymmetry);
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
      wxLogNull quiet;   // an unreadable thumbnail is shown as the plain icon
      wxBitmap bitmap(thumbnail->path(), wxBITMAP_TYPE_JPEG);
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
    if (tool != (ResourceTool*)0)
      p_builderTool->setBitMap(ewxBitmap(tool->getIcon(), wxBITMAP_TYPE_XPM));
  }
}


void CalcEd::refreshBasisSetFields()
{
  bool hasFullFragment(p_fullFrag);
  bool hasCalc(p_iCalc);

  applyDefaultBasis();

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
          && hasDetailsValues("ES.Theory"));
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
      //  Say WHY, for the one not-ready reason the user cannot deduce
      //  from the setup panels. Everything else that blocks readiness
      //  (no theory, an incomplete basis) is visible in the dialog that
      //  owns it; a missing unit cell is not, because nothing in CalcEd
      //  mentions cells at all -- it belongs to the structure, and is
      //  created in a different application.
      //
      //  Without this the Launch and Final Edit buttons simply stay
      //  greyed with no explanation, which is what "Launch doesn't do
      //  anything" looked like from the outside.
      if (requiresMissingCell()) {
        p_feedback->setMessage(
            "This code requires a periodic unit cell, and this structure "
            "has none.\n"
            "Open the structure in the Builder, show the \"Periodic "
            "Builder\" tool panel, and create a lattice.",
            WxFeedback::ERROR);
      }
      if (p_iCalc->getState() == ResourceDescriptor::STATE_READY) {
        p_feedback->setRunState(ResourceDescriptor::STATE_CREATED);
      } else {
        p_feedback->setRunState(p_iCalc->getState());
      }
    }
  
    bool togo = p_feedback->getRunState() > ResourceDescriptor::STATE_CREATED
            && p_feedback->getRunState() != ResourceDescriptor::STATE_LOADED;
    FindWindow(ID_BUTTON_CALCED_FINAL_EDIT)->Enable(togo);
    //  Unlike Final Edit, Launch has nothing to submit if a deck was
    //  never actually written -- everything ELSE about whether a bad
    //  or stale deck may be launched is Verify's job, not this one's
    //  (Andy, 2026-09-29: Verify is what should flag a bad deck; Launch
    //  should let people do what they want with it).
    FindWindow(ID_BUTTON_CALCED_LAUNCH)->Enable(togo && p_hasInputFile);
    FindWindow(ID_BUTTON_CALCED_VERIFY)->Enable(togo);

    //  Deliberately NOT running the checker here.  enableLaunch() is
    //  reached from enableAllFields(), which runs on essentially every
    //  change in the editor, and a check costs a WebDAV fetch of the
    //  input file plus a process -- a round trip behind every edit.
    //  The check runs where the input file is actually written
    //  instead: generateInput(), below.  All this has to do is stop
    //  the lamp making a claim about a file that no longer exists.
    if (!togo)
      setVerifyLight(false, VerifyFinding::GOOD,
                     "There is no input file to check yet.");
  } else {
    FindWindow(ID_BUTTON_CALCED_FINAL_EDIT)->Enable(false);
    FindWindow(ID_BUTTON_CALCED_LAUNCH)->Enable(false);
    FindWindow(ID_BUTTON_CALCED_VERIFY)->Enable(false);
    setVerifyLight(false, VerifyFinding::GOOD,
                   "There is no input file to check yet.");
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
    bool toolHidden = p_code->getBasisSetToolHidden();
    FindWindow(ID_BUTTON_CALCED_BASIS_QUICK)
            ->Show(p_code->getBasisSetQuickListSupported() || toolHidden);
    //  A code with a fixed list of basis sets (ECCE-QM) is chosen from the
    //  menu alone; the Basis Set Tool would offer what it cannot use.
    if (p_basisSetTool) p_basisSetTool->Show(!toolHidden);
    FindWindow(ID_BUTTON_CALCED_BASIS_QUICK)->SetLabel(
        toolHidden ? wxString::FromUTF8("Basis Set ▼") : wxString::FromUTF8("Quick Basis Menu ▼"));
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
  fitToScreen();
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
  if (getRuntypeNoSpherical() && !GBSRules::sphericalOnly(p_code)
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


//  One warning per line, "anchor<TAB>message".  Escaped by hand because
//  putMetaData() writes the value into XML unescaped.
static string escapeWarningText(const string& in)
{
  string out;
  for (size_t i = 0; i < in.size(); i++) {
    switch (in[i]) {
      case '%':  out += "%25"; break;
      case '<':  out += "%3C"; break;
      case '>':  out += "%3E"; break;
      case '&':  out += "%26"; break;
      case '\t': out += "%09"; break;
      case '\n': out += "%0A"; break;
      case '\r': out += "%0D"; break;
      default:   out += in[i];
    }
  }
  return out;
}

static string unescapeWarningText(const string& in)
{
  string out;
  for (size_t i = 0; i < in.size(); i++) {
    if (in[i] == '%' && i + 2 < in.size() && isxdigit(in[i+1]) &&
        isxdigit(in[i+2])) {
      out += (char)strtol(in.substr(i + 1, 2).c_str(), 0, 16);
      i += 2;
    } else {
      out += in[i];
    }
  }
  return out;
}


void CalcEd::storeInputGenWarnings()
{
  if (!p_iCalc) return;
  if (p_feedback->getRunState() > ResourceDescriptor::STATE_READY) return;

  string value;
  for (size_t w = 0; w < p_inputGenWarnings.size(); w++)
    value += escapeWarningText(p_inputGenWarnings[w].first) + "\t" +
             escapeWarningText(p_inputGenWarnings[w].second) + "\n";

  if (p_iCalc->getProp(TaskJob::inputWarningsProp()) != value)
    p_iCalc->addProp(TaskJob::inputWarningsProp(), value);
}


void CalcEd::loadInputGenWarnings()
{
  p_inputGenWarnings.clear();
  if (!p_iCalc) return;

  string value = p_iCalc->getProp(TaskJob::inputWarningsProp());
  size_t pos = 0;
  while (pos < value.size()) {
    size_t eol = value.find('\n', pos);
    if (eol == string::npos) eol = value.size();
    string line = value.substr(pos, eol - pos);
    pos = eol + 1;
    size_t tab = line.find('\t');
    if (tab != string::npos)
      p_inputGenWarnings.push_back(std::make_pair(
          unescapeWarningText(line.substr(0, tab)),
          unescapeWarningText(line.substr(tab + 1))));
  }
}


bool CalcEd::generateInput(const bool& paramFlag)
{
  bool success = false;

  if (isReady()) {
    string message;
    success = input_controller(paramFlag, getUseExpCoeff(), message);

    //  Feeds the Verify lamp/dialog below, not readiness -- a failed
    //  generation is Verify's news to carry (Andy, 2026-09-29), so
    //  nothing here touches p_iCalc's stored state or Launch.
    p_inputGenFailed = !success;
    p_inputGenError = success ? "" : cleanGeneratorMessage(message);

    // The deck on disk only changes on success; after a failure the
    // stored warnings still describe it, so put them back.
    if (success) storeInputGenWarnings();
    else         loadInputGenWarnings();

    if (!success && !message.empty()) {
      //  One line, not the generator's whole session transcript -- the
      //  detail (and the reproduction command) is for the Verify
      //  dialog, which has room for it.
      p_feedback->setMessage("Input file not generated: " +
              firstSentence(p_inputGenError) + "  See Verify for details.",
              WxFeedback::ERROR);
    }

    //  A freshly-generated deck is not a hand edit, whatever the file
    //  it just replaced was.
    if (success)
      p_handEdited = false;

    //  A deck has just been written, or the attempt to write one has
    //  just failed -- either way something changed that the lamp
    //  beside the Verify button needs to reflect (#148).  Here rather
    //  than in enableLaunch(), which runs on every edit: this is the
    //  one moment the file can have changed, so it is also the only
    //  moment worth spending a WebDAV fetch and a process on.
    verifyInput();
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
    //  A basis chosen before the code was, or by an older client, can
    //  carry Cartesian for a code that has no Cartesian functions.
    GBSRules::enforceCodeCoordSys(p_basis, p_code);
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

    // save GUIValues; the input is generated from what the box shows
    storeUseSymmetry(useSymmetryBox());
    p_iCalc->guiparams(p_GUIValues);

    // determine whether to save intermediate files for debugging
    // save if shift key is held down
    bool save_param = ::wxGetKeyState(WXK_SHIFT) || p_testKeepParams;

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

    //  On a failed generation, generateInput() has already put the one
    //  line that matters on screen -- the generator's own reason.
    //  Following it straight up with "Calculation saved" would bury
    //  that line and imply the deck is fine when it is not.
    if (!p_inputGenFailed) {
      if (p_inputGenWarnings.empty())
        p_feedback->setMessage("Calculation saved as " + p_iCalc->getName() +
                               ".", WxFeedback::INFO);
      else
        p_feedback->setMessage("Calculation saved as " + p_iCalc->getName() +
                               ". Input file generated with warnings. "
                               "See Verify.", WxFeedback::WARNING);
    }
    enableLaunch();

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
  // getTool() is NULL when that <Tool> is not registered -- which happens
  // as soon as a code or tool is disconnected from the resource files, and
  // used to be an outright crash (see ewxWindowUtils::setToolIcon for the
  // case that bit metadyn). Nothing useful can be launched without a name,
  // so do nothing rather than die.
  ResourceTool *tool =
      ResourceDescriptor::getResourceDescriptor().getTool(id);
  if (tool == (ResourceTool*)0) return;

  startApp(tool->getName(), force, url);
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

    string theory = getTheoryName().ToStdString();
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


/**
 * Launches cmd (a full "python3 <script> <args>..." command line, no
 * trailing "&") as a fully detached process via a double-fork, so that
 * calced is never left with a child of the dialog process to reap.
 *
 * The wxPython Theory/Runtype "details" dialogs launched from here
 * report their results back to calced over a localhost UDP socket
 * (see startTheoryApp()/startRuntypeApp() and OnTheoryIPC()/
 * OnRuntypeIPC()) rather than through the subprocess's exit status or
 * stdout, so calced has no need to keep the dialog process itself as a
 * child at all -- this only needs to guarantee no zombie is left
 * behind (issue #79).
 *
 * A double fork: the intermediate child forks the real command and
 * exits at once, so the dialog process is reparented to init and calced
 * never has a long-lived child to reap.
 */
bool CalcEd::launchDetachedApp(const string& cmd)
{
#ifdef _WIN32
  // No fork or sh here: run the dialog with the package's pythonw (no
  // console window).  The arguments are already double-quoted where needed.
  string line = cmd;
  if (line.compare(0, 8, "python3 ") == 0) {
    string py = string(Ecce::ecceHome()) + "/python/pythonw.exe";
    if (access(py.c_str(), 0) != 0) py = "pythonw.exe";
    line = "\"" + py + "\" " + line.substr(8);
  }
  STARTUPINFOA si;
  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi;
  if (!CreateProcessA(NULL, &line[0], NULL, NULL, FALSE, 0, NULL, NULL,
                      &si, &pi))
    return false;
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return true;
#endif
  pid_t pid = fork();

  if (pid < 0) {
    return false;
  }

  if (pid == 0) {
    // Intermediate child: fork the real command and exit immediately,
    // so the grandchild running it is orphaned straight to init rather
    // than staying a child of calced.
    pid_t pid2 = fork();
    if (pid2 == 0) {
      execl("/bin/sh", "sh", "-c", cmd.c_str(), (char*)NULL);
      _exit(127);  // exec failed
    }
    _exit((pid2 < 0) ? 1 : 0);
  }

  // Parent (calced): reap the short-lived intermediate child.  It only
  // forks the grandchild and exits, so this normally returns at once.
  //
  // This MUST NOT block indefinitely.  It runs on the GUI thread, so any
  // failure of the intermediate child to exit would freeze the whole
  // application with no way out -- and calced is a multithreaded JMS
  // client, where fork() clones only the calling thread: if another
  // thread happened to hold the malloc (or any other) lock at that
  // instant, the child can deadlock before reaching _exit(), and a
  // blocking wait here would then hang forever.  An earlier version of
  // this function did use a blocking waitpid() and is a suspect for
  // exactly that symptom.
  //
  // So poll with WNOHANG for a short bounded period instead.  If the
  // child still hasn't been reaped by then, give up and return: the
  // worst case is one short-lived zombie, which is the very thing this
  // function exists to avoid -- but a leaked zombie is vastly
  // preferable to a frozen GUI.
  const int maxWaitMs = 250;
  const int pollMs = 5;
  for (int waitedMs = 0; waitedMs < maxWaitMs; waitedMs += pollMs) {
    int status = 0;
    pid_t waited = waitpid(pid, &status, WNOHANG);
    if (waited == pid) {
      break;              // reaped
    }
    if ((waited == -1) && (errno != EINTR)) {
      break;              // ECHILD: nothing left to reap
    }
    usleep(pollMs * 1000);
  }

  return true;
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
    // Only the dialog on this machine may send: unbound, anyone on the
    // network could set this editor's values.
    bindAddress.LocalHost();
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
    cmd += "\"" + p_theoryOutFile->path() + "\" ";

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
    if (!launchDetachedApp(cmd)) {
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
    // Only the dialog on this machine may send: unbound, anyone on the
    // network could set this editor's values.
    bindAddress.LocalHost();
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
    cmd += "\"" + p_runtypeOutFile->path() + "\" ";

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
    if (!launchDetachedApp(cmd)) {
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

