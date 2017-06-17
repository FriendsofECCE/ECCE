#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "NWDirdy.H"
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
  using std::flush;
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
#include <signal.h>
//#include <stdlib.h>  // abort

#include "util/Ecce.H"
#include "util/ErrMsg.H"
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
#include "util/StringTokenizer.H"
#include "util/VizThumbnail.H"
#include "util/EcceSortedVector.H"
#include "util/STLUtil.H"
#include "util/Color.H"
#include "util/EditEvent.H"

#include "tdat/Fragment.H"
#include "tdat/TAtm.H"
#include "tdat/RefMachine.H"
#include "tdat/FFCoords.H"
#include "tdat/CGAlignmentFunction.H"
#include "tdat/CGMinimizer.H"
#include "tdat/PropVector.H"

#include "dsm/MachineConfig.H"
#include "dsm/GBSRules.H"
#include "dsm/TGBSConfig.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/EDSIGaussianBasisSetLibrary.H"
#include "dsm/CodeFactory.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceType.H"
#include "dsm/Session.H"
#include "dsm/DirDyVTSTTask.H"
#include "dsm/VDoc.H"

#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxBitmapButton.H"
#include "wxgui/ewxComboBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxNonBoldLabel.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxProgressDialog.H"
#include "wxgui/ewxRegexpValidator.H"
#include "wxgui/ewxSingleChoiceDialog.H"
#include "wxgui/ewxUnitHelper.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/WxEditSessionMgr.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/WxJMSListener.H"
#include "wxgui/WxJMSMessageDispatch.H"
#include "wxgui/WxState.H"

#include "NWDirdy.H"
#include "NWDirdySetup.H"


/** Used to subscribe to messaging, publish, identify */
static const string WHOAMI = NWDIRDY;

/**
 * Constructor.
 * Create widgets.  Some widgets like the feedback area are 
 * created in code.  Initialize GUI state and set up validators and
 * unit helpers.
 */
NWDirdy::NWDirdy(wxWindow* parent, wxWindowID id, const wxString& caption,
                 const wxPoint& pos, const wxSize& size, long style)
  : NWDirdyGUI(parent, id, caption, pos, size, style),
    WxDavAuth(),
    WxJMSListener(),
    EditListener(),
    TaskApp(WHOAMI, 1000),
    CalcDropHandler(),
    FeedbackSaveHandler()
{
   SetIcon(wxIcon(ewxBitmap::pixmapFile(
           ResourceDescriptor::getResourceDescriptor().getTool(NWDIRDY)
           ->getIcon()), wxBITMAP_TYPE_XPM));

   EDSIFactory::addAuthEventListener(this);

   initializeMessaging();

   initializeModel();

   p_dirdyTask = 0;
   p_session = 0;
   p_progressDialog = 0;
   p_ignoreCodeEventsFlag = false;
   p_touch = false;

   p_generalGUIValues = (GUIValues*)0;
   p_generalTheoryPid = 0;
   p_generalTheoryInSocket = (wxDatagramSocket*)0;
   p_generalTheoryInPort = 0;
   p_generalTheoryOutStream = (ofstream*)0;
   p_generalTheoryOutFile = (SFile*)0;
   p_generalTheoryInFlag = false;
   p_singleGUIValues = (GUIValues*)0;
   p_singleTheoryPid = 0;
   p_singleTheoryInSocket = (wxDatagramSocket*)0;
   p_singleTheoryInPort = 0;
   p_singleTheoryOutStream = (ofstream*)0;
   p_singleTheoryOutFile = (SFile*)0;
   p_singleTheoryInFlag = false;
   p_debugFlag = false;

   setState( NWDirdy::NO_SYSTEM );

   p_feedback = ((WxFeedback*)FindWindow(ID_FEEDBACK));
   p_feedback->setSaveHandler(this);
   p_feedback->setDropHandler(this);
   p_feedback->setEditStatus(WxFeedback::EDIT);

   createDynamicGUI();
}


NWDirdy::~NWDirdy()
{
  delete p_msgDispatcher;
  if (p_progressDialog != 0) {
    delete p_progressDialog;
  }

  // not sure why, but this process doesn't exit without an explicit
  // exit call while other ECCE apps don't need this.  Process actually
  // goes defunct when this is called, but that's what all ECCE apps do.
  exit(0);
}


/**
 * Sets up validators and unit helpers for input fields.
 */
void NWDirdy::checkStatus()
{
   if (checkComponent(DirDyVTSTTask::TRANSITION) &&
       checkComponent(DirDyVTSTTask::REACT1) &&
       checkComponent(DirDyVTSTTask::PROD1)) {
     setState( NWDirdy::VALID_SYSTEM );
     if (p_dirdyTask->getState()==ResourceDescriptor::STATE_CREATED &&
         !p_dirdyTask->getDataFileNames(JCode::PRIMARY_INPUT).empty()) {
       setRunState(ResourceDescriptor::STATE_READY);
     }
   }
}


/**
 * Sets up validators and unit helpers for input fields.
 */
void NWDirdy::initializeGUI()
{
   refreshGUI();
   checkStatus();
}


NWDirdyModel& NWDirdy::getNWDirdyModel() const
{
   return (NWDirdyModel&)*p_model;
}


/**
 * Process drop into feedback drop site.
 */
void NWDirdy::processDrop(const string& url)
{
   setContext(url);
}


void NWDirdy::setState(NWDirdyState state)
{

   // Visually reflect the state
   ewxButton *remoteBtn = ((ewxButton*)FindWindow(ID_BUTTON_NWDIRDY_LAUNCH));
   ewxButton *editBtn = ((ewxButton*)FindWindow(ID_BUTTON_NWDIRDY_FINAL_EDIT));
   ewxButton *setupBtn = ((ewxButton*)FindWindow(ID_BUTTON_NWDIRDY_SETUP));

   if (state == NO_SYSTEM) {
      // disable everything except builder access
      remoteBtn->Disable();
      editBtn->Disable();
      setupBtn->Disable();

   } else if (state == VALID_SYSTEM)  {
      remoteBtn->Enable(true);
      editBtn->Enable(true);
      setupBtn->Enable(true);

   } else if (state == PROVIDED_SYSTEM)  {
      remoteBtn->Enable(true);
      editBtn->Enable(true);
      setupBtn->Enable(true);

   } else if (state == VALID_TOPOLOGY)  {
      remoteBtn->Enable(true);
      editBtn->Enable(true);
      setupBtn->Enable(true);

   } else {
      cerr << "Unknown NWDirdy state " << state << endl;
   }

   p_state = state;
}



NWDirdy::NWDirdyState NWDirdy::getState() const
{
   return p_state;
}


void NWDirdy::setIgnoreFlag(bool ignore)
{
  p_ignoreCodeEventsFlag = ignore;
}


bool NWDirdy::getIgnoreFlag()
{
  return p_ignoreCodeEventsFlag;
}


bool NWDirdy::Show(bool show)
{
#if 000
   p_ignoreCodeEventsFlag = true;
   wxCommandEvent event;
   p_ignoreCodeEventsFlag = false;
#endif

   if (!show) {
      // save current settings
      Preferences prefs(WHOAMI);
      saveWindowSettings(WHOAMI, prefs, false);
      prefs.saveFile();
   } else if (show) {
      // restore settings - could optimize this to only do it the first time
      // but this is ok for now
      Preferences prefs(WHOAMI);
      restoreWindowSettings(WHOAMI, prefs, false);
   }

   return NWDirdyGUI::Show(show);
}


/**
 * Restore all GUI fields from the data model object.
 */
void NWDirdy::refreshGUI()
{
   // Change to default units since the model is always in these units
   ewxWindowUtils::setUnitFamily(this, DEFAULT_UNIT_FAMILY);

   // Now change to user prefered units
   ewxWindowUtils::processGlobalPreferenceChange(this);

   // enable/disable builder buttons and set thumbnails if available
   ewxBitmapButton *bitmap = 0;
   ewxNonBoldLabel *formulaLabel = 0;
   ewxStaticText *formulaText = 0;
   ewxComboBox *chargeCombo = 0;
   ewxStaticText *chargeLabel = 0;
   ewxChoice *spinChoice = 0;
   ewxStaticText *spinLabel = 0;
   ewxStaticText *codeCurrentLabel = 0;
   ewxStaticText *codeNoneLabel = 0;
   wxBoxSizer *codeSizer = 0;
   wxBoxSizer *calcedSizer = 0;
   DavCalculation *calc = 0;
   SFile *thumbFile = 0;
   ewxBitmap builder("gwbuilder2.xpm");
   int bitmapIds[] = { ID_BITMAPBUTTON_DIRDY_BUILD_TS,
                       ID_BITMAPBUTTON_DIRDY_BUILD_R1,
                       ID_BITMAPBUTTON_DIRDY_BUILD_R2,
                       ID_BITMAPBUTTON_DIRDY_BUILD_P1,
                       ID_BITMAPBUTTON_DIRDY_BUILD_P2 };
   int formulaTextIds[] = { ID_STATIC_DIRDY_FORMULA_TS,
                            ID_STATIC_DIRDY_FORMULA_R1,
                            ID_STATIC_DIRDY_FORMULA_R2,
                            ID_STATIC_DIRDY_FORMULA_P1,
                            ID_STATIC_DIRDY_FORMULA_P2 };
   int formulaLabelIds[] = { ID_STATIC_DIRDY_FORMULA_LABEL_TS,
                             ID_STATIC_DIRDY_FORMULA_LABEL_R1,
                             ID_STATIC_DIRDY_FORMULA_LABEL_R2,
                             ID_STATIC_DIRDY_FORMULA_LABEL_P1,
                             ID_STATIC_DIRDY_FORMULA_LABEL_P2 };
   int chargeComboIds[] = { ID_COMBOBOX_DIRDY_CHARGE_TS,
                            ID_COMBOBOX_DIRDY_CHARGE_R1,
                            ID_COMBOBOX_DIRDY_CHARGE_R2,
                            ID_COMBOBOX_DIRDY_CHARGE_P1,
                            ID_COMBOBOX_DIRDY_CHARGE_P2 };
   int chargeLabelIds[] = { ID_STATIC_DIRDY_CHARGE_TS,
                            ID_STATIC_DIRDY_CHARGE_R1,
                            ID_STATIC_DIRDY_CHARGE_R2,
                            ID_STATIC_DIRDY_CHARGE_P1,
                            ID_STATIC_DIRDY_CHARGE_P2 };
   int spinChoiceIds[] = { ID_CHOICE_DIRDY_SPIN_TS,
                           ID_CHOICE_DIRDY_SPIN_R1,
                           ID_CHOICE_DIRDY_SPIN_R2,
                           ID_CHOICE_DIRDY_SPIN_P1,
                           ID_CHOICE_DIRDY_SPIN_P2 };
   int spinLabelIds[] = { ID_STATIC_DIRDY_SPIN_TS,
                          ID_STATIC_DIRDY_SPIN_R1,
                          ID_STATIC_DIRDY_SPIN_R2,
                          ID_STATIC_DIRDY_SPIN_P1,
                          ID_STATIC_DIRDY_SPIN_P2 };
   int codeCurrentLabelIds[] = { ID_STATIC_DIRDY_CODE_CURRENT_TS,
                                 ID_STATIC_DIRDY_CODE_CURRENT_R1,
                                 ID_STATIC_DIRDY_CODE_CURRENT_R2,
                                 ID_STATIC_DIRDY_CODE_CURRENT_P1,
                                 ID_STATIC_DIRDY_CODE_CURRENT_P2 };
   int codeNoneLabelIds[] = { ID_STATIC_DIRDY_CODE_NONE_TS,
                              ID_STATIC_DIRDY_CODE_NONE_R1,
                              ID_STATIC_DIRDY_CODE_NONE_R2,
                              ID_STATIC_DIRDY_CODE_NONE_P1,
                              ID_STATIC_DIRDY_CODE_NONE_P2 };
   wxBoxSizer *codeSizers[] = { p_codeSizerTS,
                                p_codeSizerR1,
                                p_codeSizerR2,
                                p_codeSizerP1,
                                p_codeSizerP2 };
   wxBoxSizer *calcedSizers[] = { p_calcedSizerTS,
                                  p_calcedSizerR1,
                                  p_calcedSizerR2,
                                  p_calcedSizerP1,
                                  p_calcedSizerP2 };
   DirDyVTSTTask::Component component[] = { DirDyVTSTTask::TRANSITION,
                                            DirDyVTSTTask::REACT1,
                                            DirDyVTSTTask::REACT2,
                                            DirDyVTSTTask::PROD1,
                                            DirDyVTSTTask::PROD2 };
   for (int i = 0; i < 5; i++) {
      bitmap = ((ewxBitmapButton*)FindWindow(bitmapIds[i]));
      formulaText = ((ewxStaticText*)FindWindow(formulaTextIds[i]));
      formulaLabel = ((ewxNonBoldLabel*)FindWindow(formulaLabelIds[i]));
      chargeCombo = ((ewxComboBox*)FindWindow(chargeComboIds[i]));
      chargeLabel = ((ewxStaticText*)FindWindow(chargeLabelIds[i]));
      spinChoice = ((ewxChoice*)FindWindow(spinChoiceIds[i]));
      spinLabel = ((ewxStaticText*)FindWindow(spinLabelIds[i]));
      codeCurrentLabel = ((ewxStaticText*)FindWindow(codeCurrentLabelIds[i]));
      codeNoneLabel = ((ewxStaticText*)FindWindow(codeNoneLabelIds[i]));
      codeSizer = (wxBoxSizer*)codeSizers[i];
      calcedSizer = (wxBoxSizer*)calcedSizers[i];
      if (p_dirdyTask && (calc = p_dirdyTask->getComponentTask(component[i]))) {
        bitmap->Enable();
        thumbFile = TempStorage::getTempFile();
        if (calc->getThumbnail(thumbFile)) {
          wxBitmap thumb(thumbFile->path(), wxBITMAP_TYPE_JPEG);
          if (thumb.Ok()) { // bitmap data found
            bitmap->SetBitmapLabel(thumb);
          }
        } else {
          bitmap->SetBitmapLabel(builder);
        }
        if (thumbFile) {
          if (thumbFile->exists())
            thumbFile->remove();
          delete thumbFile;
        }

        // restore the charge for the component chemical system
        Fragment *frag = calc->fragment();
        if (frag != (Fragment*)0) {
          formulaLabel->Enable();
          formulaText->Enable();
          formulaText->SetLabel(frag->generateEmpiricalFormula());
          chargeLabel->Enable();
          chargeCombo->Enable();
          chargeCombo->SetValue(StringConverter::toString(frag->charge()));
          spinLabel->Enable();
          spinChoice->Enable();
          codeCurrentLabel->Enable();
          // show or hide the None label
          codeNoneLabel->Show(calc->theory()==(TTheory*)0);
          codeNoneLabel->Enable(calc->theory()==(TTheory*)0);
          reparentCodes(calc->theory()!=(TTheory*)0,
                        calc->application()->name(), codeSizer, calcedSizer);
          // calc editor access
          enableChildren(calcedSizer, true);
          (void)restrictSpin(frag->nuclearCharge()-frag->charge(), spinChoice);
          spinChoice->SetStringSelection(
                         SpinMult::toString(calc->spinMultiplicity()));
          // enable create reactants/products buttons
          if (i == 0) {
            ((ewxButton*)FindWindow(ID_BUTTON_DIRDY_CREATE_REACT))->Enable();
            ((ewxButton*)FindWindow(ID_BUTTON_DIRDY_CREATE_PROD))->Enable();

            // barrier summary field
            ewxNonBoldLabel *barrierLabel = ((ewxNonBoldLabel*)
                                  FindWindow(ID_STATIC_DIRDY_BARRIER_LABEL_TS));
            ewxStaticText *barrierText = ((ewxStaticText*)
                                  FindWindow(ID_STATIC_DIRDY_BARRIER_TS));
            barrierLabel->Enable();
            barrierText->Enable();
            setBarrierSummaryField();
          }
        } else {
          formulaLabel->Disable();
          formulaText->Disable();
          formulaText->SetLabel("");
          chargeLabel->Disable();
          chargeCombo->Disable();
          chargeCombo->SetValue("");
          spinLabel->Disable();
          spinChoice->Disable();
          codeCurrentLabel->Disable();
          codeNoneLabel->Show();
          codeNoneLabel->Disable();
          reparentCodes(false, "", codeSizer, calcedSizer);
          // calc editor access
          enableChildren(calcedSizer, false);
          (void)restrictSpin(0, spinChoice);
          spinChoice->SetSelection(0);
          // disable create reactants/products buttons
          if (i == 0) {
            ((ewxButton*)FindWindow(ID_BUTTON_DIRDY_CREATE_REACT))->Disable();
            ((ewxButton*)FindWindow(ID_BUTTON_DIRDY_CREATE_PROD))->Disable();

            // barrier summary field
            ewxNonBoldLabel *barrierLabel = ((ewxNonBoldLabel*)
                                  FindWindow(ID_STATIC_DIRDY_BARRIER_LABEL_TS));
            ewxStaticText *barrierText = ((ewxStaticText*)
                                  FindWindow(ID_STATIC_DIRDY_BARRIER_TS));
            barrierLabel->Disable();
            barrierText->Disable();
            barrierText->SetLabel("Unknown");
            barrierText->SetForegroundColour(wxColour(0, 0, 0));
          }
        }
      } else {
        if (i == 0) {
          ((ewxButton*)FindWindow(ID_BUTTON_DIRDY_CREATE_REACT))->Disable();
          ((ewxButton*)FindWindow(ID_BUTTON_DIRDY_CREATE_PROD))->Disable();

          // barrier summary field
          ewxNonBoldLabel *barrierLabel = ((ewxNonBoldLabel*)
                                FindWindow(ID_STATIC_DIRDY_BARRIER_LABEL_TS));
          ewxStaticText *barrierText = ((ewxStaticText*)
                                FindWindow(ID_STATIC_DIRDY_BARRIER_TS));
          barrierLabel->Disable();
          barrierText->Disable();
          barrierText->SetLabel("Unknown");
          barrierText->SetForegroundColour(wxColour(0, 0, 0));
        } else {
          bitmap->Disable();
        }

        bitmap->SetBitmapLabel(builder);

        formulaLabel->Disable();
        formulaText->Disable();
        formulaText->SetLabel("");
        chargeLabel->Disable();
        chargeCombo->Disable();
        chargeCombo->SetValue("");
        spinLabel->Disable();
        spinChoice->Disable();
        spinChoice->SetSelection(0);

        codeCurrentLabel->Disable();
        codeNoneLabel->Show();
        codeNoneLabel->Disable();
        reparentCodes(false, "", codeSizer, calcedSizer);
        // calc editor access
        enableChildren(calcedSizer, false);
      }
   }

   GetSizer()->Layout();
   GetSizer()->SetSizeHints(this);
}


void NWDirdy::setBarrierSummaryField()
{
  ewxStaticText *barrierText = ((ewxStaticText*)
                               FindWindow(ID_STATIC_DIRDY_BARRIER_TS));

  DavCalculation *tscalc = p_dirdyTask->getComponentTask(
                                           DirDyVTSTTask::TRANSITION);
  if (tscalc != (DavCalculation*)0) {
    if (tscalc->getState() > ResourceDescriptor::STATE_RUNNING) {
      // it is critical to flush the property cache before retrieving the
      // VIBFREQ property or else it will pick it up from the properties that
      // were previously associated with the calculation
      tscalc->flushPropertyCache();

      PropVector *propvec = (PropVector*)tscalc->getProperty("VIBFREQ");
      if (propvec != (PropVector*)0) {
        int nrows = propvec->rows();
        int irow = 0;
        while (irow<nrows && propvec->value(irow)<-1.0) {
          irow++;
        }

        if (irow == 0) {
          barrierText->SetLabel("None");
          barrierText->SetForegroundColour(wxColour(0, 0, 0));
        } else if (irow == 1) {
          barrierText->SetLabel("1 Imag. Freq.");
          barrierText->SetForegroundColour(wxColour(0, 0, 0));
        } else {
          char buf[80];
          sprintf(buf, "%d Imag. Freqs.", irow);
          barrierText->SetLabel(buf);
          barrierText->SetForegroundColour(wxColour(255, 0, 0));
          p_feedback->setMessage("The transition state has multiple "
                                 "imaginary frequencies.", WxFeedback::WARNING);
        }
      } else {
        barrierText->SetLabel("Unknown");
        barrierText->SetForegroundColour(wxColour(0, 0, 0));
      }
    } else {
      barrierText->SetLabel("Unknown");
      barrierText->SetForegroundColour(wxColour(0, 0, 0));
    }
  } else {
    barrierText->SetLabel("Unknown");
    barrierText->SetForegroundColour(wxColour(0, 0, 0));
  }
}


