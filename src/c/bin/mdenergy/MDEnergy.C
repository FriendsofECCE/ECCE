// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <iostream>
  using std::cout;
  using std::endl;
#include <fstream>
  using std::ofstream;
  using std::ifstream;
#include <strstream>
  using std::istrstream;

#include "util/Ecce.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/ResourceUtils.H"
#include "util/StringConverter.H"
#include "util/TempStorage.H"
#include "util/EditEvent.H"

#include "dsm/NWChemMDModel.H"
#include "dsm/MdTask.H"

#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxNotebook.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/ConstraintPanel.H"
#include "wxgui/ControlPanel.H"
#include "wxgui/FilesPanel.H"
#include "wxgui/InteractionPanel.H"
#include "wxgui/WxEditSessionMgr.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/WxJMSMessageDispatch.H"
#include "wxgui/WxJMSListener.H"

#include "MDEnergy.H"


MDEnergy::MDEnergy( )
        : MDEdBase(), WxJMSListener()
{
   constructor();
}

MDEnergy::MDEnergy( wxWindow* parent, wxWindowID id, const wxString& caption,
                    const wxPoint& pos, const wxSize& size, long style )
                  : MDEdBase(parent, id, caption, pos, size, style),
                    WxJMSListener()
{
   constructor();
}


string MDEnergy::getAppName() const
{
   return MDENERGY;
}

void MDEnergy::constructor()
{
   initializeMessaging();
   MDEdBase::constructor();
}


/**
 * Create the subpanels (Interaction, Constraint, Control, Files)
 * Safe to call more than once.
 */
void MDEnergy::createPanels()
{
   int numPages = p_notebook->GetPageCount();
   if (numPages == 0) {
      // Only create these the first time through
      p_interaction = new InteractionPanel(this, p_notebook);
      p_notebook->AddPage(p_interaction, _("Interactions"));
      p_interaction->setInteractionModel(getNWChemMDModel().getInteractionModel());
      p_interaction->initializeGUI();

      p_constraint = new ConstraintPanel(this, p_notebook);
      p_notebook->AddPage(p_constraint, _("Constraints"));
      p_constraint->setConstraintModel(getNWChemMDModel().getConstraintModel());
      p_constraint->initializeGUI();
      p_constraint->setFragmentSummary(&p_fragSummary);

      p_control = new ControlPanel(this, p_notebook);
      p_notebook->AddPage(p_control, _("Control"));
      p_control->setControlModel(getNWChemMDModel().getControlModel());
      p_control->initializeGUI();

      p_files = new FilesPanel(this, p_notebook);
      p_notebook->AddPage(p_files, _("Files"));
      p_files->setFilesModel(getNWChemMDModel().getFilesModel());
      p_files->initializeGUI();
      p_files->configEnergyCalc();

      p_notebook->Layout();
      p_notebook->Refresh();
      Fit();

   }
}


void MDEnergy::initializeModel()
{
   vector<NWChemMDModel::GUIPanel> panels;
   panels.push_back(NWChemMDModel::INTERACTION);
   panels.push_back(NWChemMDModel::CONSTRAINT);
   panels.push_back(NWChemMDModel::CONTROL);
   panels.push_back(NWChemMDModel::FILES);
   p_model = new NWChemMDModel(panels);
}


void MDEnergy::setContext(const string& url) throw (EcceException)
{
  wxBusyCursor busy;
  MDEdBase::setContext(url);
  p_constraint->getConstraintModel()->setURL(url);
  // Although the GUI checkbox is hidden, the model inherits from the input
  // provider.  This could cause input deck to be incorrect.
  // Force model value to false to correct this.
  p_files->getFilesModel()->setPmfData(false);
}



string MDEnergy::getTitle() const
{
   return "NWChem MD Energy";
}



/**
 * Refreshes each of the panels from the model.
 */
void MDEnergy::refreshGUI()
{
   p_interaction->refreshGUI();
   p_constraint->refreshGUI();
   p_control->refreshGUI();
   p_files->refreshGUI();

}

/**
 * Reset the values of the current panel displayed in the notebook to the
 * default values
 */
