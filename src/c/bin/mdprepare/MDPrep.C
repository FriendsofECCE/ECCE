#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "MDPrep.H"
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

#include "util/BadValueException.H"
#include "util/BrowserHelp.H"
#include "util/CommandWrapper.H"
#include "util/Ecce.H"
#include "util/EcceException.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/Preferences.H"
#include "util/ResourceUtils.H"
#include "util/SDirectory.H"
#include "util/SFile.H"
#include "util/STLUtil.H"
#include "util/StringConverter.H"
#include "util/StringTokenizer.H"
#include "util/TempStorage.H"
#include "util/TypedFile.H"
#include "util/UnitFactory.H"
#include "util/EditEvent.H"

#include "tdat/Fragment.H"
#include "tdat/FragmentSummary.H"
#include "tdat/PrepareModel.H"
#include "tdat/RefMachine.H"
#include "tdat/Residue.H"
#include "tdat/TAtm.H"
#include "tdat/TPerTab.H"

#include "dsm/MachineConfig.H"
#include "dsm/CodeFactory.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/MdTask.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceTool.H"
#include "dsm/ResourceType.H"
#include "dsm/Session.H"

#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxComboBox.H"
#include "wxgui/ewxListBox.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxProgressDialog.H"
#include "wxgui/ewxRadioButton.H"
#include "wxgui/ewxRegexpValidator.H"
#include "wxgui/ewxSingleChoiceDialog.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxUnitHelper.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/WxEditSessionMgr.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/WxFFDataConfig.H"
#include "wxgui/WxJMSMessageDispatch.H"
#include "wxgui/WxJMSListener.H"
#include "wxgui/WxState.H"

#include "comm/RCommand.H"
#include "comm/Launch.H"
#include "comm/EcceShell.H"

#include "MDPrep.H"

static const wxString ATOMREGEXP = "[0-9]+:_[a-z,A-Z]+";

/** Used to subscribe to messaging, publish, identify */
static const string WHOAMI = MDPREPARE;

/**
 * Constructor.
 * Create widgets.  Some widgets like the feedback area are 
 * created in code.  Initialize GUI state and set up validators and
 * unit helpers.
 */
MDPrep::MDPrep(wxWindow* parent, wxWindowID id, const wxString& caption,
               const wxPoint& pos, const wxSize& size, long style) :
        MDPrepGUI(parent, id, caption, pos, size, style),
        WxJMSListener(), TaskApp(WHOAMI, 1000), WxDavAuth(0)
{
   EDSIFactory::addAuthEventListener(this);

   initializeMessaging();

   initializeModel();

   p_progressDialog = 0;

   p_ignoreCodeEventsFlag = false;

   p_touch = false;

   setState( MDPrep::NO_SYSTEM );

   p_feedback = ((WxFeedback*)FindWindow(ID_FEEDBACK));
   p_feedback->setSaveHandler(this);
   p_feedback->setDropHandler(this);
   p_feedback->setEditStatus(WxFeedback::EDIT);

   // Make Lisong happy--don't make feedback area extra tall
   // p_feedback->GetSizer()->SetMinSize(-1, 160);

   // quick access to the list of commands
   p_listBox = ((ewxListBox*)FindWindow(ID_LISTBOX_MDPREP_CMDS));

   initializeGUI();

   // restore settings - could optimize this to only do it the first time
   // but this is ok for now
   Preferences prefs(WHOAMI);
   restoreWindowSettings(WHOAMI, prefs, false);

   refreshGUI();
}



MDPrep::~MDPrep()
{
  delete p_msgDispatcher;
  if (p_progressDialog != 0) {
    delete p_progressDialog;
  }
}


/**
 * Sets up validators and unit helpers for input fields.
 */
void MDPrep::initializeGUI()
{
   // Get Registry
   ResourceDescriptor rs = ResourceDescriptor::getResourceDescriptor();

   // Set desktop icon
   SetIcon(wxIcon(ewxBitmap::pixmapFile(rs.getTool(WHOAMI)->getIcon()),
                                        wxBITMAP_TYPE_XPM));

   PrepareModel &model = getPrepareModel();
   ewxChoice *choice = 0;
   ewxTextCtrl *text = 0;
   ewxStaticText *label = 0;
   ewxStaticText *NOUNITLABEL = 0;
   ewxButton *button = 0;
   ewxNumericValidator validator;

   /////////////////////////////////////////////////////////////////////////////
   // Topology page
   /////////////////////////////////////////////////////////////////////////////
   // TODO assign better validators for these 
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_LNK_ATOM1));
   text->SetValidator( ewxRegexpValidator(ATOMREGEXP, "") );
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_LNK_ATOM2));
   text->SetValidator( ewxRegexpValidator(ATOMREGEXP, "") );
   button = ((ewxButton*)FindWindow(ID_BUTTON_MDPREP_LNK_SELECT));
   string tip = "Click this button to use selected atoms from the builder";
   tip.append(" to populate the atom fields");
   button->SetToolTip(tip.c_str());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_CINUM));
   validator.setHardRange("[0,)");
   validator.setValue(model.CINUM);
   text->SetValidator(validator);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_CISCALE));
   validator.setHardRange("(0.0,)");
   validator.setValue(model.CISCALE);
   text->SetValidator(validator);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_CIGRIDSZ));
   validator.setHardRange("[1,)");
   validator.setValue(model.CIGRID);
   text->SetValidator(validator);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_CIMINDIS));
   validator.setHardRange("(0.0,)");
   validator.setValue(model.CIMINDIS);
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_CIMINDIS));
   text->SetValidator(validator);

   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label) );
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC1));
   validator.setHardRange("[0,)");
   validator.setValue(model.FRAC1);
   text->SetValidator(validator);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC2));
   validator.setHardRange("[0,)");
   validator.setValue(model.FRAC2);
   text->SetValidator(validator);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC3));
   validator.setHardRange("[0,)");
   validator.setValue(model.FRAC3);
   text->SetValidator(validator);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC4));
   validator.setHardRange("[0,)");
   validator.setValue(model.FRAC4);
   text->SetValidator(validator);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC5));
   validator.setHardRange("[0,)");
   validator.setValue(model.FRAC5);
   text->SetValidator(validator);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC6));
   validator.setHardRange("[0,)");
   validator.setValue(model.FRAC6);
   text->SetValidator(validator);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC7));
   validator.setHardRange("[0,)");
   validator.setValue(model.FRAC7);
   text->SetValidator(validator);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC8));
   validator.setHardRange("[0,)");
   validator.setValue(model.FRAC8);
   text->SetValidator(validator);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC9));
   validator.setHardRange("[0,)");
   validator.setValue(model.FRAC9);
   text->SetValidator(validator);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC10));
   validator.setHardRange("[0,)");
   validator.setValue(model.FRAC10);
   text->SetValidator(validator);


   /////////////////////////////////////////////////////////////////////////////
   // Solvate page
   /////////////////////////////////////////////////////////////////////////////
   choice = ((ewxChoice*)FindWindow(ID_CHOICE_MDPREP_BCOPT));
   choice->SetSelection(model.BCOPT);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_EXPND));
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_EXPND));
   validator.setHardRange("[0,)");
   validator.setValue(model.EXPANDVOL);
   text->SetValidator(validator);
   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label) );

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLVTOL));
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_SLVTOL));
   text->SetValidator(ewxNumericValidator(0.0, 100.0, model.SOLVENTTOL));
   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label) );

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_MINSOLV));
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_MINSOLV));
   validator.setHardRange("[0,)");
   validator.setValue(model.TOUCH);
   text->SetValidator(validator);
   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label) );

   /**
    * Default values for these field are obtained from fragment
    */
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_EDGE));
   validator.setHardRange("[0,)");
   validator.setValue(0.0);
   text->SetValidator(validator);
   label = ((ewxStaticText*)FindWindow(ID_STATICTEXT_MDPREP_SLV_EDGE_UNIT));
   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label) );

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_RAD));
   validator.setHardRange("[0,)");
   validator.setValue(0.0);
   text->SetValidator(validator);
   label = ((ewxStaticText*)FindWindow(ID_STATICTEXT_MDPREP_SLV_SPHERE_UNIT));
   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label) );

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_BOXX));
   validator.setHardRange("(,)");
   validator.setValue(0.0);
   text->SetValidator(validator);
   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", NOUNITLABEL) );

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_BOXY));
   validator.setHardRange("(,)");
   validator.setValue(0.0);
   text->SetValidator(validator);
   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", NOUNITLABEL) );

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_BOXZ));
   validator.setHardRange("(,)");
   validator.setValue(0.0);
   text->SetValidator(validator);
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_SLV_BOX_UNIT));
   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label) );

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_MINDIST));
   validator.setHardRange("(0.0,)");
   validator.setValue(model.MINSEPARATION);
   text->SetValidator(validator);
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_SLV_MINDIST));
   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label) );

   /////////////////////////////////////////////////////////////////////////////
   // Translate page
   /////////////////////////////////////////////////////////////////////////////
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_TRNS_X));
   validator.setHardRange("(,)");
   validator.setValue(model.TRNSX);
   text->SetValidator(validator);
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_XTRNS_UNIT));
   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label) );

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_TRNS_Y));
   validator.setHardRange("(,)");
   validator.setValue(model.TRNSY);
   text->SetValidator(validator);
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_YTRNS_UNIT));
   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label) );

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_TRNS_Z));
   validator.setHardRange("(,)");
   validator.setValue(model.TRNSZ);
   text->SetValidator(validator);
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_ZTRNS_UNIT));
   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label) );

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_TRNS_INIT_AT));
   text->SetValidator( ewxRegexpValidator(ATOMREGEXP, ""));
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_TRNS_FNL_AT));
   text->SetValidator( ewxRegexpValidator(ATOMREGEXP, ""));


   /////////////////////////////////////////////////////////////////////////////
   // Orient page
   /////////////////////////////////////////////////////////////////////////////
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ORNT_X));
   validator.setHardRange("(,)");
   validator.setValue(model.ORIENTX);
   text->SetValidator(validator);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ORNT_Y));
   validator.setHardRange("(,)");
   validator.setValue(model.ORIENTY);
   text->SetValidator(validator);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ORNT_Z));
   validator.setHardRange("(,)");
   validator.setValue(model.ORIENTZ);
   text->SetValidator(validator);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ORNT_ANGLE));
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_ORNT_ANGLE_UNIT));
   validator.setHardRange("(-6.3,6.3)");
   validator.setValue(model.ANGLE);
   text->SetValidator(validator);
   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Angle", label) );

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ORNT_INIT_AT));
   text->SetValidator( ewxRegexpValidator(ATOMREGEXP, "") );
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ORNT_FNL_AT));
   text->SetValidator( ewxRegexpValidator(ATOMREGEXP, "") );

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ALGNI));
   text->SetValidator( ewxRegexpValidator(ATOMREGEXP, "") );
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ALGNJ));
   text->SetValidator( ewxRegexpValidator(ATOMREGEXP, "") );
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ALGNK));
   text->SetValidator( ewxRegexpValidator(ATOMREGEXP, "") );

   /////////////////////////////////////////////////////////////////////////////
   // Remove Solvent page
   /////////////////////////////////////////////////////////////////////////////
   /**
    * defaults for each of these fields are obtained from fragment dimensions
    */
   choice = ((ewxChoice*)FindWindow(ID_CHOICE_MDPREP_RMV_OPT));
   choice->SetSelection(model.RMVOPT);
   ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_XRANGE));
   check->SetValue(model.XRANGE);
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_XMIN));
   validator.setHardRange("(,)");
   validator.setValue(0.0);
   text->SetValidator(validator);

   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", NOUNITLABEL) );
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_XMAX));
   validator.setHardRange("(,)");
   validator.setValue(0.0);
   text->SetValidator(validator);
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_SLV_XUNIT));
   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label) );

   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_YRANGE));
   check->SetValue(model.YRANGE);
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_YMIN));
   validator.setHardRange("(,)");
   validator.setValue(0.0);
   text->SetValidator(validator);
   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", NOUNITLABEL) );

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_YMAX));
   validator.setHardRange("(,)");
   validator.setValue(0.0);
   text->SetValidator(validator);
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_SLV_YUNIT));
   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label) );

   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_ZRANGE));
   check->SetValue(model.ZRANGE);
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_ZMIN));
   validator.setHardRange("(,)");
   validator.setValue(0.0);
   text->SetValidator(validator);
   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", NOUNITLABEL) );

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_ZMAX));
   validator.setHardRange("(,)");
   validator.setValue(0.0);
   text->SetValidator(validator);
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_SLV_ZUNIT));
   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label) );

   /////////////////////////////////////////////////////////////////////////////
   // Modify Cell page
   /////////////////////////////////////////////////////////////////////////////
   /**
    * Set modify cell fields (defaults for box dimension come from fragment)
    */
   choice = ((ewxChoice*)FindWindow(ID_CHOICE_MDPREP_REPOPTS));
   choice->SetSelection(model.REPOPTS);
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SETX));
   validator.setHardRange("(0.0,)");
   validator.setValue(1.0);
   text->SetValidator(validator);
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_SETX));
   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label) );

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SETY));
   validator.setHardRange("(0.0,)");
   validator.setValue(1.0);
   text->SetValidator(validator);
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_SETY));
   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label) );

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SETZ));
   validator.setHardRange("(0.0,)");
   validator.setValue(1.0);
   text->SetValidator(validator);
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_SETZ));
   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label) );

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_MINRSEP));
   validator.setHardRange("(0.0,)");
   validator.setValue(model.MCLLMINSEP);
   text->SetValidator(validator);
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_MINRSEP));
   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label) );

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_IZDIS));
   validator.setHardRange("(0.0,)");
   validator.setValue(0.0);
   text->SetValidator(validator);
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_IZDIS));
   text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label) );

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_XREP));
   validator.setHardRange("[1,)");
   validator.setValue(model.INDMODCELL);
   text->SetValidator(validator);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_YREP));
   validator.setHardRange("[1,)");
   validator.setValue(model.INDMODCELL);
   text->SetValidator(validator);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ZREP));
   validator.setHardRange("[1,)");
   validator.setValue(model.INDMODCELL);
   text->SetValidator(validator);

   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_RANX));
   check->SetValue(model.RANDXYZ);
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_RANY));
   check->SetValue(model.RANDXYZ);
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_RANZ));
   check->SetValue(model.RANDXYZ);
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_INVCPY));
   check->SetValue(model.INVCOPY);
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_USEZBOX));
   check->SetValue(model.USEZBOX);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SKIPX));
   validator.setHardRange("[1,)");
   validator.setValue(model.INDMODCELL);
   text->SetValidator(validator);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SKIPY));
   validator.setHardRange("[1,)");
   validator.setValue(model.INDMODCELL);
   text->SetValidator(validator);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SKIPZ));
   validator.setHardRange("[1,)");
   validator.setValue(model.INDMODCELL);
   text->SetValidator(validator);

   /////////////////////////////////////////////////////////////////////////////
   // Details page
   /////////////////////////////////////////////////////////////////////////////
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_MEMHEAP));
   validator.setHardRange("[0,)");
   validator.setValue(model.HEAPMEM);
   text->SetValidator(validator);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_MEMSTK));
   validator.setHardRange("[0,)");
   validator.setValue(model.STACKMEM);
   text->SetValidator(validator);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_MEMGLBL));
   validator.setHardRange("[0,)");
   validator.setValue(model.GLOBALMEM);
   text->SetValidator(validator);

   p_edgePanel->Disable();
   p_spherePanel->Disable();
   p_slvBoxPanel->Disable();

   
   /////////////////////////////////////////////////////////////////////////////
   // Thermodynamics (PMF)  page
   /////////////////////////////////////////////////////////////////////////////
   int distIds[] = { ID_TEXTCTRL_PMF_DISTANCE1, wxID_STATIC_PMF_DISTANCE1_UNIT,
                     ID_TEXTCTRL_PMF_DISTANCE2, wxID_STATIC_PMF_DISTANCE2_UNIT};
   for (int i = 0; i < 4; i+=2) {
      text = ((ewxTextCtrl*)FindWindow(distIds[i]));
      label = ((ewxStaticText*)FindWindow(distIds[i+1]));
      validator.setHardRange("(,)");
      validator.setValue(0.0);
      text->SetValidator(validator);
      text->setUnitHelper(new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label));
   }

   int angleIds[] = { ID_TEXTCTRL_PMF_ANGLE1, wxID_STATIC_PMF_ANGLE1_UNIT,
                      ID_TEXTCTRL_PMF_ANGLE2, wxID_STATIC_PMF_ANGLE2_UNIT };
   for (int i = 0; i < 4; i+=2) {
      text = ((ewxTextCtrl*)FindWindow(angleIds[i]));
      label = ((ewxStaticText*)FindWindow(angleIds[i+1]));
      validator.setHardRange("(,)");
      validator.setValue(0.0);
      text->SetValidator(validator);
      text->setUnitHelper(new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Angle", label));
   }

   int forceIds[] = { ID_TEXTCTRL_PMF_FORCE1, ID_TEXTCTRL_PMF_FORCE2,
           ID_TEXTCTRL_PMF_FORCE1_DISTANCE, ID_TEXTCTRL_PMF_FORCE2_DISTANCE,
           ID_TEXTCTRL_PMF_FORCE1_ANGLE, ID_TEXTCTRL_PMF_FORCE2_ANGLE,
           ID_TEXTCTRL_PMF_FORCE1_TORSION, ID_TEXTCTRL_PMF_FORCE2_TORSION };
   int forceUnitIds[] = { wxID_STATIC_PMF_FORCE1_UNIT,
                          wxID_STATIC_PMF_FORCE2_UNIT,
                          wxID_STATIC_PMF_FORCE1_DISTANCE_UNIT,
                          wxID_STATIC_PMF_FORCE2_DISTANCE_UNIT,
                          wxID_STATIC_PMF_FORCE1_ANGLE_UNIT,
                          wxID_STATIC_PMF_FORCE2_ANGLE_UNIT,
                          wxID_STATIC_PMF_FORCE1_TORSION_UNIT,
                          wxID_STATIC_PMF_FORCE2_TORSION_UNIT };
   double forceDefaults[] = { 1e1, 1e1, 1e5, 1e5, 1e3, 1e3, 1e1, 1e1 };
   string forceUnits[] = { "Bond Force Constant", "Bond Force Constant",
                           "Bond Force Constant", "Bond Force Constant",
                           "Bend Force Constant", "Bend Force Constant",
                           "Torsion Force Constant", "Torsion Force Constant" };
   for (int i = 0; i < 8; i++) {
      text = ((ewxTextCtrl*)FindWindow(forceIds[i]));
      label = ((ewxStaticText*)FindWindow(forceUnitIds[i]));
      validator.setHardRange("(,)");
      validator.setValue(forceDefaults[i]);
      text->SetValidator(validator);
      text->setUnitHelper(new ewxUnitHelper(DEFAULT_UNIT_FAMILY,
                                            forceUnits[i], label));
   }
}