void NWDirdy::reparentCodes(const bool& theoryFlag, const string& codeName,
                            wxBoxSizer* codeSizer, wxBoxSizer* calcedSizer)
{
  int it;
  if (theoryFlag) {
    // first, map the current code to the fully supported codes vector index
    vector<string> codes = CodeFactory::getFullySupportedCodes();
    for (it = 0; it<(int)codes.size() && codes[it]!=codeName; it++);
    if (it < (int)codes.size()) {
      unsigned long codemap = it+1;

      // now, check if there's a current code calced button and whether it's
      // the same one as the new code
      wxSizerItemList codeChildren = codeSizer->GetChildren();
      wxSizerItemList::compatibility_iterator codeNode =
                                              codeChildren.GetFirst();
      // skip past the first two labels to get to the good stuff
      codeNode = codeNode->GetNext()->GetNext();

      if (codeNode) {
        // calced button found. If it's not for the current code then reparent
        // it and find the right one to reparent as the current code
        wxSizerItem *codeChild = codeNode->GetData();

        if ((unsigned long)codeChild->GetWindow()->GetClientData() != codemap) {
          codeSizer->Detach(codeChild->GetWindow());
          calcedSizer->Add(codeChild->GetWindow());

          // find the calced button that needs to be moved to the current code
          wxSizerItemList calcChildren = calcedSizer->GetChildren();
          wxSizerItemList::compatibility_iterator calcNode =
                                                  calcChildren.GetFirst();

          wxSizerItem *calcChild;
          while (calcNode) {
            calcChild = calcNode->GetData();
            if ((unsigned long)calcChild->GetWindow()->GetClientData() == codemap) {
              calcedSizer->Detach(calcChild->GetWindow());
              codeSizer->Add(calcChild->GetWindow());
              // must show the button because it was hidden before
              // in the calced sizer since there was a previous code set
              calcChild->GetWindow()->Show();
              break;
            }
          
            calcNode = calcNode->GetNext();
          }

          // defer the layout call to the caller
          //Layout();
        }
      } else {
        // no current code calced button so find the right one and reparent it
        wxSizerItemList calcChildren = calcedSizer->GetChildren();
        wxSizerItemList::compatibility_iterator calcNode =
                                                calcChildren.GetFirst();

        wxSizerItem *calcChild;
        while (calcNode) {
          calcChild = calcNode->GetData();
          if ((unsigned long)calcChild->GetWindow()->GetClientData() == codemap) {
            calcedSizer->Detach(calcChild->GetWindow());
            codeSizer->Add(calcChild->GetWindow());
            // defer the layout call to the caller
            //Layout();
            break;
          }
          
          calcNode = calcNode->GetNext();
        }
      }

      // Don't let the user override the code from the DirDyVTST Editor
      // once they first select a code--this can be done from the Calc Editor
      wxSizerItemList calcChildren = calcedSizer->GetChildren();
      for (it=1; it<(int)calcChildren.GetCount(); it++) {
        calcedSizer->Show((size_t)it, false);
      }
    }
  } else {
    // no theory so if there's a calced button for the current code, move
    // it back down to the calced sizer
    wxSizerItemList codeChildren = codeSizer->GetChildren();
    wxSizerItemList::compatibility_iterator codeNode = codeChildren.GetFirst();
    // skip past the first two labels
    codeNode = codeNode->GetNext()->GetNext();

    if (codeNode) {
      // calced button found, reparent it
      wxSizerItem *codeChild = codeNode->GetData();
      codeSizer->Detach(codeChild->GetWindow());
      calcedSizer->Add(codeChild->GetWindow());
      // defer the layout call to the caller
      //Layout();
    }

    // Make sure the user can select a code from the full palette
    wxSizerItemList calcChildren = calcedSizer->GetChildren();
    for (it=1; it<(int)calcChildren.GetCount(); it++) {
      calcedSizer->Show((size_t)it, true);
    }
  }
}


void NWDirdy::createDynamicGUI()
{
  // create bitmap buttons for electronic structure codes other than Amica
  ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();
  ResourceType *rt;
  vector<string> codes = CodeFactory::getFullySupportedCodes();
  ewxBitmapButton *codeButton;
#ifdef __x86_64
  long it;
#else
  int it;
#endif
  for (it = 0; it < (int)codes.size(); it++) {
    rt = rd.getResourceType("virtual_document", "ecceCalculation", codes[it]);
    if (rt!=(ResourceType*)0 &&
        rt->getFactoryCategory()=="EsCalculation" && codes[it]!="Amica") {
      // add the index into the list of supported codes as a way of
      // retrieving which code button was hit in the callback
      string icon = rt->getIcon();

      codeButton = new ewxBitmapButton(this, ID_BUTTON_DIRDY_CALC_R1,
                       ewxBitmap(icon, wxBITMAP_TYPE_XPM));
      codeButton->SetClientData((void*)(it+1));
      codeButton->SetToolTip(codes[it]);
      p_calcedSizerR1->Add(codeButton, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);
      codeButton = new ewxBitmapButton(this, ID_BUTTON_DIRDY_CALC_R2,
                       ewxBitmap(icon, wxBITMAP_TYPE_XPM));
      codeButton->SetClientData((void*)(it+1));
      codeButton->SetToolTip(codes[it]);
      p_calcedSizerR2->Add(codeButton, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);
      codeButton = new ewxBitmapButton(this, ID_BUTTON_DIRDY_CALC_P1,
                       ewxBitmap(icon, wxBITMAP_TYPE_XPM));
      codeButton->SetClientData((void*)(it+1));
      codeButton->SetToolTip(codes[it]);
      p_calcedSizerP1->Add(codeButton, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);
      codeButton = new ewxBitmapButton(this, ID_BUTTON_DIRDY_CALC_P2,
                       ewxBitmap(icon, wxBITMAP_TYPE_XPM));
      codeButton->SetClientData((void*)(it+1));
      codeButton->SetToolTip(codes[it]);
      p_calcedSizerP2->Add(codeButton, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);
      codeButton = new ewxBitmapButton(this, ID_BUTTON_DIRDY_CALC_TS,
                       ewxBitmap(icon, wxBITMAP_TYPE_XPM));
      codeButton->SetClientData((void*)(it+1));
      codeButton->SetToolTip(codes[it]);
      p_calcedSizerTS->Add(codeButton, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);
    }
  }

  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}


void NWDirdy::enableChildren(wxBoxSizer* sizer, const bool& enabled)
{
  wxSizerItemList children = sizer->GetChildren();
  wxSizerItemList::compatibility_iterator node = children.GetFirst()->GetNext();

  wxSizerItem *child;
  while (node) {
     child = node->GetData();
     child->GetWindow()->Enable(enabled);
     if (!enabled) {
       child->GetWindow()->Hide();
     }

     node = node->GetNext();
  }

  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}

void NWDirdy::initializeMessaging()
{
   // Inititialize messaging.
   p_msgDispatcher = new WxJMSMessageDispatch(WHOAMI, true);

   p_msgDispatcher->subscribe("ecce_invoke",
         this, (wxJmsCBFunc)&NWDirdy::invokeMCB);
   p_msgDispatcher->subscribe("ecce_quit",
         this, (wxJmsCBFunc)&NWDirdy::quitMCB);
   p_msgDispatcher->subscribe("ecce_identify",
         this, (wxJmsCBFunc)&NWDirdy::identifyMCB);

   p_msgDispatcher->subscribe("ecce_url_renamed",
         this, (wxJmsCBFunc)&NWDirdy::renameMCB);
   p_msgDispatcher->subscribe("ecce_url_removed",
         this, (wxJmsCBFunc)&NWDirdy::removeMCB);
   p_msgDispatcher->subscribe("ecce_url_basis",
         this, (wxJmsCBFunc)&NWDirdy::basisMCB);

   p_msgDispatcher->subscribe("ecce_url_state",
         this, (wxJmsCBFunc)&NWDirdy::stateMCB, false);
   p_msgDispatcher->subscribe("ecce_url_subject",
         this, (wxJmsCBFunc)&NWDirdy::subjectMCB);
   p_msgDispatcher->subscribe("ecce_url_code",
         this, (wxJmsCBFunc)&NWDirdy::codeMCB);
   // map theory to same message callback as code because this is
   // a special case for the DirDyVTST Editor and we treat a calc
   // w/o a theory the same as one w/o a code
   p_msgDispatcher->subscribe("ecce_url_theory",
         this, (wxJmsCBFunc)&NWDirdy::codeMCB);
   p_msgDispatcher->subscribe("ecce_url_property",
         this, (wxJmsCBFunc)&NWDirdy::propertyMCB);

   p_msgDispatcher->subscribe("ecce_preferences_states",
         this, (wxJmsCBFunc)&NWDirdy::statePrefsMCB);
   p_msgDispatcher->subscribe("ecce_getselection",
         this, (wxJmsCBFunc)&NWDirdy::selectionMCB);

   p_msgDispatcher->subscribe("ecce_vizthumbnail",
         this, (wxJmsCBFunc)&NWDirdy::thumbnailMCB);

   p_msgDispatcher->subscribe("ecce_auth_changed",
         this, (wxJmsCBFunc)&NWDirdy::authMCB);

   p_msgDispatcher->startSubscriber();
   p_msgDispatcher->registerTopShell(this);
   p_msgDispatcher->notifyReady();

}


string NWDirdy::getTitle() const
{
   return "NWChem DirDyVTST Editor";
}


void NWDirdy::invokeMCB(JMSMessage& msg)
{
   if (msg.containsProperty("calcurl")) {
      string url = msg.getProperty("calcurl");

      int debug = msg.getIntProperty("debug");
      p_debugFlag = debug==1;

      setContext(url);
      p_msgDispatcher->setPollContext(url);
      Raise();
   }
}


void NWDirdy::quitMCB(JMSMessage& msg)
{
   if (msg.getSender().getName() == GATEWAY) {
      doQuit(true);
   }
}


/**
 * State color preferences have changed.
 * Update state icon in feedback area.
 */
void NWDirdy::statePrefsMCB(JMSMessage& msg)
{
   p_feedback->readPreferences();
}


/**
 * Identify message call back function
 *
 * When gateway sent out identify message, periodic table app will response by sending a ID message back.
 */
void NWDirdy::identifyMCB(JMSMessage&)
{
   ostrstream os;
   os << getTitle();

   // Add context info if we have context
   if (p_model != 0) {
      EcceURL url(p_model->getUrl());
      os << ": " << EDSIServerCentral().mapURLtoName(url);
   }
   os << ends;

   JMSPublisher publisher(WHOAMI);
   JMSMessage *msg = publisher.newMessage();
   msg->addProperty("info",os.str());
   publisher.publish("ecce_identify_reply", *msg);
   delete msg;
}


/**
 * Reset the gui to the task at the specified url.
 */
void NWDirdy::setContext(const string& urlstr) throw (EcceException)
{
  wxBusyCursor busy;

  if (p_model) {

    // Set model's url
    p_model->setUrl(urlstr);

    try {
      // Get dirdyTask from factory
      p_dirdyTask = 
        dynamic_cast<DirDyVTSTTask*>(EDSIFactory::getResource(EcceURL(urlstr)));
      if (p_dirdyTask == 0) {
        throw InvalidException("Could not get DirDyVTST task", WHERE);
      }

      // Get Session
      p_session = p_dirdyTask->getSession();
      if (p_session == 0) {
        throw InvalidException("Could not get Session", WHERE);
      }

      // @todo need checking that task is a session member

      // Set run state and gui label and deserialize the GUI's model
      p_feedback->setRunState(p_dirdyTask->getState());
      EcceURL url = p_dirdyTask->getURL();
      EDSIServerCentral servers;
      p_feedback->setContextLabel(servers.mapURLtoName(url));
      p_feedback->setContextURL(url);
      p_dirdyTask->getTaskModel(*p_model);
    }
    catch (...) {
      this->NWDirdy::~NWDirdy();
      Destroy();
    }

  }

  //Refresh GUI with model
  initializeGUI();

  // Create the Setup window now because the theory details dialog
  // below references it
  p_setupGUI = new NWDirdySetup(&getNWDirdyModel(), this, NULL);

  // This initialization would normally be done in initializeGUI but
  // p_setupGUI wasn't defined then
  p_setupGUI->restrictTheories(getSpinMult(ID_CHOICE_DIRDY_SPIN_TS), true);

  // Restore theory details
  p_generalGUIValues = p_dirdyTask->DavCalculation::guiparams("general");
  p_singleGUIValues = p_dirdyTask->DavCalculation::guiparams("single");

  if (detailsReady()) {
    if (p_generalGUIValues->size()==0)
      startGeneralTheoryApp(true);

    if (p_singleGUIValues->size()==0)
      startSingleTheoryApp(true);
  }

  // check if any of the fragments have been updated (including the geometry
  // trace property) since the last time the DirDyVTST Editor was up and
  // regenerate input file if so
  checkFragmentSave();

  updateDisabledState(p_feedback->getRunState(), true);

  setSaveState(false);
}


void NWDirdy::freeContext()
{
  if (p_generalGUIValues) {
    delete p_generalGUIValues;
    p_generalGUIValues = 0;
  }

  if (p_singleGUIValues) {
    delete p_singleGUIValues;
    p_singleGUIValues = 0;
  }

  //p_setupGUI->refreshBasisSetFields();
  p_setupGUI->Close(true);

  p_dirdyTask = 0;
  p_session = 0;

  // clear the feedback area
  p_feedback->clearMessage();
  p_feedback->setContextURL("");
  p_feedback->setContextLabel("");
  p_feedback->setRunState(ResourceDescriptor::STATE_CREATED);
  p_feedback->setEditStatus(WxFeedback::EDIT);
  setSaveState(false);

  setState( NWDirdy::NO_SYSTEM );

  refreshGUI();
}


/**
 * Implementation of TaskApp::taskAppRaise.
 * Window is raised to front.
 */
void NWDirdy::taskAppRaise()
{
   Raise();
}


/**
 * Implementation of TaskApp::taskAppShow.
 * Window is shown.
 */
void NWDirdy::taskAppShow()
{
   Show(TRUE);
}


/**
 * Implementation of TaskApp::taskAppDestroy.
 * Window is destroyed.
 */
void NWDirdy::taskAppDestroy()
{
   doQuit(true);
}


/**
 * quit but first handle any unsaved changes.
 * @param force without prompting even if error occurs during save
 */
void NWDirdy::doQuit(const bool& force)
{
   if (p_feedback->getEditStatus() == WxFeedback::MODIFIED) {
      long buttonFlags = wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION;
      if (!force) {
        buttonFlags |= wxCANCEL;
      }
      ewxMessageDialog prompt(this,"The current calculation has unsaved "
        "changes!  Do you wish to save changes before quitting?",
        "Save DirDyVTST Editor Changes?", buttonFlags);
      int status = prompt.ShowModal();
      if (status == wxID_YES) {
         try {
            (void)doSave();
         } catch (EcceException& ex) {
            p_feedback->setMessage(ex.what(), WxFeedback::ERROR);
         }
      } else if (status == wxID_CANCEL) {
         // abort the quit - not sure if this will cause problems...
         return;
      }
   }

   // Close theory details dialogs
   closeGeneralTheoryApp(true);
   closeSingleTheoryApp(true);

   // Close setup window
   p_setupGUI->Close(true);

   Destroy();
}


/**
 * Save the model, generate and save input file.
 */
bool NWDirdy::saveModelAndInputFile(const bool& reportFlag) throw(IOException)
{
   bool saveDoneFlag = false;
   NWDirdyModel& model = getNWDirdyModel();
   p_dirdyTask->setTaskModel(&model);

   // Save charge for each component calculation
   int chargeComboIds[] = { ID_COMBOBOX_DIRDY_CHARGE_TS,
                            ID_COMBOBOX_DIRDY_CHARGE_R1,
                            ID_COMBOBOX_DIRDY_CHARGE_R2,
                            ID_COMBOBOX_DIRDY_CHARGE_P1,
                            ID_COMBOBOX_DIRDY_CHARGE_P2 };
   int spinChoiceIds[] = { ID_CHOICE_DIRDY_SPIN_TS,
                           ID_CHOICE_DIRDY_SPIN_R1,
                           ID_CHOICE_DIRDY_SPIN_R2,
                           ID_CHOICE_DIRDY_SPIN_P1,
                           ID_CHOICE_DIRDY_SPIN_P2 };
   DirDyVTSTTask::Component component[] = { DirDyVTSTTask::TRANSITION,
                                            DirDyVTSTTask::REACT1,
                                            DirDyVTSTTask::REACT2,
                                            DirDyVTSTTask::PROD1,
                                            DirDyVTSTTask::PROD2 };
   DavCalculation *calc = 0;
   ewxComboBox *chargeCombo = 0;
   string valstr;
   int valint;
   SpinMult::SpinMultEnum valmult;
   unsigned long vallong;
   for (int i = 0; i < 5; i++) {
     if ((calc = p_dirdyTask->getComponentTask(component[i]))) {
       Fragment *frag = calc->fragment();
       if (frag != (Fragment*)0) {
         chargeCombo = ((ewxComboBox*)FindWindow(chargeComboIds[i]));
         valstr = chargeCombo->GetValue().c_str();
         if (StringConverter::toInt(valstr, valint) && valint!=frag->charge()) {
           saveDoneFlag = true;
           frag->charge(valint);
           calc->fragment(frag);
           notifySubject(calc->getURL().toString());
         }
       }
       valmult = getSpinMult(spinChoiceIds[i]);
       if (valmult != calc->spinMultiplicity()) {
         saveDoneFlag = true;
         calc->spinMultiplicity(valmult);
         vallong = (unsigned long)valmult - 1;
         calc->openShells(vallong);
         notifySpin(calc->getURL().toString());
       }
     }
   }

   // Save TS fragment, if any, to DirDyVTST vdoc
   Fragment *frag = p_dirdyTask->getFragment(DirDyVTSTTask::TRANSITION);
   if (frag != (Fragment*)0) {
     frag->useSymmetry(getNWDirdyModel().getAutoSym());
     p_dirdyTask->DavCalculation::fragment(frag);
     delete frag;
   }

   // Save TS multiplicity and open shells to DirDyVTST vdoc
   if ((calc = p_dirdyTask->getComponentTask(DirDyVTSTTask::TRANSITION))) {
     p_dirdyTask->DavCalculation::spinMultiplicity(calc->spinMultiplicity());
     p_dirdyTask->DavCalculation::openShells(calc->openShells());
   }

   // Save General Theory
   string cat = getNWDirdyModel().getCategory();
   string thry = getNWDirdyModel().getTheory();
   TTheory* currTheory = p_dirdyTask->DavCalculation::theory("general");
   if (currTheory==(TTheory*)0 ||
       cat != currTheory->category() || thry != currTheory->name()) {
     saveDoneFlag = true;
     TTheory theory(cat,thry);
     p_dirdyTask->DavCalculation::theory(&theory,"general");
   }
   if (currTheory != (TTheory*)0)
     delete currTheory;

   // Save Single Point Theory
   cat = getNWDirdyModel().getSPCategory();
   thry = getNWDirdyModel().getSPTheory();
   currTheory = p_dirdyTask->DavCalculation::theory("single");
   if (currTheory==(TTheory*)0 ||
       cat != currTheory->category() || thry != currTheory->name()) {
     saveDoneFlag = true;
     TTheory theory(cat,thry);
     p_dirdyTask->DavCalculation::theory(&theory,"single");
   }
   if (currTheory != (TTheory*)0)
     delete currTheory;

   // Save Theory Details
   p_dirdyTask->DavCalculation::guiparams(p_generalGUIValues, "general");
   p_dirdyTask->DavCalculation::guiparams(p_singleGUIValues, "single");

   if (p_feedback->getEditStatus() == WxFeedback::MODIFIED) {
     saveDoneFlag = true;
   }

   bool ret = false;
   if (checkComponent(DirDyVTSTTask::REACT1) &&
       checkComponent(DirDyVTSTTask::PROD1)) {
     ret = generateInputFile();
   }

   setSaveState(false);

   // only report save to the user if an input file was generated when
   // the report flag isn't set--prevents duplicate and unneeded save messages
   if (saveDoneFlag) {
     if (ret) {
       p_feedback->setMessage("New input file generated.", WxFeedback::INFO);
     }
     p_feedback->setMessage("Calculation saved as " + p_dirdyTask->getName() +
                            ".", WxFeedback::INFO);
   } else if (ret && reportFlag) {
     p_feedback->setMessage("New input file generated.", WxFeedback::INFO);
   }

   return ret;
}


SpinMult::SpinMultEnum NWDirdy::getSpinMult(const int& choiceId) const
{
  wxString spin = ((ewxChoice*)FindWindow(choiceId))->GetStringSelection();
  if (spin.IsEmpty()) {
    return SpinMult::unknown;
  } else {
    return SpinMult::toSpinMult(spin.c_str());
  }
}


