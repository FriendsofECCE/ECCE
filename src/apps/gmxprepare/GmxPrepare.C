// For compilers that support precompilation, includes "wx/wxprec.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <fstream>
#include <sstream>
  using std::ifstream;
  using std::ofstream;

#include "util/Ecce.H"
#include "util/IOException.H"
#include "util/JMSMessage.H"
#include "util/SFile.H"
#include "util/StringConverter.H"
#include "util/TempStorage.H"

#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/GromacsPrepareModel.H"
#include "dsm/JCode.H"
#include "dsm/MdTask.H"

#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxNotebook.H"
#include "wxgui/GromacsInputsPanel.H"
#include "wxgui/GromacsPreparePanel.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/WxJMSListener.H"
#include "wxgui/WxJMSMessageDispatch.H"

#include "GmxPrepare.H"

// The recipe's generator is fixed here, not read from the code's
// TaskInputGenerator, which is the .mdp's (md.gmxtask).
static const char *RECIPE_GENERATOR = "md.gmxprep";
static const char *RECIPE_NAME = "gromacs.gmxprep";


GmxPrepare::GmxPrepare() : MDEdBase(), WxJMSListener(), p_panel(0)
{
   constructor();
}

GmxPrepare::GmxPrepare( wxWindow* parent, wxWindowID id, const wxString& caption,
      const wxPoint& pos, const wxSize& size, long style )
: MDEdBase(parent, id, caption, pos, size, style), WxJMSListener(), p_panel(0)
{
   constructor();
}


string GmxPrepare::getAppName() const
{
   return GMXPREPARE;
}

string GmxPrepare::getTitle() const
{
   return "GROMACS MD Prepare";
}

void GmxPrepare::constructor()
{
   initializeMessaging();
   MDEdBase::constructor();

   // MDEdBase added the Inputs page of the other GROMACS editors; a Prepare
   // makes the topology and structure that page attaches.
   if (p_gmxInputs != 0) {
      for (size_t i = 0; i < p_pages.size(); i++) {
         if (p_pages[i].win == (wxWindow*)p_gmxInputs) {
            p_pages.erase(p_pages.begin() + i);
            break;
         }
      }
      p_gmxInputs->Destroy();
      p_gmxInputs = 0;
   }
   SetTitle(wxString::FromUTF8(("ECCE " + getTitle()).c_str()));
}


void GmxPrepare::initializeModel()
{
   p_isGromacs = true;
   p_model = new GromacsPrepareModel();
}

GromacsPrepareModel *GmxPrepare::prepModel() const
{
   return dynamic_cast<GromacsPrepareModel*>(p_model);
}

void GmxPrepare::createPanels()
{
   if (p_notebook->GetPageCount() == 0) {
      p_panel = new GromacsPreparePanel(this, p_notebook);
      p_notebook->AddPage(p_panel, _("System"));
      p_panel->setModel(prepModel());
      p_panel->refreshGUI();
      p_notebook->Refresh();
      p_notebook->Layout();
      Layout();
   }
}

void GmxPrepare::refreshGUI()
{
   p_panel->refreshGUI();
}

void GmxPrepare::updateDisabledState(ResourceDescriptor::RUNSTATE state,
                                     bool forceMsg)
{
   MDEdBase::updateDisabledState(state, forceMsg);
   if (p_panel != 0) {
      p_panel->setEditable(state < ResourceDescriptor::STATE_SUBMITTED);
   }
}


void GmxPrepare::setContext(const string& url)
{
   wxBusyCursor busy;
   ResourceDescriptor::RUNSTATE state = ResourceDescriptor::STATE_CREATED;

   if (p_model->getUrl() == url) return;
   MdTask *task = dynamic_cast<MdTask*>(EDSIFactory::getResource(EcceURL(url)));
   prepModel()->reset();
   p_model->setUrl(url);

   if (task != 0) {
      try {
         state = task->getState();
         p_feedback->setRunState(state);
         EcceURL turl = task->getURL();
         EDSIServerCentral servers;
         p_feedback->setContextLabel(servers.mapURLtoName(turl));
         p_feedback->setContextURL(turl);

         task->getTaskModel(*p_model);
         p_panel->setContext(url);

         // a new task has no recipe until the defaults are written
         bool have = false;
         vector<string> names = task->getDataFileNames(JCode::PRIMARY_INPUT);
         for (size_t i = 0; i < names.size(); i++)
            if (names[i] == RECIPE_NAME) have = true;
         if (!have) generateInputFile(task);

         updateDisabledState(p_feedback->getRunState(), true);
         p_msgDispatcher->setPollContext(url);
      } catch (...) {
         throw EcceException("Deserialize model failed", WHERE);
      }
   }

   refreshGUI();

   wxMenuBar *menuBar = GetMenuBar();
   if (menuBar) {
      wxMenuItem *item = menuBar->FindItem(ID_MENU_HELP);
      if (item) item->SetItemLabel(wxString::FromUTF8(("on " + getTitle() + "...").c_str()));
   }

   setSaveState(false);
   p_feedback->setEditStatus(state >= ResourceDescriptor::STATE_SUBMITTED
                             ? WxFeedback::READONLY : WxFeedback::EDIT);
}