/**
 * Process drop into feedback drop site.
 */
void MDPrep::processDrop(const string& url)
{
   setContext(url);
}


void MDPrep::setState(PrepState state)
{

   // Visually reflect the state
   ewxButton *bBtn = ((ewxButton*)FindWindow(ID_BUTTON_MDPREP_BUILDER));
   ewxButton *localBtn = ((ewxButton*)FindWindow(ID_BUTTON_MDPREP_LOCAL));
   ewxButton *remoteBtn = ((ewxButton*)FindWindow(ID_BUTTON_MDPREP_REMOTE));
   ewxButton *editBtn = ((ewxButton*)FindWindow(ID_BUTTON_MDPREP_FINAL_EDIT));
   wxPanel *topology = ((wxPanel*)FindWindow(ID_PANEL_MDPREP_TOPOPT));
   wxPanel *coordinates = ((wxPanel*)FindWindow(ID_PANEL_COORDINATES));
   wxPanel *thermodynamics = ((wxPanel*)FindWindow(ID_PANEL_THERMODYNAMICS));
   wxPanel *details = ((wxPanel*)FindWindow(ID_PANEL_MDPREP_DETAILS));

   if (state == NO_SYSTEM) {
      // disable everything except builder access
      localBtn->Disable();
      remoteBtn->Disable();
      bBtn->Enable();
      editBtn->Disable();
      topology->Disable();
      coordinates->Disable();
      thermodynamics->Disable();
      details->Disable();

   } else if (state == VALID_SYSTEM)  {
      localBtn->Enable(true);
      remoteBtn->Enable(true);
      bBtn->Enable(true);
      editBtn->Enable(true);
      topology->Enable(true);
      coordinates->Enable(true);
      thermodynamics->Enable(true);
      details->Enable(true);

   } else if (state == PROVIDED_SYSTEM)  {
      localBtn->Enable(true);
      remoteBtn->Enable(true);
      bBtn->Enable(true);
      editBtn->Enable(true);
      topology->Enable(true);
      coordinates->Enable(true);
      thermodynamics->Enable(true);
      details->Enable(true);

   } else if (state == VALID_TOPOLOGY)  {
      localBtn->Enable(true);
      remoteBtn->Enable(true);
      editBtn->Enable(true);
      bBtn->Enable(true);
      //topology->Disable(); // probably want to enable but not sure so...
      coordinates->Enable(true);
      thermodynamics->Enable(true);
      details->Enable(true);

   } else {
      cerr << "Unknown MDPrep state " << state << endl;
   }

   p_state = state;
}



MDPrep::PrepState MDPrep::getState() const
{
   return p_state;
}

void MDPrep::showTranslatePanel()
{
   ewxRadioButton *radio;
   radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_MDPREP_TRNS_MOL));
   bool mol = radio->GetValue();
   radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_MDPREP_TRNS_SEG));
   bool seg = radio->GetValue();
   radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_MDPREP_TRNS_ATOM));
   bool atom = radio->GetValue();
   if (mol || seg) {
      p_translateVSizer->Show(p_translateMolID, true, true);
   } else {
      p_translateVSizer->Show(p_translateMolID, false, true);
   }
   if (atom) {
      p_translateVSizer->Show(p_translateAtomID, true, true);
   } else {
      p_translateVSizer->Show(p_translateAtomID, false, true);
   }
   p_translateVSizer->Layout();
   Fit();
}

void MDPrep::showOrientPanel()
{
  ewxRadioButton *radio;
  radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_MDPREP_ORNT_MOL));
  bool mol = radio->GetValue();
  radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_MDPREP_ORNT_SEG));
  bool seg = radio->GetValue();
  radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_MDPREP_ORNT_ATOM));
  bool atom = radio->GetValue();
  if (mol || seg) {
    p_orientVSizer->Show(p_orientMolID, true, true);
  } else {
    p_orientVSizer->Show(p_orientMolID, false, true);
  }
  if (atom) {
    p_orientVSizer->Show(p_orientAtomID, true, true);
  } else {
    p_orientVSizer->Show(p_orientAtomID, false, true);
  }
  p_orientVSizer->Layout();
  Fit();
}

void MDPrep::showSolvatePanel()
{
  ewxChoice *choice;
  choice = ((ewxChoice*)FindWindow(ID_CHOICE_MDPREP_SLV_GEOM));
  string selection = choice->GetStringSelection().c_str();
  if (selection == "Cube" || selection == "Truncated Octahedron") {
    p_solvateVSizer->Show(p_edgePanel, true, true);
    p_solvateVSizer->Show(p_spherePanel, false, true);
    p_solvateVSizer->Show(p_slvBoxPanel, false, true);
  } else if (selection == "Rectangular Parallelepiped") {
    p_solvateVSizer->Show(p_edgePanel, false, true);
    p_solvateVSizer->Show(p_spherePanel, false, true);
    p_solvateVSizer->Show(p_slvBoxPanel, true, true);
  } else if (selection == "Sphere") {
    p_solvateVSizer->Show(p_edgePanel, false, true);
    p_solvateVSizer->Show(p_spherePanel, true, true);
    p_solvateVSizer->Show(p_slvBoxPanel, false, true);
  } else {
    p_solvateVSizer->Show(p_edgePanel, false, true);
    p_solvateVSizer->Show(p_spherePanel, false, true);
    p_solvateVSizer->Show(p_slvBoxPanel, false, true);
  }
  p_solvateVSizer->Layout();
  Fit();
}

void MDPrep::enableModifyPanel()
{
  ewxCheckBox *check;
  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_INVCPY));
  ewxPanel *panel = ((ewxPanel*)FindWindow(ID_PANEL_MDPREP_ZOPTS));
  if (check->GetValue()) {
    panel->Enable(true);
  } else {
    panel->Disable();
  }
}

bool MDPrep::Show( bool show)
{
   p_ignoreCodeEventsFlag = true;
   wxCommandEvent event;

   // Preserve topology status
   bool topology = getPrepareModel().getChangedTopology();
   OnChoiceMdprepSolventsSelected(event);
   OnChoiceMdprepSlvGeomSelected(event);
   OnCheckboxMdprepXrangeClick(event);
   OnCheckboxMdprepYrangeClick(event);
   OnCheckboxMdprepZrangeClick(event);
   OnCheckboxMdprepUsezboxClick(event);
   OnCheckboxMdprepInvcpyClick(event);
   OnCheckboxMdprepSetmemClick(event);
   OnRadiobuttonMdprepTrnsMolSelected(event);
   OnRadiobuttonMdprepOrntMolSelected(event);
   p_ignoreCodeEventsFlag = false;

   getPrepareModel().setChangedTopology(topology);

   return MDPrepGUI::Show(show);
}


/**
 * Restore all GUI fields from the data model object.
 */
void MDPrep::refreshGUI()
{
   // Change to default units since the model is always in these units
   ewxWindowUtils::setUnitFamily(this, DEFAULT_UNIT_FAMILY);

   //Restore PDB settings
   ewxTextCtrl *text;
   ewxListBox *box;

   refreshMoleculeFields();

   //Restore topology settings
   PrepareModel::DefaultHistidine defaultH;
   defaultH = getPrepareModel().getDefaultHistidine();
   ewxRadioButton *button = 0;
   if (defaultH == PrepareModel::HID) {
      button = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_MDPREP_HID));
   } else if (defaultH == PrepareModel::HIE) {
      button = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_MDPREP_HIE));
   } else if (defaultH == PrepareModel::HIP) {
      button = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_MDPREP_HIP));
   }
   button->SetEvtHandlerEnabled(false);
   button->SetValue(true);
   button->SetEvtHandlerEnabled(true);

   ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_MDPREP_SOLVENTS));
   choice->SetSelection(getPrepareModel().getSolventOption());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SOL_NAM));
   text->SetValue(getPrepareModel().getSolventName().c_str());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SOL_MOD));
   text->SetValue(getPrepareModel().getSolventModel().c_str());

   ewxComboBox *combo = ((ewxComboBox*)FindWindow(ID_COMBOBOX_MDPREP_CINAME));
   combo->SetValue(getPrepareModel().getCounterIonOpt().c_str());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_CINUM));
   text->setValueAsInt(getPrepareModel().getCINumber());

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC1));
   text->setValueAsInt(getPrepareModel().getFraction1());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC2));
   text->setValueAsInt(getPrepareModel().getFraction2());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC3));
   text->setValueAsInt(getPrepareModel().getFraction3());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC4));
   text->setValueAsInt(getPrepareModel().getFraction4());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC5));
   text->setValueAsInt(getPrepareModel().getFraction5());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC6));
   text->setValueAsInt(getPrepareModel().getFraction6());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC7));
   text->setValueAsInt(getPrepareModel().getFraction7());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC8));
   text->setValueAsInt(getPrepareModel().getFraction8());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC9));
   text->setValueAsInt(getPrepareModel().getFraction9());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC10));
   text->setValueAsInt(getPrepareModel().getFraction10());

   //Restore commands
   vector<string> cmds;
   cmds = getPrepareModel().getCmdList();
   if (p_listBox) {
     p_listBox->Clear();
   }
   if (cmds.size() > 0) {
      for (int i=0; i<cmds.size(); i++) {
         p_listBox->Append(cmds[i].c_str());
      }
   }

   //Restore links
   vector<string> links;
   links = getPrepareModel().getLinkList();
   box = ((ewxListBox*)FindWindow(ID_LISTBOX_MDPREP_LNK_LIST));
   if (box) {
     box->Clear();
   }
   if (links.size() > 0) {
      for (int i=0; i<links.size(); i++) {
         box->Append(links[i].c_str());
      }
   }

   //Restore counter ions
   vector<string> ions;
   ions = getPrepareModel().getCounterIonList();
   box = ((ewxListBox*)FindWindow(ID_LISTBOX_MDPREP_CILIST));
   if (box) {
     box->Clear();
   }
   if (ions.size() > 0) {
      for (int i=0; i<ions.size(); i++) {
         box->Append(ions[i].c_str());
      }
   }
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_CISCALE));
   text->setValueAsDouble(getPrepareModel().getCounterIonScale());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_CIGRIDSZ));
   text->setValueAsInt(getPrepareModel().getCIGridSize());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_CIMINDIS));
   text->setValueAsDouble(getPrepareModel().getMinAtomCIDis());

   //Restore coordinate panels
   //Translate page
   if (getPrepareModel().getTranslateOption() == 1) {
     button = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_MDPREP_TRNS_MOL));
     button->SetValue(true);
   } else if (getPrepareModel().getTranslateOption() == 2) {
     button = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_MDPREP_TRNS_SEG));
     button->SetValue(true);
   } else if (getPrepareModel().getTranslateOption() == 3) {
     button = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_MDPREP_TRNS_ATOM));
     button->SetValue(true);
   }
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_TRNS_X));
   text->setValueAsDouble(getPrepareModel().getTranslateX());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_TRNS_Y));
   text->setValueAsDouble(getPrepareModel().getTranslateY());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_TRNS_Z));
   text->setValueAsDouble(getPrepareModel().getTranslateZ());

   //Orient page
   if (getPrepareModel().getOrientOption() == 1) {
     button = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_MDPREP_ORNT_MOL));
     button->SetValue(true);
   } else if (getPrepareModel().getOrientOption() == 2) {
     button = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_MDPREP_ORNT_SEG));
     button->SetValue(true);
   } else if (getPrepareModel().getOrientOption() == 3) {
     button = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_MDPREP_ORNT_ATOM));
     button->SetValue(true);
   }
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ORNT_ANGLE));
   text->setValueAsDouble(getPrepareModel().getAngle());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ORNT_X));
   text->setValueAsDouble(getPrepareModel().getOrientX());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ORNT_Y));
   text->setValueAsDouble(getPrepareModel().getOrientY());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ORNT_Z));
   text->setValueAsDouble(getPrepareModel().getOrientZ());

   //Solvate page
   choice = ((ewxChoice*)FindWindow(ID_CHOICE_MDPREP_BCOPT));
   choice->SetSelection(getPrepareModel().getBoundaryCondition());
   choice = ((ewxChoice*)FindWindow(ID_CHOICE_MDPREP_SLV_GEOM));
   choice->SetSelection(getPrepareModel().getGeometryOption());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_MINDIST));
   text->setValueAsDouble(getPrepareModel().getMinSluSluDist());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_MINSOLV));
   text->setValueAsDouble(getPrepareModel().getMinSluSlvDist());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_EXPND));
   text->setValueAsDouble(getPrepareModel().getExpandVolPar());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLVTOL));
   text->setValueAsDouble(getPrepareModel().getEnvelopePar());

   //Remove solvent page
   choice = ((ewxChoice*)FindWindow(ID_CHOICE_MDPREP_RMV_OPT));
   choice->SetSelection(getPrepareModel().getRemoveOption());
   ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_XRANGE));
   check->SetValue(getPrepareModel().getRemoveXRange());
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_YRANGE));
   check->SetValue(getPrepareModel().getRemoveYRange());
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_ZRANGE));
   check->SetValue(getPrepareModel().getRemoveZRange());

   //Modify cell page
   choice = ((ewxChoice*)FindWindow(ID_CHOICE_MDPREP_REPOPTS));
   choice->SetSelection(getPrepareModel().getRepeatOption());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_XREP));
   text->setValueAsInt(getPrepareModel().getXRepeat());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_YREP));
   text->setValueAsInt(getPrepareModel().getYRepeat());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ZREP));
   text->setValueAsInt(getPrepareModel().getZRepeat());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_MINRSEP));
   text->setValueAsDouble(getPrepareModel().getMinRepeatSeparation());
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_RANX));
   check->SetValue(getPrepareModel().getRandomX());
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_RANY));
   check->SetValue(getPrepareModel().getRandomY());
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_RANZ));
   check->SetValue(getPrepareModel().getRandomZ());
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_INVCPY));
   check->SetValue(getPrepareModel().getInvertZCopy());
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_USEZBOX));
   check->SetValue(getPrepareModel().getUseZBoxDim());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SKIPX));
   text->setValueAsInt(getPrepareModel().getSkipX());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SKIPY));
   text->setValueAsInt(getPrepareModel().getSkipY());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SKIPZ));
   text->setValueAsInt(getPrepareModel().getSkipZ());

   //Restore details
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_SETMEM));
   check->SetValue(getPrepareModel().getUseMemory());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_MEMHEAP));
   text->setValueAsInt(getPrepareModel().getHeapMemory());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_MEMSTK));
   text->setValueAsInt(getPrepareModel().getStackMemory());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_MEMGLBL));
   text->setValueAsInt(getPrepareModel().getGlobalMemory());
   choice = ((ewxChoice*)FindWindow(ID_CHOICE_MDPREP_PRTLEV));
   choice->SetSelection(getPrepareModel().getPrintLevel());

   // Restore PMF directives
   vector<string> pmfDirectives = getPrepareModel().getPmfList();
   box = ((ewxListBox*) FindWindow(ID_LISTBOX_PMF_DIRECTIVES));
   if (box) {
     box->Clear();
   }
   for (int i = 0; i < pmfDirectives.size(); i++) {
     box->Append(pmfDirectives[i]);
   }

   // This will hide some fields on the PMF panel
   doPmfOptionSelected(((wxChoice*) FindWindow(ID_CHOICE_PMF_OPTION))->
           GetString(0));

   // Now that some PMF checkboxes are hidden, the size needs adjusting
   SetMinSize(GetBestSize());

   // Now change to user prefered units
   ewxWindowUtils::processGlobalPreferenceChange(this);

   // Now make sure all of the validators contain the forced values above
   TransferDataFromWindow();
}


/**
 * Subscribe to messages and initiate messaging.
 */
void MDPrep::initializeMessaging()
{
   p_msgDispatcher = new WxJMSMessageDispatch(WHOAMI, true);

   p_msgDispatcher->subscribe("ecce_invoke",
         this, (wxJmsCBFunc)&MDPrep::invokeMCB);
   p_msgDispatcher->subscribe("ecce_quit",
         this, (wxJmsCBFunc)&MDPrep::quitMCB);
   p_msgDispatcher->subscribe("ecce_identify",
         this, (wxJmsCBFunc)&MDPrep::identifyMCB);

   p_msgDispatcher->subscribe("ecce_preferences_states",
         this, (wxJmsCBFunc)&MDPrep::statePrefsMCB);
   p_msgDispatcher->subscribe("ecce_preferences_misc",
         this, (wxJmsCBFunc)&MDPrep::prefsMCB);

   p_msgDispatcher->subscribe("ecce_url_renamed", 
         this, (wxJmsCBFunc)&MDPrep::urlRenamedMCB);
   p_msgDispatcher->subscribe("ecce_url_removed", 
         this, (wxJmsCBFunc)&MDPrep::urlRemovedMCB);

   p_msgDispatcher->subscribe("ecce_url_state",
         this, (wxJmsCBFunc)&MDPrep::urlStateMCB, false);
   p_msgDispatcher->subscribe("ecce_url_subject",
         this, (wxJmsCBFunc)&MDPrep::urlSubjectMCB);

   p_msgDispatcher->subscribe("ecce_url_mol_selection",
         this, (wxJmsCBFunc)&MDPrep::molSelectionMCB);
   p_msgDispatcher->subscribe("ecce_getcharge",
         this, (wxJmsCBFunc)&MDPrep::getChargeMCB);
   p_msgDispatcher->subscribe("ecce_getNWChemSelection",
         this, (wxJmsCBFunc)&MDPrep::getNWChemSelectionMCB);
   p_msgDispatcher->subscribe("ecce_getNWChemDistance",
         this, (wxJmsCBFunc)&MDPrep::getNWChemDistanceMCB);
   p_msgDispatcher->subscribe("ecce_getNWChemAngle",
         this, (wxJmsCBFunc)&MDPrep::getNWChemAngleMCB);

   p_msgDispatcher->subscribe("ecce_auth_changed",
         this, (wxJmsCBFunc)&MDPrep::authMCB);

   p_msgDispatcher->startSubscriber();
   p_msgDispatcher->registerTopShell(this);
   p_msgDispatcher->notifyReady();
}