void NWDirdy::notifySubject(const string& url)
{
   JMSPublisher publisher(WHOAMI);
   JMSMessage *msg = publisher.newMessage();
   msg->addProperty("url", url);
   publisher.publish("ecce_url_subject", *msg);
   delete msg;
}


void NWDirdy::notifySpin(const string& url)
{
   JMSPublisher publisher(WHOAMI);
   JMSMessage *msg = publisher.newMessage();
   msg->addProperty("url", url);
   publisher.publish("ecce_url_spin", *msg);
   delete msg;
}


/**
 * Implementation of TaskApp::setUnitFamily.
 * All unitted fields are updated to reflect the new unit family.
 */
void NWDirdy::setUnitFamily(const string& family)
{
   ewxWindowUtils::setUnitFamily(this, family);
   Fit();
}


/**
 * Creates a new PrepareModel instance.
 * If there is an existing instance, it is destroyed.
 */
void NWDirdy::initializeModel()
{
  if (p_model != 0) {
     delete p_model;
  }
  p_model = new NWDirdyModel();

}


void NWDirdy::processSave()
{
  wxBusyCursor busy;
  try {
    if (p_feedback->getEditStatus() == WxFeedback::MODIFIED) {
      (void)doSave();
    }
  } catch (EcceException& ex) {
    p_feedback->setMessage(ex.what(), WxFeedback::ERROR);
  }
}


void NWDirdy::doSaveButtonClick(wxMouseEvent &event)
{
  processSave();
  event.Skip();
}


/**
 * Send message(s) to request startup of builder with this context.
 */
void NWDirdy::requestBuilder(const EcceURL &url, const string& actionTopic, bool forceNew)
{
   processSave();

   JMSPublisher publisher(WHOAMI);
   JMSMessage *msg = publisher.newMessage();
   msg->addProperty("action","start");
   publisher.publish("ecce_activity",*msg);
   delete msg;

   // Send the start app request to gateway:
   Target gateway(GATEWAY, "");
   msg = publisher.newMessage(gateway);
   // Ensure that this message will carry original sender info for
   // sending point to point responses after startup
   msg->markAsPointToPoint();

   // which builder do we invoke? find out now!
   Resource *resource = EDSIFactory::getResource(url);
   if (resource) {
      ResourceType *resourceType = resource->getDescriptor();
      if (resourceType) {
         vector<ResourceTool*> tools = resourceType->getTools();
         for (int i = 0; i < (int)tools.size(); i++) {
            string name = tools[i]->getName();
            if (name == BUILDER) {
               publisher.invoke(*msg, name, 0, url.toString(), actionTopic);
               break;
            }
         }
      }
   }

   delete msg;
}


/**
 * Send message(s) to request startup of calc editor.
 */
void NWDirdy::requestCalcEditor(const EcceURL& url, const string& codename)
{
   processSave();

   JMSPublisher publisher(WHOAMI);
   JMSMessage *msg = publisher.newMessage();
   msg->addProperty("action","start");
   publisher.publish("ecce_activity",*msg);
   delete msg;

   // Send the start app request to gateway:
   Target gateway(GATEWAY, "");
   msg = publisher.newMessage(gateway);

   if (codename != "") {
     msg->addProperty("codename", codename);
   }

   publisher.invoke(*msg, CALCED, 0, url.toString());
   delete msg;
}


/**
 * Send message(s) to request startup of calc editor.
 */
void NWDirdy::requestBasisTool(const string& actionTopic, bool forceNew)
{
   processSave();

   JMSPublisher publisher(WHOAMI);
   JMSMessage *msg = publisher.newMessage();
   msg->addProperty("action","start");
   publisher.publish("ecce_activity",*msg);
   delete msg;

   // Send the start app request to gateway:
   Target gateway(GATEWAY, "");
   msg = publisher.newMessage(gateway);
   publisher.invoke(*msg, BASISTOOL, 0,
             p_dirdyTask->DavCalculation::getURL().toString(), actionTopic);
   delete msg;
}


/**
 * Send message(s) to request startup of launcher.
 */
void NWDirdy::requestLauncher(const EcceURL &url, bool forceNew)
{
   processSave();

   JMSPublisher publisher(WHOAMI);
   JMSMessage *msg = publisher.newMessage();
   msg->addProperty("action","start");
   publisher.publish("ecce_activity",*msg);
   delete msg;

   // Send the start app request to gateway:
   Target gateway(GATEWAY, "");
   msg = publisher.newMessage(gateway);
   publisher.invoke(*msg, LAUNCHER, 0, url.toString());
   delete msg;
}


/**
 * A resource was renamed.
 */
void NWDirdy::renameMCB(JMSMessage& msg)
{
  string oldURL = msg.getProperty("oldurl");
  string newURL = msg.getProperty("newurl");

  if (p_dirdyTask && p_dirdyTask->getURL().isChildOrMe(oldURL)) {
    EDSIFactory::renamePoolResource(oldURL, newURL);
    string calcurl = p_dirdyTask->getURL().toString();
    p_feedback->setContextLabel(EDSIServerCentral().mapURLtoName(calcurl));
    setContext(calcurl);
  }
}


/**
 * A resource was removed.
 */
void NWDirdy::removeMCB(JMSMessage& msg)
{
  string url = msg.getProperty("url");
  if (p_dirdyTask && p_dirdyTask->getURL().isChildOrMe(url)) {
    EDSIFactory::removePoolResource(url);

    // free context does not work for restoring the context so force the
    // app to quit instead
    //freeContext();
    doQuit(true);
  }
}


/**
 * A basis set has changed
 */
void NWDirdy::basisMCB(JMSMessage& msg)
{
   string url = msg.getProperty("url");

   string durl = p_dirdyTask->getURL().toString();

   if (url == durl) {
     EDSIFactory::changePoolResource(url);

     p_setupGUI->refreshBasisSetFields();

     (void)doSave();
   }
}


/**
 * State has changed.
 */
void NWDirdy::stateMCB(JMSMessage& msg)
{
   string fullurl = msg.getProperty("url");
   string url = fullurl;
   string durl = p_dirdyTask->getURL().toString();

   if (url != durl) {
     string::size_type slash;
     if ((slash = url.rfind('/')) != string::npos) {
       if (url.length() == slash+1) {
         url.resize(slash);
         if ((slash = url.rfind('/')) != string::npos)
           url.resize(slash);
       } else {
         url.resize(slash);
       }
     }

     if ((slash = durl.rfind('/')) != string::npos)
       durl.resize(slash);

     if (url == durl) {
       EDSIFactory::changePoolResource(fullurl);

       // check if it was the transition state task
       if (fullurl.rfind("/Transition-State") != string::npos)
         setBarrierSummaryField();
     }
   } else {
     EDSIFactory::changePoolResource(fullurl);

     string statestr = msg.getProperty("state");
     ResourceDescriptor::RUNSTATE state =
                                  ResourceUtils::stringToState(statestr);

     // make sure this is actually a new run state
     if (state != p_feedback->getRunState()) {
       updateDisabledState(state);
       p_feedback->setRunState(state);
       if (state >= ResourceDescriptor::STATE_SUBMITTED) {
          p_feedback->setEditStatus(WxFeedback::READONLY);
       } else if (!isUnsavedState()) {
          p_feedback->setEditStatus(WxFeedback::EDIT);
       }
     }

     if (state <= ResourceDescriptor::STATE_READY) {
       // check if any of the reaction component fragments are newer than the
       // DirDyVTST input file.  If so, generate a new input file.
       checkFragmentSave();
     }
   }
}


void NWDirdy::checkFragmentSave()
{
  MetaDataRequest request;
  vector<MetaDataRequest> requests;
  vector<MetaDataResult> infile_results;

  request.name = "DAV:getlastmodified";
  requests.push_back(request);

  vector<EcceURL> inputfile = p_dirdyTask->getDataFiles(JCode::INPUT);
  if (inputfile.size() > 0) {
    EcceURL iUrl = inputfile[0];
    EDSI *edsi = EDSIFactory::getEDSI(iUrl);
    edsi->getMetaData(requests, infile_results);
  }

  if (infile_results.size() > 0) {
    DavCalculation *calc;
    DirDyVTSTTask::Component component[] = { DirDyVTSTTask::TRANSITION,
                                             DirDyVTSTTask::REACT1,
                                             DirDyVTSTTask::REACT2,
                                             DirDyVTSTTask::PROD1,
                                             DirDyVTSTTask::PROD2 };
    long fragSecs = 0;
    long calcSecs;
    for (int i = 0; i < 5; i++) {
      if ((calc = p_dirdyTask->getComponentTask(component[i]))) {
        calcSecs = calc->getFragmentModifiedDate();
        if (calcSecs > fragSecs) {
          fragSecs = calcSecs;
        }
      }
    }

    long inSecs = TDateTime::toSeconds(infile_results[0].value.c_str());
    if (fragSecs > inSecs) {
      (void)doSave();
    }
  } else {
    // no current input file
    // the doSave() method will check whether the conditions are satisfied
    // to generate an input file and do nothing if not
    // passing reportFlag as fault avoids duplicate and unneeded save messages
    // being displayed to the user
    (void)doSave(false);
  }
}


void NWDirdy::updateDisabledState(ResourceDescriptor::RUNSTATE state, bool forceMsg)
{
  if (state >= ResourceDescriptor::STATE_SUBMITTED) {
    setFrameCustomDisabledStyle(false);
    p_setupGUI->setFrameCustomDisabledStyle(false);

    if (forceMsg || state == ResourceDescriptor::STATE_SUBMITTED) {
      WxFeedback *feedback = ((WxFeedback*)p_setupGUI->FindWindow(
                NWDirdySetup::ID_FEEDBACK_NWDRDYSET));
      p_feedback->setMessage("This calculation is read only and setup cannot "
                             "be changed without terminating during the run or "
                             "resetting after completion.", WxFeedback::INFO);
      feedback->setMessage("This calculation is read only and setup cannot "
                           "be changed without terminating during the run or "
                           "resetting after completion.", WxFeedback::INFO);
    }
  } else {
    setFrameCustomDisabledStyle(true);
    p_setupGUI->setFrameCustomDisabledStyle(true);
  }

  // Close button should never be disabled
  ewxButton *btn;
  btn = ((ewxButton*)p_setupGUI->FindWindow(NWDirdySetup::ID_BUTTON_NWDRDYSET_CLOSE));
  btn->setCustomDisabledStyle(true);

  btn = ((ewxButton*)FindWindow(ID_BUTTON_NWDIRDY_LAUNCH));
  btn->setCustomDisabledStyle(true);
  btn = ((ewxButton*)FindWindow(ID_BUTTON_NWDIRDY_FINAL_EDIT));
  btn->setCustomDisabledStyle(true);
  btn = ((ewxButton*)FindWindow(ID_BUTTON_NWDIRDY_SETUP));
  btn->setCustomDisabledStyle(true);

  ewxBitmapButton *bitmap;
  bitmap = ((ewxBitmapButton*)FindWindow(ID_BITMAPBUTTON_DIRDY_BUILD_TS));
  bitmap->setCustomDisabledStyle(true);
  bitmap = ((ewxBitmapButton*)FindWindow(ID_BITMAPBUTTON_DIRDY_BUILD_R1));
  bitmap->setCustomDisabledStyle(true);
  bitmap = ((ewxBitmapButton*)FindWindow(ID_BITMAPBUTTON_DIRDY_BUILD_R2));
  bitmap->setCustomDisabledStyle(true);
  bitmap = ((ewxBitmapButton*)FindWindow(ID_BITMAPBUTTON_DIRDY_BUILD_P1));
  bitmap->setCustomDisabledStyle(true);
  bitmap = ((ewxBitmapButton*)FindWindow(ID_BITMAPBUTTON_DIRDY_BUILD_P2));
  bitmap->setCustomDisabledStyle(true);
}


/**
 * A molecule has changed.
 */
void NWDirdy::subjectMCB(JMSMessage& msg)
{
   string fullurl = msg.getProperty("url");
   string url = fullurl;

   string durl = p_dirdyTask->getURL().toString();

   string::size_type slash;
   if ((slash = url.rfind('/')) != string::npos)
     url.resize(slash);
   if ((slash = durl.rfind('/')) != string::npos)
     durl.resize(slash);

   if (url == durl) {
     EDSIFactory::changePoolResource(fullurl);

     (void)doSave();
   }

   // if subject of managed calcs changed, update the GUI (thumbnails)
   if (EcceURL(url).isChildOf(EcceURL(durl).getParent())) {
     // check if this is the transition state for a calculation that hasn't
     // been edited by the user and, if so, need to set the default basis set
     // for the transition state calculation and also the DirDyVTST
     // calculation since that uses the transition state structure
     DavCalculation *tscalc;
     if ((tscalc = p_dirdyTask->getComponentTask(DirDyVTSTTask::TRANSITION)) &&
         tscalc->getURL().toString()==fullurl &&
         tscalc->getState()<ResourceDescriptor::STATE_READY) {
       setDefaultBasisSet(tscalc, tscalc->fragment());
       setDefaultDirdyBasisSet((DavCalculation*)p_dirdyTask,
                               tscalc->fragment());
     }

     refreshGUI();
   }
}


/**
 * A code has changed.
 */
void NWDirdy::codeMCB(JMSMessage& msg)
{
   string fullurl = msg.getProperty("url");
   string url = fullurl;

   string durl = p_dirdyTask->getURL().toString();

   string::size_type slash;
   if ((slash = url.rfind('/')) != string::npos)
     url.resize(slash);
   if ((slash = durl.rfind('/')) != string::npos)
     durl.resize(slash);

   if (url == durl) {
     EDSIFactory::changePoolResource(fullurl);

     (void)doSave();
   }

   // if code of managed calcs changed, update the GUI
   if (EcceURL(url).isChildOf(EcceURL(durl).getParent())) {
     refreshGUI();
   }
}


void NWDirdy::propertyMCB(JMSMessage& msg)
{
   string url = msg.getProperty("url");

   string durl = p_dirdyTask->getURL().toString();

   if (url == durl) {
     EDSIFactory::changePoolResource(url);

     // The user can change the annotation even for >ready state calculations
     // so make sure to check that to avoid overwriting the input file
     if (p_dirdyTask->getState() <= ResourceDescriptor::STATE_READY) {
       (void)doSave();
     }
   }
}


/**
 * Builder is sending a message with selection
 */
void NWDirdy::selectionMCB(JMSMessage& msg)
{
   int natoms = msg.getIntProperty("numatoms");
   string selection = msg.getProperty("selection");
   vector<int> highlight;
   vector<string> thumbCalcs;
   if (natoms == 0) {
     Raise();
     string msg = "No atoms have been selected in the Builder.  Do you wish "
                  "to define a single ";
     msg += getNWDirdyModel().getProductsFlag()? "product": "reactant";
     msg += " with the same structure as the transition state?";
     ewxMessageDialog prompt(this, msg, "ECCE Builder Selection",wxOK|wxCANCEL);
     if (prompt.ShowModal() != wxID_OK) {
       natoms = -1;
     }
   } else {
     StringTokenizer tkn(selection);
     for (int i=0; i<natoms; i++) {
       string iatom = tkn.next();
       highlight.push_back(atoi(iatom.c_str()));
     }
   }
   if (p_model) {
     Fragment *TS = p_dirdyTask->getFragment(DirDyVTSTTask::TRANSITION);
     if (TS != 0) {
       getNWDirdyModel().setNumAtomsTS(TS->numAtoms());
       if (getNWDirdyModel().getProductsFlag()) {
         // If operation is canceled, then natoms will be returned as -1
         if (natoms >= 0) {
           Fragment *P1 = getNWDirdyModel().getProduct1();
           Fragment *P2 = getNWDirdyModel().getProduct2();
           if (P1)
             delete P1;
           if (P2)
             delete P2;
           if (highlight.size() > 0 && highlight.size() < TS->numAtoms()) {
             // Two distinct fragments selected
             P1 = TS->createSubFragment(highlight,false);
             P2 = TS->createSubFragment(highlight,true);
             getNWDirdyModel().setNumAtomsProd1(P1->numAtoms());
             getNWDirdyModel().setNumAtomsProd2(P2->numAtoms());
             thumbCalcs.push_back(
                    addComponent(DirDyVTSTTask::PROD1)->getURL().toString());
             thumbCalcs.push_back(
                    addComponent(DirDyVTSTTask::PROD2)->getURL().toString());
             p_dirdyTask->setFragment(P1, DirDyVTSTTask::PROD1);
             p_dirdyTask->setFragment(P2, DirDyVTSTTask::PROD2);

             notifySubject(p_dirdyTask->getComponentTask(
                                  DirDyVTSTTask::PROD1)->getURL().toString());
             notifySubject(p_dirdyTask->getComponentTask(
                                  DirDyVTSTTask::PROD2)->getURL().toString());

             setDefaultBasisSet(p_dirdyTask->getComponentTask(
                                             DirDyVTSTTask::PROD1), P1);
             setDefaultBasisSet(p_dirdyTask->getComponentTask(
                                             DirDyVTSTTask::PROD2), P2);

           } else {
             // Just copy existing TS Fragment to P1
             P1 = new Fragment(*TS);
             getNWDirdyModel().setNumAtomsProd1(P1->numAtoms());
             thumbCalcs.push_back(
                    addComponent(DirDyVTSTTask::PROD1)->getURL().toString());
             p_dirdyTask->setFragment(P1, DirDyVTSTTask::PROD1);

             if (checkComponent(DirDyVTSTTask::PROD2)) {
               removeComponent(DirDyVTSTTask::PROD2);
             }
             P2 = (Fragment*)0;

             notifySubject(p_dirdyTask->getComponentTask(
                                  DirDyVTSTTask::PROD1)->getURL().toString());

             setDefaultBasisSet(p_dirdyTask->getComponentTask(
                                             DirDyVTSTTask::PROD1), P1);
           }
         }
       } else {
         // If operation is canceled, then natoms will be returned as -1
         if (natoms >= 0) {
           Fragment *R1 = getNWDirdyModel().getReactant1();
           Fragment *R2 = getNWDirdyModel().getReactant2();
           if (R1)
             delete R1;
           if (R2)
             delete R2;
           if (highlight.size() > 0 && highlight.size() < TS->numAtoms()) {
             // Two distinct fragments selected
             R1 = TS->createSubFragment(highlight,false);
             R2 = TS->createSubFragment(highlight,true);
             getNWDirdyModel().setNumAtomsReact1(R1->numAtoms());
             getNWDirdyModel().setNumAtomsReact2(R2->numAtoms());
             thumbCalcs.push_back(
                    addComponent(DirDyVTSTTask::REACT1)->getURL().toString());
             thumbCalcs.push_back(
                    addComponent(DirDyVTSTTask::REACT2)->getURL().toString());
             p_dirdyTask->setFragment(R1, DirDyVTSTTask::REACT1);
             p_dirdyTask->setFragment(R2, DirDyVTSTTask::REACT2);

             notifySubject(p_dirdyTask->getComponentTask(
                                  DirDyVTSTTask::REACT1)->getURL().toString());
             notifySubject(p_dirdyTask->getComponentTask(
                                  DirDyVTSTTask::REACT2)->getURL().toString());

             setDefaultBasisSet(p_dirdyTask->getComponentTask(
                                             DirDyVTSTTask::REACT1), R1);
             setDefaultBasisSet(p_dirdyTask->getComponentTask(
                                             DirDyVTSTTask::REACT2), R2);
           } else {
             // Just copy existing TS Fragment to R1
             R1 = new Fragment(*TS);
             getNWDirdyModel().setNumAtomsReact1(R1->numAtoms());
             thumbCalcs.push_back(
                    addComponent(DirDyVTSTTask::REACT1)->getURL().toString());
             p_dirdyTask->setFragment(R1, DirDyVTSTTask::REACT1);

             if (checkComponent(DirDyVTSTTask::REACT2)) {
               removeComponent(DirDyVTSTTask::REACT2);
             }
             R2 = (Fragment*)0;

             notifySubject(p_dirdyTask->getComponentTask(
                                  DirDyVTSTTask::REACT1)->getURL().toString());

             setDefaultBasisSet(p_dirdyTask->getComponentTask(
                                             DirDyVTSTTask::REACT1), R1);
           }
         }
       }
       delete TS;
     }
     VizThumbnail::thumbnail(WHOAMI, thumbCalcs);
     refreshGUI();

     // enforce charge/spin consistency for newly created reactants/products
     wxCommandEvent event;
     OnComboboxDirdyChargeTsSelected(event);

     checkStatus();
   }
}


