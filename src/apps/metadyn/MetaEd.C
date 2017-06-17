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

#include "dsm/EDSIFactory.H"
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

#include "MetaEd.H"
#include "MetaPotentials.H"
#include "QMMMPotentials.H"

//#define DEBUG

IMPLEMENT_CLASS( MetaEd, MetaEdGUI )

BEGIN_EVENT_TABLE(MetaEd, MetaEdGUI)
  EVT_SOCKET(wxID_THEORY_CHANGE, MetaEd::OnTheoryIPC)
  EVT_SOCKET(wxID_RUNTYPE_CHANGE, MetaEd::OnRuntypeIPC)
END_EVENT_TABLE()

MetaEd::MetaEd( )
  : MetaEdGUI(),
    WxDavAuth(),
    JMSPublisher(METADYNAMICS),
    FeedbackSaveHandler(),
    CalcDropHandler(),
    EditListener(),
    p_feedback(NULL),
    p_builderTool(NULL),
    p_code(NULL),
    p_GUIValues(NULL),
    p_iCalc(NULL),
    p_frag(NULL),
    p_fullFrag(NULL),
    p_lastTheory(),
    p_lastRuntype(),
    p_metaPotentials(NULL),
    p_qmmmPotentials(NULL),
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


MetaEd::MetaEd( wxWindow* parent, wxWindowID id, const wxString& caption,
                const wxPoint& pos, const wxSize& size, long style)
  : MetaEdGUI(),
    WxDavAuth(),
    JMSPublisher(METADYNAMICS),
    FeedbackSaveHandler(),
    CalcDropHandler(),
    EditListener(),
    p_feedback(NULL),
    p_builderTool(NULL),
    p_code(NULL),
    p_GUIValues(NULL),
    p_iCalc(NULL),
    p_frag(NULL),
    p_fullFrag(NULL),
    p_lastTheory(),
    p_lastRuntype(),
    p_metaPotentials(NULL),
    p_qmmmPotentials(NULL),
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


MetaEd::~MetaEd( )
{
  closeTheoryApp(true);
  closeRuntypeApp(true);
  ewxConfig::closeConfigs();
}


bool MetaEd::Create( wxWindow* parent, wxWindowID id, const wxString& caption,
                     const wxPoint& pos, const wxSize& size, long style)
{
  if (!MetaEdGUI::Create(parent, id, caption, pos, size, style)) {
    wxFAIL_MSG( wxT("MetaEd creation failed") );
    return false;
  }

  CreateControls();
  //GetSizer()->SetSizeHints(this);
  //GetSizer()->SetMinSize(SYMBOL_METAEDGUI_SIZE);
  Centre();

  // set desktop icon
  SetIcon(wxIcon(ewxBitmap::pixmapFile(
          ResourceDescriptor::getResourceDescriptor().getTool(METADYNAMICS)
          ->getIcon()), wxBITMAP_TYPE_XPM));
  EDSIFactory::addAuthEventListener(this);

  return true;
}