/**
 * Get title from registry (label).
 */
string MDPrep::getTitle() const
{
   return "NWChem MD Prepare";
}


/**
 * State color preferences have changed.
 * Update state icon in feedback area.
 */
void MDPrep::statePrefsMCB(JMSMessage& msg)
{
   p_feedback->readPreferences();
}


/**
 * An object was renamed. 
 * If it was our MDPrepare object, reset variables.
 */
void MDPrep::urlRenamedMCB(JMSMessage& msg)
{
   string oldURL = msg.getProperty("oldurl");
   string newURL = msg.getProperty("newurl");

   if (p_model) {
     EcceURL url = p_model->getUrl();
     if (url.isChildOrMe(oldURL)) {
       string calcurl = p_model->getUrl();
       EDSIFactory::renamePoolResource(oldURL, newURL);
       calcurl = calcurl.replace(0, oldURL.length(), newURL);
       p_model->setUrl(calcurl);

       // Update the path in the feedback area
       p_feedback->setContextLabel(EDSIServerCentral().mapURLtoName(calcurl));
       p_feedback->setContextURL(calcurl);

       p_msgDispatcher->setPollContext(calcurl);
     }
   }
}


/**
 * An object was deleted. 
 * If it was our MDPrepare object, clear and disable the gui.
 */
void MDPrep::urlRemovedMCB(JMSMessage& msg)
{
   string urlstr = msg.getProperty("url");
    
   if (p_model) {
     EcceURL url = p_model->getUrl();
     if (url.isChildOrMe(urlstr)) {
       p_feedback->setMessage("The task or a parent study or project "
                              "has been deleted.", WxFeedback::WARNING);
       clearAll();

       EDSIFactory::removePoolResource(urlstr);
     }
   }
}

/**
 */
void MDPrep::getChargeMCB(JMSMessage& msg)
{
   string value = msg.getProperty("charge");
   int stat = msg.getIntProperty("residue_status");
   string report;
   if (stat == 1) {
     report = "Total charge of system is ";
     report.append(value);
   } else if (stat == 0) {
     report = "Some residues are missing hydrogens";
   } else if (stat == -1) {
     report = "System contains unknown or incomplete residues";
   } else if (stat == 2) {
     report = "System contains unresolved Histidine residues. "
              "Use the MD Toolkit to assign specific protonation "
              "states to each Histidine.";
   } else {
     report = "System is not divided into residues";
   }
   Raise();
   ewxMessageDialog prompt(this, report.c_str(), "ECCE Get Charge", wxOK);
   (void)prompt.ShowModal();
}

/**
 */
void MDPrep::getNWChemSelectionMCB(JMSMessage& msg)
{
  string selection = msg.getProperty("selection");
  wxTextCtrl * text = dynamic_cast<wxTextCtrl*>(FindWindow(pmfSelectionId));
  text->SetValue(selection);

  // now that we got what we wanted from the builder, make sure we're back
  // on top to aid in user friendliness
  Raise();
}


void MDPrep::getNWChemDistanceMCB(JMSMessage& msg)
{
  ewxTextCtrl * text1 = dynamic_cast<ewxTextCtrl*>(
          FindWindow(ID_TEXTCTRL_PMF_DISTANCE1));
  ewxTextCtrl * text2 = dynamic_cast<ewxTextCtrl*>(
          FindWindow(ID_TEXTCTRL_PMF_DISTANCE2));

  string value = msg.getProperty("value");

  // distance is always returned in Angstroms, we must convert it appropriately
  ewxUnitHelper *helper = text1->getUnitHelper();
  if (helper) {
    string whatIAm = "Angstrom";
    UnitFactory& uf = UnitFactory::getInstance();
    UnitFamily& currentFamily = uf.getUnitFamily(helper->getFamily());
    string whatIWant = currentFamily.get(helper->getUnitClass());
    if (whatIAm != whatIWant) {
      // get converter and convert
      UnitConverter& uconv = uf.getUnitConverter(helper->getUnitClass());
      uconv.setBaseUnits(whatIAm);
      value = uconv.convertTo(value, whatIWant);
    }
  }
  text1->SetValue(value);
  text2->SetValue(value);

  // now that we got what we wanted from the builder, make sure we're back
  // on top to aid in user friendliness
  Raise();
}


void MDPrep::getNWChemAngleMCB(JMSMessage& msg)
{
  ewxTextCtrl * text1 = dynamic_cast<ewxTextCtrl*>(
          FindWindow(ID_TEXTCTRL_PMF_ANGLE1));
  ewxTextCtrl * text2 = dynamic_cast<ewxTextCtrl*>(
          FindWindow(ID_TEXTCTRL_PMF_ANGLE2));
  
  string value = msg.getProperty("value");

  // angle is always returned in Radian, we must convert it appropriately
  ewxUnitHelper *helper = text1->getUnitHelper();
  if (helper) {
    string whatIAm = "Radian";
    UnitFactory& uf = UnitFactory::getInstance();
    UnitFamily& currentFamily = uf.getUnitFamily(helper->getFamily());
    string whatIWant = currentFamily.get(helper->getUnitClass());
    if (whatIAm != whatIWant) {
      // get converter and convert
      UnitConverter& uconv = uf.getUnitConverter(helper->getUnitClass());
      uconv.setBaseUnits(whatIAm);
      value = uconv.convertTo(value, whatIWant);
    }
  }
  text1->SetValue(value);
  text2->SetValue(value);

  // now that we got what we wanted from the builder, make sure we're back
  // on top to aid in user friendliness
  Raise();
}


void MDPrep::invokeMCB(JMSMessage& msg)
{
   if (msg.containsProperty("calcurl")) {
      string url = msg.getProperty("calcurl");
      setContext(url);
      p_msgDispatcher->setPollContext(url);
      Raise();
   }
}

void MDPrep::quitMCB(JMSMessage& msg)
{
  if (msg.getSender().getName() == GATEWAY) {
    doQuit(true, false);
  }
}

/**
 * Identify message call back function
 *
 * When gateway sent out identify message, periodic table app will response by sending a ID message back.
 */
void MDPrep::identifyMCB(JMSMessage&)
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
 * Handle the preference change message which indicates a unit family change.
 */
void MDPrep::prefsMCB(JMSMessage& msg)
{
   ewxWindowUtils::processGlobalPreferenceChange(this);
}


/**
 * Reset the gui to the task at the specified url.
 */
void MDPrep::setContext(const string& urlstr) throw (EcceException)
{
   wxBusyCursor busy;

   bool topology;

   if (p_model) {

      // bail if it is the same context rather than resetting the context
      if (urlstr == p_model->getUrl())
        return;

      bool isValidFragment = false;
      bool isProvidedSystem = false;

      // Set model's url
      p_model->setUrl(urlstr);

      // Get task model from db (deserialize)
      MdTask *mdTask =  
        dynamic_cast<MdTask*>(EDSIFactory::getResource(EcceURL(urlstr)));
      if (mdTask != 0) {
         try {
            reloadFragSummary(mdTask);

            p_feedback->setRunState(mdTask->getState());
            updateDisabledState(p_feedback->getRunState(), true);
            EcceURL url = mdTask->getURL();
            EDSIServerCentral servers;
            p_feedback->setContextLabel(servers.mapURLtoName(url));
            p_feedback->setContextURL(url);
            mdTask->getTaskModel(*p_model);
            topology = getPrepareModel().getChangedTopology();

            isValidFragment = mdTask->hasInputFragment();

            // Check to see if pdb is provided by previous task
            MdTask *pdbProvider = mdTask->getPdbProvider();
            if (pdbProvider != 0) {
               pdbProvider = 0;
               getPrepareModel().setPDBSource("(Using output from previous task)");
               isProvidedSystem = true;
               if (mdTask->getState() == ResourceDescriptor::STATE_CREATED) {
                  generateInputFile(mdTask);
                  setRunState(ResourceDescriptor::STATE_READY);
               }
            } else {
               // If the state is created, check to see if there is already a
               // chemical system and generate the input file if so
               if (mdTask->getState() == ResourceDescriptor::STATE_CREATED &&
                   isValidFragment) {
                  generateInputFile(mdTask);
                  setRunState(ResourceDescriptor::STATE_READY);
               }
            }
         }
         catch (...) {
            if (mdTask != 0) {
               mdTask = 0;
            }
            p_feedback->setMessage("Deserialize model and set PDB source failed.");
         }
         mdTask = 0;
      }

      //Refresh GUI with model
      refreshGUI();
      getPrepareModel().setChangedTopology(topology);

      // Do this AFTER refresh GUI since refreshing may trigger change callbacks
      // Regardless of runstate, we are in edit mode.
      setSaveState(false);

      // Update GUI to reflect current state
      if (isProvidedSystem) {
         setState(PROVIDED_SYSTEM);
      } else if (isValidFragment) {
         setState(VALID_SYSTEM);
      } else {
         setState(NO_SYSTEM);
      }
   }
}


/**
 * Clears the gui and disables it except for the drop site.
 */
void MDPrep::clearAll()
{
   // Don't all attempts to save anymore
   setSaveState(false);

   // Disable GUI
   wxWindow *main = FindWindow(ID_NOTEBOOK_MDPREP_MAIN);
   main->Disable();
   FindWindow(ID_BUTTON_MDPREP_BUILDER)->Disable();
   FindWindow(ID_BUTTON_MDPREP_LOCAL)->Disable();
   FindWindow(ID_BUTTON_MDPREP_REMOTE)->Disable();
   FindWindow(ID_BUTTON_MDPREP_FINAL_EDIT)->Disable();

}

/**
 * Implementation of TaskApp::taskAppRaise.
 * Window is raised to front.
 */
void MDPrep::taskAppRaise()
{
   Raise();
}



/**
 * Implementation of TaskApp::taskAppShow.
 * Window is shown.
 */
void MDPrep::taskAppShow()
{
   Show(true);
}



/**
 * Implementation of TaskApp::taskAppDestroy.
 * Window is destroyed.
 */
void MDPrep::taskAppDestroy()
{
   doQuit(true, false);
}


/**
 * quit but first handle any unsaved changes.
 * @param force without prompting even if error occurs during save
 * @todo implement force param
 */
void MDPrep::doQuit(bool force, bool notifyEditor)
{
   if (!force && p_feedback->getEditStatus() == WxFeedback::MODIFIED) {
      ewxMessageDialog prompt(this,"You have unsaved changes!  Do you "
        "want to save your changes?", "ECCE Save Changes",
        wxYES_NO | wxCANCEL | wxICON_QUESTION);
      int status = prompt.ShowModal();
      if (status == wxID_YES) {
         try {
            MdTask *task = 
               dynamic_cast<MdTask*>(EDSIFactory::getResource(p_model->getUrl()));
            saveModelAndInputFile(task);
         } catch (EcceException& ex) {
            p_feedback->setMessage(ex.what(), WxFeedback::ERROR);
         }
      } else if (status == wxID_CANCEL) {
         // abort the quit
         return;
      }
   }

   if (notifyEditor) {
      notifyClose();
   }

   // save current settings
   Preferences prefs(WHOAMI);
   saveWindowSettings(WHOAMI, prefs, false);
   prefs.saveFile();

   Show(false);
   Destroy();
}

/**
 * Save the model, generate and save input file.
 */
void MDPrep::saveModelAndInputFile(MdTask *task) throw(IOException)
{

   PrepareModel& model = getPrepareModel();
   if (task) {
      // These can throw an exception
      Session *session = task->getSession();
      if (session != 0) {
        if (task->getInputProvider(session) != 0) {
          model.setIsFirstTask(false);
        } else {
          model.setIsFirstTask(true);
        }
      }

      task->setTaskModel(&model);
      generateInputFile(task);

      setSaveState(false);
   }
}


/**
 * Implementation of TaskApp::setUnitFamily.
 * All unitted fields are updated to reflect the new unit family.
 */
void MDPrep::setUnitFamily(const string& family)
{
   ewxWindowUtils::setUnitFamily(this, family);
   Fit();
}



/**
 * Creates a new PrepareModel instance.
 * If there is an existing instance, it is destroyed.
 */
void MDPrep::initializeModel()
{
   if (p_model != 0) {
      delete p_model;
   }

   p_model = new PrepareModel();
}



PrepareModel& MDPrep::getPrepareModel() const
{
   return (PrepareModel&)*p_model;
}


/**
 * Reset all molecule related fields in the gui from the model.
 */
void MDPrep::refreshMoleculeFields()
{
   ewxStaticText* label;
   ewxStaticText* label2;
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_FRAG_NAME));
   label->SetLabel(p_fragSummary.getName());

   label = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_SLCT_MOD));
   char buf[12];
   sprintf(buf,"%d",p_fragSummary.getSelectedModel());
   label->SetLabel(buf);

   bool show = p_fragSummary.getAltLocation() != "" && 
               p_fragSummary.getAltLocation() != " ";
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_ALT_LOC));
   label2 = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_ALT_LOC_LABEL));
   label->SetLabel(p_fragSummary.getAltLocation().c_str());
   label2->Show(show);
   label->Show(show);

   label = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_CHAIN));
   string chain = p_fragSummary.getChain();
   if (chain == " " || chain == "" ) {
      chain = "All";
   }
   label->SetLabel(chain.c_str());
}

/**
 * Convenience function for resetting text values
 */

void MDPrep::setTextDefault(ewxTextCtrl *text, double val)
{
  bool isEnabled = true;
  if (!text->IsEnabled()) {
    isEnabled = false;
    text->Enable(true);
  }
  text->setValueAsDouble(val, "CGS");
  if (!isEnabled) {
    text->Disable();
  }
}

/**
 * Reset all default values for all fields in the gui that depend on
 * details of the fragment and are not saved as part of the model.
 */
void MDPrep::refreshFragmentDefaults(double *dims)
{
  double dx = dims[1] - dims[0];
  double dy = dims[3] - dims[2];
  double dz = dims[5] - dims[4];
  double rval = dx > dy ? dx : dy;
  rval = dz > rval ? dz : rval;
  double radius = sqrt(dx*dx + dy*dy + dz*dz)/2.0;

  // This is not executed unless the Builder is up. In order for the defaults
  // to be updated, the input fields must all be temporarily enabled if they are
  // currently disabled.

  // Reset solvation box parameters
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_EDGE));
  setTextDefault(text, rval);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_RAD));
  setTextDefault(text, radius + 10.0);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_BOXX));
  setTextDefault(text, dx);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_BOXY));
  setTextDefault(text, dy);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_BOXZ));
  setTextDefault(text, dz);

  // Reset remove solvent parameters
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_XMIN));
  setTextDefault(text, dims[0]);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_XMAX));
  setTextDefault(text, dims[1]);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_YMIN));
  setTextDefault(text, dims[2]);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_YMAX));
  setTextDefault(text, dims[3]);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_ZMIN));
  setTextDefault(text, dims[4]);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_ZMAX));
  setTextDefault(text, dims[5]);

  // Reset modify cell parameters
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SETX));
  setTextDefault(text, dx);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SETY));
  setTextDefault(text, dy);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SETZ));
  setTextDefault(text, dz);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_IZDIS));
  setTextDefault(text, dz);
}


void MDPrep::processSave()
{
   wxBusyCursor busy;
   try {
      MdTask *task = 
         dynamic_cast<MdTask*>(EDSIFactory::getResource(p_model->getUrl()));
      saveModelAndInputFile(task);
   } catch (EcceException& ex) {
      p_feedback->setMessage(ex.what(), WxFeedback::ERROR);
   }
}


//void MDPrep::doSaveButtonClick(wxCommandEvent &event)
void MDPrep::doSaveButtonClick(wxMouseEvent &event)
{
   processSave();
}


/**
 * Reloads fragment summary from data server and updates gui.
 */
void MDPrep::reloadFragSummary(MdTask *task)
{
   double dims[6];
   task->getFragmentSummary(p_fragSummary);
   p_fragSummary.getDimensions(dims);
   getPrepareModel().setDefaultDims(dims);
   refreshFragmentDefaults(dims);
}


void MDPrep::OnButtonMdprepBuilderClick(wxCommandEvent &event)
{
   wxBusyCursor busy;
   requestBuilder();
}

void MDPrep::OnButtonMdprepChemsysClick(wxCommandEvent &event)
{
   wxBusyCursor busy;
   requestBuilder();
}


/**
 * Send message(s) to request startup of builder with this context.
 * It allows for arbitrary properties to be added to the message rather than
 * just the actionTopic message.
 */
void MDPrep::requestBuilder(const string& actionTopic, bool forceNew,
                            map<string, string> * properties)
{
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


   // Add arbitrary number of extra properties, if available
   if (properties) {
     map<string,string>::iterator props;
     for (props = properties->begin(); props != properties->end(); props++) {
        msg->addProperty(props->first, props->second);
     }
   }

   // which builder do we invoke?  find out now!
   Resource *resource = EDSIFactory::getResource(p_model->getUrl());
   if (resource) {
     ResourceType *resourceType = resource->getDescriptor();
     if (resourceType) {
       vector<ResourceTool*> tools = resourceType->getTools();
       for (int i = 0; i < tools.size(); i++) {
         string name = tools[i]->getName();
         if (name == BUILDER) {
           publisher.invoke(*msg, name, /*forceNew ? 1 : 0*/0,
                            getPrepareModel().getUrl(), actionTopic);
           break;
         }
       }
     }
   }

   delete msg;
}