void NWDirdy::setDefaultDirdyBasisSet(DavCalculation* calc, Fragment* frag)
{
  if (calc->gbsConfig("general") == (TGBSConfig*)0) {
    EDSIServerCentral central;
    EDSIGaussianBasisSetLibrary *gbsFactory = new EDSIGaussianBasisSetLibrary(
                                    central.getDefaultBasisSetLibrary());

    TGBSConfig *basis = gbsFactory->simpleLookup("6-31++G**",
                                                 frag->uniqueTagStr().c_str());
    GBSRules::autoOptimize(basis, calc->application(), calc->theory());

    calc->gbsConfig(basis, "general");
    calc->gbsConfig(basis, "single");
    p_setupGUI->refreshBasisSetFields();

    delete basis;

    JMSPublisher publisher(WHOAMI);
    JMSMessage *msg = publisher.newMessage();
    msg->addProperty("url", calc->getURL().toString());
    publisher.publish("ecce_url_basis", *msg);
    delete msg;
  }
}

void NWDirdy::setDefaultBasisSet(DavCalculation* calc, Fragment* frag)
{
  if (calc->gbsConfig() == (TGBSConfig*)0) {
    EDSIServerCentral central;
    EDSIGaussianBasisSetLibrary *gbsFactory = new EDSIGaussianBasisSetLibrary(
                                    central.getDefaultBasisSetLibrary());

    TGBSConfig *basis = gbsFactory->simpleLookup("6-31++G**",
                                                 frag->uniqueTagStr().c_str());
    GBSRules::autoOptimize(basis, calc->application(), calc->theory());

    calc->gbsConfig(basis);

    delete basis;

    JMSPublisher publisher(WHOAMI);
    JMSMessage *msg = publisher.newMessage();
    msg->addProperty("url", calc->getURL().toString());
    publisher.publish("ecce_url_basis", *msg);
    delete msg;
  }
}


void NWDirdy::thumbnailMCB(JMSMessage& msg)
{
  refreshGUI();
}


void NWDirdy::authMCB(JMSMessage& msg)
{
  AuthCache::getCache().msgIn(msg, p_msgDispatcher->getMyID());
}


/**
 *  Separate two Fragments a fixed distance from each other by moving along
 *  the coordinate in the transition state that appears to get "broken". This
 *  function assumes that the atom fragments have been aligned as much as
 *  possible with the corresponding atoms in the transition state.
 */
void NWDirdy::separateFragments(Fragment *frag1, Fragment *frag2, Fragment *ts)
{
  // return without doing anything if one of the Fragment pointers is null
  if (frag1 == (Fragment*)0 || frag2 == (Fragment*)0 || ts == (Fragment*)0) {
    return;
  }
  // get atom lists for fragments
  vector<TAtm*>* atoms1 = frag1->atoms();
  vector<TAtm*>* atoms2 = frag2->atoms();
  vector<TAtm*>* atomsTS = ts->atoms();
  TAtm *atom1 = (TAtm*)0;
  TAtm *atom2 = (TAtm*)0;
  double RSEP = getNWDirdyModel().getFragmentSep();

  int numAtoms1 = atoms1->size();
  int numAtoms2 = atoms2->size();
  int numAtomsTS = atomsTS->size();

  int i, j;

  // do an exhaustive search to find bond in transition state that is to be
  // broken. Start by grouping transition state atoms into according to 
  // whether or not they end up in frag1 or frag2.
  vector<TAtm*> grp1;
  vector<TAtm*> grp2;
  for (i=0; i<numAtomsTS; i++) {
    for (j=0; j<numAtoms1; j++) {
      if ((*atomsTS)[i]->atomType() == (*atoms1)[j]->atomType()) {
        grp1.push_back((*atomsTS)[i]);
      }
    }
    for (j=0; j<numAtoms2; j++) {
      if ((*atomsTS)[i]->atomType() == (*atoms2)[j]->atomType()) {
        grp2.push_back((*atomsTS)[i]);
      }
    }
  }

  // Find coordinate in transition state that breaks
  double minSep = -1.0;
  double rsep,dx,dy,dz;
  const double *crds1, *crds2;
  double crds[3];
  TAtm *atmi;
  TAtm *atmj;
  int ngrp1 = grp1.size();
  int ngrp2 = grp2.size();
  for (i=0; i<ngrp1; i++) {
    atmi = grp1[i];
    crds1 = atmi->coordinates();
    for (j=0; j<ngrp2; j++) {
      atmj = grp2[j];
      crds2 = atmj->coordinates();
      dx = crds1[0] - crds2[0];
      dy = crds1[1] - crds2[1];
      dz = crds1[2] - crds2[2];
      rsep = sqrt(dx*dx+dy*dy+dz*dz);
      if (rsep < minSep && minSep > 0.0) {
        atom1 = atmi;
        atom2 = atmj;
        minSep = rsep;
      } else if (minSep < 0.0) {
        atom1 = atmi;
        atom2 = atmj;
        minSep = rsep;
      }
    }
  }
  // Find separation vector
  TAtm *fragAt;
  double ax, ay, az;
  if (atom1 && atom2) {
    crds1 = atom1->coordinates();
    crds2 = atom2->coordinates();
    dx = crds1[0] - crds2[0];
    dy = crds1[1] - crds2[1];
    dz = crds1[2] - crds2[2];
    rsep = sqrt(dx*dx+dy*dy+dz*dz);
    dx /= rsep;
    dy /= rsep;
    dz /= rsep;
    // Find atom in frag1 that corresponds to atom1 in TS
    for (i=0; i<numAtoms1; i++) {
      if (atom1->atomType() == (*atoms1)[i]->atomType()) {
        fragAt = (*atoms1)[i];
        break;
      }
    }
    // Find translation vector that shifts fragAt to atom1 location
    crds1 = fragAt->coordinates();
    crds2 = atom1->coordinates();
    ax = crds1[0] - crds2[0];
    ay = crds1[1] - crds2[1];
    az = crds1[2] - crds2[2];
    for (i=0; i<numAtoms1; i++) {
      atmi = (*atoms1)[i];
      crds1 = atmi->coordinates();
      crds[0] = crds1[0] + 0.5*RSEP*dx - ax;
      crds[1] = crds1[1] + 0.5*RSEP*dy - ay;
      crds[2] = crds1[2] + 0.5*RSEP*dz - az;
      atmi->coordinates(crds);
    }
    // Find atom in frag1 that corresponds to atom2 in TS
    for (i=0; i<numAtoms2; i++) {
      if (atom2->atomType() == (*atoms2)[i]->atomType()) {
        fragAt = (*atoms2)[i];
        break;
      }
    }
    // Find translation vector that shifts fragAt to atom1 location
    crds1 = fragAt->coordinates();
    crds2 = atom2->coordinates();
    ax = crds1[0] - crds2[0];
    ay = crds1[1] - crds2[1];
    az = crds1[2] - crds2[2];
    for (i=0; i<numAtoms2; i++) {
      atmi = (*atoms2)[i];
      crds2 = atmi->coordinates();
      crds[0] = crds2[0] - 0.5*RSEP*dx - ax;
      crds[1] = crds2[1] - 0.5*RSEP*dy - ay;
      crds[2] = crds2[2] - 0.5*RSEP*dz - az;
      atmi->coordinates(crds);
    }
  }

  if (atomsTS) delete atomsTS;
  if (atoms1) delete atoms1;
  if (atoms2) delete atoms2;
}


/**
 * Align fragment as closely as possible with corresponding atoms in
 * Transition State
 */
void NWDirdy::alignFragment(Fragment *frag)
{
  Fragment *trans =  p_dirdyTask->getFragment(DirDyVTSTTask::TRANSITION);
  vector<TAtm*> *tatoms = trans->atoms();
  vector<TAtm*> *atoms = frag->atoms();

  int numTAtoms = trans->numAtoms();
  int numAtoms = frag->numAtoms();

  double zero = 0.0;
  FFCoordinates *coords = new FFCoordinates(numAtoms,3,&zero);
  FFCoordinates *tcoords = new FFCoordinates(numAtoms,3,&zero);
  double **crds = coords->coordinates();
  double **tcrds = tcoords->coordinates();

  int i, j, k;
  string AtIdx, TAtIdx;
  for (i=0; i<numAtoms; i++) {
    AtIdx = (*atoms)[i]->atomType();
    for (j=0; j<numTAtoms; j++) {
      TAtIdx = (*tatoms)[j]->atomType();
      if (AtIdx == TAtIdx) {
        for (k=0; k<3; k++) {
          crds[i][k] = (*atoms)[i]->coordinates()[k];
          tcrds[i][k] = (*tatoms)[j]->coordinates()[k];
        }
      }
    }
  }

  CGAlignmentFunction *objective = new CGAlignmentFunction;
  objective->setTargetCoordinates(tcoords);
  objective->setInitialFragmentCoordinates(coords);

  FFCoordinates *transf = new FFCoordinates(2,3,&zero);
  transf->coordinates()[0][0] = 0.335283;
  transf->coordinates()[0][1] = 0.181271;
  transf->coordinates()[0][2] = 0.727288;
  // Must initialize this memory because certain platforms (64-bit Scientific
  // Linux 4.2 for instance) will blow up and start putting out "nan" values
  // everywhere and go into an infinite loop
  // GDB and BJP 6/8/09
  transf->coordinates()[1][0] = 0.0;
  transf->coordinates()[1][1] = 0.0;
  transf->coordinates()[1][2] = 0.0;
  CGMinimizer CGMin(objective,transf);
  transf = CGMin.optimize(0.0001, 10, 1.0, 0.1, 10);
  FFCoordinates *result = new FFCoordinates(numAtoms,3,&zero);
  objective->getFragmentCoordinates(transf,result);

  // Set fragment coordinates to transformed coordinates
  double x, y, z;
  for (i=0; i<numAtoms; i++) {
    x = result->row(i)[0];
    y = result->row(i)[1];
    z = result->row(i)[2];
    (*atoms)[i]->coordinates(x,y,z);
  }

  delete result;
  delete transf;
  delete coords;
  delete tcoords;
  delete atoms;
  delete tatoms;
  delete trans;
  delete objective;
}


/**
 * Process user close button.
 * Create a window close event where save processing is handled.
 */
void NWDirdy::OnExitClick( wxCommandEvent& event )
{
    // This generates window close so no need to repeat save code here.
    Close(true);

    event.Skip();
}


/**
 * Make sure changes are saved if any.
 * @todo prompt them about unsaved changes instead of just doing it
 */
void NWDirdy::OnCloseWindow( wxCloseEvent& event )
{
   doQuit(false);
}


/**
 * Re-generate the input file from the model and upload it to the data server.
 * @throw IOException if the input file generation failed for some reason or
 *       we couldn't upload the data.
 */
bool NWDirdy::generateInputFile() throw(IOException)
{
  if (!p_dirdyTask->removeInputFiles()) {
    p_feedback->setMessage("Unable to remove input file before regeneration.",
                           WxFeedback::WARNING);
  }

  SDirectory *dir =
    TempStorage::getTempJobDirectory(p_dirdyTask->getURL().getFilePathTail().c_str());
  string path = dir->path();

  //  Find all geometry files
  string ts,r1,r2,p1,p2;
  bool hasTrans = checkComponent(DirDyVTSTTask::TRANSITION);
  bool hasReact1 = checkComponent(DirDyVTSTTask::REACT1);
  bool hasReact2 = checkComponent(DirDyVTSTTask::REACT2);
  bool hasProd1 = checkComponent(DirDyVTSTTask::PROD1);
  bool hasProd2 = checkComponent(DirDyVTSTTask::PROD2);
  Fragment *ts_frag = 0;
  Fragment *r1_frag = 0;
  Fragment *r2_frag = 0;
  Fragment *p1_frag = 0;
  Fragment *p2_frag = 0;

  // Get fragments and align them with transition state atoms
  if (hasTrans) {
    ts_frag = p_dirdyTask->getFragment(DirDyVTSTTask::TRANSITION);
  }

  if (!checkUniqueTSAtomIds(ts_frag)) {
    return false;
  }

  if (hasReact1) {
    r1_frag = p_dirdyTask->getFragment(DirDyVTSTTask::REACT1);
  }
  if (hasReact2) {
    r2_frag = p_dirdyTask->getFragment(DirDyVTSTTask::REACT2);
  }

  if (!checkUniqueAtomIds(r1_frag, r2_frag, ts_frag, true)) {
    return false;
  }

  if (hasReact1) {
    alignFragment(r1_frag);
  }
  if (hasReact2) {
    alignFragment(r2_frag);
  }
  if (hasReact1 && hasReact2) {
    separateFragments(r1_frag, r2_frag, ts_frag);
  }

  if (hasProd1) {
    p1_frag = p_dirdyTask->getFragment(DirDyVTSTTask::PROD1);
  }
  if (hasProd2) {
    p2_frag = p_dirdyTask->getFragment(DirDyVTSTTask::PROD2);
  }

  if (!checkUniqueAtomIds(p1_frag, p2_frag, ts_frag, false)) {
    return false;
  }

  if (hasProd1) {
    alignFragment(p1_frag);
  }
  if (hasProd2) {
    alignFragment(p2_frag);
  }
  if (hasProd1 && hasProd2) {
    separateFragments(p1_frag, p2_frag, ts_frag);
  }

  // Write out transition state and reactants and products
  if (hasTrans) {
    ts = path;
    ts.append("/transition.frag");
    ofstream outfile(ts.c_str());
    ts_frag->dumpMVM(outfile);
    ((NWDirdyModel*)p_model)->setLinearTS(ts_frag->isLinear());
    ((NWDirdyModel*)p_model)->setNumAtomsTS(ts_frag->numAtoms());
    outfile.flush();
    outfile.close();
  }
  if (hasReact1) {
    r1 = path;
    r1.append("/reactant1.frag");
    ofstream outfile(r1.c_str());
    r1_frag->dumpMVM(outfile);
    ((NWDirdyModel*)p_model)->setLinearReact1(r1_frag->isLinear());
    ((NWDirdyModel*)p_model)->setNumAtomsReact1(r1_frag->numAtoms());
    outfile.flush();
    outfile.close();
  } 
  if (hasReact2) {
    r2 = path;
    r2.append("/reactant2.frag");
    ofstream outfile(r2.c_str());
    r2_frag->dumpMVM(outfile);
    ((NWDirdyModel*)p_model)->setLinearReact2(r2_frag->isLinear());
    ((NWDirdyModel*)p_model)->setNumAtomsReact2(r2_frag->numAtoms());
    outfile.flush();
    outfile.close();
  }
  if (hasProd1) {
    p1 = path;
    p1.append("/product1.frag");
    ofstream outfile(p1.c_str());
    p1_frag->dumpMVM(outfile);
    ((NWDirdyModel*)p_model)->setLinearProd1(p1_frag->isLinear());
    ((NWDirdyModel*)p_model)->setNumAtomsProd1(p1_frag->numAtoms());
    outfile.flush();
    outfile.close();
  }
  if (hasProd2) {
    p2 = path;
    p2.append("/product2.frag");
    ofstream outfile(p2.c_str());
    p2_frag->dumpMVM(outfile);
    ((NWDirdyModel*)p_model)->setLinearProd2(p2_frag->isLinear());
    ((NWDirdyModel*)p_model)->setNumAtomsProd2(p2_frag->numAtoms());
    outfile.flush();
    outfile.close();
  }

  if (ts_frag) delete ts_frag;
  if (r1_frag) delete r1_frag;
  if (r2_frag) delete r2_frag;
  if (p1_frag) delete p1_frag;
  if (p2_frag) delete p2_frag;

  // Get NWDirDy VTST parameters
  string spath = path;
  spath.append("/path.param");
  ofstream sOutfile(spath.c_str());
  sOutfile << p_dirdyTask->serializeModel(p_model);
  sOutfile.flush();
  sOutfile.close();

  // Get general basis set
  NWDirdyModel& model = getNWDirdyModel();
  bool useExpCoeff = model.getExpCoeff();

  TGBSConfig *genGbsConfig = p_dirdyTask->DavCalculation::gbsConfig("general");
  if (genGbsConfig != (TGBSConfig*)0)
  {
    // Produce NWChem formatted general theory basis set file
    const JCode* code = CodeFactory::lookup("NWChem");
    ostrstream stdStream;
    stdStream << genGbsConfig->dump(code->name().c_str(), !useExpCoeff);
    stdStream << ends;
    char* charData = stdStream.str();

    string gbpath = path;
    gbpath.append("/general.basis");
    ofstream gbOutFile(gbpath.c_str());
    if (gbOutFile && charData!=NULL && charData[0]!='\0') {
      gbOutFile << charData;
      gbOutFile.close();
    }
  }

  useExpCoeff = model.getSPExpCoeff();
  TGBSConfig *spGbsConfig = p_dirdyTask->DavCalculation::gbsConfig("single");
  if (spGbsConfig != (TGBSConfig*)0)
  {
    // Produce NWChem formatted single point theory basis set file
    const JCode* code = CodeFactory::lookup("NWChem");
    ostrstream stdStream;
    stdStream << spGbsConfig->dump(code->name().c_str(), !useExpCoeff);
    stdStream << ends;
    char* charData = stdStream.str();

    string spbpath = path;
    spbpath.append("/single.basis");
    ofstream spbOutFile(spbpath.c_str());
    if (spbOutFile && charData!=NULL && charData[0]!='\0') {
      spbOutFile << charData;
      spbOutFile.close();
    }
  }

  // Produce general theory parameter file
  string genParamPath = path;
  genParamPath.append("/general.param");
  ofstream genOs(genParamPath.c_str());
  if (genOs) {
    if (p_generalGUIValues != (GUIValues*)0)
      p_generalGUIValues->dumpKeyVals(genOs);

    // Add the calculation name for a title card
    // Without this the output fu30 file contains null characters in
    // the first line
    genOs << "title: " << p_dirdyTask->getName() << endl;

    string annotation = p_dirdyTask->getName();
    Resource *resource = dynamic_cast<Resource*>(p_dirdyTask);
    if (resource != (Resource*)0) {
      string annoprop = resource->getProp(VDoc::getEcceNamespace() +
                                          ":annotation");

      // remove any whitespace from the annoation including newlines
      // this is necessary because the annotation editor in the Organizer
      // is rather casual about whitespace and we want to make sure we
      // get something valid for this field for the input file
      STLUtil::stripLeadingAndTrailingWhiteSpace(annoprop);

      // If you clear an annotation in the Organizer, it actually leaves
      // a newline character
      if (annoprop != "") {
        if (annoprop.size() > 80)
          annoprop.resize(80);
        annotation = annoprop;
      }
    }
    genOs << "annotation: " << annotation << endl;


    // Throw in the transition state spin multiplicity and open shells
    SpinMult::SpinMultEnum spinMult =
                           p_dirdyTask->DavCalculation::spinMultiplicity();
    genOs << "multiplicity: " << SpinMult::toString(spinMult) << endl;
    genOs << "openshells: " <<
                            p_dirdyTask->DavCalculation::openShells() << endl;

    genOs.close();
  }

  // Produce single point theory parameter file
  string spParamPath = path;
  spParamPath.append("/single.param");
  ofstream spOs(spParamPath.c_str());
  if (spOs) {
    if (p_singleGUIValues != (GUIValues*)0)
      p_singleGUIValues->dumpKeyVals(spOs);

    spOs.close();
  }

  // Get the input generator script
  string inputGenFile;
  const JCode *codecap = p_dirdyTask->application();
  if (!codecap) {
    throw IOException("Could not retrieve NWDirDyVTST's EDML file.", WHERE);
  }
  codecap->get_string("InputGenerator",inputGenFile);
  if (inputGenFile.empty()) {
    throw IOException("Could not find RX input generator file name", WHERE);
  }

  SFile *tmpInput = TempStorage::getTempFile();
  string cmd = Ecce::ecceDataControllersPath();
  cmd += "/" + inputGenFile;

  string args = " -p";
  if (checkComponent(DirDyVTSTTask::TRANSITION)) {
    args.append("t");
  }
  if (checkComponent(DirDyVTSTTask::REACT1)) {
    args.append("c");
  }
  if (checkComponent(DirDyVTSTTask::REACT2)) {
    args.append("d");
  }
  if (checkComponent(DirDyVTSTTask::PROD1)) {
    args.append("e");
  }
  if (checkComponent(DirDyVTSTTask::PROD2)) {
    args.append("f");
  }
  if (genGbsConfig != (TGBSConfig*)0) {
    args.append("g");
  }
  if (spGbsConfig != (TGBSConfig*)0) {
    args.append("h");
  }
  if (p_generalGUIValues != (GUIValues*)0) {
    args.append("i");
  }
  if (p_singleGUIValues != (GUIValues*)0) {
    args.append("j");
  }
  cmd += args;
  cmd += " > ";
  cmd += tmpInput->path();

// cout << "*** input file generation command (" << cmd << ")" << endl;

  chdir(path.c_str());
  CommandWrapper syscmd(cmd);
  syscmd.execute();

  // change working directory to the bin directory so other system
  // calls work such as invoking the theory details dialog
  string binDir = Ecce::ecceHome();
  binDir += "/";
  binDir += getenv("ECCE_SYSDIR");
  binDir += "bin";
  chdir(binDir.c_str());

  // copy input file to web server
  ifstream ifs(tmpInput->path().c_str());
  if (!p_dirdyTask->putInputFile(codecap->getCodeFile(JCode::PRIMARY_INPUT).name(), &ifs)) {
    p_feedback->setMessage("Input file copy to web server failed.",
       WxFeedback::ERROR);
    ifs.close();
    return false;
  }
  ifs.close();

  tmpInput->remove();
  delete tmpInput;

  dir->remove();
  delete dir;

  return true;
}