void MetaEd::setContext(const string& url)
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
  } else if (p_iCalc && url==p_context) {

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
    doSetContext();
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


void MetaEd::doSetContext()
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

  // initialize application code and its visible presence on the frame
  p_code = CodeFactory::lookup("Metadynamics");

  p_frag = p_iCalc->fragment();
  if (p_frag) {
    p_fullFrag = new Fragment(*p_frag);
    if (p_frag->useSymmetry()) {
      p_fullFrag->generateFullMolecule();
    }

    // initialize spin multiplicities list and selection
    populateSpinMultiplicities();
    setSpinMult(p_iCalc->spinMultiplicity(), false);
  }

  // initialize GUIValues
  // depends on fragment, code, and runstate

  if (isReady() && p_iCalc->getDataFileNames(JCode::INPUT).size()==0) {
    p_GUIValues = p_iCalc->guiparams();
    enableSave();
  } else if (isReady() && p_iCalc->isFragmentNew()) {
    p_feedback->setMessage("Chemical system has changed since the "
            "Electronic Structure Editor was last up.  Resetting theory and "
            "runtype details dialog values to defaults.", WxFeedback::WARNING);
    p_GUIValues = new GUIValues();
    enableSave();
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

  ChemistryTask *inputTask = dynamic_cast<ChemistryTask*>(p_iCalc->getInputProvider(p_iCalc->getSession()));
  if (inputTask)
    if (inputTask->getApplicationType() == ResourceDescriptor::AT_METADYNAMICS){
      // Chained metadynamics tasks so clear out GUIValues entries for steepest
      // descent
      p_GUIValues->deletePrefix("ES.Runtype.SD");
    }

  updateAllFields();

  if (isDetailsReady() && p_GUIValues->size() == 0) {
    startTheoryApp(true);
    startRuntypeApp(true);
  }

  // GDB 2/19/13  Fix for dialog states getting cleared between app invocations
  // Always restore the underlying model regardless of whether dialog is invoked
  updatePotentialModel();
  p_metaPotentials = new MetaPotentials(this, this, wxID_ANY, "Metadynamics Potential Terms");

  updateQMMMModel();
  p_qmmmPotentials = new QMMMPotentials(this, this, wxID_ANY, "QM/MM Potential Terms");
}


void MetaEd::freeContext()
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

  if (p_metaPotentials) {
    delete p_metaPotentials;
    p_metaPotentials = 0;
  } 

  if (p_qmmmPotentials) {
    delete p_qmmmPotentials;
    p_qmmmPotentials = 0;
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

  updateAllFields();
}


string MetaEd::getContext() const
{
  return p_context;
}


void MetaEd::setEditStatus(WxFeedback::EditStatus status)
{
  p_feedback->setEditStatus(status);
}


Fragment *MetaEd::getFrag() const
{
  return p_frag;
}


WxFeedback *MetaEd::getFeedback()
{
  return p_feedback;
}


void MetaEd::processSave()
{
  doSave();
}


void MetaEd::processDrop(const string& url)
{
  setContext(url);
}


void MetaEd::processEditCompletion(const EditEvent& ee)
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
 * The user clicked on one of the EcceTools (Builder, Launcher).
 */
void MetaEd::OnToolClick(wxCommandEvent &event)
{
  doSave();
  startApp(event.GetId(), 0, p_context);
}


void MetaEd::OnCloseWindow( wxCloseEvent& event )
{
  saveSettings();
  doClose(event.CanVeto());
}


void MetaEd::OnSaveClick( wxCommandEvent& event )
{
  doSave();
  event.Skip();
}


void MetaEd::OnExitClick( wxCommandEvent& event )
{
  saveSettings();
  doClose();
}


void MetaEd::OnHelpClick( wxCommandEvent& event )
{
  BrowserHelp help;
  help.showPage(help.URL("CalculationEditor"));
}


void MetaEd::OnMenuFeedbackClick( wxCommandEvent& event )
{
  BrowserHelp help;
  help.showFeedbackPage();
}


void MetaEd::OnComboboxMetaedChargeSelected( wxCommandEvent& event )
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


void MetaEd::OnComboboxMetaedChargeEnter( wxCommandEvent& event )
{
  OnComboboxMetaedChargeSelected(event);
}


void MetaEd::OnComboboxMetaedSpinMultSelected( wxCommandEvent& event )
{
  setSpinMult(SpinMult::toSpinMult(event.GetString().c_str()), true);
  setOpenShells(getSpinMult() - 1);
  enableSave();

  event.Skip();
}


void MetaEd::OnComboboxMetaedSpinMultEnter( wxCommandEvent& event )
{
  string valstr = event.GetString().c_str();
  if (valstr != "") {
    setSpinMult(SpinMult::toSpinMult(valstr), true);
    setOpenShells(getSpinMult() - 1);
    enableSave();
  }

  event.Skip();
}


void MetaEd::OnButtonMetaedTheoryClick( wxCommandEvent& event )
{
  startTheoryApp(false);

  event.Skip();
}


void MetaEd::OnButtonMetaedRuntypeClick( wxCommandEvent& event )
{
  startRuntypeApp(false);

  event.Skip();
}


void MetaEd::OnButtonMetaedQmmmClick( wxCommandEvent& event )
{
  if (p_qmmmPotentials == 0) {
      updateQMMMModel();
      p_qmmmPotentials = new QMMMPotentials(this, this, wxID_ANY, "QM/MM Potential Terms");
  }
  p_qmmmPotentials->Show();

  event.Skip();
}


void MetaEd::OnButtonMetaedPotentialClick( wxCommandEvent& event )
{
  if (p_metaPotentials == 0) {
    updatePotentialModel();
    p_metaPotentials = new MetaPotentials(this, this, wxID_ANY,
        "Metadynamics Potentials");
  }

  enableMetaPotentials();
  p_metaPotentials->Show(true);
  p_metaPotentials->Raise();

  event.Skip();
}


/**
 * Persistently delete the qmmm model and clean up memory.
 */
void MetaEd::clearQMMMModel()
{
   ChemistryTask *task = dynamic_cast<ChemistryTask*>(p_iCalc);
   if (task) {
      task->setQMMMModel(0);
      if (p_frag) {
        QMMMModel *model = p_frag->getQMMMModel();
        delete model;
        p_frag->setQMMMModel(0);
      }
   }
}


/**
 * Persistently delete the potential model and clean up memory
 */
void MetaEd::clearPotentialModel()
{
   ChemistryTask *task = dynamic_cast<ChemistryTask*>(p_iCalc);
   if (task) {
      task->setMetaPotentialModel(0);
      if (p_frag) {
         MetaPotentialModel *model = p_frag->getPotentials();
         delete model;
         p_frag->setPotentials(0);
      }
      task->setQMMMModel(0);
      if (p_frag) {
         QMMMModel *model = p_frag->getQMMMModel();
         delete model;
         p_frag->setQMMMModel(0);
      }
   }
}


void MetaEd::updatePotentialModel()
{
  if (p_frag) {
    MetaPotentialModel *metaModel = new MetaPotentialModel(*p_frag);
    QMMMModel *qmmmModel = new QMMMModel(*p_frag);
    p_frag->setPotentials(metaModel);
    if (p_iCalc) {
      ChemistryTask *calc = dynamic_cast<ChemistryTask*>(p_iCalc);
      if (calc) {
        calc->getMetaPotentialModel(*metaModel);
        calc->getQMMMModel(*qmmmModel);
      }
    }
  }
}


void MetaEd::updateQMMMModel()
{
  if (p_frag) {
    QMMMModel *terms = new QMMMModel(*p_frag);
    p_frag->setQMMMModel(terms);
    if (p_iCalc) {
      ChemistryTask *calc = dynamic_cast<ChemistryTask*>(p_iCalc);
      if (calc) {
        calc->getQMMMModel(*terms);
      }
    }
  }
}


void MetaEd::OnButtonMetaedFinalEditClick( wxCommandEvent& event )
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
                        "Metadynamics Input");
      } else {
        p_feedback->setMessage("Input file not found.", WxFeedback::WARNING);
      }
    } catch (EcceException& ex) {
      p_feedback->setMessage(ex.what(), WxFeedback::ERROR);
    }
  }

  event.Skip();
}