/**
 * A molecule has changed.
 * We need to reload the fragment summary.
 */
void MDPrep::urlSubjectMCB(JMSMessage& msg)
{
   string url = msg.getProperty("url");

   if (url == getPrepareModel().getUrl()) {
      EDSIFactory::changePoolResource(url);

      MdTask *mdTask =  
         dynamic_cast<MdTask*>(EDSIFactory::getResource(EcceURL(url)));

      if (mdTask != 0) {
         try {
            reloadFragSummary(mdTask);
            refreshMoleculeFields();
            getPrepareModel().setChangedTopology(true);
            setRunState(ResourceDescriptor::STATE_READY);
            setState(VALID_SYSTEM);

         } catch (...) {
            mdTask = 0;
            p_feedback->setMessage("Retrieve fragment summary failed.");
         }
         mdTask = 0;
      }
   }
}

/**
 * A molecule selection has changed.
 * The number of atoms in the selection list can be 0-4.
 */
void MDPrep::molSelectionMCB(JMSMessage& msg)
{
   string url = msg.getProperty("url");
   if (url == getPrepareModel().getUrl()) {
      istrstream is(msg.getProperty("frag_sum").c_str());
      is >> p_fragSummary;
   }
}

/**
 * State has changed.
 */
void MDPrep::urlStateMCB(JMSMessage& msg)
{
   string url = msg.getProperty("url");

   if (url == p_model->getUrl()) {
     EDSIFactory::changePoolResource(url);

     string statestr = msg.getProperty("state");
     ResourceDescriptor::RUNSTATE state = ResourceUtils::stringToState(statestr);
     updateDisabledState(state);
     p_feedback->setRunState(state);
     if (state >= ResourceDescriptor::STATE_SUBMITTED) {
        p_feedback->setEditStatus(WxFeedback::READONLY);
     } else if (!isUnsavedState()) {
        p_feedback->setEditStatus(WxFeedback::EDIT);
     }
     if (state >= ResourceDescriptor::STATE_COMPLETED) {

       // For remote launches, no progress dialog
       if (p_progressDialog != 0) {
         p_feedback->setMessage("Finished local NWChem MD Prepare task.",
                                WxFeedback::INFO);
         p_progressDialog->Update(100, "NWChem MD Prepare task done!!");
         sleep(1);
         //p_progressDialog->Destroy();
         //p_progressDialog = 0;
       } else {
         p_feedback->setMessage("Finished NWChem MD Prepare task.",
                                WxFeedback::INFO);
       }

       if (state == ResourceDescriptor::STATE_COMPLETED) {
          // we ran successfully
         string origpdb = getPrepareModel().getPDBSource();

         string newpdb = origpdb;
         int pos = newpdb.rfind('/', newpdb.size());
         newpdb.replace(pos+1, newpdb.length()-pos, "chemsys.pdb");
         MdTask *calc = dynamic_cast<MdTask*>(EDSIFactory::getResource(p_model->getUrl()));
         struct Jobdata jobdata = calc->jobdata();
         newpdb.replace(0, pos, jobdata.jobpath);
         getPrepareModel().setPDBSource(origpdb);
         requestBuilder("refresh");
         // Force a save to the data base to record the fact that the
         // topologyChanged setting is now false
         getPrepareModel().setChangedTopology(false);
         calc->setTaskModel(&getPrepareModel());

         PrepState state = getState();
         if (state == VALID_SYSTEM) {
            // This must have been a topology run so now we must have
            // a topology
            setState( VALID_TOPOLOGY);
         } else {
            // we must be doing coordinates stuff - no state change defined
         }
       } else {
          wxArrayString choices;
          choices.Add("View output file");
          choices.Add("Get xterm in run directory");
          ewxSingleChoiceDialog prompt(this, 
              "NWChem detected an error.  Please choose one of the following "
              "options or Cancel.","ECCE - Debug run failure!",
              choices);
          if (prompt.ShowModal() == wxID_OK) {
             int choice = prompt.GetSelection();
             if (choice == 0) {
                // TODO don't go through text string
                MdTask *task = 
                   dynamic_cast<MdTask*>(EDSIFactory::getResource(p_model->getUrl()));
                WxEditSessionMgr sessionMgr;
                istream* is = task->getDataFile(JCode::PRIMARY_OUTPUT);
                if (is) {
                   string text;
                   StringConverter::streamToText(*is, text);
                   delete is;
                   sessionMgr.edit(text, "testing", this, true, 
                         "NWChem MD Prepare Output");
                }
             } else {
                MdTask *task = 
                  dynamic_cast<MdTask*>(EDSIFactory::getResource(p_model->getUrl()));
                const Launchdata& launch = task->launchdata();
                string machineName = launch.machine;
                string shell = launch.remoteShell;
                string user = launch.user;
                string pathBase = launch.rundir;
                Jobdata job = task->jobdata();
                string pathFull = job.jobpath;
                if (!pathFull.empty()) {
                   EcceShell eshell;
                   string msg = eshell.dirshell("Prepare", machineName,
                         shell, user, "", pathBase, pathFull);
                   if (msg != "") {
                      p_feedback->setMessage(msg.c_str(), 
                            (eshell.lastStatus()==-1) ?
                             WxFeedback::ERROR : WxFeedback::WARNING);
                   }


                } else {
                   p_feedback->setMessage("No job data available.", WxFeedback::ERROR);
                }



             }
          }
       }
     }
   }
}

/**
 * Authorization cache needs to be updated.
 */
void MDPrep::authMCB(JMSMessage& msg)
{
  AuthCache::getCache().msgIn(msg, p_msgDispatcher->getMyID());
}



/**
 * Process user close button.
 * Create a window close event where save processing is handled.
 */
void MDPrep::OnExitClick( wxCommandEvent& event )
{

    // This generates window close so no need to repeat save code here.
    Close(true);

    event.Skip();
}


/*******************************************************************************
 *  Topology event handlers
 ******************************************************************************/

void MDPrep::OnChoiceMdprepFfchoiceSelected( wxCommandEvent& event )
{
   event.Skip();
}

void MDPrep::OnButtonMdprepCiaddClick( wxCommandEvent& event )
{
   ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_CINUM));
   int inum = atoi(text->GetValue().c_str());
   ewxComboBox *combo = ((ewxComboBox*)FindWindow(ID_COMBOBOX_MDPREP_CINAME));
   ostringstream os;
   os << "  counter "<<inum<<" "<<combo->GetValue();
   ewxListBox *list = ((ewxListBox*)FindWindow(ID_LISTBOX_MDPREP_CILIST));
   list->Append(os.str().c_str());
   getPrepareModel().appendCounterIonList(os.str());
   getPrepareModel().setChangedTopology(true);
   setSaveState(true);
   event.Skip();
}

void MDPrep::OnButtonMdprepCidelClick( wxCommandEvent& event )
{
   wxArrayInt selections;
   ewxListBox *list = ((ewxListBox*)FindWindow(ID_LISTBOX_MDPREP_CILIST));
   list->GetSelections(selections); 
   if (selections.size() > 0) {
      int selected = selections[0];
      list->Delete(selected);
      getPrepareModel().deleteCounterIonList(selected);
      if (selected > 0) {
        list->SetSelection(selected-1);
      }
   }
   getPrepareModel().setChangedTopology(true);
   setSaveState(true);
   event.Skip();
}

void MDPrep::OnTextctrlMdprepCiscaleEnter( wxCommandEvent& event )
{
   ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_CISCALE));
   double rval = atof(text->GetValue().c_str());
   getPrepareModel().setCounterIonScale(rval);
   getPrepareModel().setChangedTopology(true);
   setSaveState(true);
   event.Skip();
}

void MDPrep::OnTextctrlMdprepCigridszEnter( wxCommandEvent& event )
{
   ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_CIGRIDSZ));
   getPrepareModel().setCIGridSize(text->getValueAsInt());
   getPrepareModel().setChangedTopology(true);
   setSaveState(true);
   event.Skip();
}

void MDPrep::OnTextctrlMdprepCimindisEnter( wxCommandEvent& event )
{
   ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_CIMINDIS));
   getPrepareModel().setMinAtomCIDis(text->getBaseValueAsDouble());
   getPrepareModel().setChangedTopology(true);
   setSaveState(true);
   event.Skip();
}

void MDPrep::OnRadiobuttonMdprepHidSelected( wxCommandEvent& event )
{
   getPrepareModel().setDefaultHistidine(PrepareModel::HID);
   getPrepareModel().setChangedTopology(true);
   setSaveState(true);
   event.Skip();
}

void MDPrep::OnRadiobuttonMdprepHieSelected( wxCommandEvent& event )
{

   getPrepareModel().setDefaultHistidine(PrepareModel::HIE);
   getPrepareModel().setChangedTopology(true);
   setSaveState(true);
   event.Skip();
}

void MDPrep::OnRadiobuttonMdprepHipSelected( wxCommandEvent& event )
{
   getPrepareModel().setDefaultHistidine(PrepareModel::HIP);
   getPrepareModel().setChangedTopology(true);
   setSaveState(true);
   event.Skip();
}

void MDPrep::OnTextctrlMdprepSlvMindistEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_MINDIST));
  getPrepareModel().setMinSluSluDist(text->getBaseValueAsDouble());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnComboboxMdprepCinameSelected( wxCommandEvent& event )
{
  ewxComboBox *combo = ((ewxComboBox*)FindWindow(ID_COMBOBOX_MDPREP_CINAME));
  string value = combo->GetValue().c_str();
  STLUtil::stripLeadingAndTrailingWhiteSpace(value);
  char atsym[3];
  if (value.size()>0) {
    atsym[0] = toupper(value[0]);
    atsym[1] = '\0';
  }
  if (value.size()>1) {
    atsym[1] = tolower(value[1]);
    atsym[2] = '\0';
  }
  bool isValidSymbol = false;
  if (value.size()>0) {
    TPerTab pertab;
    isValidSymbol = pertab.isValid(atsym);
  }
  if (isValidSymbol) {
    getPrepareModel().setCounterIonOpt(combo->GetValue().c_str());
    getPrepareModel().setChangedTopology(true);
    setSaveState(true);
  } else {
    p_feedback->setMessage("Not a valid atomic symbol",
                           WxFeedback::WARNING);
    combo->SetValue(getPrepareModel().getCounterIonOpt().c_str());
  }
  event.Skip();
}

void MDPrep::OnComboboxMdprepCinameEnter( wxCommandEvent& event )
{
  OnComboboxMdprepCinameSelected( event );
}

void MDPrep::OnTextctrlMdprepCinumEnter( wxCommandEvent& event )
{
   ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_CINUM));
   getPrepareModel().setCINumber(atoi(text->GetValue().c_str()));
   getPrepareModel().setChangedTopology(true);
   setSaveState(true);
   event.Skip();
}

void MDPrep::OnTextctrlMdprepFrac1Enter( wxCommandEvent& event )
{
   ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC2));
   int frac2 = text->getValueAsInt();
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC1));
   int frac = text->getValueAsInt();
   if (frac >= frac2 && frac2 > 0) {
     text->setValueAsInt(getPrepareModel().getFraction1());
     p_feedback->setMessage("Fractions must be series of increasing integers",
                            WxFeedback::WARNING);
   } else {
     getPrepareModel().setFraction1(frac);
   }
   getPrepareModel().setChangedTopology(true);
   setSaveState(true);
   event.Skip();
}

void MDPrep::OnTextctrlMdprepFrac2Enter( wxCommandEvent& event )
{
   ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC1));
   int frac1 = text->getValueAsInt();
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC3));
   int frac3 = text->getValueAsInt();
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC2));
   int frac = text->getValueAsInt();
   bool useVal = true;
   if (frac >= frac3 && frac3 > 0)
     useVal = false;
   if (frac <= frac1 && frac1 > 0)
     useVal = false;
   if (frac1 == 0)
     useVal = false;
   if (frac == 0 && frac3 == 0)
     useVal = true;
   if (useVal) {
     getPrepareModel().setFraction2(frac);
   } else {
     text->setValueAsInt(getPrepareModel().getFraction2());
     p_feedback->setMessage("Fractions must be series of increasing integers",
                            WxFeedback::WARNING);
   }
   getPrepareModel().setChangedTopology(true);
   setSaveState(true);
   event.Skip();
}

void MDPrep::OnTextctrlMdprepFrac3Enter( wxCommandEvent& event )
{
   ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC2));
   int frac2 = text->getValueAsInt();
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC4));
   int frac4 = text->getValueAsInt();
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC3));
   int frac = text->getValueAsInt();
   bool useVal = true;
   if (frac >= frac4 && frac4 > 0)
     useVal = false;
   if (frac <= frac2 && frac2 > 0)
     useVal = false;
   if (frac2 == 0)
     useVal = false;
   if (frac == 0 && frac4 == 0)
     useVal = true;
   if (useVal) {
     getPrepareModel().setFraction3(frac);
   } else {
     text->setValueAsInt(getPrepareModel().getFraction3());
     p_feedback->setMessage("Fractions must be series of increasing integers",
                            WxFeedback::WARNING);
   }
   getPrepareModel().setChangedTopology(true);
   setSaveState(true);
   event.Skip();
}

void MDPrep::OnTextctrlMdprepFrac4Enter( wxCommandEvent& event )
{
   ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC3));
   int frac3 = text->getValueAsInt();
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC5));
   int frac5 = text->getValueAsInt();
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC4));
   int frac = text->getValueAsInt();
   bool useVal = true;
   if (frac >= frac5 && frac5 > 0)
     useVal = false;
   if (frac <= frac3 && frac3 > 0)
     useVal = false;
   if (frac3 == 0)
     useVal = false;
   if (frac == 0 && frac5 == 0)
     useVal = true;
   if (useVal) {
     getPrepareModel().setFraction4(frac);
   } else {
     text->setValueAsInt(getPrepareModel().getFraction4());
     p_feedback->setMessage("Fractions must be series of increasing integers",
                            WxFeedback::WARNING);
   }
   getPrepareModel().setChangedTopology(true);
   setSaveState(true);
   event.Skip();
}

void MDPrep::OnTextctrlMdprepFrac5Enter( wxCommandEvent& event )
{
   ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC4));
   int frac4 = text->getValueAsInt();
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC6));
   int frac6 = text->getValueAsInt();
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC5));
   int frac = text->getValueAsInt();
   bool useVal = true;
   if (frac >= frac6 && frac6 > 0)
     useVal = false;
   if (frac <= frac4 && frac4 > 0)
     useVal = false;
   if (frac4 == 0)
     useVal = false;
   if (frac == 0 && frac6 == 0)
     useVal = true;
   if (useVal) {
     getPrepareModel().setFraction5(frac);
   } else {
     text->setValueAsInt(getPrepareModel().getFraction5());
     p_feedback->setMessage("Fractions must be series of increasing integers",
                            WxFeedback::WARNING);
   }
   getPrepareModel().setChangedTopology(true);
   setSaveState(true);
   event.Skip();
}

void MDPrep::OnTextctrlMdprepFrac6Enter( wxCommandEvent& event )
{
   ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC5));
   int frac5 = text->getValueAsInt();
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC7));
   int frac7 = text->getValueAsInt();
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC6));
   int frac = text->getValueAsInt();
   bool useVal = true;
   if (frac >= frac7 && frac7 > 0)
     useVal = false;
   if (frac <= frac5 && frac5 > 0)
     useVal = false;
   if (frac5 == 0)
     useVal = false;
   if (frac == 0 && frac7 == 0)
     useVal = true;
   if (useVal) {
     getPrepareModel().setFraction6(frac);
   } else {
     text->setValueAsInt(getPrepareModel().getFraction6());
     p_feedback->setMessage("Fractions must be series of increasing integers",
                            WxFeedback::WARNING);
   }
   getPrepareModel().setChangedTopology(true);
   setSaveState(true);
   event.Skip();
}

void MDPrep::OnTextctrlMdprepFrac7Enter( wxCommandEvent& event )
{
   ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC6));
   int frac6 = text->getValueAsInt();
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC8));
   int frac8 = text->getValueAsInt();
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC7));
   int frac = text->getValueAsInt();
   bool useVal = true;
   if (frac >= frac8 && frac8 > 0)
     useVal = false;
   if (frac <= frac6 && frac6 > 0)
     useVal = false;
   if (frac6 == 0)
     useVal = false;
   if (frac == 0 && frac8 == 0)
     useVal = true;
   if (useVal) {
     getPrepareModel().setFraction7(frac);
   } else {
     text->setValueAsInt(getPrepareModel().getFraction7());
     p_feedback->setMessage("Fractions must be series of increasing integers",
                            WxFeedback::WARNING);
   }
   getPrepareModel().setChangedTopology(true);
   setSaveState(true);
   event.Skip();
}

void MDPrep::OnTextctrlMdprepFrac8Enter( wxCommandEvent& event )
{
   ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC7));
   int frac7 = text->getValueAsInt();
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC9));
   int frac9 = text->getValueAsInt();
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC8));
   int frac = text->getValueAsInt();
   bool useVal = true;
   if (frac >= frac9 && frac9 > 0)
     useVal = false;
   if (frac <= frac7 && frac7 > 0)
     useVal = false;
   if (frac7 == 0)
     useVal = false;
   if (frac == 0 && frac9 == 0)
     useVal = true;
   if (useVal) {
     getPrepareModel().setFraction8(frac);
   } else {
     text->setValueAsInt(getPrepareModel().getFraction8());
     p_feedback->setMessage("Fractions must be series of increasing integers",
                            WxFeedback::WARNING);
   }
   getPrepareModel().setChangedTopology(true);
   setSaveState(true);
   event.Skip();
}