bool NWDirdy::checkUniqueTSAtomIds(Fragment *ts)
{
  bool ret = true;

  vector<TAtm*> *atoms = ts->atoms();
  int ival;
  for (int it=0; it<(int)ts->numAtoms(); it++) {
    if (!StringConverter::toInt((*atoms)[it]->atomType(), ival) ||
        ival!=it+1) {
      ret = false;
      p_feedback->setMessage("Transition state atoms do not have the proper "
                             "unique atom types!  Unable to generate input "
                             "file.", WxFeedback::ERROR);
      break;
    }
  }

  return ret;
}


bool NWDirdy::checkUniqueAtomIds(Fragment *frag1, Fragment *frag2,
                                Fragment *ts, const bool& reactFlag)
{
  bool ret = true;

  string fragType = reactFlag? "reactant": "product";

  int frag1AtomCount = 0;
  vector<TAtm*> *frag1Atoms = 0;
  if (frag1 != (Fragment*)0) {
    frag1AtomCount = frag1->numAtoms();
    frag1Atoms = frag1->atoms();
  }

  int frag2AtomCount = 0;
  vector<TAtm*> *frag2Atoms = 0;
  if (frag2 != (Fragment*)0) {
    frag2AtomCount = frag2->numAtoms();
    frag2Atoms = frag2->atoms();
  }

  if (frag1AtomCount + frag2AtomCount == (int)ts->numAtoms()) {
    EcceSortedVector<int, less<int> > sortedTypes;

    int it, ival;
    for (it=0; it<frag1AtomCount; it++) {
      if (StringConverter::toInt((*frag1Atoms)[it]->atomType(), ival)) {
        sortedTypes.push_back(ival);
      } else {
        ret = false;
        p_feedback->setMessage("Unable to generate input file.  Each atom"
                               " for the first " + fragType +
                               " must have an atom type assigned which is an"
                               " integer value that uniquely maps to a"
                               " transition state atom type!",
                               WxFeedback::ERROR);
        return ret;
      }
    }

    for (it=0; it<frag2AtomCount; it++) {
      if (StringConverter::toInt((*frag2Atoms)[it]->atomType(), ival)) {
        sortedTypes.push_back(ival);
      } else {
        ret = false;
        p_feedback->setMessage("Unable to generate input file.  Each atom"
                               " for the second " + fragType +
                               " must have an atom type assigned which is an"
                               " integer value that uniquely maps to a"
                               " transition state atom type!",
                               WxFeedback::ERROR);
        return ret;
      }
    }

    // check that the sorted atom types are all accounted for
    if (sortedTypes.size() != ts->numAtoms()) {
      // this should not be possible unless duplicate values are
      // removed from the sorted vector
      ret = false;
      string capFragType = reactFlag? "Reactant": "Product";
      p_feedback->setMessage("Unable to generate input file.  " +
                             capFragType +" atom types do not correctly map"
                             " to transition state atom types!",
                             WxFeedback::ERROR);
    } else {
      for (it=0; it<(int)sortedTypes.size(); it++) {
        if (sortedTypes[it] != it+1) {
          ret = false;
          string capFragType = reactFlag? "Reactant": "Product";
          p_feedback->setMessage("Unable to generate input file.  " +
                                 capFragType +" atom types do not correctly map"
                                 " to transition state atom types!",
                                 WxFeedback::ERROR);
          break;
        }
      }
    }
  } else {
    ret = false;
    p_feedback->setMessage("Unable to generate input file.  Total number of " +
                           fragType + " atoms is different than the number of "
                           "transition state atoms!", WxFeedback::ERROR);
  }

  return ret;
}

/** 
 * Adds a new component to the reaction study.  The new component will
 * be linked to the DirDyVTST task, as an input provider.  If the 
 * component already exists, then this is a no-op.  
 * 
 * @param comp type of component to add
 *
 * @return returns true if the component is created or it already
 *         exists.  Otherwise false is returned.
 *
 */
Resource *NWDirdy::addComponent(DirDyVTSTTask::Component comp)
{
  string linkName = DirDyVTSTTask::componentTypeToString(comp);

  Resource *component = getComponent(comp);

  if (component == 0) {
    try {
      ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();
      ResourceType *resType = 
        rd.getResourceType(ResourceUtils::resourceTypeToString(
                                   ResourceDescriptor::RT_VIRTUAL_DOCUMENT),
                           ResourceUtils::contentTypeToString(
                                   ResourceDescriptor::CT_CALCULATION),
                           "NWChem");

      if (resType) {
        component = p_session->createChild(linkName, resType);
      }

      if (component == 0) {
        throw InvalidException("Resource::createChild() failed", WHERE);
      }

      p_session->addMember(component);

      // Add both input provider and primary path links
      p_session->addLink(component, 
                        p_dirdyTask,
                        Session::INPUT_PROVIDER,
                        linkName);
      p_session->addLink(component, 
                        p_dirdyTask,
                        Session::PRIMARY_PATH,
                        linkName);
      // Save model
      p_session->saveLinkbase();

      // Send notification so Organizer refreshes
      JMSPublisher publisher(WHOAMI);
      JMSMessage *msg = publisher.newMessage();
      msg->addProperty("url", component->getURL().toString());
      publisher.publish("ecce_url_created", *msg);
      delete msg;
    }
    catch (InvalidException ex) {

      p_feedback->setMessage("Failed to create reaction study task - " +
                             string(ex.what()));
      // The TaskModel::link() method may have thrown 
      // an exception, so will need to remove the unlinked
      // component from the model as well as from DAV (ie rollback).
      if (component != 0) {
        component->remove();
      }
    }
  }

  return component;
}


bool NWDirdy::checkComponent(DirDyVTSTTask::Component comp)
{
  bool ret = false;
  string linkName = DirDyVTSTTask::componentTypeToString(comp);

  vector<Resource*> sources = 
    p_session->getSources(p_dirdyTask, Session::INPUT_PROVIDER, linkName);

  // There can only be one source task for the link name
  if (!sources.empty()) {
    ret = true;
  }

  return ret;
}


bool NWDirdy::removeComponent(DirDyVTSTTask::Component comp)
{
  bool ret = false;
  string linkName = DirDyVTSTTask::componentTypeToString(comp);

  vector<Resource*> sources = 
    p_session->getSources(p_dirdyTask, Session::INPUT_PROVIDER, linkName);

  // There can only be one source task for the link name
  if (!sources.empty()) {
    // save the url because it will be gone after the remove()
    string url = sources[0]->getURL().toString();
    if (sources[0]->remove()) {
      // Sync the task model if the remove was successful
      ret = true;

      // Send notification so Organizer refreshes
      JMSPublisher publisher(WHOAMI);
      JMSMessage *msg = publisher.newMessage();
      msg->addProperty("url", url);
      publisher.publish("ecce_url_removed", *msg);
      delete msg;
    }
  }

  return ret;
}


Resource *NWDirdy::getComponent(DirDyVTSTTask::Component comp)
{
  Resource *ret = 0;
  string linkName = DirDyVTSTTask::componentTypeToString(comp);

  vector<Resource*> sources = 
    p_session->getSources(p_dirdyTask, Session::INPUT_PROVIDER, linkName);

  // There can only be one source task for the link name
  if (!sources.empty()) {
    ret = sources[0];
  }

  return ret;
}


void NWDirdy::initProgressDialog()
{
   if (p_progressDialog != 0 ) {
      delete p_progressDialog;
      p_progressDialog = 0;
   }

   p_progressDialog = new ewxProgressDialog("ECCE NWChem DirDyVTST Run", "",100,
         this, wxPD_AUTO_HIDE|wxPD_CAN_ABORT|wxPD_ELAPSED_TIME);
}


void NWDirdy::OnButtonNwdirdySetupClick(wxCommandEvent &event)
{
  p_setupGUI->Show(true);
  p_setupGUI->Raise();

  event.Skip();
}


void NWDirdy::OnButtonNwdirdyLaunchClick(wxCommandEvent &event)
{
  requestLauncher(p_dirdyTask->getURL());
  event.Skip();
}


/**
 * Changes gui to show the user whether there are unsaved changes.
 */
void NWDirdy::setSaveState(bool unsavedChanges)
{
   if (!p_ignoreCodeEventsFlag) {
      wxMenuItem *item = 0;
      wxMenuBar *menuBar = GetMenuBar();
      if (menuBar) {
         item = menuBar->FindItem(wxID_SAVE);
      }

      if (item) item->Enable(unsavedChanges);
      if (unsavedChanges) {
         if (p_dirdyTask->getState()==ResourceDescriptor::STATE_CREATED &&
             checkComponent(DirDyVTSTTask::TRANSITION) &&
             checkComponent(DirDyVTSTTask::REACT1) &&
             checkComponent(DirDyVTSTTask::PROD1) &&
             !p_dirdyTask->getDataFileNames(JCode::PRIMARY_INPUT).empty()) {
           setState( NWDirdy::VALID_SYSTEM );
           setRunState(ResourceDescriptor::STATE_READY);
         }
         p_feedback->setEditStatus(WxFeedback::MODIFIED);
      }  else {
         p_feedback->setEditStatus(WxFeedback::EDIT);
      }
   }
}


bool NWDirdy::doSave(const bool& reportFlag)
{
  bool ret = true;

  // if the state is already "ready", a new input file and save model
  // should be done.
  // if the state is created or ready, this method should check whether a
  // transition state, reactant, and product have been defined.
  // if so, save the model and attempt to generate an input file.
  // if input file generation succeeds, set the state to ready, otherwise
  // set state back to created
  if (p_dirdyTask->getState() <= ResourceDescriptor::STATE_READY &&
      checkComponent(DirDyVTSTTask::TRANSITION)) {
    if (saveModelAndInputFile(reportFlag)) {
      if (p_dirdyTask->getState() == ResourceDescriptor::STATE_CREATED) {
        p_dirdyTask->setState(ResourceDescriptor::STATE_READY);
        p_feedback->setRunState(ResourceDescriptor::STATE_READY);

        JMSPublisher publisher(WHOAMI);
        JMSMessage *msg = publisher.newMessage();
        msg->addProperty("url", p_dirdyTask->getURL().toString());
        msg->addProperty("state", ResourceUtils::stateToString(
                                          ResourceDescriptor::STATE_READY));
        publisher.publish("ecce_url_state",*msg);
        delete msg;
      }
    } else {
      ret = false;
      if (p_dirdyTask->getState() == ResourceDescriptor::STATE_READY) {
        p_dirdyTask->setState(ResourceDescriptor::STATE_CREATED);
        p_feedback->setRunState(ResourceDescriptor::STATE_CREATED);

        JMSPublisher publisher(WHOAMI);
        JMSMessage *msg = publisher.newMessage();
        msg->addProperty("url", p_dirdyTask->getURL().toString());
        msg->addProperty("state", ResourceUtils::stateToString(
                                          ResourceDescriptor::STATE_CREATED));
        publisher.publish("ecce_url_state",*msg);
        delete msg;
      }
    }
  }

  return ret;
}


/**
 * Set calculation state on data server, update feedback area,
 * and notify others that the state has changed.
 */
void NWDirdy::setRunState(ResourceDescriptor::RUNSTATE state) 
{
  p_dirdyTask->setState(state);
  p_feedback->setRunState(state);

  // Notify others...
  JMSPublisher publisher(WHOAMI);
  JMSMessage *msg = publisher.newMessage();
  msg->addProperty("url", p_dirdyTask->getURL().toString());
  msg->addProperty("state", ResourceUtils::stateToString(state));
  publisher.publish("ecce_url_state",*msg);
  delete msg;
}


/**
 * Determines if there are unsaved changes.
 * This just looks at the gui indicators so don't do something like
 * call isUnsavedState to determine whether to call setSaveState.
 */
bool NWDirdy::isUnsavedState()
{
   return (p_feedback->getEditStatus() == WxFeedback::MODIFIED);
}


/**
 * Process menu save button click.
 * The data is saved to the server and the gui updated.
 */
void NWDirdy::OnSaveClick( wxCommandEvent& event )
{
  processSave();
  event.Skip();
}


void NWDirdy::OnMenuFeedbackClick( wxCommandEvent& event )
{
  event.Skip();
}


void NWDirdy::OnMenuNwdirdyHelpClick( wxCommandEvent& event )
{
  event.Skip();
}


void NWDirdy::OnButtonNwdirdyFinalEditClick( wxCommandEvent& event )
{
  // if modified or no input file, start with a save
  if (p_feedback->getEditStatus()==WxFeedback::MODIFIED ||
      p_dirdyTask->getDataFileNames(JCode::PRIMARY_INPUT).empty()) {
     if (!doSave()) {
       return;
     }
  }

  istream* is = p_dirdyTask->getDataFile(JCode::PRIMARY_INPUT);
  if (is) {
     bool roFlag = p_feedback->getRunState() >
                   ResourceDescriptor::STATE_READY;
     if (roFlag) {
       p_feedback->setMessage("Input file is read only.  "
                              "Changes cannot be saved.", WxFeedback::INFO);
     } else {
       p_feedback->setMessage("In order for Final Edit changes to be"
                              " applied you must launch the task "
                              "without making any further changes.",
                              WxFeedback::INFO);
     }

     string text;
     StringConverter::streamToText(*is, text);
     delete is;

     WxEditSessionMgr sessionMgr;
     sessionMgr.edit(text, "testing", this, roFlag,"NWChem DirDyVTST Input");

  } else {
     p_feedback->setMessage("Input file not found.", WxFeedback::ERROR);
  }

  event.Skip();
}


/**
 * EditListener interface.
 * An edit session has terminated.
 */
void NWDirdy::processEditCompletion(const EditEvent& ee)
{
   wxBusyCursor busy;
   ifstream ifs(ee.filename.c_str());

   string infile;
   TypedFile tinfile;

   p_dirdyTask->getDataFile(JCode::PRIMARY_INPUT, tinfile);
   infile = tinfile.name();

   if (!p_dirdyTask->putInputFile(infile, &ifs)) {
      p_feedback->setMessage("Input file could not be copied back to DAV",
            WxFeedback::ERROR);
   } else {
      p_feedback->setMessage("The input file has been saved. Any changes "
            "you make to the gui will override your manual edits.");
   }

   ifs.close();
}


/**
 *  Event handlers for buttons that create calculations and bring up the editor
 *  and builder for calculations on the transition state, reactants, and
 *  products.
 */

/**
 * Bring up builder with Reactant 1 geometry
 */
void NWDirdy::OnBitmapbuttonDirdyBuildR1Click( wxCommandEvent& event )
{
  wxBusyCursor busy;

  Resource *r1Task = addComponent(DirDyVTSTTask::REACT1); 

  // Go ahead and save model if the Transistion-State task
  // was successfully added and bring up Calc Editor with the 
  // the Transition-state task as its context.
  if (r1Task != 0) {
    requestBuilder(r1Task->getURL());
  }

  event.Skip();
}


/**
 * Bring up calculation editor for Reactant 1
 */
void NWDirdy::OnButtonDirdyCalcR1Click( wxCommandEvent& event )
{
  wxBusyCursor busy;

  Resource *r1Task = addComponent(DirDyVTSTTask::REACT1); 

  // Go ahead and save model if the Transistion-State task
  // was successfully added and bring up Calc Editor with the 
  // the Transition-state task as its context.
  if (r1Task != 0) {
    ewxBitmapButton *codeButton = (ewxBitmapButton*)event.GetEventObject();
    unsigned long icode = (unsigned long)codeButton->GetClientData();
    string codename = "";
    if (icode > 0) {
      vector<string> codes = CodeFactory::getFullySupportedCodes();
      codename = codes[icode-1];
    }

    requestCalcEditor(r1Task->getURL(), codename);
  }

  event.Skip();
}


void NWDirdy::OnComboboxDirdyChargeR1Selected( wxCommandEvent& event )
{
  Fragment *tsfrag;
  if ((tsfrag = p_dirdyTask->getFragment(DirDyVTSTTask::TRANSITION))) {
    delete tsfrag;
    ewxComboBox *tscombo =
                 ((ewxComboBox*)FindWindow(ID_COMBOBOX_DIRDY_CHARGE_TS));
    string valstr = tscombo->GetValue().c_str();
    int tsval;
    if (StringConverter::toInt(valstr, tsval)) {
      // implement rules for conserving charge
      // reactant1 was changed, so update reactant2 as needed
      Fragment *r1frag;
      if ((r1frag = p_dirdyTask->getFragment(DirDyVTSTTask::REACT1))) {
        ewxComboBox *r1combo =
                     ((ewxComboBox*)FindWindow(ID_COMBOBOX_DIRDY_CHARGE_R1));
        valstr = r1combo->GetValue().c_str();
        int r1val = 0;
        if (!StringConverter::toInt(valstr, r1val)) {
          r1val = 0;
        }

        bool r1Flag = false;
        Fragment *r2frag = 0;
        if ((r2frag = p_dirdyTask->getFragment(DirDyVTSTTask::REACT2))) {
          ewxComboBox *r2combo =
                     ((ewxComboBox*)FindWindow(ID_COMBOBOX_DIRDY_CHARGE_R2));
          valstr = r2combo->GetValue().c_str();
          int r2val = 0;
          int r2set = tsval - r1val;
          if (StringConverter::toInt(valstr, r2val) && r2val!=r2set) {
            r2combo->SetValue(StringConverter::toString(r2set));
            r2frag->charge(r2set);
            if (restrictSpin(r2frag->numElectrons(),
                             (ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_R2))) {
              wxCommandEvent event;
              OnChoiceDirdySpinR2Selected(event);
            }
            ((ewxStaticText*)FindWindow(ID_STATIC_DIRDY_FORMULA_R2))->
                             SetLabel(r2frag->generateEmpiricalFormula());
          }
          delete r2frag;
        }

        // no reactant2 so reactant1 charge forced to ts charge
        else {
          if (r1val != tsval) {
            r1Flag = true;
            r1combo->SetValue(StringConverter::toString(tsval));
            r1frag->charge(tsval);
            if (restrictSpin(r1frag->numElectrons(),
                             (ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_R1))) {
              wxCommandEvent event;
              OnChoiceDirdySpinR1Selected(event);
            }
            ((ewxStaticText*)FindWindow(ID_STATIC_DIRDY_FORMULA_R1))->
                             SetLabel(r1frag->generateEmpiricalFormula());
          }
        }

        if (!r1Flag) {
          r1frag->charge(r1val);
          if (restrictSpin(r1frag->numElectrons(),
                           (ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_R1))) {
            wxCommandEvent event;
            OnChoiceDirdySpinR1Selected(event);
          }
          ((ewxStaticText*)FindWindow(ID_STATIC_DIRDY_FORMULA_R1))->
                           SetLabel(r1frag->generateEmpiricalFormula());
        }

        delete r1frag;

        if (p_feedback->getEditStatus() != WxFeedback::READONLY) {
          setSaveState(true);
        }
      }
    }
  }
}


void NWDirdy::OnComboboxDirdyChargeR1Enter( wxCommandEvent& event )
{
  OnComboboxDirdyChargeR1Selected(event);
}