void MetaEd::OnButtonMetaedLaunchClick( wxCommandEvent& event )
{
  ResourceTool *tool =
          ResourceDescriptor::getResourceDescriptor().getTool(LAUNCHER);
  event.SetId(tool->getId());
  OnToolClick(event);
}


void MetaEd::moveMCB(wxCommandEvent& event)
{
  p_feedback->setContextURL(p_iCalc->getURL());
}


void MetaEd::deleteMCB(wxCommandEvent& event)
{
  freeContext();
}


void MetaEd::launchMCB(wxCommandEvent& event)
{
  enableAllFields();
}


void MetaEd::stateMCB(wxCommandEvent& event)
{
  if (p_iCalc->getState() > ResourceDescriptor::STATE_READY) {
    setEditStatus(WxFeedback::READONLY);
  } else {
    setEditStatus(WxFeedback::EDIT);
  }
  enableAllFields();
}


void MetaEd::subjectMCB(wxCommandEvent& event)
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
  if (p_qmmmPotentials) p_qmmmPotentials->initDisplay(false);

  clearPotentialModel();
  if (p_metaPotentials) p_metaPotentials->initDisplay(false);

  p_frag = p_iCalc->fragment();
  if (p_frag) {

    p_fullFrag = new Fragment(*p_frag);
    if (p_frag->useSymmetry()) {
      p_fullFrag->generateFullMolecule();
    }
    updatePotentialModel();
    updateQMMMModel();
  }

  setSpinMult(p_iCalc->spinMultiplicity(), false);
  restrictSpinByCharge();

  updateAllFields();

  if (isDetailsReady() && p_GUIValues->size() == 0) {
    startTheoryApp(true);
    startRuntypeApp(true);
  }

  // auto-save
  enableSave();
  doSave();
}