void MDPrep::OnTextctrlMdprepFrac9Enter( wxCommandEvent& event )
{
   ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC8));
   int frac8 = text->getValueAsInt();
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC10));
   int frac10 = text->getValueAsInt();
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC9));
   int frac = text->getValueAsInt();
   bool useVal = true;
   if (frac >= frac10 && frac10 > 0)
     useVal = false;
   if (frac <= frac8 && frac8 > 0)
     useVal = false;
   if (frac8 == 0)
     useVal = false;
   if (frac == 0 && frac10 == 0)
     useVal = true;
   if (useVal) {
     getPrepareModel().setFraction9(frac);
   } else {
     text->setValueAsInt(getPrepareModel().getFraction9());
     p_feedback->setMessage("Fractions must be series of increasing integers",
                            WxFeedback::WARNING);
   }
   getPrepareModel().setChangedTopology(true);
   setSaveState(true);
   event.Skip();
}

void MDPrep::OnTextctrlMdprepFrac10Enter( wxCommandEvent& event )
{
   ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC9));
   int frac9 = text->getValueAsInt();
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_FRAC10));
   int frac = text->getValueAsInt();
   bool useVal = true;
   if (frac <= frac9 && frac9 > 0 && frac > 0)
     useVal = false;
   if (frac9 == 0)
     useVal = false;
   if (useVal) {
     getPrepareModel().setFraction10(frac);
   } else {
     text->setValueAsInt(getPrepareModel().getFraction10());
   }
   getPrepareModel().setChangedTopology(true);
   setSaveState(true);
   event.Skip();
}

/*******************************************************************************
 *  Solvate event handlers
 ******************************************************************************/
void MDPrep::OnChoiceMdprepBcoptSelected( wxCommandEvent& event )
{
  ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_MDPREP_BCOPT));
  getPrepareModel().setBoundaryCondition(choice->GetSelection());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnButtonMdprepSetbcClick( wxCommandEvent& event )
{
  ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_MDPREP_BCOPT));
  string selection = choice->GetStringSelection().c_str();
  if (selection == "Periodic") {
    addListCmd("  periodic");
  } else if (selection == "Vacuum") {
    addListCmd("  vacuo");
  }
  event.Skip();
}

void MDPrep::OnChoiceMdprepSolventsSelected( wxCommandEvent& event )
{
   ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_MDPREP_SOLVENTS));
   string selection = choice->GetStringSelection().c_str();
   ewxTextCtrl *name = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SOL_NAM));
   ewxTextCtrl *model = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SOL_MOD));
   string sName, sModel;
   if (selection == "Water") {
      sName = "HOH";
      sModel = "spce";
   } else if (selection == "Methanol") {
      sName = "meoh";
      sModel = "meoh";
   } else if (selection == "Chloroform") {
      sName = "clfm";
      sModel = "clfm";
   } else if (selection == "Tetrahydrofuran") {
      sName = "thfs";
      sModel = "thfs";
   }
   getPrepareModel().setSolventOption(choice->GetSelection());
   if (selection != "Other") {
     name->SetValue(sName.c_str());
     model->SetValue(sModel.c_str());
     getPrepareModel().setSolventName(sName);
     getPrepareModel().setSolventModel(sModel);
   }
   getPrepareModel().setChangedTopology(true);
   setSaveState(true);
   event.Skip();
}



void MDPrep::OnTextctrlMdprepSolNamEnter( wxCommandEvent& event )
{
  ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_MDPREP_SOLVENTS));
  choice->SetStringSelection("Other");
  getPrepareModel().setSolventOption(choice->GetSelection());
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SOL_NAM));
  getPrepareModel().setSolventName(text->GetValue().c_str());
  getPrepareModel().setChangedTopology(true);
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnTextctrlMdprepSolModEnter( wxCommandEvent& event )
{
  ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_MDPREP_SOLVENTS));
  choice->SetStringSelection("Other");
  getPrepareModel().setSolventOption(choice->GetSelection());
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SOL_MOD));
  getPrepareModel().setSolventModel(text->GetValue().c_str());
  getPrepareModel().setChangedTopology(true);
  setSaveState(true);
  event.Skip();
}

/**
 *    Manage control of Solvent geometry input
 */
void MDPrep::OnChoiceMdprepSlvGeomSelected( wxCommandEvent& event )
{
   ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_MDPREP_SLV_GEOM));
   string selection = choice->GetStringSelection().c_str();
   if (selection == "Cube" || selection == "Truncated Octahedron") {
      p_edgePanel->Enable();
      p_spherePanel->Disable();
      p_slvBoxPanel->Disable();
   } else if (selection == "Rectangular Parallelepiped") {
      p_edgePanel->Disable();
      p_spherePanel->Disable();
      p_slvBoxPanel->Enable();
   } else if (selection == "Sphere") {
      p_edgePanel->Disable();
      p_spherePanel->Enable();
      p_slvBoxPanel->Disable();
   } else {
      p_edgePanel->Disable();
      p_spherePanel->Disable();
      p_slvBoxPanel->Disable();
   }
   getPrepareModel().setGeometryOption(choice->GetSelection());
   wxPanel *panel = ((wxPanel*)FindWindow(ID_PANEL_MDPREP_SOLVATE));
   showSolvatePanel();
   panel->Layout();
   event.Skip();
}



/**
 * Add NWChem SOLVATE command.
 */
void MDPrep::OnButtonMdprepSolvateClick( wxCommandEvent& event )
{
  ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_MDPREP_SLV_GEOM));
  string selection = choice->GetStringSelection().c_str();

  string cmd = "  solvate";

  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_MINDIST));
  double dist = text->getBaseValueAsDouble();
  if (dist != getPrepareModel().MINSEPARATION) {
     cmd += " ";
     cmd += text->getBaseValueAsDoubleString().c_str();
  }
  if (selection == "Cube") {
    text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_EDGE));
    cmd += " cube ";
    cmd += text->getBaseValueAsDoubleString().c_str();

  } else if (selection == "Rectangular Parallelepiped") {
    char buf[120];
    text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_BOXX));
    double x = atof(text->getBaseValueAsDoubleString().c_str());
    text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_BOXY));
    double y = atof(text->getBaseValueAsDoubleString().c_str());
    text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_BOXZ));
    double z = atof(text->getBaseValueAsDoubleString().c_str());
    sprintf(buf," box %f %f %f",x,y,z);
    cmd.append(buf);

  } else if (selection == "Sphere") {
    text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_RAD));
    cmd += " sphere ";
    cmd += text->getBaseValueAsDoubleString().c_str();

  } else if (selection == "Truncated Octahedron") {
    text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_EDGE));
    cmd += " troct ";
    cmd += text->getBaseValueAsDoubleString().c_str();
  }
  choice = ((ewxChoice*)FindWindow(ID_CHOICE_MDPREP_BCOPT));
  choice->SetSelection(0);
  addListCmd(cmd);
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnTextctrlMdprepMinsolvEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_MINSOLV));
  getPrepareModel().setMinSluSlvDist(text->getBaseValueAsDouble());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnButtonMdprepSetMinslvClick( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_MINSOLV));
  ostringstream os;
  os << "  touch " << text->getBaseValueAsDoubleString();
  addListCmd(os.str());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnTextctrlMdprepExpndEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_EXPND));
  getPrepareModel().setExpandVolPar(text->getBaseValueAsDouble());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnButtonMdprepSetExpndClick( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_EXPND));
  ostringstream os;
  os << "  expand " << text->getBaseValueAsDoubleString();
  addListCmd(os.str());
  setSaveState(true);

  event.Skip();
}

void MDPrep::OnTextctrlMdprepSlvtolEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLVTOL));
  getPrepareModel().setEnvelopePar(text->getBaseValueAsDouble());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnButtonMdprepSetSlvtolClick( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLVTOL));
  ostringstream os;
  os << "  envelope " << text->getBaseValueAsDoubleString();
  addListCmd(os.str());

  setSaveState(true);
  event.Skip();
}


/**
 *  Remove solvent event handlers
 */
void MDPrep::OnChoiceMdprepRmvOptSelected( wxCommandEvent& event )
{
  ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_MDPREP_RMV_OPT));
  getPrepareModel().setRemoveOption(choice->GetSelection());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnCheckboxMdprepXrangeClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_XRANGE));
  ewxTextCtrl *text1 = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_XMIN));
  ewxTextCtrl *text2 = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_XMAX));
  ewxStaticText *text3 = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_XMIN));
  ewxStaticText *text4 = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_XMAX));
  if (check->GetValue()) {
     text1->Enable(true);
     text2->Enable(true);
     text3->Enable(true);
     text4->Enable(true);
  } else {
     text1->Disable();
     text2->Disable();
     text3->Disable();
     text4->Disable();
  }
  getPrepareModel().setRemoveXRange(check->GetValue());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnCheckboxMdprepYrangeClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_YRANGE));
  ewxTextCtrl *text1 = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_YMIN));
  ewxTextCtrl *text2 = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_YMAX));
  ewxStaticText *text3 = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_YMIN));
  ewxStaticText *text4 = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_YMAX));
  if (check->GetValue()) {
     text1->Enable(true);
     text2->Enable(true);
     text3->Enable(true);
     text4->Enable(true);
  } else {
     text1->Disable();
     text2->Disable();
     text3->Disable();
     text4->Disable();
  }
  getPrepareModel().setRemoveYRange(check->GetValue());
  setSaveState(true);
  event.Skip();
}



void MDPrep::OnCheckboxMdprepZrangeClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_ZRANGE));
  ewxTextCtrl *text1 = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_ZMIN));
  ewxTextCtrl *text2 = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_ZMAX));
  ewxStaticText *text3 = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_ZMIN));
  ewxStaticText *text4 = ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_ZMAX));
  if (check->GetValue()) {
     text1->Enable(true);
     text2->Enable(true);
     text3->Enable(true);
     text4->Enable(true);
  } else {
     text1->Disable();
     text2->Disable();
     text3->Disable();
     text4->Disable();
  }
  getPrepareModel().setRemoveZRange(check->GetValue());
  setSaveState(true);
  event.Skip();
}



void MDPrep::OnButtonMdprepSlvRmvClick( wxCommandEvent& event )
{
  string cmd("  remove solvent");
  ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_MDPREP_RMV_OPT));
  if (choice->GetStringSelection() == "Inside Range") {
    cmd.append(" inside");
  } else {
    cmd.append(" outside");
  }
  ewxTextCtrl *text1, *text2;
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_XRANGE));
  bool valid = false;
  if (check->GetValue()) {
    text1 = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_XMIN));
    text2 = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_XMAX));
    cmd.append(" x ");
    cmd.append(text1->getBaseValueAsDoubleString());
    cmd.append(" ");
    cmd.append(text2->getBaseValueAsDoubleString());
    valid = true;
  }
  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_YRANGE));
  if (check->GetValue()) {
    text1 = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_YMIN));
    text2 = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_YMAX));
    cmd.append(" y ");
    cmd.append(text1->getBaseValueAsDoubleString());
    cmd.append(" ");
    cmd.append(text2->getBaseValueAsDoubleString());
    valid = true;
  }
  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_ZRANGE));
  if (check->GetValue()) {
    text1 = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_ZMIN));
    text2 = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SLV_ZMAX));
    cmd.append(" z ");
    cmd.append(text1->getBaseValueAsDoubleString());
    cmd.append(" ");
    cmd.append(text2->getBaseValueAsDoubleString());
    valid = true;
  }
  if (valid) {
     addListCmd(cmd);
     setSaveState(true);
  }
  event.Skip();
}

/**
 *  Replicate solvent event handlers
 */
void MDPrep::OnButtonMdprepSetdimClick( wxCommandEvent& event )
{
  ewxTextCtrl *text;
  string cmd("  box ");
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SETX));
  cmd.append(text->getBaseValueAsDoubleString().c_str());
  cmd.append(" ");
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SETY));
  cmd.append(text->getBaseValueAsDoubleString().c_str());
  cmd.append(" ");
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SETZ));
  cmd.append(text->getBaseValueAsDoubleString().c_str());
  addListCmd(cmd);
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnChoiceMdprepRepoptsSelected( wxCommandEvent& event )
{
  ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_MDPREP_REPOPTS));
  getPrepareModel().setRepeatOption(choice->GetSelection());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnTextctrlMdprepXrepEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_XREP));
  getPrepareModel().setXRepeat(text->getValueAsInt());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnTextctrlMdprepYrepEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_YREP));
  getPrepareModel().setYRepeat(text->getValueAsInt());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnTextctrlMdprepZrepEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ZREP));
  getPrepareModel().setZRepeat(text->getValueAsInt());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnTextctrlMdprepMinrsepEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_MINRSEP));
  getPrepareModel().setMinRepeatSeparation(text->getBaseValueAsDouble());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnCheckboxMdprepRanxClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_RANX));
  getPrepareModel().setRandomX(check->GetValue());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnCheckboxMdprepRanyClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_RANY));
  getPrepareModel().setRandomY(check->GetValue());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnCheckboxMdprepRanzClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_RANZ));
  getPrepareModel().setRandomZ(check->GetValue());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnCheckboxMdprepInvcpyClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_INVCPY));
  if (check->GetValue()) {
    p_invertZPanel->Enable(true);
  } else {
    p_invertZPanel->Disable();
  }
  getPrepareModel().setInvertZCopy(check->GetValue());
  enableModifyPanel();
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnCheckboxMdprepUsezboxClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_USEZBOX));
  if (!check->GetValue()) {
    p_invertZDis->Enable(true);
  } else {
    p_invertZDis->Disable();
  }
  getPrepareModel().setUseZBoxDim(check->GetValue());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnTextctrlMdprepSkipxEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SKIPX));
  getPrepareModel().setSkipX(text->getValueAsInt());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnTextctrlMdprepSkipyEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SKIPY));
  getPrepareModel().setSkipY(text->getValueAsInt());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnTextctrlMdprepSkipzEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SKIPZ));
  getPrepareModel().setSkipZ(text->getValueAsInt());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnButtonMdprepReplicClick( wxCommandEvent& event )
{
  string cmd = "  repeat";
  ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_MDPREP_REPOPTS));

  ewxCheckBox *check;
  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_RANX));
  if (check->GetValue()) {
    cmd.append(" randomx");
  }
  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_RANY));
  if (check->GetValue()) {
    cmd.append(" randomy");
  }
  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_RANZ));
  if (check->GetValue()) {
    cmd.append(" randomz");
  }

  if (choice->GetStringSelection() == "Chains") {
    cmd.append(" chains");
  } else if (choice->GetStringSelection() == "Molecules") {
    cmd.append(" molecules");
  } else if (choice->GetStringSelection() == "Fractions") {
    cmd.append(" fractions");
  }

  ewxTextCtrl *text;
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_XREP));
  char buf[120];
  sprintf(buf," %d",text->getValueAsInt());
  cmd.append(buf);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_YREP));
  sprintf(buf," %d",text->getValueAsInt());
  cmd.append(buf);

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_INVCPY));
  bool invcopy;
  if (check->GetValue()) {
    cmd.append(" -2");
    invcopy = true;
  } else {
    text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ZREP));
    sprintf(buf," %d",text->getValueAsInt());
    cmd.append(buf);
    invcopy = false;
  }

  cmd.append(" ");
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_MINRSEP));
  cmd.append(text->getBaseValueAsDoubleString());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_USEZBOX));
  if (invcopy) {
    if (check->GetValue()) {
      cmd.append(" -1.0");
    } else {
      cmd.append(" ");
      text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_IZDIS));
      cmd.append(text->getBaseValueAsDoubleString());
    }
  }
  addListCmd(cmd); 
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnButtonMdprepSkipClick( wxCommandEvent& event )
{
  string cmd = "  skip";
  char buf[120];
  ewxTextCtrl *text;
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SKIPX));
  sprintf(buf," %d",text->getValueAsInt());
  cmd.append(buf);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SKIPY));
  sprintf(buf," %d",text->getValueAsInt());
  cmd.append(buf);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_SKIPZ));
  sprintf(buf," %d",text->getValueAsInt());
  cmd.append(buf);
  addListCmd(cmd); 
  setSaveState(true);
  event.Skip();
}

/**
 *  Translate event handlers
 */
void MDPrep::OnButtonMdprepCenterClick( wxCommandEvent& event )
{
  addListCmd("  center");
  setSaveState(true);
  event.Skip();
}



void MDPrep::OnButtonMdprepCenterxClick( wxCommandEvent& event )
{
  addListCmd("  centerx");
  setSaveState(true);
  event.Skip();
}


void MDPrep::OnButtonMdprepCenteryClick( wxCommandEvent& event )
{
  addListCmd("  centery");
  setSaveState(true);
  event.Skip();
}



void MDPrep::OnButtonMdprepCenterzClick( wxCommandEvent& event )
{
  addListCmd("  centerz");
  setSaveState(true);
  event.Skip();
}



void MDPrep::OnRadiobuttonMdprepTrnsAtomSelected( wxCommandEvent& event )
{
  p_translateAtomID->Enable(true);
  p_translateMolID->Disable();
  getPrepareModel().setTranslateOption(3);
  showTranslatePanel();
  setSaveState(true);
  event.Skip();
}



void MDPrep::OnRadiobuttonMdprepTrnsMolSelected( wxCommandEvent& event )
{
  ewxStaticText *label =
    ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_TRNS_ID));
  label->SetLabel("Molecule ID:");
  p_translateMolID->Enable(true);
  p_translateAtomID->Disable();
  getPrepareModel().setTranslateOption(1);
  showTranslatePanel();
  setSaveState(true);
  event.Skip();
}