void NWDirdy::OnChoiceDirdySpinR1Selected( wxCommandEvent& event )
{
  Fragment *tsfrag;
  if ((tsfrag = p_dirdyTask->getFragment(DirDyVTSTTask::TRANSITION))) {
    delete tsfrag;

    ewxChoice *tschoice = ((ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_TS));
    string spinstr = tschoice->GetStringSelection().c_str();
    int tsspin = (int)SpinMult::toSpinMult(spinstr) - 1;

    // implement rules for conserving charge
    // reactant1 was changed, so update reactant2 as needed
    Fragment *r1frag;
    if ((r1frag = p_dirdyTask->getFragment(DirDyVTSTTask::REACT1))) {
      ewxChoice *r1choice = ((ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_R1));
      spinstr = r1choice->GetStringSelection().c_str();
      int r1spin = (int)SpinMult::toSpinMult(spinstr) - 1;

      Fragment *r2frag = 0;
      if ((r2frag = p_dirdyTask->getFragment(DirDyVTSTTask::REACT2))) {
        ewxChoice *r2choice = ((ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_R2));
        spinstr = r2choice->GetStringSelection().c_str();
        int r2spin = (int)SpinMult::toSpinMult(spinstr) - 1;

        // magic to find a valid spin for reactant2
        for (int itry = r2spin % 2; itry<=8; itry+=2) {
          if (abs(r1spin-itry)<=tsspin && tsspin<=r1spin+itry) {
            if (r2spin != itry) {
              r2choice->SetStringSelection(
                      SpinMult::toString((SpinMult::SpinMultEnum)(itry+1)));
            }
            break;
          }
        }

      // no reactant2 so the reactant1 spin is set to the same
      // spin as the transition state
      } else {
        if (r1spin != tsspin) {
          r1choice->SetStringSelection(
                    SpinMult::toString((SpinMult::SpinMultEnum)(tsspin+1)));
        }
      }
    }

    if (p_feedback->getEditStatus() != WxFeedback::READONLY) {
      setSaveState(true);
    }
  }
}


/**
 * Bring up builder with Reactant 2 geometry
 */
void NWDirdy::OnBitmapbuttonDirdyBuildR2Click( wxCommandEvent& event )
{
  wxBusyCursor busy;

  Resource *r2Task = addComponent(DirDyVTSTTask::REACT2); 

  // Go ahead and save model if the Transistion-State task
  // was successfully added and bring up Calc Editor with the 
  // the Transition-state task as its context.
  if (r2Task != 0) {
    requestBuilder(r2Task->getURL());
  }

  event.Skip();
}


/**
 * Bring up calculation editor for Reactant 2
 */
void NWDirdy::OnButtonDirdyCalcR2Click( wxCommandEvent& event )
{
  wxBusyCursor busy;

  Resource *r2Task = addComponent(DirDyVTSTTask::REACT2); 

  // Go ahead and save model if the Transistion-State task
  // was successfully added and bring up Calc Editor with the 
  // the Transition-state task as its context.
  if (r2Task != 0) {
    ewxBitmapButton *codeButton = (ewxBitmapButton*)event.GetEventObject();
    unsigned long icode = (unsigned long)codeButton->GetClientData();
    string codename = "";
    if (icode > 0) {
      vector<string> codes = CodeFactory::getFullySupportedCodes();
      codename = codes[icode-1];
    }

    requestCalcEditor(r2Task->getURL(), codename);
  }

  event.Skip();
}


void NWDirdy::OnComboboxDirdyChargeR2Selected( wxCommandEvent& event )
{
  Fragment *tsfrag;
  if ((tsfrag = p_dirdyTask->getFragment(DirDyVTSTTask::TRANSITION))) {
    delete tsfrag;
    ewxComboBox *tscombo =
                 ((ewxComboBox*)FindWindow(ID_COMBOBOX_DIRDY_CHARGE_TS));
    string valstr = tscombo->GetValue().c_str();
    int tsval;
    if (StringConverter::toInt(valstr, tsval)) {
      // implement rules for conserving charge
      // reactant2 was changed, so update reactant1 as needed
      Fragment *r2frag;
      if ((r2frag = p_dirdyTask->getFragment(DirDyVTSTTask::REACT2))) {
        ewxComboBox *r2combo =
                     ((ewxComboBox*)FindWindow(ID_COMBOBOX_DIRDY_CHARGE_R2));
        valstr = r2combo->GetValue().c_str();
        int r2val = 0;
        if (!StringConverter::toInt(valstr, r2val)) {
          r2val = 0;
        }

        bool r2Flag = false;
        Fragment *r1frag = 0;
        if ((r1frag = p_dirdyTask->getFragment(DirDyVTSTTask::REACT1))) {
          ewxComboBox *r1combo =
                     ((ewxComboBox*)FindWindow(ID_COMBOBOX_DIRDY_CHARGE_R1));
          valstr = r1combo->GetValue().c_str();
          int r1val = 0;
          int r1set = tsval - r2val;
          if (StringConverter::toInt(valstr, r1val) && r1val!=r1set) {
            r1combo->SetValue(StringConverter::toString(r1set));
            r1frag->charge(r1set);
            if (restrictSpin(r1frag->numElectrons(),
                             (ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_R1))) {
              wxCommandEvent event;
              OnChoiceDirdySpinR1Selected(event);
            }
            ((ewxStaticText*)FindWindow(ID_STATIC_DIRDY_FORMULA_R1))->
                             SetLabel(r1frag->generateEmpiricalFormula());
          }
          delete r1frag;
        }

        // no reactant1 so reactant2 charge forced to ts charge
        else {
          if (r2val != tsval) {
            r2Flag = true;
            r2combo->SetValue(StringConverter::toString(tsval));
            r2frag->charge(tsval);
            if (restrictSpin(r2frag->numElectrons(),
                             (ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_R2))) {
              wxCommandEvent event;
              OnChoiceDirdySpinR2Selected(event);
            }
            ((ewxStaticText*)FindWindow(ID_STATIC_DIRDY_FORMULA_R2))->
                             SetLabel(r2frag->generateEmpiricalFormula());
          }
        }

        if (!r2Flag) {
          r2frag->charge(r2val);
          if (restrictSpin(r2frag->numElectrons(),
                           (ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_R2))) {
            wxCommandEvent event;
            OnChoiceDirdySpinR2Selected(event);
          }
          ((ewxStaticText*)FindWindow(ID_STATIC_DIRDY_FORMULA_R2))->
                           SetLabel(r2frag->generateEmpiricalFormula());
        }

        delete r2frag;

        if (p_feedback->getEditStatus() != WxFeedback::READONLY) {
          setSaveState(true);
        }
      }
    }
  }
}


void NWDirdy::OnComboboxDirdyChargeR2Enter( wxCommandEvent& event )
{
  OnComboboxDirdyChargeR2Selected(event);
}


void NWDirdy::OnChoiceDirdySpinR2Selected( wxCommandEvent& event )
{
  Fragment *tsfrag;
  if ((tsfrag = p_dirdyTask->getFragment(DirDyVTSTTask::TRANSITION))) {
    delete tsfrag;

    ewxChoice *tschoice = ((ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_TS));
    string spinstr = tschoice->GetStringSelection().c_str();
    int tsspin = (int)SpinMult::toSpinMult(spinstr) - 1;

    // implement rules for conserving charge
    // reactant2 was changed, so update reactant1 as needed
    Fragment *r2frag;
    if ((r2frag = p_dirdyTask->getFragment(DirDyVTSTTask::REACT2))) {
      ewxChoice *r2choice = ((ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_R2));
      spinstr = r2choice->GetStringSelection().c_str();
      int r2spin = (int)SpinMult::toSpinMult(spinstr) - 1;

      Fragment *r1frag = 0;
      if ((r1frag = p_dirdyTask->getFragment(DirDyVTSTTask::REACT1))) {
        ewxChoice *r1choice = ((ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_R1));
        spinstr = r1choice->GetStringSelection().c_str();
        int r1spin = (int)SpinMult::toSpinMult(spinstr) - 1;

        // magic to find a valid spin for reactant1
        for (int itry = r1spin % 2; itry<=8; itry+=2) {
          if (abs(itry-r2spin)<=tsspin && tsspin<=itry+r2spin) {
            if (r1spin != itry) {
              r1choice->SetStringSelection(
                        SpinMult::toString((SpinMult::SpinMultEnum)(itry+1)));
            }
            break;
          }
        }

      // no reactant1 so the reactant2 spin is set to the same
      // spin as the transition state
      } else {
        if (r2spin != tsspin) {
          r2choice->SetStringSelection(
                    SpinMult::toString((SpinMult::SpinMultEnum)(tsspin+1)));
        }
      }
    }

    if (p_feedback->getEditStatus() != WxFeedback::READONLY) {
      setSaveState(true);
    }
  }
}


void NWDirdy::OnBitmapbuttonDirdyBuildTsClick( wxCommandEvent& event )
{
  wxBusyCursor busy;

  Resource *tsTask = addComponent(DirDyVTSTTask::TRANSITION); 

  // Go ahead and save model if the Transistion-State task
  // was successfully added and bring up Calc Editor with the 
  // the Transition-state task as its context.
  if (tsTask != 0) {
    requestBuilder(tsTask->getURL());
  }

  event.Skip();
}


/**
 * Launches the Calc Editor with the Transition-State task as its
 * context.  If the Transition-State task does not exist then
 * one will be created.
 */
void NWDirdy::OnButtonDirdyCalcTsClick( wxCommandEvent& event )
{
  wxBusyCursor busy;

  Resource *tsTask = addComponent(DirDyVTSTTask::TRANSITION); 

  // Go ahead and save model if the Transistion-State task
  // was successfully added and bring up Calc Editor with the 
  // the Transition-state task as its context.
  if (tsTask != 0) {
    ewxBitmapButton *codeButton = (ewxBitmapButton*)event.GetEventObject();
    unsigned long icode = (unsigned long)codeButton->GetClientData();
    string codename = "";
    if (icode > 0) {
      vector<string> codes = CodeFactory::getFullySupportedCodes();
      codename = codes[icode-1];
    }

    requestCalcEditor(tsTask->getURL(), codename);
  }

  event.Skip();
}


void NWDirdy::OnComboboxDirdyChargeTsSelected( wxCommandEvent& event )
{
  Fragment *tsfrag;
  if ((tsfrag = p_dirdyTask->getFragment(DirDyVTSTTask::TRANSITION))) {
    ewxComboBox *tscombo =
                 ((ewxComboBox*)FindWindow(ID_COMBOBOX_DIRDY_CHARGE_TS));
    string valstr = tscombo->GetValue().c_str();
    int tsval;
    if (StringConverter::toInt(valstr, tsval)) {
      // implement rules for conserving charge
      // transition state was changed, so update reactant and product
      // charges as needed

      // first figure out how to balance the transition state charge between
      // two reactants/products in case that is needed
      int charge1, charge2;
      charge1 = charge2 = tsval / 2;
      if (tsval % 2 != 0) {
        charge2 = tsval>0? charge2+1: charge2-1;
      }

      // start with reactants
      int r1set = tsval;
      Fragment *r2frag;
      if ((r2frag = p_dirdyTask->getFragment(DirDyVTSTTask::REACT2))) {
        r1set = charge1;
        ewxComboBox *r2combo =
                   ((ewxComboBox*)FindWindow(ID_COMBOBOX_DIRDY_CHARGE_R2));
        valstr = r2combo->GetValue().c_str();
        int r2val = 0;
        if (StringConverter::toInt(valstr, r2val) && r2val!=charge2) {
          r2combo->SetValue(StringConverter::toString(charge2));
          r2frag->charge(charge2);
          if (restrictSpin(r2frag->numElectrons(),
                           (ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_R2))) {
            wxCommandEvent event;
            OnChoiceDirdySpinR2Selected(event);
          }
          ((ewxStaticText*)FindWindow(ID_STATIC_DIRDY_FORMULA_R2))->
                           SetLabel(r2frag->generateEmpiricalFormula());
        }
        delete r2frag;
      }

      Fragment *r1frag;
      if ((r1frag = p_dirdyTask->getFragment(DirDyVTSTTask::REACT1))) {
        ewxComboBox *r1combo =
                   ((ewxComboBox*)FindWindow(ID_COMBOBOX_DIRDY_CHARGE_R1));
        valstr = r1combo->GetValue().c_str();
        int r1val = 0;
        if (StringConverter::toInt(valstr, r1val) && r1val!=r1set) {
          r1combo->SetValue(StringConverter::toString(r1set));
          r1frag->charge(r1set);
          if (restrictSpin(r1frag->numElectrons(),
                           (ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_R1))) {
            wxCommandEvent event;
            OnChoiceDirdySpinR1Selected(event);
          }
          ((ewxStaticText*)FindWindow(ID_STATIC_DIRDY_FORMULA_R1))->
                           SetLabel(r1frag->generateEmpiricalFormula());
        }
        delete r1frag;
      }

      // now products
      int p1set = tsval;
      Fragment *p2frag;
      if ((p2frag = p_dirdyTask->getFragment(DirDyVTSTTask::PROD2))) {
        p1set = charge1;
        ewxComboBox *p2combo =
                     ((ewxComboBox*)FindWindow(ID_COMBOBOX_DIRDY_CHARGE_P2));
        valstr = p2combo->GetValue().c_str();
        int p2val = 0;
        if (StringConverter::toInt(valstr, p2val) && p2val!=charge2) {
          p2combo->SetValue(StringConverter::toString(charge2));
          p2frag->charge(charge2);
          if (restrictSpin(p2frag->numElectrons(),
                           (ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_P2))) {
            wxCommandEvent event;
            OnChoiceDirdySpinP2Selected(event);
          }
          ((ewxStaticText*)FindWindow(ID_STATIC_DIRDY_FORMULA_P2))->
                           SetLabel(p2frag->generateEmpiricalFormula());
        }
        delete p2frag;
      }

      Fragment *p1frag;
      if ((p1frag = p_dirdyTask->getFragment(DirDyVTSTTask::PROD1))) {
        ewxComboBox *p1combo =
                   ((ewxComboBox*)FindWindow(ID_COMBOBOX_DIRDY_CHARGE_P1));
        valstr = p1combo->GetValue().c_str();
        int p1val = 0;
        if (StringConverter::toInt(valstr, p1val) && p1val!=p1set) {
          p1combo->SetValue(StringConverter::toString(p1set));
          p1frag->charge(p1set);
          if (restrictSpin(p1frag->numElectrons(),
                           (ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_P1))) {
            wxCommandEvent event;
            OnChoiceDirdySpinP1Selected(event);
          }
          ((ewxStaticText*)FindWindow(ID_STATIC_DIRDY_FORMULA_P1))->
                           SetLabel(p1frag->generateEmpiricalFormula());
        }
        delete p1frag;
      }

      // finally, restrict transition state spin multiplicity based on charge
      tsfrag->charge(tsval);
      if (restrictSpin(tsfrag->numElectrons(),
                       (ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_TS), true)) {
        wxCommandEvent event;
        OnChoiceDirdySpinTsSelected(event);
      }
      ((ewxStaticText*)FindWindow(ID_STATIC_DIRDY_FORMULA_TS))->
                       SetLabel(tsfrag->generateEmpiricalFormula());

      if (p_feedback->getEditStatus() != WxFeedback::READONLY) {
        setSaveState(true);
      }
    }

    delete tsfrag;

    // immediately save any charge/spin changes to keep the reaction component
    // chemical systems consistent with the GUI
    processSave();
  }
}


void NWDirdy::OnComboboxDirdyChargeTsEnter( wxCommandEvent& event )
{
  OnComboboxDirdyChargeTsSelected(event);
}


void NWDirdy::OnChoiceDirdySpinTsSelected( wxCommandEvent& event )
{
  Fragment *tsfrag;
  if ((tsfrag = p_dirdyTask->getFragment(DirDyVTSTTask::TRANSITION))) {
    delete tsfrag;

    ewxChoice *tschoice = ((ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_TS));
    string spinstr = tschoice->GetStringSelection().c_str();
    int tsspin = (int)SpinMult::toSpinMult(spinstr) - 1;

    // implement rules for conserving charge

    // start with the reactants
    Fragment *r1frag;
    if ((r1frag = p_dirdyTask->getFragment(DirDyVTSTTask::REACT1))) {
      ewxChoice *r1choice = ((ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_R1));
      spinstr = r1choice->GetStringSelection().c_str();
      int r1spin = (int)SpinMult::toSpinMult(spinstr) - 1;

      Fragment *r2frag = 0;
      if ((r2frag = p_dirdyTask->getFragment(DirDyVTSTTask::REACT2))) {
        ewxChoice *r2choice = ((ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_R2));
        spinstr = r2choice->GetStringSelection().c_str();
        int r2spin = (int)SpinMult::toSpinMult(spinstr) - 1;

        // magic to find a valid spins for the reactants
        int r1try, r2try;
        for (r1try = r1spin % 2, r2try = r2spin % 2; r1try<=8 && r2try<=8;) {
          if (abs(r1try-r2try)<=tsspin && tsspin<=r1try+r2try) {
            if (r1spin != r1try) {
              r1choice->SetStringSelection(
                        SpinMult::toString((SpinMult::SpinMultEnum)(r1try+1)));
            }
            if (r2spin != r2try) {
              r2choice->SetStringSelection(
                        SpinMult::toString((SpinMult::SpinMultEnum)(r2try+1)));
            }
            break;
          }
          if (r1try <= r2try) {
            r1try += 2;
          } else {
            r2try += 2;
          }
        }

      // no reactant2 so the reactant1 spin is set to the same
      // spin as the transition state
      } else {
        if (r1spin != tsspin) {
          r1choice->SetStringSelection(
                    SpinMult::toString((SpinMult::SpinMultEnum)(tsspin+1)));
        }
      }
    }

    // now products
    Fragment *p1frag;
    if ((p1frag = p_dirdyTask->getFragment(DirDyVTSTTask::PROD1))) {
      ewxChoice *p1choice = ((ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_P1));
      spinstr = p1choice->GetStringSelection().c_str();
      int p1spin = (int)SpinMult::toSpinMult(spinstr) - 1;

      Fragment *p2frag = 0;
      if ((p2frag = p_dirdyTask->getFragment(DirDyVTSTTask::PROD2))) {
        ewxChoice *p2choice = ((ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_P2));
        spinstr = p2choice->GetStringSelection().c_str();
        int p2spin = (int)SpinMult::toSpinMult(spinstr) - 1;

        // magic to find valid spins for products
        int p1try, p2try;
        for (p1try = p1spin % 2, p2try = p2spin % 2; p1try<=8 && p2try<=8;) {
          if (abs(p1try-p2try)<=tsspin && tsspin<=p1try+p2try) {
            if (p1spin != p1try) {
              p1choice->SetStringSelection(
                        SpinMult::toString((SpinMult::SpinMultEnum)(p1try+1)));
            }
            if (p2spin != p2try) {
              p2choice->SetStringSelection(
                        SpinMult::toString((SpinMult::SpinMultEnum)(p2try+1)));
            }
            break;
          }
          if (p1try <= p2try) {
            p1try += 2;
          } else {
            p2try += 2;
          }
        }

      // no product2 so the product1 spin is set to the same
      // spin as the transition state
      } else {
        if (p1spin != tsspin) {
          p1choice->SetStringSelection(
                    SpinMult::toString((SpinMult::SpinMultEnum)(tsspin+1)));
        }
      }
    }

    p_setupGUI->restrictTheories(getSpinMult(ID_CHOICE_DIRDY_SPIN_TS), false);

    if (p_feedback->getEditStatus() != WxFeedback::READONLY) {
      setSaveState(true);
    }
  }
}


/**
 *  Create Reactant geometries based on selection in builder
 */
void NWDirdy::OnButtonDirdyCreateReactClick( wxCommandEvent& event )
{
  wxBusyCursor busy;

  if (p_model) {

    Fragment *TS = p_dirdyTask->getFragment(DirDyVTSTTask::TRANSITION);

    DavCalculation *calc1 = p_dirdyTask->
                              getComponentTask(DirDyVTSTTask::REACT1);
    DavCalculation *calc2 = p_dirdyTask->
                              getComponentTask(DirDyVTSTTask::REACT2);

    if ((calc1!=(DavCalculation*)0 && 
         calc1->getState()>=ResourceDescriptor::STATE_SUBMITTED) ||
        (calc2!=(DavCalculation*)0 && 
         calc2->getState()>=ResourceDescriptor::STATE_SUBMITTED)) {
      p_feedback->setMessage("Calculations have already been performed on "
                  "reactants.  Must reset these calculation states to create "
                  "new reactants.", WxFeedback::ERROR);
      return;
    }

    if (calc1 != (DavCalculation*)0) {
      ewxMessageDialog prompt(this, "Reactants already exist. Do you wish to "
         "redefine the reactants?", "ECCE Create Reactants",wxOK|wxCANCEL);
      if (prompt.ShowModal() == wxCANCEL) {
        return;
      }
    }
    if (TS != (Fragment*)0) { 
      // Add component
      Resource *tsTask = addComponent(DirDyVTSTTask::TRANSITION); 

      // Assign integer names to atoms
      /*
      int i, natoms = TS->numAtoms();
      TAtm *atom;
      char buf[5];
      buf[4] = '\0';
      for (i=0; i<natoms; i++) {
        atom = TS->atomRef(i);
        if (i+1 < 10) {
          sprintf(buf,"   %d",i+1);
        } else if (i+1 < 100) {
          sprintf(buf,"  %d",i+1);
        } else if (i+1 < 1000) {
          sprintf(buf," %d",i+1);
        } else if (i+1 < 10000) {
          sprintf(buf,"%d",i+1);
        } else {
          ewxMessageDialog prompt(this, "Too many atoms in transition state",
              "ECCE Create Reactants",wxOK);
          bool status = prompt.ShowModal();
        }
      }
      */

      // Go ahead and save model if the Transistion-State task
      // was successfully added and bring up Calc Editor with the 
      // the Transition-state task as its context.
      getNWDirdyModel().setProductsFlag(false);
      if (tsTask != 0) {
        requestBuilder(tsTask->getURL(),"getselection");
      }
      delete TS;
    } else {
      ewxMessageDialog prompt(this, "You must create the transition state "
         "prior to creating reactants.", "ECCE Create Reactants", wxOK);
      (void)prompt.ShowModal();
    }
  }

  (void)doSave();
  event.Skip();
}


