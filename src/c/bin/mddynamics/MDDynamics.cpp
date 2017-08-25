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
#include "dsm/MdTask.H"

#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxNotebook.H"
#include "wxgui/ConstraintPanel.H"
#include "wxgui/ControlPanel.H"
#include "wxgui/DynamicsPanel.H"
#include "wxgui/FilesPanel.H"
#include "wxgui/InteractionPanel.H"
#include "wxgui/ThermodynamicsPanel.H"
#include "wxgui/WxJMSMessageDispatch.H"
#include "wxgui/WxJMSListener.H"

#include "MDDynamics.H"


MDDynamics::MDDynamics( ) : MDEdBase(), WxJMSListener()
{
   constructor();
}

MDDynamics::MDDynamics( wxWindow* parent, wxWindowID id, const wxString& caption,
                    const wxPoint& pos, const wxSize& size, long style )
                  : MDEdBase(parent, id, caption, pos, size, style),
                    WxJMSListener()
{
   constructor();
}


string MDDynamics::getAppName() const
{
   return MDDYNAMICS;
}


void MDDynamics::constructor()
{
   initializeMessaging();
   MDEdBase::constructor();
}

/**
 * Create the subpanels (Interaction, Constraint, Control, Files)
 * Safe to call more than once.
 */
void MDDynamics::createPanels()
{
   int numPages = p_notebook->GetPageCount();
   if (numPages == 0) {

      p_interaction = new InteractionPanel(this, p_notebook);
      p_notebook->AddPage(p_interaction, _("Interactions"));
      p_interaction->setInteractionModel(getNWChemMDModel().getInteractionModel());
      p_interaction->initializeGUI();

      p_constraint = new ConstraintPanel(this, p_notebook);
      p_notebook->AddPage(p_constraint, _("Constraints"));
      p_constraint->setConstraintModel(getNWChemMDModel().getConstraintModel());
      p_constraint->initializeGUI();
      p_constraint->setFragmentSummary(&p_fragSummary);

      p_dynamics = new DynamicsPanel(this, p_notebook);
      p_notebook->AddPage(p_dynamics, _("Dynamics"));
      p_dynamics->setDynamicsModel(getNWChemMDModel().getDynamicsModel());
      p_dynamics->initializeGUI();

      p_thermo = new ThermodynamicsPanel(this, p_notebook);
      p_notebook->AddPage(p_thermo, _("Thermodynamics"));
      p_thermo->setThermodynamicsModel(getNWChemMDModel().getThermodynamicsModel());
      p_thermo->initializeGUI();

      p_control = new ControlPanel(this, p_notebook);
      p_notebook->AddPage(p_control, _("Control"));
      p_control->setControlModel(getNWChemMDModel().getControlModel());
      p_control->initializeGUI();

      p_files = new FilesPanel(this, p_notebook);
      p_notebook->AddPage(p_files, _("Files"));
      p_files->setFilesModel(getNWChemMDModel().getFilesModel());
      p_files->initializeGUI();

      p_notebook->Refresh();
      p_notebook->Layout();
      p_notebook->SetSelection(2);
      Layout();
   }
}


void MDDynamics::initializeModel()
{
   vector<NWChemMDModel::GUIPanel> panels;
   panels.push_back(NWChemMDModel::INTERACTION);
   panels.push_back(NWChemMDModel::CONSTRAINT);
   panels.push_back(NWChemMDModel::DYNAMICS);
   panels.push_back(NWChemMDModel::THERMODYNAMICS);
   panels.push_back(NWChemMDModel::CONTROL);
   panels.push_back(NWChemMDModel::FILES);
   p_model = new NWChemMDModel(panels);
}


void MDDynamics::setContext(const string& url) throw (EcceException)
{
  wxBusyCursor busy;
  MDEdBase::setContext(url);
  p_constraint->getConstraintModel()->setURL(url);
  p_dynamics->getDynamicsModel()->setURL(url);

  // enable/disable the resume simulation checkbox depending on whether
  // the preceeding task was a dynamics task or not
  p_dynamics->checkResume();

  // the thermo gui always exists, but we hide it from user if this isn't pmf
  MdTask * task = dynamic_cast<MdTask*>(EDSIFactory::getResource(url));
  if (task && task->hasPmf()) {
    if (p_notebook->GetPageCount() == 6) // thermo page exists, remove it
      p_notebook->RemovePage(3);
  } else {
    if (p_notebook->GetPageCount() != 6) // thermo page gone, add it
      p_notebook->InsertPage(3, p_thermo, _("Thermodynamics"));
  }
}



string MDDynamics::getTitle() const
{
   return "NWChem MD Dynamics";
}



/**
 * Refreshes each of the panels from the model.
 */
void MDDynamics::refreshGUI()
{
   p_interaction->refreshGUI();
   p_constraint->refreshGUI();
   p_dynamics->refreshGUI();
   p_thermo->refreshGUI();
   enableLaunch(p_dynamics->getDynamicsModel()->getDataSteps() > 0);
   p_control->refreshGUI();
   p_files->refreshGUI();
}

/**
 * Reset the values of the current panel displayed in the notebook to the
 * default values
 */