void GmxPrepare::saveModelAndInputFile(MdTask *task)
{
   if (task == 0) return;
   string why;
   if (!p_panel->validate(why)) {
      p_feedback->setMessage(why, WxFeedback::ERROR);
      return;
   }
   task->setTaskModel(p_model);
   setSaveState(false);
   if (p_feedback->getRunState() == ResourceDescriptor::STATE_READY) {
      generateInputFile(task);
   }
}


/**
 * The model through md.gmxprep, stored as the task's gromacs.gmxprep.
 * @throw IOException with md.gmxprep's own message if it refuses a value.
 */
void GmxPrepare::generateInputFile(MdTask *task)
{
   SFile *tmpModel = TempStorage::getTempFile();
   ofstream outfile(tmpModel->path().c_str());
   outfile << task->serializeModel(p_model);
   outfile.close();

   SFile *tmpInput = TempStorage::getTempFile();
   SFile *tmpErr = TempStorage::getTempFile();
   string cmd = Ecce::scriptCommand(string(Ecce::ecceDataControllersPath()) +
                                    "/" + RECIPE_GENERATOR) +
                " < \"" + tmpModel->path() + "\" > \"" + tmpInput->path() +
                "\" 2> \"" + tmpErr->path() + "\"";
   int istatus = Ecce::runCommand(cmd);
   string err;
   if (istatus != 0) {
      ifstream ef(tmpErr->path().c_str());
      std::ostringstream es;
      es << ef.rdbuf();
      err = es.str();
   }

   if (istatus == 0) {
      ifstream ifs(tmpInput->path().c_str());
      if (task->putInputFile(RECIPE_NAME, &ifs)) {
         task->setState(ResourceDescriptor::STATE_READY);
         setRunState(ResourceDescriptor::STATE_READY);
      } else {
         p_feedback->setMessage("Failed to save input file", WxFeedback::ERROR);
      }
   }

   SFile *tmps[] = { tmpModel, tmpInput, tmpErr };
   for (int i = 0; i < 3; i++) { tmps[i]->remove(); delete tmps[i]; }

   if (istatus != 0) {
      throw IOException("Input file generation failed. " + err, WHERE);
   }
}


void GmxPrepare::resetPanel()
{
   ewxMessageDialog prompt(this, "Are you sure you want to reset the panel?",
                           "ECCE Reset Panel", wxYES_NO);
   if (prompt.ShowModal() == wxID_YES) {
      prepModel()->reset();
      refreshGUI();
      setSaveState(true);
   }
}

void GmxPrepare::OnButtonResetaClick(wxCommandEvent&)
{
   resetPanel();
}


/**
 * set <field> <value>   as a person would: ff water ignh merge box dist conc
 *                       pname structure (builder or a PDB path)
 * fields                the model and the water choices, in words
 * recipe                the stored gromacs.gmxprep, lines joined by " | "
 * model                 the model XML as stored
 */