void MDEnergy::resetPanel()
{
  ewxMessageDialog prompt(this,"Are you sure you want to reset the panel?",
      "ECCE Reset Panel",wxYES_NO);
  int status = prompt.ShowModal();
  if (status == wxID_YES) {
    int isel = p_notebook->GetSelection();
    if (isel == 0) {
      p_interaction->getInteractionModel()->reset();
      p_interaction->refreshGUI();
    } else if (isel == 1) {
      p_constraint->getConstraintModel()->reset();
      p_constraint->refreshGUI();
    } else if (isel == 2) {
      p_control->getControlModel()->reset();
      p_control->refreshGUI();
    } else if (isel == 3) {
      p_files->getFilesModel()->reset();
      p_files->refreshGUI();
    }
  }
}


/**
 * Initialize messaging and subscribe to messages.
 */
void MDEnergy::initializeMessaging()
{
   // Inititialize messaging.
   p_msgDispatcher = new WxJMSMessageDispatch(getAppName(), true);

   p_msgDispatcher->subscribe("ecce_invoke",
         this, (wxJmsCBFunc)&MDEnergy::invokeMCB);
   p_msgDispatcher->subscribe("ecce_quit",
         this, (wxJmsCBFunc)&MDEnergy::quitMCB);
   p_msgDispatcher->subscribe("ecce_identify",
         this, (wxJmsCBFunc)&MDEnergy::identifyMCB);

   p_msgDispatcher->subscribe("ecce_preferences_states",
         this, (wxJmsCBFunc)&MDEnergy::statePrefsMCB);
   p_msgDispatcher->subscribe("ecce_preferences_misc",
         this, (wxJmsCBFunc)&MDEnergy::prefsMCB);

   p_msgDispatcher->subscribe("ecce_url_mol_selection",
         this, (wxJmsCBFunc)&MDEnergy::molSelectionMCB);


   p_msgDispatcher->subscribe("ecce_url_renamed",
         this, (wxJmsCBFunc)&MDEnergy::renameMCB, false);
   p_msgDispatcher->subscribe("ecce_url_removed",
         this, (wxJmsCBFunc)&MDEnergy::removeMCB, false);

   p_msgDispatcher->subscribe("ecce_url_state",
         this, (wxJmsCBFunc)&MDEnergy::stateMCB, false);

   p_msgDispatcher->subscribe("ecce_auth_changed",
         this, (wxJmsCBFunc)&MDEnergy::authMCB, false);

   p_msgDispatcher->startSubscriber();
   p_msgDispatcher->registerTopShell(this);
   p_msgDispatcher->notifyReady();
}


void MDEnergy::invokeMCB(JMSMessage& msg)
{
   _invokeMCB(msg);
}

void MDEnergy::quitMCB(JMSMessage& msg)
{
  _quitMCB(msg);
}


/**
 * Respond to request for identification.
 * Return tool name and context information.
 *
 */
void MDEnergy::identifyMCB(JMSMessage& msg)
{
   _identifyMCB(msg);
}


/**
 * Handle the preference change message which indicates a unit family change.
 */
void MDEnergy::prefsMCB(JMSMessage& msg)
{
   _prefsMCB(msg);
}

/**
 * State has changed.
 */
void MDEnergy::stateMCB(JMSMessage& msg)
{
   _stateMCB(msg);
}

/**
 *  Selection has changed
 */
void MDEnergy::molSelectionMCB(JMSMessage& msg)
{
   _molSelectionMCB(msg);
}

/**
 * An object was deleted.
 * If it was our object, clear and disable the gui.
 */
void MDEnergy::removeMCB(JMSMessage& msg)
{
   _removeMCB(msg);
}

/**
 * An object was renamed.
 * If it was our object, reset variables.
 */
void MDEnergy::renameMCB(JMSMessage& msg)
{
   _renameMCB(msg);
}

/**
 * State color preferences have changed.
 * Update state icon in feedback area.
 */
void MDEnergy::statePrefsMCB(JMSMessage& msg)
{
   _statePrefsMCB(msg);
}

/**
 * Authorization cache needs to be updated.
 */
void MDEnergy::authMCB(JMSMessage& msg)
{
   _authMCB(msg, p_msgDispatcher->getMyID());
}
