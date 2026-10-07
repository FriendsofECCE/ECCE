// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "util/JMSMessage.H"

#include "dsm/NWChemMDModel.H"
#include "dsm/EDSIFactory.H"
#include "dsm/MdTask.H"

#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxNotebook.H"
#include "wxgui/ConstraintPanel.H"
#include "wxgui/ControlPanel.H"
#include "wxgui/FilesPanel.H"
#include "wxgui/InteractionPanel.H"
#include "wxgui/OptimizePanel.H"
#include "wxgui/ThermodynamicsPanel.H"
#include "wxgui/WxJMSListener.H"
#include "wxgui/WxJMSMessageDispatch.H"

#include "MDOptimize.H"


MDOptimize::MDOptimize() : MDEdBase(), WxJMSListener()
{
   constructor();
}

MDOptimize::MDOptimize( wxWindow* parent, wxWindowID id, const wxString& caption,
      const wxPoint& pos, const wxSize& size, long style )
: MDEdBase(parent, id, caption, pos, size, style), WxJMSListener()
{
   constructor();
}


/**
 * Returns name used to identify tool in registry.
 */
string MDOptimize::getAppName() const
{
   return MDOPTIMIZE;
}

void MDOptimize::constructor()
{
   initializeMessaging();
   MDEdBase::constructor();
}


/**
 * Create the subpanels (Interaction, Constraint, Control, Files)
 * Safe to call more than once.
 */
void MDOptimize::createPanels()
{
   int numPages = p_notebook->GetPageCount();
   if (numPages == 0) {

      p_interaction = new InteractionPanel(this, p_notebook);
      p_notebook->AddPage(p_interaction, _("Interactions"));
      p_interaction->setInteractionModel(getMDModel().getInteractionModel());
      p_interaction->initializeGUI();

      p_constraint = new ConstraintPanel(this, p_notebook);
      p_notebook->AddPage(p_constraint, _("Constraints"));
      p_constraint->setConstraintModel(getMDModel().getConstraintModel());
      p_constraint->initializeGUI();
      p_constraint->setFragmentSummary(&p_fragSummary);

      p_optimize = new OptimizePanel(this, p_notebook);
      p_notebook->AddPage(p_optimize, _("Optimize"));
      p_optimize->setOptimizeModel(getMDModel().getOptimizeModel());
      p_optimize->initializeGUI();

      p_thermo = new ThermodynamicsPanel(this, p_notebook);
      p_notebook->AddPage(p_thermo, _("Thermodynamics"));
      p_thermo->setThermodynamicsModel(getMDModel().getThermodynamicsModel());
      p_thermo->initializeGUI();
      p_thermo->configOptimizeCalc();

      p_control = new ControlPanel(this, p_notebook);
      p_notebook->AddPage(p_control, _("Control"));
      p_control->setControlModel(getMDModel().getControlModel());
      p_control->initializeGUI();

      p_files = new FilesPanel(this, p_notebook);
      p_notebook->AddPage(p_files, _("Files"));
      p_files->setFilesModel(getMDModel().getFilesModel());
      p_files->initializeGUI();

      p_notebook->Refresh();
      p_notebook->Layout();
      Layout();
   }
}


void MDOptimize::initializeModel()
{
   p_panelSet.clear();
   p_panelSet.push_back(NWChemMDModel::INTERACTION);
   p_panelSet.push_back(NWChemMDModel::CONSTRAINT);
   p_panelSet.push_back(NWChemMDModel::OPTIMIZE);
   p_panelSet.push_back(NWChemMDModel::THERMODYNAMICS);
   p_panelSet.push_back(NWChemMDModel::CONTROL);
   p_panelSet.push_back(NWChemMDModel::FILES);
   MDEdBase::initializeModel();
}


void MDOptimize::bindModels()
{
   p_interaction->setInteractionModel(getMDModel().getInteractionModel());
   p_constraint->setConstraintModel(getMDModel().getConstraintModel());
   p_optimize->setOptimizeModel(getMDModel().getOptimizeModel());
   p_thermo->setThermodynamicsModel(getMDModel().getThermodynamicsModel());
   p_control->setControlModel(getMDModel().getControlModel());
   p_files->setFilesModel(getMDModel().getFilesModel());
}

void MDOptimize::applyCodeProfile(bool gromacs)
{
   p_interaction->applyGromacsProfile(gromacs);
   p_constraint->applyGromacsProfile(gromacs);
   p_optimize->applyGromacsProfile(gromacs);
   p_files->applyGromacsProfile(gromacs);
   // the Control page is NWChem's parallel setup (load balancing,
   // processor and cell layout); a GROMACS job is told its processors
   // when it is launched
   setPageShown(p_control, !gromacs);
}


void MDOptimize::setContext(const string& url)
{
   wxBusyCursor busy;
   MDEdBase::setContext(url);
   p_constraint->getConstraintModel()->setURL(url);

  // the thermo gui always exists, but is shown only for tasks it means
  // something for
  MdTask * task = dynamic_cast<MdTask*>(EDSIFactory::getResource(url));
  setPageShown(p_thermo, !(p_isGromacs || (task && task->hasPmf())));
}