void MDPrep::OnRadiobuttonMdprepTrnsSegSelected( wxCommandEvent& event )
{
  ewxStaticText *label =
    ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_TRNS_ID));
  label->SetLabel("Segment/Residue ID:");
  p_translateMolID->Enable(true);
  p_translateAtomID->Disable();
  getPrepareModel().setTranslateOption(2);
  showTranslatePanel();
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnTextctrlMdprepTrnsXEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_TRNS_X));
  getPrepareModel().setTranslateX(text->getBaseValueAsDouble());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnTextctrlMdprepTrnsYEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_TRNS_Y));
  getPrepareModel().setTranslateY(text->getBaseValueAsDouble());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnTextctrlMdprepTrnsZEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_TRNS_Z));
  getPrepareModel().setTranslateZ(text->getBaseValueAsDouble());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnButtonMdprepTrnsSolClick( wxCommandEvent& event )
{
   string cmd = "  translate";
   ewxRadioButton *radio = 
      ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_MDPREP_TRNS_ATOM));

   bool atom = radio->GetValue();
   radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_MDPREP_TRNS_MOL));
   bool mol = radio->GetValue();
   radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_MDPREP_TRNS_SEG));
   bool seg = radio->GetValue();
   ewxTextCtrl *text;
   if (atom) {
      cmd.append(" atom");
      text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_TRNS_INIT_AT));
      cmd += " ";
      cmd += text->GetBaseValue().c_str();

      text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_TRNS_FNL_AT));
      cmd += " ";
      cmd += text->GetBaseValue().c_str();
   } else if (mol) {
      cmd += " molecule";
      text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_TRNS_ID));
      cmd += " ";
      cmd += text->GetValue().c_str();
   } else if (seg) {
      cmd += " segment";
      text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_TRNS_ID));
      cmd += " ";
      cmd += text->GetValue().c_str();
   }

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_TRNS_X));
   cmd += " ";
   cmd += text->getBaseValueAsDoubleString().c_str();

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_TRNS_Y));
   cmd += " ";
   cmd += text->getBaseValueAsDoubleString().c_str();

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_TRNS_Z));
   cmd += " ";
   cmd += text->getBaseValueAsDoubleString().c_str();

   addListCmd(cmd);

   setSaveState(true);
   event.Skip();
}


/**
 *  Orient event handlers
 */
void MDPrep::OnButtonMdprepOrientClick( wxCommandEvent& event )
{
   addListCmd("  orient");
   setSaveState(true);
   event.Skip();
}



void MDPrep::OnRadiobuttonMdprepOrntAtomSelected( wxCommandEvent& event )
{
   p_orientMolID->Disable();
   p_orientAtomID->Enable(true);
   getPrepareModel().setOrientOption(3);
   setSaveState(true);
   showOrientPanel();
   event.Skip();
}



void MDPrep::OnRadiobuttonMdprepOrntMolSelected( wxCommandEvent& event )
{
   ewxStaticText *label =
      ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_ORNT_ID));
   label->SetLabel("Molecule ID:");
   p_orientMolID->Enable(true);
   p_orientAtomID->Disable();
   getPrepareModel().setOrientOption(1);
   setSaveState(true);
   showOrientPanel();
   event.Skip();
}



void MDPrep::OnRadiobuttonMdprepOrntSegSelected( wxCommandEvent& event )
{
   ewxStaticText *label =
      ((ewxStaticText*)FindWindow(wxID_STATIC_MDPREP_ORNT_ID));
   label->SetLabel("Segment/Residue ID:");
   p_orientMolID->Enable(true);
   p_orientAtomID->Disable();
   getPrepareModel().setOrientOption(2);
   setSaveState(true);
   showOrientPanel();
   event.Skip();
}

void MDPrep::OnTextctrlMdprepOrntAngleEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ORNT_ANGLE));
  getPrepareModel().setAngle(text->getBaseValueAsDouble());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnTextctrlMdprepOrntXEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ORNT_X));
  getPrepareModel().setOrientX(text->getBaseValueAsDouble());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnTextctrlMdprepOrntYEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ORNT_Y));
  getPrepareModel().setOrientY(text->getBaseValueAsDouble());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnTextctrlMdprepOrntZEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ORNT_Z));
  getPrepareModel().setOrientZ(text->getBaseValueAsDouble());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnButtonMdprepRotateClick( wxCommandEvent& event )
{
   string cmd = "  rotate";
   ewxRadioButton *radio
      = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_MDPREP_ORNT_ATOM));
   bool atom = radio->GetValue();
   radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_MDPREP_ORNT_MOL));
   bool mol = radio->GetValue();
   radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_MDPREP_ORNT_SEG));
   bool seg = radio->GetValue();
   ewxTextCtrl *text;
   char buf[120];
   if (atom) {
      cmd.append(" atom");
      text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ORNT_INIT_AT));
      sprintf(buf," %s",text->GetBaseValue().c_str());
      cmd.append(buf);
      text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ORNT_FNL_AT));
      sprintf(buf," %s",text->GetBaseValue().c_str());
      cmd.append(buf);
   } else if (mol) {
      cmd.append(" molecule");
      text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ORNT_ID));
      sprintf(buf," %s",text->GetValue().c_str());
      cmd.append(buf);
   } else if (seg) {
      cmd.append(" segment");
      text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ORNT_ID));
      sprintf(buf," %s",text->GetValue().c_str());
      cmd.append(buf);
   }
   cmd.append(" ");
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ORNT_ANGLE));
   cmd.append(text->getBaseValueAsDoubleString());
   cmd.append(" ");
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ORNT_X));
   cmd.append(text->getBaseValueAsDoubleString());
   cmd.append(" ");
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ORNT_Y));
   cmd.append(text->getBaseValueAsDoubleString());
   cmd.append(" ");
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ORNT_Z));
   cmd.append(text->getBaseValueAsDoubleString());
   addListCmd(cmd);
   setSaveState(true);
   event.Skip();
}



void MDPrep::OnButtonMdprepAlignClick( wxCommandEvent& event )
{
   ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ALGNI));
   string cmd("  align ");
   string atm1(text->GetBaseValue().c_str());
   cmd.append(atm1);
   cmd.append(" ");
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ALGNJ));
   string atm2(text->GetBaseValue().c_str());
   cmd.append(atm2);
   cmd.append(" ");
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ALGNK));
   string atm3(text->GetBaseValue().c_str());
   cmd.append(atm3);
   addListCmd(cmd);
   setSaveState(true);
   event.Skip();
}

void MDPrep::OnButtonMdprepOrntslClick( wxCommandEvent& event )
{
   ewxTextCtrl *atm1 = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ALGNI));
   ewxTextCtrl *atm2 = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ALGNJ));
   ewxTextCtrl *atm3 = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_ALGNK));
   int natom = p_fragSummary.getNumSelected();
   if (natom == 3) {
      atm1->SetValue(p_fragSummary.getAtomString(0));
      atm2->SetValue(p_fragSummary.getAtomString(1));
      atm3->SetValue(p_fragSummary.getAtomString(2));
   } else {
     p_feedback->setMessage("You must select three atoms", WxFeedback::ERROR);
     requestBuilder();
   }
   setSaveState(true);
   event.Skip();
}

void MDPrep::addListCmd(string cmd)
{
   int index = p_listBox->Append(cmd.c_str());
   p_listBox->SetFirstItem(index);
   getPrepareModel().appendCmdList(cmd);
}

/**
 *  Details panel event handlers
 */
void MDPrep::OnCheckboxMdprepSetmemClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_MDPREP_SETMEM));
  getPrepareModel().setUseMemory(check->GetValue());
  wxPanel *panel = ((wxPanel*)FindWindow(ID_PANEL_MDPREP_MEMORY));
  setSaveState(true);
  if (check->GetValue()) {
    panel->Enable(true);
  } else {
    panel->Disable();
  }
  event.Skip();
}

void MDPrep::OnTextctrlMdprepMemheapEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_MEMHEAP));
  getPrepareModel().setHeapMemory(text->getValueAsInt());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnTextctrlMdprepMemstkEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_MEMSTK));
  getPrepareModel().setStackMemory(text->getValueAsInt());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnTextctrlMdprepMemglblEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_MEMGLBL));
  getPrepareModel().setGlobalMemory(text->getValueAsInt());
  setSaveState(true);
  event.Skip();
}

void MDPrep::OnChoiceMdprepPrtlevSelected( wxCommandEvent& event )
{
  ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_MDPREP_PRTLEV));
  getPrepareModel().setPrintLevel(choice->GetSelection());
  setSaveState(true);
  event.Skip();
}
/**
 * Link editor event handlers
 */
void MDPrep::OnButtonMdprepLnkSelectClick( wxCommandEvent& event )
{
   ewxTextCtrl *atm1 = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_LNK_ATOM1));
   ewxTextCtrl *atm2 = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_LNK_ATOM2));
   int natom = p_fragSummary.getNumSelected();
   if (natom == 2) {
      atm1->SetValue(p_fragSummary.getAtomString(0));
      atm2->SetValue(p_fragSummary.getAtomString(1));
   } else {
     p_feedback->setMessage("You must select two atoms", WxFeedback::ERROR);
     requestBuilder();
   }
   setSaveState(true);
   event.Skip();
}



void MDPrep::OnButtonMdprepLnkAddClick( wxCommandEvent& event )
{
   ewxTextCtrl *atm1 = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_LNK_ATOM1));
   ewxTextCtrl *atm2 = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MDPREP_LNK_ATOM2));
   string cmd("  link");
   cmd.append(" ");
   cmd.append(atm1->GetBaseValue().c_str());
   cmd.append(" ");
   cmd.append(atm2->GetBaseValue().c_str());
   ewxListBox *list = ((ewxListBox*)FindWindow(ID_LISTBOX_MDPREP_LNK_LIST));
   list->Append(cmd.c_str());
   getPrepareModel().appendLinkList(cmd);
   getPrepareModel().setChangedTopology(true);
   setSaveState(true);
   event.Skip();
}



void MDPrep::OnButtonMdprepLnkDeleteClick( wxCommandEvent& event )
{
   wxArrayInt selections;
   ewxListBox *list = ((ewxListBox*)FindWindow(ID_LISTBOX_MDPREP_LNK_LIST));
   list->GetSelections(selections); 
   if (selections.size() > 0) {
      int selected = selections[0];
      list->Delete(selected);
      getPrepareModel().deleteLinkList(selected);
      if (selected > 0) {
        list->SetSelection(selected-1, true);
      }
   }
   getPrepareModel().setChangedTopology(true);
   setSaveState(true);
   event.Skip();
}



/**
 * Make sure changes are saved if any.
 */
void MDPrep::OnCloseWindow( wxCloseEvent& event )
{
   doQuit(false, true);
   
}


/**
 * Re-generate the input file from the model and upload it to the data server.
 * @throw IOException if the input file generation failed for some reason or
 *       we couldn't upload the data.
 */
void MDPrep::generateInputFile(MdTask *task) throw(IOException)
{
   PrepareModel& model = getPrepareModel();

   const JCode *codecap = task->application();
   if (!codecap) {
     throw IOException("Could not retrieve MD Prepare's EDML file.", WHERE);
   }

   // Write the model to a file
   SFile *tmpModel = TempStorage::getTempFile();
   ofstream outfile(tmpModel->path().c_str());
   outfile << task->serializeModel(&model);
   outfile.flush();
   outfile.close();

   // Run the conversion script
   string inputGenFile;
   codecap->get_string("PrepareInputGenerator", inputGenFile);
   if (inputGenFile.empty()) {
     throw IOException("Could not find prepare input generator file name", WHERE);
   }
   SFile *tmpInput = TempStorage::getTempFile();
   string cmd = Ecce::ecceDataControllersPath();
   cmd += "/" + inputGenFile + " < ";
   cmd += tmpModel->path();
   cmd += " > ";
   cmd += tmpInput->path();
   CommandWrapper syscmd(cmd);
   syscmd.execute();

   // copy input file to web server
   ifstream ifs(tmpInput->path().c_str());
   if (!task->putInputFile(codecap->getCodeFile(JCode::PRIMARY_INPUT).name(), &ifs)) {
      p_feedback->setMessage("Input file copy to web server failed.",
         WxFeedback::ERROR);
      return;
   } else {
     // check whether the force field files and directories exist or not
     // so we can give the user a nice warning if they don't, potentially
     // saving them a lot of heartache in getting the prepare task to work
     Preferences prefs("Builder");

     // Default to NWChem
     string ffname = "NWChem";
     prefs.getString("FFTYPE", ffname);

     vector<string> files;
     string keytag = ffname + "FF";
     if (prefs.getStringList(keytag, files)) {
       for (int idx=0; idx<files.size(); idx++ ) {
         SFile checkFile(files[idx]);
         if (!checkFile.exists())
           p_feedback->setMessage("Force Field parameter file does not exist--"
                                   + files[idx], WxFeedback::WARNING);
       }
     }

     vector<string> dirs;
     keytag = ffname + "SEG";
     if (prefs.getStringList(keytag, dirs)) {
       for (int idx=0; idx<dirs.size(); idx++ ) {
         SDirectory checkDir(dirs[idx]);
         if (!checkDir.exists())
           p_feedback->setMessage("Force Field segment directory does not exist--"
                                   + dirs[idx], WxFeedback::WARNING);
       }
     }


   }
   ifs.close();

   tmpInput->remove();
   delete tmpInput;

   tmpModel->remove();
   delete tmpModel;
}

void MDPrep::initProgressDialog()
{
   if (p_progressDialog != 0 ) {
      delete p_progressDialog;
      p_progressDialog = 0;
   }

   p_progressDialog = new ewxProgressDialog("ECCE MD Task Run", "", 100,
         this, wxPD_AUTO_HIDE|wxPD_CAN_ABORT|wxPD_ELAPSED_TIME);
}


/**
 * Execute a local prepare job.
 */
void MDPrep::OnButtonMdprepLocalClick(wxCommandEvent &event)
{

   string whereami = RCommand::whereami();
   RefMachine* refMachine = RefMachine::abbrevLookup(whereami);
   if (!refMachine) {
      p_feedback->setMessage("Local host must be a registered compute server to run NWChem MD Prepare job.", WxFeedback::ERROR);
      return;
   }

   p_feedback->setMessage("Starting local NWChem MD Prepare task...",
                          WxFeedback::INFO);
   initProgressDialog();
   p_progressDialog->Show();
   p_progressDialog->Update(10, "Creating run directory");

   try {
      PrepareModel& model = getPrepareModel();
      MdTask *calc = 
           dynamic_cast<MdTask*>(EDSIFactory::getResource(model.getUrl()));
      const JCode *codecap = calc->application();
      if (!codecap) {
        throw IOException("Could not retrieve MD Prepare's EDML file.", WHERE);
      }
      
      // If there are unsaved changes, save and generate input
      if (p_feedback->getEditStatus() == WxFeedback::MODIFIED) {
         saveModelAndInputFile(calc);
      } else {
         // Need to make sure the input file exists and if not create it
         vector<EcceURL> inputs = calc->getDataFiles(JCode::PRIMARY_INPUT);
         if (inputs.size() == 0) {
            generateInputFile(calc);
         }
      }

      string remdir, dirpath, scrpath;
      const MachineConfig *conf = MachineConfig::userPref(refMachine->refname());
      EcceURL url = model.getUrl();
      string fullFile = TempStorage::getJobRunDirectoryPath(url);
      string user = Ecce::realUser();
      // If we have a machine configuration and it has a remote directory
      // we'll use it, otherwise, we just need to make a directory up.
      if (conf == (MachineConfig*)0 || (conf && conf->remoteDir() == "") ||
          (conf && conf->userName() != user)) {
         remdir = TempStorage::getJobDirectoryRoot();
         dirpath = scrpath = remdir + fullFile;
      } else {

         remdir = conf->remoteDir();
         dirpath = remdir + fullFile;
         scrpath = conf->scratchDir();
      }

      p_progressDialog->Update(20, "Creating input file");

      // Remove any existing files in the run directory
      string cmd = "/bin/rm -f ";
      cmd += dirpath;
      cmd += "/*";
      CommandWrapper syscmd(cmd);
      syscmd.execute();

      p_progressDialog->Update(30, "Setting up for job launch");

      struct Launchdata launchdata;
      launchdata.machine = refMachine->refname();
      launchdata.rundir = remdir;
      launchdata.remoteShell = "ssh"; // we auto-register the ssh shell
      launchdata.user = user;
      launchdata.nodes = 1;
      launchdata.totalprocs = 1;
      launchdata.queue = "";
      launchdata.maxmemory = 0;
      launchdata.minscratch = 0;
      launchdata.maxwall = "";
      calc->launchdata(launchdata);

      struct Jobdata jobdata;
      jobdata.jobpath = dirpath;
      calc->jobdata(jobdata);

      EcceMap kvargs;
      kvargs["##input##"] = codecap->getCodeFile(JCode::PRIMARY_INPUT).name();
      kvargs["##output##"] = codecap->getCodeFile(JCode::PRIMARY_OUTPUT).name();
      kvargs["##parse##"] = "ecce.out";
      kvargs["##property##"] =
                           codecap->getCodeFile(JCode::PROPERTY_OUTPUT).name();
      kvargs["##auxiliary##"] =
                           codecap->getCodeFile(JCode::AUXILIARY_OUTPUT).name();
      kvargs["##output_frag##"] = calc->getOutputFragmentName();
      kvargs["##restart##"] = calc->getRestartName();
      kvargs["##topology##"] = calc->getTopologyName();

      string tmp = calc->getName();
      kvargs["##title##"] = tmp;

      kvargs["##account_no##"] = "";
      kvargs["##maxmemory##"] = "";
      kvargs["##minscratch##"] = "";
      kvargs["##queue##"] = "";
      kvargs["##numNodes##"] = "1";
      kvargs["##numProcs##"] = "1";
      kvargs["##scratch##"] = scrpath;
      kvargs["##runDir##"] = dirpath;
      kvargs["##priority##"] = "";

      Launch *launch = new Launch(calc, kvargs, true);
      bool valid = true;

      int progress = 40;
      string msg;
      while (valid && !launch->done()) {
         msg = "Launching job: "  + launch->description();
         p_progressDialog->Update(progress, msg.c_str());
         progress += 5;

         p_feedback->setMessage(launch->description(), WxFeedback::INFO);

         if (!(valid = launch->nextOperation())) {
            p_feedback->setMessage(launch->message(), WxFeedback::ERROR);
         } else if (launch->message() != "") {
            p_feedback->setMessage(launch->message(), WxFeedback::WARNING);
         } else if (launch->info() != "") {
            p_feedback->setMessage(launch->info(), WxFeedback::INFO);
         }
      }

      if (!launch->done()) {
         p_feedback->setMessage("NWChem MD Prepare launch failed!  " + launch->message(), WxFeedback::ERROR);
         p_progressDialog->Update(100, "NWChem MD Prepare launch failed!");
         sleep(3);
         //p_progressDialog->Destroy();
         //p_progressDialog = 0;
      } else if (valid) {
         p_progressDialog->Update(95, "NWChem MD Prepare task running!");
         string statestr = ResourceUtils::stateToString(calc->getState());
         if (statestr == "Complete") {
           try {
             MdTask *task = 
                 dynamic_cast<MdTask*>(EDSIFactory::getResource(p_model->getUrl()));
             saveModelAndInputFile(task);
           } catch (EcceException& ex) {
             p_feedback->setMessage("Unable to save state at end of Prepare Task",
                                    WxFeedback::ERROR);
           }
         }
      }
      delete launch;
   } catch (EcceException& ex) {
     p_feedback->setMessage(ex.what(), WxFeedback::ERROR);
   }
}