void MetaEd::propertyMCB(wxCommandEvent& event)
{
  // this could be an annotation change
  if (p_iCalc->getState() <= ResourceDescriptor::STATE_READY) {
    // auto-save
    enableSave();
    doSave();
  }
}


void MetaEd::CreateControls()
{
  p_feedback = new WxFeedback(this);
  p_feedback->setDropHandler(this);
  p_feedback->setSaveHandler(this);
  GetSizer()->Add(p_feedback, 0, wxEXPAND, 0);

  replaceBuilderButton();
}


void MetaEd::replaceBuilderButton()
{
  // Hack to support duality of Builders (wx vs motif)
  // Create an EcceTool now to fill the space.  When setContext is called
  // with a valid calculation, we'll determine which Builder is to be 
  // invoked and reset the ID of the EcceTool.
  ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();
  ResourceTool *tool = rd.getTool(BUILDER);
  p_builderTool = new EcceTool(this, tool);
  Connect( tool->getId(), wxEVT_COMMAND_BUTTON_CLICKED,
           wxCommandEventHandler( MetaEd::OnToolClick ) );

  // find and replace the Builder button placeholder
  wxWindow *placeHolder = FindWindow(ID_BUTTON_METAED_BUILDER);
  placeHolder->GetContainingSizer()->Replace(placeHolder, p_builderTool);
  placeHolder->Destroy();
}


void MetaEd::fixBuilderButtonId()
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
              wxCommandEventHandler( MetaEd::OnToolClick ) );
  p_builderTool->SetId((*tool)->getId());
  Connect( p_builderTool->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
           wxCommandEventHandler( MetaEd::OnToolClick ) );
}


/**
 * Get currently selected theory.
 * If none selected, theory name and category fields will be empty.
 */
TTheory MetaEd::getTheory() const
{
  return TTheory("NWPW", "PSPW");
}


TRunType MetaEd::getRuntype() const
{
  return TRunType("Car-Parrinello", true);
}


SpinMult::SpinMultEnum MetaEd::getSpinMult() const
{
  return p_spinMult;
}