string MDOptimize::getTitle() const
{
   return codeTitle() + " MD Optimize";
}


/**
 * Refreshes each of the panels from the model.
 */
void MDOptimize::refreshGUI()
{
   p_interaction->refreshGUI();
   p_constraint->refreshGUI();
   p_optimize->refreshGUI();
   p_thermo->refreshGUI();
   p_control->refreshGUI();
   p_files->refreshGUI();
   p_files->disableBatchAndKeepOptions();

}

/**
 * Reset the values of the current panel displayed in the notebook to the
 * default values
 */
void MDOptimize::resetPanel()
{
  ewxMessageDialog prompt(this,"Are you sure you want to reset the panel?",
      "ECCE Reset Panel",wxYES_NO);
  int status = prompt.ShowModal();
  if (status == wxID_YES) {
    wxWindow *cur = p_notebook->GetCurrentPage();
    if (cur == p_interaction) {
      p_interaction->getInteractionModel()->reset();
      p_interaction->refreshGUI();
    } else if (cur == p_constraint) {
      p_constraint->getConstraintModel()->reset();
      p_constraint->refreshGUI();
    } else if (cur == p_optimize) {
      p_optimize->getOptimizeModel()->reset();
      p_optimize->refreshGUI();
    } else if (cur == p_thermo) {
      p_thermo->getThermodynamicsModel()->reset();
      p_thermo->refreshGUI();
    } else if (cur == p_control) {
      p_control->getControlModel()->reset();
      p_control->refreshGUI();
    } else if (cur == p_files) {
      p_files->getFilesModel()->reset();
      p_files->refreshGUI();
    }
  }
}


/**
 * Initialize messaging and subscribe to messages.
 */
void MDOptimize::initializeMessaging()
{
   // Inititialize messaging.
   p_msgDispatcher = new WxJMSMessageDispatch(getAppName(), true);

   p_msgDispatcher->subscribe("ecce_invoke",
         this, (wxJmsCBFunc)&MDOptimize::invokeMCB);
   p_msgDispatcher->subscribe("ecce_quit",
         this, (wxJmsCBFunc)&MDOptimize::quitMCB);
   p_msgDispatcher->subscribe("ecce_identify",
         this, (wxJmsCBFunc)&MDOptimize::identifyMCB);

   p_msgDispatcher->subscribe("ecce_preferences_states",
         this, (wxJmsCBFunc)&MDOptimize::statePrefsMCB);
   p_msgDispatcher->subscribe("ecce_preferences_misc",
         this, (wxJmsCBFunc)&MDOptimize::prefsMCB);

   p_msgDispatcher->subscribe("ecce_url_mol_selection",
         this, (wxJmsCBFunc)&MDOptimize::molSelectionMCB);

   p_msgDispatcher->subscribe("ecce_url_renamed",
         this, (wxJmsCBFunc)&MDOptimize::renameMCB, false);
   p_msgDispatcher->subscribe("ecce_url_removed",
         this, (wxJmsCBFunc)&MDOptimize::removeMCB, false);

   p_msgDispatcher->subscribe("ecce_url_state",
         this, (wxJmsCBFunc)&MDOptimize::stateMCB, false);

   p_msgDispatcher->subscribe("ecce_auth_changed",
         this, (wxJmsCBFunc)&MDOptimize::authMCB, false);

   p_msgDispatcher->startSubscriber();
   p_msgDispatcher->registerTopShell(this);
   p_msgDispatcher->notifyReady();
}


void MDOptimize::invokeMCB(JMSMessage& msg)
{
   _invokeMCB(msg);
}

void MDOptimize::quitMCB(JMSMessage& msg)
{
  _quitMCB(msg);
}


/**
 * Respond to request for identification.
 * Return tool name and context information.
 *
 */
void MDOptimize::identifyMCB(JMSMessage& msg)
{
   _identifyMCB(msg);
}


/**
 * Handle the preference change message which indicates a unit family change.
 */
void MDOptimize::prefsMCB(JMSMessage& msg)
{
   _prefsMCB(msg);
}

/**
 * State has changed.
 */
void MDOptimize::stateMCB(JMSMessage& msg)
{
   _stateMCB(msg);
}

/**
 *  Selection has changed
 */
void MDOptimize::molSelectionMCB(JMSMessage& msg)
{
   _molSelectionMCB(msg);
}

/**
 * An object was deleted.
 * If it was our object, clear and disable the gui.
 */
void MDOptimize::removeMCB(JMSMessage& msg)
{
   _removeMCB(msg);
}

/**
 * An object was renamed.
 * If it was our object, reset variables.
 */
void MDOptimize::renameMCB(JMSMessage& msg)
{
   _renameMCB(msg);
}

/**
 * State color preferences have changed.
 * Update state icon in feedback area.
 */
void MDOptimize::statePrefsMCB(JMSMessage& msg)
{
   _statePrefsMCB(msg);
}

/**
 * Authorization cached needs to be updated.
 */
void MDOptimize::authMCB(JMSMessage& msg)
{
   _authMCB(msg, p_msgDispatcher->getMyID());
}