string GmxPrepare::runTestCommand(const string& line)
{
   string cmd = line.substr(0, line.find(' '));
   string arg = line.size() > cmd.size() + 1 ? line.substr(cmd.size() + 1) : "";
   for (int i = 0; i < 4; i++) { wxYield(); }

   if (cmd == "set") {
      size_t sp = arg.find(' ');
      if (sp == string::npos) return "usage: set <field> <value>";
      string message;
      bool ok = p_panel->testSet(arg.substr(0, sp), arg.substr(sp + 1), message);
      return ok ? "ok" : "refused: " + message;
   } else if (cmd == "fields") {
      return p_panel->summary();
   } else if (cmd == "recipe" || cmd == "model") {
      MdTask *task = dynamic_cast<MdTask*>(EDSIFactory::getResource(p_model->getUrl()));
      if (task == 0) return "no task";
      istream *is = 0;
      std::istringstream xml(task->serializeModel(p_model));
      if (cmd == "recipe") is = task->getDataFile(JCode::PRIMARY_INPUT);
      if (cmd == "recipe" && is == 0) return "no recipe stored";
      istream& in = is ? *is : static_cast<istream&>(xml);
      string l, out;
      while (std::getline(in, l)) out += (out.empty() ? "" : " | ") + l;
      delete is;
      return out;
   }
   return MDEdBase::runTestCommand(line);
}


/**
 * Initialize messaging and subscribe to messages.
 */
void GmxPrepare::initializeMessaging()
{
   // Inititialize messaging.
   p_msgDispatcher = new WxJMSMessageDispatch(getAppName(), true);

   p_msgDispatcher->subscribe("ecce_invoke",
         this, (wxJmsCBFunc)&GmxPrepare::invokeMCB);
   p_msgDispatcher->subscribe("ecce_quit",
         this, (wxJmsCBFunc)&GmxPrepare::quitMCB);
   p_msgDispatcher->subscribe("ecce_identify",
         this, (wxJmsCBFunc)&GmxPrepare::identifyMCB);

   p_msgDispatcher->subscribe("ecce_preferences_states",
         this, (wxJmsCBFunc)&GmxPrepare::statePrefsMCB);
   p_msgDispatcher->subscribe("ecce_preferences_misc",
         this, (wxJmsCBFunc)&GmxPrepare::prefsMCB);

   p_msgDispatcher->subscribe("ecce_url_mol_selection",
         this, (wxJmsCBFunc)&GmxPrepare::molSelectionMCB);

   p_msgDispatcher->subscribe("ecce_url_renamed",
         this, (wxJmsCBFunc)&GmxPrepare::renameMCB, false);
   p_msgDispatcher->subscribe("ecce_url_removed",
         this, (wxJmsCBFunc)&GmxPrepare::removeMCB, false);

   p_msgDispatcher->subscribe("ecce_url_state",
         this, (wxJmsCBFunc)&GmxPrepare::stateMCB, false);

   p_msgDispatcher->subscribe("ecce_auth_changed",
         this, (wxJmsCBFunc)&GmxPrepare::authMCB, false);

   p_msgDispatcher->startSubscriber();
   p_msgDispatcher->registerTopShell(this);
   p_msgDispatcher->notifyReady();
}


void GmxPrepare::invokeMCB(JMSMessage& msg)
{
   _invokeMCB(msg);
}

void GmxPrepare::quitMCB(JMSMessage& msg)
{
  _quitMCB(msg);
}


/**
 * Respond to request for identification.
 * Return tool name and context information.
 *
 */
void GmxPrepare::identifyMCB(JMSMessage& msg)
{
   _identifyMCB(msg);
}


/**
 * Handle the preference change message which indicates a unit family change.
 */
void GmxPrepare::prefsMCB(JMSMessage& msg)
{
   _prefsMCB(msg);
}

/**
 * State has changed.
 */
void GmxPrepare::stateMCB(JMSMessage& msg)
{
   _stateMCB(msg);
}

/**
 *  Selection has changed
 */
void GmxPrepare::molSelectionMCB(JMSMessage& msg)
{
   _molSelectionMCB(msg);
}

/**
 * An object was deleted.
 * If it was our object, clear and disable the gui.
 */
void GmxPrepare::removeMCB(JMSMessage& msg)
{
   _removeMCB(msg);
}

/**
 * An object was renamed.
 * If it was our object, reset variables.
 */
void GmxPrepare::renameMCB(JMSMessage& msg)
{
   _renameMCB(msg);
}

/**
 * State color preferences have changed.
 * Update state icon in feedback area.
 */
void GmxPrepare::statePrefsMCB(JMSMessage& msg)
{
   _statePrefsMCB(msg);
}

/**
 * Authorization cached needs to be updated.
 */
void GmxPrepare::authMCB(JMSMessage& msg)
{
   _authMCB(msg, p_msgDispatcher->getMyID());
}