void MetaEd::setOpenShells(const int& value)
{
  string valueString = StringConverter::toString(value);
  ewxStaticText *text =
          (ewxStaticText*)FindWindow(ID_STATICTEXT_METAED_OPEN_SHELLS);
  text->SetLabel(valueString);
  text->Show(value >= 0);
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
bool MetaEd::setSpinMult(const SpinMult::SpinMultEnum& value,
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

  ewxComboBox *combo = (ewxComboBox*) FindWindow(ID_COMBOBOX_METAED_SPIN_MULT);

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
 * Set spin choices based on number of electrons in full fragment.
 * This function clears the current spin selection.
 * Will not perform without a valid full fragment set.
 */
void MetaEd::populateSpinMultiplicities()
{
  ewxComboBox *combo = (ewxComboBox*) FindWindow(ID_COMBOBOX_METAED_SPIN_MULT);
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


void MetaEd::populateSummaryFields()
{
  populateSummaryField("TheorySummary");
  populateSummaryField("RuntypeSummary");
}


void MetaEd::populateSummaryField(const string& summaryType)
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
void MetaEd::resetTheoryDetails()
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
void MetaEd::resetRuntypeDetails()
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
 * Repopulate spin choices and reselect current spin.
 * Spin is populated based on the number of electrons in the current fragment.
 * If our previously selected spin is not found in the new list of choices,
 * we pick a new spin intelligently and inform the user.
 */
void MetaEd::restrictSpinByCharge()
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
  }
}


bool MetaEd::isDetailsReady() const
{
  return p_frag && !p_frag->containsNubs();
}


bool MetaEd::isReady() const
{
  bool ret = p_iCalc->getState() < ResourceDescriptor::STATE_SUBMITTED &&
             isDetailsReady();

  return ret;
}


void MetaEd::updateAllFields()
{
  refreshAllFields();
  enableAllFields();
  showAllFields();
  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}


void MetaEd::updateChemSysFields()
{
  refreshChemSysFields();
  enableChemSysFields();
  showChemSysFields();
  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}


void MetaEd::updateDetailsFields()
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
void MetaEd::refreshAllFields()
{
  refreshChemSysFields();
  refreshDetailsFields();
}


/**
 * Set text values associated with chemical system.
 */
void MetaEd::refreshChemSysFields()
{
  refreshChemSysThumb();

  if (p_frag && p_fullFrag) {
    ((ewxComboBox*)FindWindow(ID_COMBOBOX_METAED_CHARGE))
            ->SetValue(StringConverter::toString(p_fullFrag->charge()));
    ((ewxStaticText*)FindWindow(ID_STATICTEXT_METAED_SYMMETRY))
            ->SetLabel(p_fullFrag->pointGroup());
    ((ewxStaticText*)FindWindow(ID_STATICTEXT_METAED_ATOMS))
            ->SetLabel(StringConverter::toString(p_fullFrag->numAtoms()));
    ((ewxStaticText*)FindWindow(ID_STATICTEXT_METAED_ELECTRONS))
            ->SetLabel(StringConverter::toString(p_fullFrag->numElectrons()));
  } else {
    // clear GUIValues because they would be inconsistent
    if (p_GUIValues) {
      delete p_GUIValues;
      p_GUIValues = new GUIValues();
      updateDetailsFields();
    }

    ((ewxComboBox*)FindWindow(ID_COMBOBOX_METAED_CHARGE))->SetValue("0");
    ((ewxComboBox*)FindWindow(ID_COMBOBOX_METAED_SPIN_MULT))->SetValue("Singlet");
    ((ewxStaticText*)FindWindow(ID_STATICTEXT_METAED_SYMMETRY))->SetLabel("");
    ((ewxStaticText*)FindWindow(ID_STATICTEXT_METAED_ATOMS))->SetLabel("");
    ((ewxStaticText*)FindWindow(ID_STATICTEXT_METAED_ELECTRONS))->SetLabel("");
  }
}


/**
 * Set the bitmap for the chemical system or just the vanilla bitmap.
 */
void MetaEd::refreshChemSysThumb()
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


void MetaEd::refreshDetailsFields()
{
  populateSummaryFields();
}


void MetaEd::enableAllFields()
{
  enableChemSysFields();
  enableDetailsFields();
  enableLaunch();
  enableMetaPotentials();
}


/**
 * Enable/Disable fields based on whether a chemical system has been specified.
 */
void MetaEd::enableChemSysFields()
{
  bool hasFragment(p_frag);
  bool hasCalc(p_iCalc);
  bool isSubmitted(hasCalc
          && p_iCalc->getState() >= ResourceDescriptor::STATE_SUBMITTED);

  p_builderTool->Enable(hasCalc);

  bool enabled = !isSubmitted && hasFragment;
  FindWindow(ID_STATIC_METAED_CHARGE)->Enable(enabled);
  FindWindow(ID_COMBOBOX_METAED_CHARGE)->Enable(enabled);
  FindWindow(ID_STATIC_METAED_SPIN_MULT)->Enable(enabled);
  FindWindow(ID_COMBOBOX_METAED_SPIN_MULT)->Enable(enabled);
  FindWindow(ID_CHECKBOX_METAED_IRREDUCIBLE)->Enable(enabled);
}


void MetaEd::enableDetailsFields()
{
  bool hasFragment(p_frag);

  p_detailsBox->Enable(hasFragment);
  FindWindow(ID_BUTTON_METAED_THEORY)->Enable(isDetailsReady()
          && p_GUIValues->containsKeyPrefix("ES.Theory"));
  FindWindow(ID_BUTTON_METAED_RUNTYPE)->Enable(isDetailsReady()
          && p_GUIValues->containsKeyPrefix("ES.Runtype"));
}


void MetaEd::enableLaunch()
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
    FindWindow(ID_BUTTON_METAED_FINAL_EDIT)->Enable(togo);
    FindWindow(ID_BUTTON_METAED_LAUNCH)->Enable(togo);
  } else {
    FindWindow(ID_BUTTON_METAED_FINAL_EDIT)->Enable(false);
    FindWindow(ID_BUTTON_METAED_LAUNCH)->Enable(false);
  }
}