void MDPrep::OnButtonMdprepRemoteClick(wxCommandEvent &event)
{

   try {

      PrepareModel& model = getPrepareModel();
      MdTask *calc = 
           dynamic_cast<MdTask*>(EDSIFactory::getResource(model.getUrl()));

      if (p_feedback->getEditStatus() == WxFeedback::MODIFIED) {
         saveModelAndInputFile(calc);
      }

      JMSPublisher publisher(WHOAMI);
      JMSMessage *msg = publisher.newMessage();
      msg->addProperty("action","start");
      publisher.publish("ecce_activity",*msg);
      delete msg;

      // Send the start app request to gateway:
      Target gateway(GATEWAY, "");
      msg = publisher.newMessage(gateway);

      msg->addProperty("appname",LAUNCHER);
      msg->addProperty("calcurl",model.getUrl());

      publisher.publish("ecce_get_app",*msg);
   } catch (EcceException& ex) {
      p_feedback->setMessage(ex.what(), WxFeedback::ERROR);
   }
}

void MDPrep::OnButtonMdprepResetClick( wxCommandEvent& event )
{
  ewxMessageDialog prompt(this,"Are you sure you want to reset all values?",
      "ECCE Reset",wxYES_NO);
  int status = prompt.ShowModal();
  if (status == wxID_YES) {
    getPrepareModel().reset();
    refreshGUI();
  }
}


/**
 * Request builder's ff config dialog to see if we can get away with this design
 * rather than editing the .nwchemrc file.
 */
void MDPrep::OnButtonMdprepConfigureClick( wxCommandEvent& event )
{
   setSaveState(true);
   WxFFDataConfig dialog(this);
   dialog.ShowModal();
}



/**
 * Changes gui to show the user whether there are unsaved changes.
 * MDPrepare is a little bit special in that after its run, we still want
 * to allow the user to change things and rerun easily (no manual reset step
 * required).  So if the state is changing back to ready, we do a couple of
 * things for consistency:
 *  <li>change state
 *  <li>save changes
 *  <li>obliterate the output fragment because the user should see the
 *     input frag that will be used with the next run.
 * NOTE: The implication is that you must call setSaveState(true) AFTER
 * you make changes to the model or the saved model will not match the model
 * in memory.
 */
void MDPrep::setSaveState(bool unsavedChanges)
{
   if (!p_ignoreCodeEventsFlag) {
      wxMenuItem *item = 0;
      wxMenuBar *menuBar = GetMenuBar();
      if (menuBar) {
         item = menuBar->FindItem(wxID_SAVE);
      }

      if (item) item->Enable(unsavedChanges);
      if (unsavedChanges) {
         if (p_feedback->getEditStatus() != WxFeedback::MODIFIED) {
            p_feedback->setEditStatus(WxFeedback::MODIFIED);
         }
      } else if (p_feedback->getRunState() >=
                 ResourceDescriptor::STATE_SUBMITTED) {
         p_feedback->setEditStatus(WxFeedback::READONLY);
      } else {
         p_feedback->setEditStatus(WxFeedback::EDIT);
      }
   }
}


/**
 * Disable the gui depending on the state.
 * This method should be called before setting the feedback state to
 * avoid annoying redundant messages.
 * @param state new state that will be taking effect. 
 * @todo optimize so call is not made if there is no effective change
 */
void MDPrep::updateDisabledState(ResourceDescriptor::RUNSTATE state, bool forceMsg)
{

   if (state >= ResourceDescriptor::STATE_SUBMITTED) {
      p_feedback->setEditStatus(WxFeedback::READONLY);
      setFrameCustomDisabledStyle(false);
      if (state==ResourceDescriptor::STATE_RUNNING ||
          (forceMsg && state>ResourceDescriptor::STATE_RUNNING)) {
         p_feedback->setMessage("This calculation is read only and setup cannot be changed without resetting after completion.", WxFeedback::INFO);
      }
   } else {
      setFrameCustomDisabledStyle(true);
   }

   // A few buttons should never be disabled
   ewxButton *btn = ((ewxButton*)FindWindow(ID_BUTTON_MDPREP_BUILDER));
   btn->setCustomDisabledStyle(true);
   btn = ((ewxButton*)FindWindow(ID_BUTTON_MDPREP_FINAL_EDIT));
   btn->setCustomDisabledStyle(true);
   btn = ((ewxButton*)FindWindow(ID_BUTTON_MDPREP_CHEMSYS));
   btn->setCustomDisabledStyle(true);
   int ids[] = { ID_BUTTON_PMF_SHOW_ATOMS1, ID_BUTTON_PMF_SHOW_ATOMS2,
                 ID_BUTTON_PMF_SHOW_ATOMS3, ID_BUTTON_PMF_SHOW_ATOMS4 };
   for (int i = 0; i < 4; i ++) {
     ((ewxButton*)FindWindow(ids[i]))->setCustomDisabledStyle(true);
   }
}


/**
 * Set calculation state on data server, update feedback area,
 * and notify others that the state has changed.
 */
void MDPrep::setRunState(ResourceDescriptor::RUNSTATE state) 
{
   string url = getPrepareModel().getUrl();
   MdTask *mdTask =  
      dynamic_cast<MdTask*>(EDSIFactory::getResource(EcceURL(url)));
   if (mdTask) {
      mdTask->setState(state);
      // Call before actually changing state so it can decide whether 
      // or not to post a message
      updateDisabledState(state);
      p_feedback->setRunState(state);
   }
}


/**
 * Determines if there are unsaved changes.
 * This just looks at the gui indicators so don't do something like
 * call isUnsavedState to determine whether to call setSaveState.
 */
bool MDPrep::isUnsavedState()
{
   return (p_feedback->getEditStatus() == WxFeedback::MODIFIED);
}


/**
 * Process menu save button click.
 * The data is saved to the server and the gui updated.
 */
void MDPrep::OnSaveClick( wxCommandEvent& event )
{
   try {
      MdTask *task = 
         dynamic_cast<MdTask*>(EDSIFactory::getResource(p_model->getUrl()));
      saveModelAndInputFile(task);
   } catch (EcceException& ex) {
      p_feedback->setMessage(ex.what(), WxFeedback::ERROR);
   }
}



void MDPrep::OnMenuFeedbackClick( wxCommandEvent& event )
{
  BrowserHelp help;
  help.showFeedbackPage();
}



void MDPrep::OnMenuMdprepHelpClick( wxCommandEvent& event )
{
  string helpkey = getTitle();
  size_t ind;
  while ((ind = helpkey.find(' ')) != string::npos)
    helpkey.erase(ind, 1);

  BrowserHelp help;
  help.showPage(help.URL(helpkey));
}



void MDPrep::OnButtonMdprepFinalEditClick( wxCommandEvent& event )
{
   try {

     PrepareModel& model = getPrepareModel();
     MdTask *task = dynamic_cast<MdTask*>(EDSIFactory::getResource(model.getUrl()));

     // Save if needed
     if (p_feedback->getEditStatus() == WxFeedback::MODIFIED) {
       saveModelAndInputFile(task);
     }

     // Generate an input file if one doesn't exist
     if (task->getDataFileNames(JCode::PRIMARY_INPUT).empty()) {
       generateInputFile(task);
     }

     istream* is = task->getDataFile(JCode::PRIMARY_INPUT);
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
        sessionMgr.edit(text, "testing", this, roFlag,"NWChem MD Prepare Input");

     } else {
        p_feedback->setMessage("Input file not found.", WxFeedback::WARNING);
     }

   } catch (EcceException& ex) {
      p_feedback->setMessage(ex.what(), WxFeedback::ERROR);
   }
}



/**
 * EditListener interface.
 * An edit session has terminated.
 */
void MDPrep::processEditCompletion(const EditEvent& ee)
{
   wxBusyCursor busy;
   ifstream ifs(ee.filename.c_str());

   string infile;
   TypedFile tinfile;
   MdTask *task = 
       dynamic_cast<MdTask*>(EDSIFactory::getResource(getPrepareModel().getUrl()));
   task->getDataFile(JCode::PRIMARY_INPUT, tinfile);
   infile = tinfile.name();

   if (!task->putInputFile(infile, &ifs)) {
      p_feedback->setMessage("Input file could not be copied back to DAV",
            WxFeedback::ERROR);
   } else {
     p_feedback->setMessage("The input file has been saved. Any changes "
            "you make to the gui will override your manual edits.",
            WxFeedback::INFO);
   }
   ifs.close();


}