/**
 *  Create Product geometries based on selection in builder
 */
void NWDirdy::OnButtonDirdyCreateProdClick( wxCommandEvent& event )
{
  wxBusyCursor busy;

  if (p_model) {

    Fragment *TS = p_dirdyTask->getFragment(DirDyVTSTTask::TRANSITION);

    DavCalculation *calc1 = p_dirdyTask->
                              getComponentTask(DirDyVTSTTask::PROD1);
    DavCalculation *calc2 = p_dirdyTask->
                              getComponentTask(DirDyVTSTTask::PROD2);

    if ((calc1!=(DavCalculation*)0 && 
         calc1->getState()>=ResourceDescriptor::STATE_SUBMITTED) ||
        (calc2!=(DavCalculation*)0 && 
         calc2->getState()>=ResourceDescriptor::STATE_SUBMITTED)) {
      p_feedback->setMessage("Calculations have already been performed on "
                  "products.  Must reset these calculation states to create "
                  "new products.", WxFeedback::ERROR);
      return;
    }

    if (calc1 != (DavCalculation*)0) {
      ewxMessageDialog prompt(this, "Products already exist. Do you wish to "
         "redefine the products?", "ECCE Create Products", wxOK|wxCANCEL);
      if (prompt.ShowModal() == wxCANCEL) {
        return;
      }
    }
    if (TS != (Fragment*)0) { 

      // Add component
      Resource *tsTask = addComponent(DirDyVTSTTask::TRANSITION); 

      // Assign integer names to atoms
      /*
      int i, natoms = TS->numAtoms();
      TAtm *atom;
      char buf[5];
      buf[4] = '\0';
      for (i=0; i<natoms; i++) {
        atom = TS->atomRef(i);
        if (i+1 < 10) {
          sprintf(buf,"   %d",i+1);
        } else if (i+1 < 100) {
          sprintf(buf,"  %d",i+1);
        } else if (i+1 < 1000) {
          sprintf(buf," %d",i+1);
        } else if (i+1 < 10000) {
          sprintf(buf,"%d",i+1);
        } else {
          ewxMessageDialog prompt(this, "Too many atoms in transition state",
              "ECCE Create Products",wxOK);
          bool status = prompt.ShowModal();
        }
      }
      */

      // Go ahead and save model if the Transistion-State task
      // was successfully added and bring up Calc Editor with the 
      // the Transition-state task as its context.
      getNWDirdyModel().setProductsFlag(true);
      if (tsTask != 0) {
        requestBuilder(tsTask->getURL(),"getselection");
      }
      delete TS;
    } else {
      ewxMessageDialog prompt(this, "You must create the transition state "
         "prior to creating products.", "ECCE Create Products", wxOK);
      (void)prompt.ShowModal();
    }
  }

  (void)doSave();
  event.Skip();
}


/**
 * Bring up builder with Product 1 geometry
 */
void NWDirdy::OnBitmapbuttonDirdyBuildP1Click( wxCommandEvent& event )
{
  wxBusyCursor busy;

  Resource *p1Task = addComponent(DirDyVTSTTask::PROD1); 

  // Go ahead and save model if the Transistion-State task
  // was successfully added and bring up Calc Editor with the 
  // the Transition-state task as its context.
  if (p1Task != 0) {
    requestBuilder(p1Task->getURL());
  }

  event.Skip();
}


/**
 * Bring up calculation editor for Product 1
 */
void NWDirdy::OnButtonDirdyCalcP1Click( wxCommandEvent& event )
{
  wxBusyCursor busy;

  Resource *p1Task = addComponent(DirDyVTSTTask::PROD1); 

  // Go ahead and save model if the Transistion-State task
  // was successfully added and bring up Calc Editor with the 
  // the Transition-state task as its context.
  if (p1Task != 0) {
    ewxBitmapButton *codeButton = (ewxBitmapButton*)event.GetEventObject();
    unsigned long icode = (unsigned long)codeButton->GetClientData();
    string codename = "";
    if (icode > 0) {
      vector<string> codes = CodeFactory::getFullySupportedCodes();
      codename = codes[icode-1];
    }

    requestCalcEditor(p1Task->getURL(), codename);
  }

  event.Skip();
}


void NWDirdy::OnComboboxDirdyChargeP1Selected( wxCommandEvent& event )
{
  Fragment *tsfrag;
  if ((tsfrag = p_dirdyTask->getFragment(DirDyVTSTTask::TRANSITION))) {
    delete tsfrag;
    ewxComboBox *tscombo =
                 ((ewxComboBox*)FindWindow(ID_COMBOBOX_DIRDY_CHARGE_TS));
    string valstr = tscombo->GetValue().c_str();
    int tsval;
    if (StringConverter::toInt(valstr, tsval)) {
      // implement rules for conserving charge
      // product1 was changed, so update product2 as needed
      Fragment *p1frag;
      if ((p1frag = p_dirdyTask->getFragment(DirDyVTSTTask::PROD1))) {
        ewxComboBox *p1combo =
                     ((ewxComboBox*)FindWindow(ID_COMBOBOX_DIRDY_CHARGE_P1));
        valstr = p1combo->GetValue().c_str();
        int p1val = 0;
        if (!StringConverter::toInt(valstr, p1val)) {
          p1val = 0;
        }

        bool p1Flag = false;
        Fragment *p2frag;
        if ((p2frag = p_dirdyTask->getFragment(DirDyVTSTTask::PROD2))) {
          ewxComboBox *p2combo =
                     ((ewxComboBox*)FindWindow(ID_COMBOBOX_DIRDY_CHARGE_P2));
          valstr = p2combo->GetValue().c_str();
          int p2val = 0;
          int p2set = tsval - p1val;
          if (StringConverter::toInt(valstr, p2val) && p2val!=p2set) {
            p2combo->SetValue(StringConverter::toString(p2set));
            p2frag->charge(p2set);
            if (restrictSpin(p2frag->numElectrons(),
                             (ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_P2))) {
              wxCommandEvent event;
              OnChoiceDirdySpinP2Selected(event);
            }
            ((ewxStaticText*)FindWindow(ID_STATIC_DIRDY_FORMULA_P2))->
                             SetLabel(p2frag->generateEmpiricalFormula());
          }
          delete p2frag;
        }

        // no product2 so product1 charge forced to ts charge
        else {
          if (p1val != tsval) {
            p1Flag = true;
            p1combo->SetValue(StringConverter::toString(tsval));
            p1frag->charge(tsval);
            if (restrictSpin(p1frag->numElectrons(),
                             (ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_P1))) {
              wxCommandEvent event;
              OnChoiceDirdySpinP1Selected(event);
            }
            ((ewxStaticText*)FindWindow(ID_STATIC_DIRDY_FORMULA_P1))->
                             SetLabel(p1frag->generateEmpiricalFormula());
          }
        }

        if (!p1Flag) {
          p1frag->charge(p1val);
          if (restrictSpin(p1frag->numElectrons(),
                           (ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_P1))) {
            wxCommandEvent event;
            OnChoiceDirdySpinP1Selected(event);
          }
          ((ewxStaticText*)FindWindow(ID_STATIC_DIRDY_FORMULA_P1))->
                           SetLabel(p1frag->generateEmpiricalFormula());
        }

        delete p1frag;

        if (p_feedback->getEditStatus() != WxFeedback::READONLY) {
          setSaveState(true);
        }
      }
    }
  }
}


void NWDirdy::OnComboboxDirdyChargeP1Enter( wxCommandEvent& event )
{
  OnComboboxDirdyChargeP1Selected(event);
}


void NWDirdy::OnChoiceDirdySpinP1Selected( wxCommandEvent& event )
{
  Fragment *tsfrag;
  if ((tsfrag = p_dirdyTask->getFragment(DirDyVTSTTask::TRANSITION))) {
    delete tsfrag;

    ewxChoice *tschoice = ((ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_TS));
    string spinstr = tschoice->GetStringSelection().c_str();
    int tsspin = (int)SpinMult::toSpinMult(spinstr) - 1;

    // implement rules for conserving charge
    // product1 was changed, so update product2 as needed
    Fragment *p1frag;
    if ((p1frag = p_dirdyTask->getFragment(DirDyVTSTTask::PROD1))) {
      ewxChoice *p1choice = ((ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_P1));
      spinstr = p1choice->GetStringSelection().c_str();
      int p1spin = (int)SpinMult::toSpinMult(spinstr) - 1;

      Fragment *p2frag = 0;
      if ((p2frag = p_dirdyTask->getFragment(DirDyVTSTTask::PROD2))) {
        ewxChoice *p2choice = ((ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_P2));
        spinstr = p2choice->GetStringSelection().c_str();
        int p2spin = (int)SpinMult::toSpinMult(spinstr) - 1;

        // magic to find a valid spin for product2
        for (int itry = p2spin % 2; itry<=8; itry+=2) {
          if (abs(p1spin-itry)<=tsspin && tsspin<=p1spin+itry) {
            if (p2spin != itry) {
              p2choice->SetStringSelection(
                        SpinMult::toString((SpinMult::SpinMultEnum)(itry+1)));
            }
            break;
          }
        }

      // no product2 so the product1 spin is set to the same
      // spin as the transition state
      } else {
        if (p1spin != tsspin) {
          p1choice->SetStringSelection(
                    SpinMult::toString((SpinMult::SpinMultEnum)(tsspin+1)));
        }
      }
    }

    if (p_feedback->getEditStatus() != WxFeedback::READONLY) {
      setSaveState(true);
    }
  }
}


/**
 * Bring up builder with Product 2 geometry
 */
void NWDirdy::OnBitmapbuttonDirdyBuildP2Click( wxCommandEvent& event )
{
  wxBusyCursor busy;

  Resource *p2Task = addComponent(DirDyVTSTTask::PROD2); 

  // Go ahead and save model if the Transistion-State task
  // was successfully added and bring up Calc Editor with the 
  // the Transition-state task as its context.
  if (p2Task != 0) {
    requestBuilder(p2Task->getURL());
  }

  event.Skip();
}


/**
 * Bring up calculation editor for Product 2
 */
void NWDirdy::OnButtonDirdyCalcP2Click( wxCommandEvent& event )
{
  wxBusyCursor busy;

  Resource *p2Task = addComponent(DirDyVTSTTask::PROD2); 

  // Go ahead and save model if the Transistion-State task
  // was successfully added and bring up Calc Editor with the 
  // the Transition-state task as its context.
  if (p2Task != 0) {
    ewxBitmapButton *codeButton = (ewxBitmapButton*)event.GetEventObject();
    unsigned long icode = (unsigned long)codeButton->GetClientData();
    string codename = "";
    if (icode > 0) {
      vector<string> codes = CodeFactory::getFullySupportedCodes();
      codename = codes[icode-1];
    }

    requestCalcEditor(p2Task->getURL(), codename);
  }

  event.Skip();
}


void NWDirdy::OnComboboxDirdyChargeP2Selected( wxCommandEvent& event )
{
  Fragment *tsfrag;
  if ((tsfrag = p_dirdyTask->getFragment(DirDyVTSTTask::TRANSITION))) {
    delete tsfrag;
    ewxComboBox *tscombo =
                 ((ewxComboBox*)FindWindow(ID_COMBOBOX_DIRDY_CHARGE_TS));
    string valstr = tscombo->GetValue().c_str();
    int tsval;
    if (StringConverter::toInt(valstr, tsval)) {
      // implement rules for conserving charge
      // product2 was changed, so update product1 as needed
      Fragment *p2frag;
      if ((p2frag = p_dirdyTask->getFragment(DirDyVTSTTask::PROD2))) {
        ewxComboBox *p2combo =
                     ((ewxComboBox*)FindWindow(ID_COMBOBOX_DIRDY_CHARGE_P2));
        valstr = p2combo->GetValue().c_str();
        int p2val = 0;
        if (!StringConverter::toInt(valstr, p2val)) {
          p2val = 0;
        }

        bool p2Flag = false;
        Fragment *p1frag = 0;
        if ((p1frag = p_dirdyTask->getFragment(DirDyVTSTTask::PROD1))) {
          ewxComboBox *p1combo =
                     ((ewxComboBox*)FindWindow(ID_COMBOBOX_DIRDY_CHARGE_P1));
          valstr = p1combo->GetValue().c_str();
          int p1val = 0;
          int p1set = tsval - p2val;
          if (StringConverter::toInt(valstr, p1val) && p1val!=p1set) {
            p1combo->SetValue(StringConverter::toString(p1set));
            p1frag->charge(p1set);
            if (restrictSpin(p1frag->numElectrons(),
                             (ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_P1))) {
              wxCommandEvent event;
              OnChoiceDirdySpinP1Selected(event);
            }
            ((ewxStaticText*)FindWindow(ID_STATIC_DIRDY_FORMULA_P1))->
                             SetLabel(p1frag->generateEmpiricalFormula());
          }
          delete p1frag;
        }

        // no product1 so product2 charge forced to ts charge
        else {
          if (p2val != tsval) {
            p2Flag = true;
            p2combo->SetValue(StringConverter::toString(tsval));
            p2frag->charge(tsval);
            if (restrictSpin(p2frag->numElectrons(),
                             (ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_P2))) {
              wxCommandEvent event;
              OnChoiceDirdySpinP2Selected(event);
            }
            ((ewxStaticText*)FindWindow(ID_STATIC_DIRDY_FORMULA_P2))->
                           SetLabel(p2frag->generateEmpiricalFormula());
          }
        }

        if (!p2Flag) {
          p2frag->charge(p2val);
          if (restrictSpin(p2frag->numElectrons(),
                           (ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_P2))) {
            wxCommandEvent event;
            OnChoiceDirdySpinP2Selected(event);
          }
          ((ewxStaticText*)FindWindow(ID_STATIC_DIRDY_FORMULA_P2))->
                           SetLabel(p2frag->generateEmpiricalFormula());
        }

        delete p2frag;

        if (p_feedback->getEditStatus() != WxFeedback::READONLY) {
          setSaveState(true);
        }
      }
    }
  }
}


void NWDirdy::OnComboboxDirdyChargeP2Enter( wxCommandEvent& event )
{
  OnComboboxDirdyChargeP2Selected(event);
}


void NWDirdy::OnChoiceDirdySpinP2Selected( wxCommandEvent& event )
{
  Fragment *tsfrag;
  if ((tsfrag = p_dirdyTask->getFragment(DirDyVTSTTask::TRANSITION))) {
    delete tsfrag;

    ewxChoice *tschoice = ((ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_TS));
    string spinstr = tschoice->GetStringSelection().c_str();
    int tsspin = (int)SpinMult::toSpinMult(spinstr) - 1;

    // implement rules for conserving charge
    // product2 was changed, so update product1 as needed
    Fragment *p2frag;
    if ((p2frag = p_dirdyTask->getFragment(DirDyVTSTTask::PROD2))) {
      ewxChoice *p2choice = ((ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_P2));
      spinstr = p2choice->GetStringSelection().c_str();
      int p2spin = (int)SpinMult::toSpinMult(spinstr) - 1;

      Fragment *p1frag = 0;
      if ((p1frag = p_dirdyTask->getFragment(DirDyVTSTTask::PROD1))) {
        ewxChoice *p1choice = ((ewxChoice*)FindWindow(ID_CHOICE_DIRDY_SPIN_P1));
        spinstr = p1choice->GetStringSelection().c_str();
        int p1spin = (int)SpinMult::toSpinMult(spinstr) - 1;

        // magic to find a valid spin for product1
        for (int itry = p1spin % 2; itry<=8; itry+=2) {
          if (abs(itry-p2spin)<=tsspin && tsspin<=itry+p2spin) {
            if (p1spin != itry) {
              p1choice->SetStringSelection(
                        SpinMult::toString((SpinMult::SpinMultEnum)(itry+1)));
            }
            break;
          }
        }

      // no product1 so the product2 spin is set to the same
      // spin as the transition state
      } else {
        if (p2spin != tsspin) {
          p2choice->SetStringSelection(
                    SpinMult::toString((SpinMult::SpinMultEnum)(tsspin+1)));
        }
      }
    }

    if (p_feedback->getEditStatus() != WxFeedback::READONLY) {
      setSaveState(true);
    }
  }
}


void NWDirdy::startGeneralTheoryApp(const bool& initFlag)
{
#if 0000
  holdMessages();
#endif

  const JCode* code = CodeFactory::lookup("NWChem");
  string theorycmd, runcmd;

  if (!code->getTheoryRunTypeEditorNames(theorycmd, runcmd))
    EE_RT_ASSERT(0, EE_FATAL,
                 "Theory and Runtype dialog executables not defined.");

  if (theorycmd != "") {
    // Initialize details dialog communication
    closeGeneralTheoryApp(true);

    p_generalTheoryOutFile = TempStorage::getTempFile();
    p_generalTheoryOutStream = new ofstream(p_generalTheoryOutFile->path().c_str());

    // Setup socket based server for wxPython detail dialog IPC
    // Create datagram socket, Let Wx select a port for us - (Service(0))
    wxIPV4address bindAddress;
    bindAddress.Service(0);
    p_generalTheoryInSocket = new wxDatagramSocket(bindAddress);

    // Check state of new socket, if its OK:
    //   Configure socket as non-blocking
    //   Set socket event handler and enable event notification
    //   Save port number for future reference
    if (p_generalTheoryInSocket->Ok()) {
      p_generalTheoryInSocket->SetFlags(wxSOCKET_NOWAIT);
      Connect(wxEVT_SOCKET,
              wxSocketEventHandler(NWDirdy::OnGeneralTheoryChange));
      p_generalTheoryInSocket->SetEventHandler(*this);
      p_generalTheoryInSocket->SetNotify(wxSOCKET_INPUT_FLAG);
      p_generalTheoryInSocket->Notify(true);
      p_generalTheoryInSocket->GetLocal(bindAddress);
      p_generalTheoryInPort = bindAddress.Service();
    }
    else {
      cerr << "General theory dialog IPC socket creation failed." << endl;
    }

    p_generalTheoryInFlag = false;
    p_generalTheoryInitFlag = false;

    string cmd = theorycmd;
    cmd += " ";
    cmd += p_generalTheoryOutFile->path() + " ";

    char buf[32];
    sprintf(buf, "%hd", p_generalTheoryInPort);
    cmd += buf;

    if (initFlag) {
      p_generalTheoryHoldFlag = true;
      cmd += " NO_GUIValues";
    } else {
      p_generalTheoryHoldFlag = false;
      cmd += " GUIValues";

      p_generalGUIValues->dump(*p_generalTheoryOutStream);
      *p_generalTheoryOutStream << "END_GUIValues" << endl << flush;
    }

    p_generalTheoryOutStream->close();
    delete p_generalTheoryOutStream;
    p_generalTheoryOutStream = (ofstream*)0;

    cmd += buildTheoryArgs("general");

//#define DEBUG
#ifdef DEBUG
cout << "Starting wxPython general theory dialog with (" << cmd << ")" << endl;
#endif
    string err;
    // if (!RCommand::bgcommand(cmd, err))
    if (system((cmd+"&").c_str()) < 0)
      p_feedback->setMessage("Unable to invoke theory details dialog",
                             WxFeedback::ERROR);
    else {
      // This inspired bit of logic nests an event loop inside the main event
      // loop for only processing the initialization messages from the
      // details dialogs.  This prevents the user from doing anything to the
      // main calced window such as starting the details dialog a second time
      // before the details dialogs are ready.

      // But we need some mechanism to recognize that the detail dialog apps
      // may be out to lunch and never come up since we are ignoring the status
      // as being unreliable (plus it is a good idea to have redundancy anyway
      // for something which will hang the caller forever).  This is done by
      // having a fixed timeout period that should it be called before the
      // "STARTED" message is received from the details apps will indicate that
      // there is a problem invoking them.

#if 0000
      p_theoryTimerId = XtAppAddTimeOut(tu_global_app_context, 30000,
                                        theoryStartTimer, this);
      while (!p_theoryInitFlag)
        if (XtAppPending(tu_global_app_context) &
                         (XtIMAlternateInput | XtIMTimer))
          XtAppProcessEvent(tu_global_app_context,XtIMAlternateInput|XtIMTimer);
#else
      // To-Do: Add wxTimer dervied class to add a timeout capability to
      // starting the details dialog.  See wxgui/wxtools/WxEditSessionMgr.C
      // and search for WxEditTimer for an example of how to do this with
      // a "helper" class.  For now, don't use a timeout and hope that
      // nothing hangs when bringing up the details dialog.
      p_generalTheoryHoldFlag = false;
#endif
    }
  }
#if 0000
  // don't do this unless we can get the inner event processing loop above
  // implemented in wxWidgets
  else
    gdmSumText::update(p_GUIValues);
#endif

#if 0000
  resumeMessaging();
#endif
}