void MetaEd::enableMetaPotentials()
{
  showPotentialEditor();
}


void MetaEd::enableSave(const bool& enable)
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


void MetaEd::showAllFields()
{
  showChemSysFields();
  showDetailsFields();
}


void MetaEd::showChemSysFields()
{
  bool hasCode(p_code);
  bool isIrreducibleFragmentSupported(hasCode
          && p_code->getIrreducibleFragmentSupported());
  FindWindow(ID_CHECKBOX_METAED_IRREDUCIBLE)->Show(
          isIrreducibleFragmentSupported);
}


/**
 * Determine whether we need to display the details buttons.
 */
void MetaEd::showDetailsFields()
{
  if (p_code) {
    string theoryCommand, runCommand;
    if (!p_code->getTheoryRunTypeEditorNames(theoryCommand, runCommand)) {
      INVALIDEXCEPTION(0,"Theory and Runtype dialog executables not defined.");
    }
    FindWindow(ID_BUTTON_METAED_THEORY)->Show(!theoryCommand.empty());
    p_theoryDetailsSizer->Show(!theoryCommand.empty());
    FindWindow(ID_BUTTON_METAED_RUNTYPE)->Show(!runCommand.empty());
    p_runtypeDetailsSizer->Show(!runCommand.empty());
    showPotentialEditor();
    showQMMMEditor();
  }
}


void MetaEd::showPotentialEditor()
{
  bool hasFragment(p_frag);
  bool hasCalc(p_iCalc);

  FindWindow(ID_BUTTON_METAED_CONSTRAINT)->Show(hasFragment && hasCalc);
  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}


void MetaEd::showQMMMEditor()
{
  bool hasFragment(p_frag);
  bool hasCalc(p_iCalc);

  FindWindow(ID_BUTTON_METAED_QMMM)->Show(hasFragment && hasCalc);
}


void MetaEd::urlChangeNotify(const string& topic) const
{
  JMSMessage *msg = newMessage();
  msg->addProperty("url", p_iCalc->getURL().toString());
  publish(topic, *msg);
  delete msg;
}


bool MetaEd::generateInput(const bool& paramFlag)
{
  bool success = false;

  if (isReady()) {
    string message;
    success = input_controller(paramFlag, false, message);
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


wxWindowID MetaEd::doClose(const bool& allowCancel)
{
  wxWindowID answer = wxID_ANY;

  if (p_feedback->getEditStatus() == WxFeedback::MODIFIED) {
    long buttonFlags = wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION;
    buttonFlags |= allowCancel ? wxCANCEL : 0;
    ewxMessageDialog *dialog = new ewxMessageDialog(this,
            "The current calculation has unsaved changes!  Do you "
            "want to save changes before quitting?",
            "Save Metadynamics Editor Changes?", buttonFlags);
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


void MetaEd::doSave()
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
              || p_iCalc->openShells() != 0) {
        p_iCalc->spinMultiplicity(getSpinMult());
        p_iCalc->openShells(0);
        saveSpin = true;
      }

    }

    // Save the underlying models
    ChemistryTask *task = dynamic_cast<ChemistryTask*>(p_iCalc);
    if (p_frag) {
      task->setMetaPotentialModel(p_frag->getPotentials());
      task->setQMMMModel(p_frag->getQMMMModel());
    }
    else {
      // make sure to remove model if there was one
      task->setMetaPotentialModel(0);
      task->setQMMMModel(0);
    }

    // save theory
    bool saveTheory = false;
    TTheory *currentTheory = p_iCalc->theory();
    if (!currentTheory
            || "NWPW" != currentTheory->category()
            || "PSPW" != currentTheory->name()) {
      TTheory theory = getTheory();
      p_iCalc->theory(&theory);
      saveTheory = true;
    }
    if (currentTheory) delete currentTheory;

    // save runtype
    bool saveRuntype = false;
    TRunType currentRuntype = p_iCalc->runtype();
    if (currentRuntype.name() != "Car-Parrinello") {
      TRunType runtype = getRuntype();
      p_iCalc->runtype(runtype);
      saveRuntype = true;
    }

    // save GUIValues
    p_iCalc->guiparams(p_GUIValues);

    // determine whether to save intermediate files for debugging
    // save intermediate files for debugging if shift key is held down
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
    
    // Details always change because any calced change
    // triggers them to be reset
    urlChangeNotify("ecce_url_details");
  }
}