void MDDynamics::resetPanel()
{
  ewxMessageDialog prompt(this,"Are you sure you want to reset the panel?",
      "ECCE Reset Panel",wxYES_NO);
  int status = prompt.ShowModal();
  if (status == wxID_YES) {
    wxString text = p_notebook->GetPageText(p_notebook->GetSelection());
    if (text.IsSameAs(_("Interactions"))) {
      p_interaction->getInteractionModel()->reset();
      p_interaction->refreshGUI();
    } else if (text.IsSameAs(_("Constraints"))) {
      p_constraint->getConstraintModel()->reset();
      p_constraint->refreshGUI();
    } else if (text.IsSameAs(_("Dynamics"))) {
      p_dynamics->getDynamicsModel()->reset();
      p_dynamics->refreshGUI();
    } else if (text.IsSameAs(_("Thermodynamics"))) {
      p_thermo->getThermodynamicsModel()->reset();
      p_thermo->refreshGUI();
    } else if (text.IsSameAs(_("Control"))) {
      p_control->getControlModel()->reset();
      p_control->refreshGUI();
    } else if (text.IsSameAs(_("Files"))) {
      p_files->getFilesModel()->reset();
      p_files->refreshGUI();
    }
  }
}

/**
 *  Overwrite this function from MDEdBase class to implement disabling launch if
 *  insufficient parameters are set by user
 */
void MDDynamics::processSave()
{
  MDEdBase::processSave();
  enableLaunch(p_dynamics->getDynamicsModel()->getDataSteps() > 0);
}

/**
 *  Overwrite this function from MDEdBase class to implement inter-panel
 *  constraint logic between text field values
 */
void MDDynamics::notifyChange()
{
  p_files->constrainSteps(p_dynamics->getDynamicsModel()->getDataSteps());
}


/**
 * Initialize messaging and subscribe to messages.
 */
void MDDynamics::initializeMessaging()
{
   // Inititialize messaging.
   p_msgDispatcher = new WxJMSMessageDispatch(getAppName(), true);

   p_msgDispatcher->subscribe("ecce_invoke",
         this, (wxJmsCBFunc)&MDDynamics::invokeMCB);
   p_msgDispatcher->subscribe("ecce_quit",
         this, (wxJmsCBFunc)&MDDynamics::quitMCB);
   p_msgDispatcher->subscribe("ecce_identify",
         this, (wxJmsCBFunc)&MDDynamics::identifyMCB);

   p_msgDispatcher->subscribe("ecce_preferences_states",
         this, (wxJmsCBFunc)&MDDynamics::statePrefsMCB);
   p_msgDispatcher->subscribe("ecce_preferences_misc",
         this, (wxJmsCBFunc)&MDDynamics::prefsMCB);

   p_msgDispatcher->subscribe("ecce_url_mol_selection",
         this, (wxJmsCBFunc)&MDDynamics::molSelectionMCB);

   p_msgDispatcher->subscribe("ecce_url_renamed",
         this, (wxJmsCBFunc)&MDDynamics::renameMCB, false);
   p_msgDispatcher->subscribe("ecce_url_removed",
         this, (wxJmsCBFunc)&MDDynamics::removeMCB, false);

   p_msgDispatcher->subscribe("ecce_url_state",
         this, (wxJmsCBFunc)&MDDynamics::stateMCB, false);

   p_msgDispatcher->subscribe("ecce_auth_changed",
         this, (wxJmsCBFunc)&MDDynamics::authMCB, false);

   p_msgDispatcher->startSubscriber();
   p_msgDispatcher->registerTopShell(this);
   p_msgDispatcher->notifyReady();
}


void MDDynamics::invokeMCB(JMSMessage& msg)
{
   _invokeMCB(msg);
}

void MDDynamics::quitMCB(JMSMessage& msg)
{
  _quitMCB(msg);
}


/**
 * Respond to request for identification.
 * Return tool name and context information.
 *
 */
void MDDynamics::identifyMCB(JMSMessage& msg)
{
   _identifyMCB(msg);
}


/**
 * Handle the preference change message which indicates a unit family change.
 */
void MDDynamics::prefsMCB(JMSMessage& msg)
{
   _prefsMCB(msg);
}

/**
 * State has changed.
 */
void MDDynamics::stateMCB(JMSMessage& msg)
{
   _stateMCB(msg);
}

/**
 *  Selection has changed
 */
void MDDynamics::molSelectionMCB(JMSMessage& msg)
{
   _molSelectionMCB(msg);
}

/**
 * An object was deleted.
 * If it was our object, clear and disable the gui.
 */
void MDDynamics::removeMCB(JMSMessage& msg)
{
   _removeMCB(msg);
}

/**
 * An object was renamed.
 * If it was our object, reset variables.
 */
void MDDynamics::renameMCB(JMSMessage& msg)
{
   _renameMCB(msg);
}

/**
 * State color preferences have changed.
 * Update state icon in feedback area.
 */
void MDDynamics::statePrefsMCB(JMSMessage& msg)
{
   _statePrefsMCB(msg);
}

/**
 * Authorization cache needs to be updated.
 */
void MDDynamics::authMCB(JMSMessage& msg)
{
   _authMCB(msg, p_msgDispatcher->getMyID());
}