void MDPrep::OnButtonMdprepDeleteClick( wxCommandEvent& event )
{
    // Insert custom code here
    wxArrayInt selections;
    p_listBox->GetSelections(selections);
    if (selections.size() > 0) {
      int selected = selections[0];
      p_listBox->Delete(selected);
      getPrepareModel().deleteCmdList(selected);
      if (selected > 0) {
        p_listBox->SetSelection(selected-1, true);
      }
      setSaveState(true);
    }
    event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_UP
 */

void MDPrep::OnButtonMdprepUpClick( wxCommandEvent& event )
{
    // Insert custom code here
    wxArrayInt selections;
    p_listBox->GetSelections(selections);
    if (selections.size() > 0) {
      int save = selections[0];
      if (save > 0 && p_listBox->GetCount() > 1) {
        wxString buf = p_listBox->GetString(save);
        p_listBox->Delete(selections[0]);
        p_listBox->Insert(buf,save-1);
        p_listBox->SetSelection(save-1);
        getPrepareModel().deleteCmdList(selections[0]);
        getPrepareModel().insertCmdList(save-1, buf.c_str());
        setSaveState(true);
      }
    }
    event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_DOWN
 */

void MDPrep::OnButtonMdprepDownClick( wxCommandEvent& event )
{
    // Insert custom code here
    wxArrayInt selections;
    p_listBox->GetSelections(selections);
    if (selections.size() > 0) {
      int save = selections[0];
      int count = p_listBox->GetCount();
      if (save < count-1 && count > 1) {
        wxString buf = p_listBox->GetString(save);
        p_listBox->Delete(selections[0]);
        getPrepareModel().deleteCmdList(selections[0]);
        if (save != count-2) {
          p_listBox->Insert(buf,save+1);
          getPrepareModel().insertCmdList(save+1, buf.c_str());
        } else {
          p_listBox->Append(buf);
          getPrepareModel().appendCmdList(buf.c_str());
        }
        p_listBox->SetSelection(save+1);
        setSaveState(true);
      }
    }
    event.Skip();
}


/**
 * Send message asking for a builder and for the builder to send a charge.
 */
void MDPrep::OnButtonMdprepGetchargeClick(wxCommandEvent&)
{
   requestBuilder("getcharge");
}



void MDPrep::OnChoicePmfOptionSelected( wxCommandEvent& event )
{
  doPmfOptionSelected(event.GetString());
}


void MDPrep::doPmfOptionSelected(wxString choice)
{
  // must explicitly "select" the choice
  ((wxChoice*) FindWindow(ID_CHOICE_PMF_OPTION))->SetStringSelection(choice);
  
  // hide/show "all" checkbox
  if (choice.StartsWith("zalign")) {
    FindWindow(ID_CHECKBOX_PMF_ALL)->Show();
  } else {
    FindWindow(ID_CHECKBOX_PMF_ALL)->Hide();
  }

  // hide/show "combine" checkbox
  if (choice.StartsWith("xyplane")) {
    FindWindow(ID_CHECKBOX_PMF_COMBINE)->Show();
  } else {
    FindWindow(ID_CHECKBOX_PMF_COMBINE)->Hide();
  }

  // hide/show "constraint" checkbox
  if (choice.EndsWith("distance")) {
    FindWindow(ID_CHECKBOX_PMF_CONSTRAINT)->Show();
  } else {
    FindWindow(ID_CHECKBOX_PMF_CONSTRAINT)->Hide();
  }

  // hide/show atom selection 2
  if (!(choice.StartsWith("zalign")) ||
       (choice.StartsWith("xyplane"))) {
    FindWindow(ID_PANEL_PMF_SELECT_ATOMS2)->Show();
  } else {
    FindWindow(ID_PANEL_PMF_SELECT_ATOMS2)->Hide();
  }

  // hide/show atom selection 3
  if (choice.StartsWith("angle") ||
      choice.StartsWith("torsion") ||
      choice.StartsWith("zaxis")) {
    FindWindow(ID_PANEL_PMF_SELECT_ATOMS3)->Show();
  } else {
    FindWindow(ID_PANEL_PMF_SELECT_ATOMS3)->Hide();
  }

  // hide/show atom selection 4
  if (choice.StartsWith("torsion")) {
    FindWindow(ID_PANEL_PMF_SELECT_ATOMS4)->Show();
  } else {
    FindWindow(ID_PANEL_PMF_SELECT_ATOMS4)->Hide();
  }

  // hide/show non-distance and non-angle force
  if (choice.StartsWith("zalign") ||
      choice.StartsWith("xyplane")) {
    FindWindow(ID_PANEL_PMF_FORCE)->Show();
  } else {
    FindWindow(ID_PANEL_PMF_FORCE)->Hide();
  }

  // hide/show distance
  if (choice.StartsWith("distance") ||
      choice.StartsWith("zdistance") ||
      choice.StartsWith("basepair") ||
      choice.StartsWith("zaxis")) {
    FindWindow(ID_PANEL_PMF_DISTANCE)->Show();
    FindWindow(ID_PANEL_PMF_FORCE_DISTANCE)->Show();
  } else {
    FindWindow(ID_PANEL_PMF_DISTANCE)->Hide();
    FindWindow(ID_PANEL_PMF_FORCE_DISTANCE)->Hide();
  }

  // hide/show angle
  if (choice.StartsWith("angle") ||
      choice.StartsWith("torsion")) {
    FindWindow(ID_PANEL_PMF_ANGLE)->Show();
  } else {
    FindWindow(ID_PANEL_PMF_ANGLE)->Hide();
  }

  // hide/show angle force
  if (choice.StartsWith("angle")) {
    FindWindow(ID_PANEL_PMF_FORCE_ANGLE)->Show();
  } else {
    FindWindow(ID_PANEL_PMF_FORCE_ANGLE)->Hide();
  }

  // hide/show torsion force
  if (choice.StartsWith("torsion")) {
    FindWindow(ID_PANEL_PMF_FORCE_TORSION)->Show();
  } else {
    FindWindow(ID_PANEL_PMF_FORCE_TORSION)->Hide();
  }

  FindWindow(ID_PANEL_THERMODYNAMICS)->Layout();
}


void MDPrep::OnTextctrlPmfSelectAtoms1Updated( wxCommandEvent& event )
{
  doPmfSelectAtomsUpdated(ID_TEXTCTRL_PMF_SELECT_ATOMS1,
                          ID_STATICTEXT_PMF_SELECT_ATOMS1);
}


void MDPrep::OnButtonPmfSelectAtoms1Click( wxCommandEvent& event )
{
  doPmfSelectAtoms(ID_TEXTCTRL_PMF_SELECT_ATOMS1);
}


void MDPrep::OnButtonPmfShowAtoms1Click( wxCommandEvent& event )
{
  doPmfShowAtoms(ID_TEXTCTRL_PMF_SELECT_ATOMS1);
}


void MDPrep::OnTextctrlPmfSelectAtoms2Updated( wxCommandEvent& event )
{
  doPmfSelectAtomsUpdated(ID_TEXTCTRL_PMF_SELECT_ATOMS2,
                          ID_STATICTEXT_PMF_SELECT_ATOMS2);
}


void MDPrep::OnButtonPmfSelectAtoms2Click( wxCommandEvent& event )
{
  doPmfSelectAtoms(ID_TEXTCTRL_PMF_SELECT_ATOMS2);
}


void MDPrep::OnButtonPmfShowAtoms2Click( wxCommandEvent& event )
{
  doPmfShowAtoms(ID_TEXTCTRL_PMF_SELECT_ATOMS2);
}


void MDPrep::OnTextctrlPmfSelectAtoms3Updated( wxCommandEvent& event )
{
  doPmfSelectAtomsUpdated(ID_TEXTCTRL_PMF_SELECT_ATOMS3, 
                          ID_STATICTEXT_PMF_SELECT_ATOMS3);
}


void MDPrep::OnButtonPmfSelectAtoms3Click( wxCommandEvent& event )
{
  doPmfSelectAtoms(ID_TEXTCTRL_PMF_SELECT_ATOMS3);
}


void MDPrep::OnButtonPmfShowAtoms3Click( wxCommandEvent& event )
{
  doPmfShowAtoms(ID_TEXTCTRL_PMF_SELECT_ATOMS3);
}


void MDPrep::OnTextctrlPmfSelectAtoms4Updated( wxCommandEvent& event )
{
  doPmfSelectAtomsUpdated(ID_TEXTCTRL_PMF_SELECT_ATOMS4, 
                          ID_STATICTEXT_PMF_SELECT_ATOMS4);
}


void MDPrep::OnButtonPmfSelectAtoms4Click( wxCommandEvent& event )
{
  doPmfSelectAtoms(ID_TEXTCTRL_PMF_SELECT_ATOMS4);
}


void MDPrep::OnButtonPmfShowAtoms4Click( wxCommandEvent& event )
{
  doPmfShowAtoms(ID_TEXTCTRL_PMF_SELECT_ATOMS4);
}


void MDPrep::doPmfSelectAtomsUpdated( int idText, int idIndex )
{
  wxTextCtrl * text = dynamic_cast<wxTextCtrl*>(FindWindow(idText));
  wxStaticText * indexText = dynamic_cast<wxStaticText*>(FindWindow(idIndex));
  int selection = getPrepareModel().getPmfSelection(text->GetValue().c_str());
  if (selection == -1) {
    indexText->SetLabel("(#)");
  } else {
    wxString label;
    label << "(" << selection << ")";
    indexText->SetLabel(label);
  }
}


void MDPrep::doPmfSelectAtoms( int id )
{
  pmfSelectionId = id;
  requestBuilder("getNWChemSelection");  
}


void MDPrep::doPmfShowAtoms( int id )
{
  wxTextCtrl * text = dynamic_cast<wxTextCtrl*>(FindWindow(id));
  map<string,string> * properties = new map<string,string>;
  (*properties)["selection"] = text->GetValue().c_str();
  requestBuilder("showNWChemSelection", true, properties);  
  delete properties;
}


void MDPrep::OnButtonPmfDefaultsDistanceClick( wxCommandEvent& event )
{
  wxTextCtrl * text1 = dynamic_cast<wxTextCtrl*>(
          FindWindow(ID_TEXTCTRL_PMF_SELECT_ATOMS1));
  wxTextCtrl * text2 = dynamic_cast<wxTextCtrl*>(
          FindWindow(ID_TEXTCTRL_PMF_SELECT_ATOMS2));

  map<string,string> * properties = new map<string,string>;
  (*properties)["selection1"] = text1->GetValue().c_str();
  (*properties)["selection2"] = text2->GetValue().c_str();
  requestBuilder("getNWChemDistance", true, properties);
  delete properties;
}


void MDPrep::OnButtonPmfDefaultsAngleClick( wxCommandEvent& event )
{
  wxTextCtrl * text1 = dynamic_cast<wxTextCtrl*>(
          FindWindow(ID_TEXTCTRL_PMF_SELECT_ATOMS1));
  wxTextCtrl * text2 = dynamic_cast<wxTextCtrl*>(
          FindWindow(ID_TEXTCTRL_PMF_SELECT_ATOMS2));
  wxTextCtrl * text3 = dynamic_cast<wxTextCtrl*>(
          FindWindow(ID_TEXTCTRL_PMF_SELECT_ATOMS3));

  map<string,string> * properties = new map<string,string>;
  (*properties)["selection1"] = text1->GetValue().c_str();
  (*properties)["selection2"] = text2->GetValue().c_str();
  (*properties)["selection3"] = text3->GetValue().c_str();

  // calculates either the regular angle or torsion angle, depending on choice
  wxChoice * choice = dynamic_cast<wxChoice*>(FindWindow(ID_CHOICE_PMF_OPTION));
  if (! choice->GetStringSelection().IsSameAs("torsion", false)) {
    requestBuilder("getNWChemAngle", true, properties);
  } else {
    wxTextCtrl * text4 = dynamic_cast<wxTextCtrl*>(
            FindWindow(ID_TEXTCTRL_PMF_SELECT_ATOMS4));
    (*properties)["selection4"] = text4->GetValue().c_str();
    requestBuilder("getNWChemTorsionAngle", true, properties);
  }
  delete properties;
}


/**
 * Adds PMF directive to list based on fields in the thermodynamics PMF panel.
 * If a text control is shown and empty, this is an error and the user is 
 * notified (and no directive will be added to list in that case.)
 */
void MDPrep::OnButtonPmfAddDirectiveClick( wxCommandEvent& event )
{
  try {
    string missingMsg = "An input field is missing a value. Please correct.";

    // pointers we'll be using throughout the remainder of this function
    ewxTextCtrl *textCtrl;
    wxCheckBox  *checkBox;
  
    // we are going to build our command
    wxString command = "pmf ";

    // process the optional checkboxes, if they are shown
    checkBox = (wxCheckBox*) FindWindow(ID_CHECKBOX_PMF_ALL);
    if (checkBox->IsShownOnScreen() && checkBox->IsChecked()) {
      command << "all ";
    }
    checkBox = (wxCheckBox*) FindWindow(ID_CHECKBOX_PMF_COMBINE);
    if (checkBox->IsShownOnScreen() && checkBox->IsChecked()) {
      command << "combine ";
    }
    checkBox = (wxCheckBox*) FindWindow(ID_CHECKBOX_PMF_CONSTRAINT);
    if (checkBox->IsShownOnScreen() && checkBox->IsChecked()) {
      command << "constraint ";
    }
    
    // was "bias" checked?  
    if (((wxCheckBox*) FindWindow(ID_CHECKBOX_PMF_BIAS))->IsChecked()) {
      command << "bias ";
    }

    // add the choicebox string (the actual directive)
    wxString directive = ((wxChoice*) FindWindow(ID_CHOICE_PMF_OPTION))-> GetStringSelection();
    command << directive << " ";
  
    // atom selections come next
    // if the fields are hidden, then they don't apply (except #1 of course)
    // we also check to see if selection has been made previously, and use the
    // stored index or otherwise create a new index
    int selectAtomIds[] = { ID_TEXTCTRL_PMF_SELECT_ATOMS1,
                            ID_TEXTCTRL_PMF_SELECT_ATOMS2,
                            ID_TEXTCTRL_PMF_SELECT_ATOMS3,
                            ID_TEXTCTRL_PMF_SELECT_ATOMS4 };
    int selectAtomIndices[] = { ID_STATICTEXT_PMF_SELECT_ATOMS1,
                                ID_STATICTEXT_PMF_SELECT_ATOMS2,
                                ID_STATICTEXT_PMF_SELECT_ATOMS3,
                                ID_STATICTEXT_PMF_SELECT_ATOMS4 };
    for (int i = 0; i < 4; i++) {
      textCtrl = (ewxTextCtrl*) FindWindow(selectAtomIds[i]);
      if (textCtrl->IsShownOnScreen()) {
        if (textCtrl->IsEmpty()) { throw missingMsg; }
        command << getPrepareModel().
                addPmfSelection(textCtrl->GetValue().c_str()) << " ";
        doPmfSelectAtomsUpdated( selectAtomIds[i], selectAtomIndices[i] );
      }
    }
  
    // distance, angle, and other are exclusive
    // if the fields are hidden, then they don't apply
    // force always comes last
    int fieldIds[] = { ID_TEXTCTRL_PMF_DISTANCE1,
                       ID_TEXTCTRL_PMF_DISTANCE2,
                       ID_TEXTCTRL_PMF_ANGLE1,
                       ID_TEXTCTRL_PMF_ANGLE2,
                       ID_TEXTCTRL_PMF_FORCE1,
                       ID_TEXTCTRL_PMF_FORCE2,
                       ID_TEXTCTRL_PMF_FORCE1_DISTANCE,
                       ID_TEXTCTRL_PMF_FORCE2_DISTANCE,
                       ID_TEXTCTRL_PMF_FORCE1_ANGLE,
                       ID_TEXTCTRL_PMF_FORCE2_ANGLE,
                       ID_TEXTCTRL_PMF_FORCE1_TORSION,
                       ID_TEXTCTRL_PMF_FORCE2_TORSION };
    for (int i = 0; i < 12; i++) {
      textCtrl = (ewxTextCtrl*) FindWindow(fieldIds[i]);
      if (textCtrl->IsShownOnScreen()) {
        if (textCtrl->IsEmpty()) { throw missingMsg; }
        command += textCtrl->GetBaseValue() + " ";
      }
    }
    
    // add finished command to directives list
    wxListBox * listBox = (wxListBox*) FindWindow(ID_LISTBOX_PMF_DIRECTIVES);
    listBox->SetFirstItem(listBox->Append(command));
    getPrepareModel().appendPmfCmdList(command.c_str());
    setSaveState(true);

  } catch (string missingMsg) {
    p_feedback->setMessage(missingMsg, WxFeedback::ERROR);
  }
}


void MDPrep::OnListboxPmfDirectivesSelected( wxCommandEvent& event )
{
  wxListBox * listBox = (wxListBox*) FindWindow(ID_LISTBOX_PMF_DIRECTIVES);
  if (listBox->GetSelection() != wxNOT_FOUND) {
    doPmfRestoreDirective(listBox->GetStringSelection());
  }
}


void MDPrep::doPmfRestoreDirective( wxString directive )
{
  wxString option;
  bool hasGroup[] = { true, false, false, false };
  bool hasDistance = false;
  bool hasAngle = false;
  bool hasTorsion = false;

  StringTokenizer tokenizer(directive.c_str(), " ");
  wxString token = tokenizer.next(); // "pmf"
  token = tokenizer.next(); // "all|combine|constraint|bias|<command>"

  // the easiest (and safest) thing to do is to clear all checkboxes
  // and then restore them based on the token
  ((wxCheckBox*) FindWindow(ID_CHECKBOX_PMF_ALL))->SetValue(false);
  ((wxCheckBox*) FindWindow(ID_CHECKBOX_PMF_BIAS))->SetValue(false);
  ((wxCheckBox*) FindWindow(ID_CHECKBOX_PMF_COMBINE))->SetValue(false);
  ((wxCheckBox*) FindWindow(ID_CHECKBOX_PMF_CONSTRAINT))->SetValue(false);
  while(true) {
    if (token.IsSameAs("all")) {
      ((wxCheckBox*) FindWindow(ID_CHECKBOX_PMF_ALL))->SetValue(true);
    } else if (token.IsSameAs("bias")) {
      ((wxCheckBox*) FindWindow(ID_CHECKBOX_PMF_BIAS))->SetValue(true);
    } else if (token.IsSameAs("combine")) {
      ((wxCheckBox*) FindWindow(ID_CHECKBOX_PMF_COMBINE))->SetValue(true);
    } else if (token.IsSameAs("constraint")) {
      ((wxCheckBox*) FindWindow(ID_CHECKBOX_PMF_CONSTRAINT))->SetValue(true);
    } else {
      break;
    }
    token = tokenizer.next(); // "bias|<command>"
  }

  // the rest of the directive is parsed depending on the command
  // <command> === zalign|xyplane|distance|zdistance|angle|torsion|basepair|
  // zaxis|zaxis-cog
  if (token.IsSameAs("zalign")) {
    // do nothing
  } else if (token.IsSameAs("xyplane")) {
    // do nothing
  } else if (token.IsSameAs("distance") || token.IsSameAs("zdistance")) {
    hasGroup[1] = hasDistance = true;
  } else if (token.IsSameAs("angle")) {
    hasGroup[1] = hasGroup[2] = hasAngle = true;
  } else if (token.IsSameAs("torsion")) {
    hasGroup[1] = hasGroup[2] = hasGroup[3] = hasAngle = hasTorsion = true;
  } else if (token.IsSameAs("basepair")) {
    hasGroup[1] = hasDistance = true;
  } else if (token.IsSameAs("zaxis") || token.IsSameAs("zaxis-cog")) {
    hasGroup[1] = hasGroup[2] = hasDistance = true;
  } else {
    // fall-through, must be error
    throw "restore PMF directive error";
  }

  // hide/show fields based on command
  doPmfOptionSelected(token);

  int selectAtomIds[] = { ID_TEXTCTRL_PMF_SELECT_ATOMS1,
                          ID_TEXTCTRL_PMF_SELECT_ATOMS2,
                          ID_TEXTCTRL_PMF_SELECT_ATOMS3,
                          ID_TEXTCTRL_PMF_SELECT_ATOMS4 };
  int index;
  for (int i = 0; i < 4; i++) {
    if (hasGroup[i]) {
      StringConverter::toInt(tokenizer.next(), index);
      ((wxTextCtrl*) FindWindow(selectAtomIds[i]))->
              SetValue(getPrepareModel().getPmfSelection(index));
    } else {
      break;
    }
  }

  if (hasTorsion) {
    ((ewxTextCtrl*) FindWindow(ID_TEXTCTRL_PMF_ANGLE1))->
            SetBaseValue(tokenizer.next());
    ((ewxTextCtrl*) FindWindow(ID_TEXTCTRL_PMF_ANGLE2))->
            SetBaseValue(tokenizer.next());
    ((ewxTextCtrl*) FindWindow(ID_TEXTCTRL_PMF_FORCE1_TORSION))->
            SetBaseValue(tokenizer.next());
    ((ewxTextCtrl*) FindWindow(ID_TEXTCTRL_PMF_FORCE2_TORSION))->
            SetBaseValue(tokenizer.next());
  } else if (hasAngle) {
    ((ewxTextCtrl*) FindWindow(ID_TEXTCTRL_PMF_ANGLE1))->
            SetBaseValue(tokenizer.next());
    ((ewxTextCtrl*) FindWindow(ID_TEXTCTRL_PMF_ANGLE2))->
            SetBaseValue(tokenizer.next());
    ((ewxTextCtrl*) FindWindow(ID_TEXTCTRL_PMF_FORCE1_ANGLE))->
            SetBaseValue(tokenizer.next());
    ((ewxTextCtrl*) FindWindow(ID_TEXTCTRL_PMF_FORCE2_ANGLE))->
            SetBaseValue(tokenizer.next());
  } else if (hasDistance) {
    ((ewxTextCtrl*) FindWindow(ID_TEXTCTRL_PMF_DISTANCE1))->
            SetBaseValue(tokenizer.next());
    ((ewxTextCtrl*) FindWindow(ID_TEXTCTRL_PMF_DISTANCE2))->
            SetBaseValue(tokenizer.next());
    ((ewxTextCtrl*) FindWindow(ID_TEXTCTRL_PMF_FORCE1_DISTANCE))->
            SetBaseValue(tokenizer.next());
    ((ewxTextCtrl*) FindWindow(ID_TEXTCTRL_PMF_FORCE2_DISTANCE))->
            SetBaseValue(tokenizer.next());
  } else {
    ((ewxTextCtrl*) FindWindow(ID_TEXTCTRL_PMF_FORCE1))->
            SetBaseValue(tokenizer.next());
    ((ewxTextCtrl*) FindWindow(ID_TEXTCTRL_PMF_FORCE2))->
            SetBaseValue(tokenizer.next());
  }
}


void MDPrep::OnButtonPmfUpClick( wxCommandEvent& event )
{
  wxListBox * listBox = (wxListBox*) FindWindow(ID_LISTBOX_PMF_DIRECTIVES);
  int index = listBox->GetSelection();
  if (index == wxNOT_FOUND) {
    return;
  }

  int count = listBox->GetCount();
  if (count < 2 || index == 0) {
    return;
  }

  wxString value = listBox->GetString(index);
  listBox->Delete(index);
  getPrepareModel().deletePmfCmdList(index);
  listBox->Insert(value, index - 1);
  getPrepareModel().insertPmfCmdList(index - 1, value.c_str());
  setSaveState(true);
  listBox->Select(index - 1);
  listBox->SetFirstItem(index - 1);
  listBox->SetFocus();
}


void MDPrep::OnButtonPmfDownClick( wxCommandEvent& event )
{
  wxListBox * listBox = (wxListBox*) FindWindow(ID_LISTBOX_PMF_DIRECTIVES);
  int index = listBox->GetSelection();
  if (index == wxNOT_FOUND) {
    return;
  }

  int count = listBox->GetCount();
  if (count < 2 || index + 1 == count) {
    return;
  }

  wxString value = listBox->GetString(index);
  listBox->Delete(index);
  getPrepareModel().deletePmfCmdList(index);
  listBox->Insert(value, index + 1);
  getPrepareModel().insertPmfCmdList(index + 1, value.c_str());
  setSaveState(true);
  listBox->Select(index + 1);
  listBox->SetFirstItem(index + 1);
  listBox->SetFocus();
}


void MDPrep::OnButtonPmfDeleteClick( wxCommandEvent& event )
{
  wxListBox * listBox = (wxListBox*) FindWindow(ID_LISTBOX_PMF_DIRECTIVES);
  int index = wxNOT_FOUND;
  if ((index = listBox->GetSelection()) != wxNOT_FOUND) {
    // clean-up select statement references --- UGLY and copied code elsewhere
    wxString directive = listBox->GetString(index);
    StringTokenizer tokenizer(directive.c_str(), " ");
    wxString token = tokenizer.next(); // "pmf"
    token = tokenizer.next(); // "all|combine|constraint|bias|command"
    if (token.IsSameAs("all") ||
        token.IsSameAs("combine") ||
        token.IsSameAs("constraint")) {
      token = tokenizer.next(); // bias||command
    }
    if (token.IsSameAs("bias")) {
      token = tokenizer.next(); // command
    }
    int count = 1;
    if (token.IsSameAs("zalign")) {
      // do nothing
    } else if (token.IsSameAs("xyplane") ||
        token.IsSameAs("distance") ||
        token.IsSameAs("zdistance") ||
        token.IsSameAs("basepair")) {
      count = 2;
    } else if (token.IsSameAs("angle") ||
        token.IsSameAs("zaxis") ||
        token.IsSameAs("zaxis-cog")) {
      count = 3;
    } else if (token.IsSameAs("torsion")) {
      count = 4;
    }
    int value;
    for (int i = 0; i < count; i++) {
      StringConverter::toInt(tokenizer.next(), value);
      getPrepareModel().removePmfSelection(value);
    }
    
    // do the actual pmf directive removal
    listBox->Delete(index);
    getPrepareModel().deletePmfCmdList(index);
    setSaveState(true);
  }
}