/**
 * Support method for changing the theory category/name.
 */
void MetaEd::doTheoryChange()
{
  enableSave();
  resetTheoryDetails();
  updateDetailsFields();
  enableLaunch();
}


void MetaEd::doRuntypeChange()
{
  enableSave();
  resetRuntypeDetails();
  updateDetailsFields();
  enableLaunch();
}


void MetaEd::saveSettings()
{
  ewxConfig::closeConfigs();
}


void MetaEd::restoreSettings()
{
  ewxConfig::closeConfigs();
}


void MetaEd::startApp(int id, int force, const string& url) const
{
  startApp(ResourceDescriptor::getResourceDescriptor().getTool(id)->getName(),
           force, url);
}


void MetaEd::startApp(const string& app, int force, const string& url) const
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


string MetaEd::buildTheoryRuntypeArgs(const bool& isTheory) const
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
  args += "NWPW\"";
  args += " \"PSPW\"";
  args += " \"Car-Parrinello\"";
  args += " \"" + p_iCalc->getName() + "\"";

  Resource *parent = EDSIFactory::getResource(p_iCalc->getURL().getParent());
  if (parent->getApplicationType() == ResourceDescriptor::AT_CONDENSED_REACTION_STUDY) {
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
  sprintf(buf, " %d %d %d %d %d %d", electrons, spin, frozenOrbs,
          occupiedOrbs, virtualOrbs, normalModes);
  args += buf;

  ChemistryTask *inputTask = dynamic_cast<ChemistryTask*>(p_iCalc->getInputProvider(p_iCalc->getSession()));
  if (inputTask)
    if (inputTask->getApplicationType() == ResourceDescriptor::AT_METADYNAMICS){
      args += " 1";
    } else {
      args += " 0";
    }

  return args;
}


void MetaEd::getOrbitalParams(int &electrons, int &spin, int &frozenOrbs, 
                           int &occupiedOrbs, int &virtualOrbs,
                           int &normalModes) const
{
  // Note: Our definition for total number of orbitals is:
  // #TotalOrbs = #Occupied + #Unoccupied + #OpenShells
  // where #Unoccupied is equivalent to #Virtual

  electrons = spin = occupiedOrbs = normalModes = 0;
  frozenOrbs = virtualOrbs = -1;

  if (p_fullFrag) {
    electrons = p_fullFrag->nuclearCharge() - p_fullFrag->charge();

    spin = getSpinMult();

    normalModes = 3 * p_frag->numAtoms();

    occupiedOrbs = electrons/2;
  }
}


void MetaEd::startTheoryApp(const bool& localInitFlag)
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


void MetaEd::startRuntypeApp(const bool& localInitFlag)
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


void MetaEd::OnTheoryIPC(wxSocketEvent& event)
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


void MetaEd::OnRuntypeIPC(wxSocketEvent& event)
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


void MetaEd::processTheoryInput(const char* databuf)
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

    FindWindow(ID_BUTTON_METAED_THEORY)->Enable(isDetailsReady() &&
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


void MetaEd::processRuntypeInput(const char* databuf)
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

    FindWindow(ID_BUTTON_METAED_RUNTYPE)->Enable(isDetailsReady() &&
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


void MetaEd::closeTheoryApp(const bool& sendTerm)
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


void MetaEd::closeRuntypeApp(const bool& sendTerm)
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