void NWDirdy::startSingleTheoryApp(const bool& initFlag)
{
#if 0000
  holdMessages();
#endif

  const JCode* code = CodeFactory::lookup("NWChem");
  string theorycmd, runcmd;

  if (!code->getTheoryRunTypeEditorNames(theorycmd, runcmd))
    EE_RT_ASSERT(0, EE_FATAL,
                 "Theory and Runtype dialog executables not defined.");

  if (theorycmd != "") {
    // Initialize details dialog communication
    closeSingleTheoryApp(true);

    p_singleTheoryOutFile = TempStorage::getTempFile();
    p_singleTheoryOutStream = new ofstream(p_singleTheoryOutFile->path().c_str());

    // Setup socket based server for wxPython detail dialog IPC
    // Create datagram socket, Let Wx select a port for us - (Service(0))
    wxIPV4address bindAddress;
    bindAddress.Service(0);
    p_singleTheoryInSocket = new wxDatagramSocket(bindAddress);

    // Check state of new socket, if its OK:
    //   Configure socket as non-blocking
    //   Set socket event handler and enable event notification
    //   Save port number for future reference
    if (p_singleTheoryInSocket->Ok()) {
      p_singleTheoryInSocket->SetFlags(wxSOCKET_NOWAIT);
      Connect(wxEVT_SOCKET,
              wxSocketEventHandler(NWDirdy::OnSingleTheoryChange));
      p_singleTheoryInSocket->SetEventHandler(*this);
      p_singleTheoryInSocket->SetNotify(wxSOCKET_INPUT_FLAG);
      p_singleTheoryInSocket->Notify(true);
      p_singleTheoryInSocket->GetLocal(bindAddress);
      p_singleTheoryInPort = bindAddress.Service();
    }
    else {
      cerr << "Single Point theory dialog IPC socket creation failed." << endl;
    }

    p_singleTheoryInFlag = false;
    p_singleTheoryInitFlag = false;

    string cmd = theorycmd;
    cmd += " ";
    cmd += p_singleTheoryOutFile->path() + " ";

    char buf[32];
    sprintf(buf, "%hd", p_singleTheoryInPort);
    cmd += buf;

    if (initFlag) {
      p_singleTheoryHoldFlag = true;
      cmd += " NO_GUIValues";
    } else {
      p_singleTheoryHoldFlag = false;
      cmd += " GUIValues";

      p_singleGUIValues->dump(*p_singleTheoryOutStream);
      *p_singleTheoryOutStream << "END_GUIValues" << endl << flush;
    }

    p_singleTheoryOutStream->close();
    delete p_singleTheoryOutStream;
    p_singleTheoryOutStream = (ofstream*)0;

    cmd += buildTheoryArgs("single");

//#define DEBUG
#ifdef DEBUG
cout << "Starting wxPython single point theory dialog with (" << cmd << ")" << endl;
#endif
    string err;
    // if (!RCommand::bgcommand(cmd, err))
    if (system((cmd+"&").c_str()) < 0)
      p_feedback->setMessage("Unable to invoke theory details dialog",
                             WxFeedback::ERROR);
    else {
      // This inspired bit of logic nests an event loop inside the main event
      // loop for only processing the initialization messages from the
      // details dialogs.  This prevents the user from doing anything to the
      // main calced window such as starting the details dialog a second time
      // before the details dialogs are ready.

      // But we need some mechanism to recognize that the detail dialog apps
      // may be out to lunch and never come up since we are ignoring the status
      // as being unreliable (plus it is a good idea to have redundancy anyway
      // for something which will hang the caller forever).  This is done by
      // having a fixed timeout period that should it be called before the
      // "STARTED" message is received from the details apps will indicate that
      // there is a problem invoking them.

#if 0000
      p_theoryTimerId = XtAppAddTimeOut(tu_global_app_context, 30000,
                                        theoryStartTimer, this);
      while (!p_theoryInitFlag)
        if (XtAppPending(tu_global_app_context) &
                         (XtIMAlternateInput | XtIMTimer))
          XtAppProcessEvent(tu_global_app_context,XtIMAlternateInput|XtIMTimer);
#else
      // To-Do: Add wxTimer dervied class to add a timeout capability to
      // starting the details dialog.  See wxgui/wxtools/WxEditSessionMgr.C
      // and search for WxEditTimer for an example of how to do this with
      // a "helper" class.  For now, don't use a timeout and hope that
      // nothing hangs when bringing up the details dialog.
      p_singleTheoryHoldFlag = false;
#endif
    }
  }
#if 0000
  // don't do this unless we can get the inner event processing loop above
  // implemented in wxWidgets
  else
    gdmSumText::update(p_GUIValues);
#endif

#if 0000
  resumeMessaging();
#endif
}


void NWDirdy::OnGeneralTheoryChange(wxSocketEvent& event)
{
#if 000
cout << "**** OnGeneralTheoryChange handler called!!" << endl;
#endif

  static char databuf[8192];

  p_generalTheoryInSocket->Read(databuf, 8191);
  int nbytes = p_generalTheoryInSocket->LastCount();
  if ((!p_generalTheoryInSocket->Error() > 0) && (nbytes > 0)) {
    databuf[nbytes] = '\0';
    processGeneralTheoryInput(databuf);
  }
}


void NWDirdy::OnSingleTheoryChange(wxSocketEvent& event)
{
#if 000
cout << "**** OnSingleTheoryChange handler called!!" << endl;
#endif

  static char databuf[8192];

  p_singleTheoryInSocket->Read(databuf, 8191);
  int nbytes = p_singleTheoryInSocket->LastCount();
  if ((!p_singleTheoryInSocket->Error() > 0) && (nbytes > 0)) {
    databuf[nbytes] = '\0';
    processSingleTheoryInput(databuf);
  }
}


void NWDirdy::processGeneralTheoryInput(const char* databuf)
{
#if 111
  if (getenv("ECCE_NWDIRDY_DEBUG"))
    cout << "General theory data (" << databuf << ")" << endl;
#endif

  if (p_generalGUIValues->append(databuf)) {
    p_generalTheoryInFlag = true;
    if (!p_generalTheoryHoldFlag) {
#if 0000
      gdmSumText::update(p_GUIValues);
#endif
      if (p_generalTheoryInitFlag &&
          p_feedback->getEditStatus()!=WxFeedback::READONLY) {
        setSaveState(true);
#ifdef DEBUG
cout << "**** processGeneralTheoryInput set modified" << endl;
#endif
      }
    }
  } else if (strstr(databuf, "#READONLY") != NULL)
    p_generalTheoryInFlag = true;

  // These comparisons are not mutually exclusive (else if...) because a single
  // getTheoryInput invocation may contain multiple messages
  char* start;
  if ((start = (char*)strstr(databuf, "#STARTED")) != NULL) {
#if 0000
    XtRemoveTimeOut(p_theoryTimerId);
#endif

    // Grab the process id as the second argument
    start = strchr(start, ' ');
    if (start != NULL)
      p_generalTheoryPid = (int)strtol(start, NULL, 10);
    else
      p_generalTheoryPid = 0;
  }

  if (strstr(databuf, "#INITIALIZED") != NULL) {
    p_generalTheoryOutFile->remove();
    delete p_generalTheoryOutFile;

    p_generalTheoryInitFlag = true;

    ewxButton *detailsBtn = ((ewxButton*)p_setupGUI->FindWindow(
            NWDirdySetup::ID_BUTTON_NWDRDYSET_DTLS));
    detailsBtn->Enable(detailsReady() && p_generalTheoryInFlag);
    if (p_generalTheoryHoldFlag) {
      p_generalTheoryHoldFlag = false;
#if 0000
      gdmSumText::update(p_GUIValues);
#endif

      if (p_feedback->getEditStatus() != WxFeedback::READONLY) {
        setSaveState(true);
      }
    }
  }

  if (strstr(databuf, "#CLOSING") != NULL)
    closeGeneralTheoryApp(false);
}


void NWDirdy::processSingleTheoryInput(const char* databuf)
{
#if 111
  if (getenv("ECCE_NWDIRDY_DEBUG"))
    cout << "Single point theory data (" << databuf << ")" << endl;
#endif

  if (p_singleGUIValues->append(databuf)) {
    p_singleTheoryInFlag = true;
    if (!p_singleTheoryHoldFlag) {
#if 0000
      gdmSumText::update(p_GUIValues);
#endif
      if (p_singleTheoryInitFlag &&
          p_feedback->getEditStatus()!=WxFeedback::READONLY) {
        setSaveState(true);
#ifdef DEBUG
cout << "**** processSingleTheoryInput set modified" << endl;
#endif
      }
    }
  } else if (strstr(databuf, "#READONLY") != NULL)
    p_singleTheoryInFlag = true;

  // These comparisons are not mutually exclusive (else if...) because a single
  // getTheoryInput invocation may contain multiple messages
  char* start;
  if ((start = (char*)strstr(databuf, "#STARTED")) != NULL) {
#if 0000
    XtRemoveTimeOut(p_theoryTimerId);
#endif

    // Grab the process id as the second argument
    start = strchr(start, ' ');
    if (start != NULL)
      p_singleTheoryPid = (int)strtol(start, NULL, 10);
    else
      p_singleTheoryPid = 0;
  }

  if (strstr(databuf, "#INITIALIZED") != NULL) {
    p_singleTheoryOutFile->remove();
    delete p_singleTheoryOutFile;

    p_singleTheoryInitFlag = true;

    string spthry = getNWDirdyModel().getSPTheory();
    ewxButton *detailsBtn = ((ewxButton*)p_setupGUI->FindWindow(
            NWDirdySetup::ID_BUTTON_NWDRDYSET_SPDTLS));
    detailsBtn->Enable(spthry!="None" && detailsReady() &&p_singleTheoryInFlag);
    if (p_singleTheoryHoldFlag) {
      p_singleTheoryHoldFlag = false;
#if 0000
      gdmSumText::update(p_GUIValues);
#endif

      if (p_feedback->getEditStatus() != WxFeedback::READONLY) {
        setSaveState(true);
      }
    }
  }

  if (strstr(databuf, "#CLOSING") != NULL)
    closeSingleTheoryApp(false);
}


bool NWDirdy::detailsReady(void)
{
  bool ret = false;

  Fragment *frag = p_dirdyTask->DavCalculation::fragment();
  if (frag != (Fragment*)0) {
   ret = !frag->containsNubs();
   delete frag;
  }
  return ret;
}


string NWDirdy::buildTheoryArgs(const string& theoryType)
{
  string args;

  if (p_feedback->getRunState() > ResourceDescriptor::STATE_READY)
    args = " ReadOnly ";
  else
    args = " Writable ";

  args += p_debugFlag? "DebugOn \"": "DebugOff \"";

  string cat, thry;
  if (theoryType == "general") {
    cat = getNWDirdyModel().getCategory();
    thry = getNWDirdyModel().getTheory();
  } else {
    cat = getNWDirdyModel().getSPCategory();
    thry = getNWDirdyModel().getSPTheory();
  }

  // quote the theory and runtype args because they may contain special
  // characters such as parentheses that the shell will complain about
  args += cat + "\" \"";
  args += thry + "\" \"";

  // this is normally the runtype argument, but that isn't needed for
  // the DirDyVTST task so instead this is used in the theory details
  // dialog to determine whether to hide certain panels like the
  // geometry options and memory/disk limits
  args += "DirDyVTST\"";

  args += " \"" + p_dirdyTask->getName() + "\"";

  // hardwire the reaction study flag because it's always a reaction study
  // for the DirDyVTST task.
  args += " 1";

  char buf[80];
  Fragment *frag = p_dirdyTask->DavCalculation::fragment();
  if (frag != (Fragment*)0) {
    string symmetry = frag->pointGroup();
    strcpy(buf, symmetry.c_str());
    buf[0] = toupper(buf[0]);
    for (int idx=strlen(buf)-1; idx>=1; idx--)
        buf[idx] = tolower(buf[idx]);
  } else
    strcpy(buf, "C1");

  args += " \"";
  args += buf;

  int numElectrons, spin, numFrozenOrbs;
  int numOccupiedOrbs, numVirtualOrbs, numNormalModes;
  orbitalParams(frag, theoryType, numElectrons, spin, numFrozenOrbs,
                numOccupiedOrbs, numVirtualOrbs, numNormalModes);
  sprintf(buf, "%d %d %d %d %d %d",
          numElectrons, spin, numFrozenOrbs, numOccupiedOrbs,
          numVirtualOrbs, numNormalModes);

  args += "\" ";
  args += buf;

  if (frag != (Fragment*)0)
    delete frag;

  args += " \"";
  args += Color::WINDOW;
  args += "\" \"";
  args += Color::INPUT;
  args += "\" \"";
  args += Color::READONLY;
  args += "\"";

  return args;
}


bool NWDirdy::restrictSpin(const int& numElectrons, ewxChoice* choice,
                           const bool& tsFlag)
{
  bool ret = false;
  bool evenFlag = (numElectrons % 2 == 0);
  wxString spin = choice->GetStringSelection();
  int mult = 0;
  if (!spin.IsEmpty()) {
    mult = (int)SpinMult::toSpinMult(spin.c_str());
  }
  wxString currMult;

  switch (mult) {
    case 0:
      currMult = ""; break;
    case 1:
      currMult = "Singlet"; break;
    case 2:
      currMult = "Doublet"; break;
    case 3:
      currMult = "Triplet"; break;
    case 4:
      currMult = "Quartet"; break;
    case 5:
      currMult = "Quintet"; break;
    case 6:
      currMult = "Sextet"; break;
    case 7:
      currMult = "Septet"; break;
    case 8:
      currMult = "Octet"; break;
    case 9:
      currMult = "Nontet";
  }
  choice->Clear();

  if (evenFlag) {
    choice->Append("Singlet");
    choice->Append("Triplet");
    choice->Append("Quintet");
    choice->Append("Septet");
    choice->Append("Nontet");

    if (currMult=="" || currMult=="Doublet" || currMult=="Quartet" ||
        currMult=="Sextet" || currMult=="Octet") {
      ret = true;
      choice->SetStringSelection("Singlet");
      if (tsFlag && p_setupGUI!=(NWDirdySetup*)0) {
        p_setupGUI->restrictTheories(SpinMult::singlet, false);
      }
    } else {
      choice->SetStringSelection(currMult);
    }
  } else {
    choice->Append("Doublet");
    choice->Append("Quartet");
    choice->Append("Sextet");
    choice->Append("Octet");

    if (currMult=="" || currMult=="Singlet" || currMult=="Triplet" ||
        currMult=="Quintet" || currMult=="Septet" || currMult=="Nontet") {
      ret = true;
      choice->SetStringSelection("Doublet");
      if (tsFlag && p_setupGUI!=(NWDirdySetup*)0) {
        p_setupGUI->restrictTheories(SpinMult::doublet, false);
      }
    } else {
      choice->SetStringSelection(currMult);
    }
  }

  return ret;
}


void NWDirdy::orbitalParams(Fragment* frag, const string& theoryType,
                            int& numElectrons, int& spin,
                            int& numFrozenOrbs, int& numOccupiedOrbs,
                            int& numVirtualOrbs, int& numNormalModes)
{
  // Note:  Our definition for total number of orbitals is:
  // #TotalOrbs = #Occupied + #Unoccupied + #OpenShells
  // where #Unoccupied is equivalent to #Virtual

  // -1 is a flag to the theory dialogs that we can't determine a value
  // because it is dependent upon the basis set which hasn't been set yet.
  // otherwise we don't let them invoke the details dialogs until the
  // basis has been set and Theresa did not like that idea at all.
  numElectrons = 0;
  spin = 0;
  numFrozenOrbs = -1;
  numOccupiedOrbs = 0;
  numVirtualOrbs = -1;
  numNormalModes = 0;

  if (frag != (Fragment*)0) {
    // *** important value being set
    numElectrons = frag->nuclearCharge() - frag->charge();

    SpinMult::SpinMultEnum spinMult =
                           p_dirdyTask->DavCalculation::spinMultiplicity();
    // *** important value being set
    spin = (int)spinMult;

    vector<TAtm*> *atms = frag->atoms();

    int numAtoms = frag->numAtoms();

    // *** important value being set
    numNormalModes = 3*numAtoms;

    int coreElec = 0;
    for (int iat=0; iat<numAtoms; iat++)
      coreElec += coreElectrons((*atms)[iat]->atomicNumber());

    delete atms;  // delete vector, not atoms

    TGBSConfig *basis = p_dirdyTask->DavCalculation::gbsConfig(theoryType);
    if (basis != (TGBSConfig*)0) {
      TagCountMap* tcmap = frag->tagCountsSTL();

      // subtract off ECP electron contributions

      // *** important value being set
      numFrozenOrbs = (coreElec - basis->num_ecpCoreElectrons(*tcmap))/2;
    }

    string thry = getNWDirdyModel().getTheory();
    int openShells = 0;
    if (thry.find("RO")==0 || thry.find('U')==0)
      openShells = p_dirdyTask->DavCalculation::openShells();

    // *** important value being set
    numOccupiedOrbs = (numElectrons - openShells)/2;

    if (basis != (TGBSConfig*)0) {
      TagCountMap* tcmap = frag->tagCountsSTL();

      // *** important value being set
      numVirtualOrbs = basis->num_functions(*tcmap) -
                              numOccupiedOrbs - openShells;

      delete basis;
    }
  }
}


//////////////////////////////////////////////////////////////////////////////
// Description:
//  Returns the number of core electrons for an atom of the given
//  atomic number.
//////////////////////////////////////////////////////////////////////////////
unsigned long NWDirdy::coreElectrons(const unsigned long atomicNumber) const
{
  static unsigned long electrons[] = {0, 2, 10, 18, 36, 54, 86, 999};

  int ie;
  for (ie=1; atomicNumber > electrons[ie]; ie++);

  return (electrons[ie-1]);
}


void NWDirdy::closeGeneralTheoryApp(const bool& sendTerm)
{
  if (sendTerm && p_generalTheoryPid!=0)
    (void)kill(p_generalTheoryPid, SIGTERM);

  if (p_generalTheoryInPort != 0) {
    p_generalTheoryInSocket->Notify(false);
    p_generalTheoryInSocket->Destroy();
    p_generalTheoryInPort = 0;
  }
}


void NWDirdy::closeSingleTheoryApp(const bool& sendTerm)
{
  if (sendTerm && p_singleTheoryPid!=0)
    (void)kill(p_singleTheoryPid, SIGTERM);

  if (p_singleTheoryInPort != 0) {
    p_singleTheoryInSocket->Notify(false);
    p_singleTheoryInSocket->Destroy();
    p_singleTheoryInPort = 0;
  }
}


SpinMult::SpinMultEnum NWDirdy::tsSpinMultiplicity(void)
{
  SpinMult::SpinMultEnum spin = SpinMult::singlet;

  DavCalculation *tscalc;
  if ((tscalc = p_dirdyTask->getComponentTask(DirDyVTSTTask::TRANSITION))) {
    spin = getSpinMult(ID_CHOICE_DIRDY_SPIN_TS);
  }

  return spin;
}

int NWDirdy::tsCharge(void)
{
  int charge = 0;

  Fragment *tsfrag;
  if ((tsfrag = p_dirdyTask->getFragment(DirDyVTSTTask::TRANSITION))) {
    ewxComboBox *tscombo =
                 ((ewxComboBox*)FindWindow(ID_COMBOBOX_DIRDY_CHARGE_TS));
    string valstr = tscombo->GetValue().c_str();
    if (!StringConverter::toInt(valstr, charge)) {
      charge = 0;
    }
    delete tsfrag;
  }

  return charge;
}

int NWDirdy::tsNuclearCharge(void)
{
  int nuke = 0;

  Fragment *tsfrag;
  if ((tsfrag = p_dirdyTask->getFragment(DirDyVTSTTask::TRANSITION))) {
    nuke = tsfrag->nuclearCharge();
    delete tsfrag;
  }

  return nuke;
}

WxFeedback *NWDirdy::getFeedback(void)
{
  return p_feedback;
}

DirDyVTSTTask *NWDirdy::getDirDyVTSTTask(void)
{
  return p_dirdyTask;
}


