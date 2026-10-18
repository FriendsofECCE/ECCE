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

#include <algorithm>
#include <cmath>
#include "wxgui/ewxButton.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxNotebook.H"
#include "wxgui/GromacsInputsPanel.H"
#include "wxgui/GromacsRunPanel.H"
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
   if (p_gmxRun != 0) {
      for (size_t i = 0; i < p_pages.size(); i++) {
         if (p_pages[i].win == (wxWindow*)p_gmxRun) {
            p_pages.erase(p_pages.begin() + i);
            break;
         }
      }
      p_gmxRun->Destroy();
      p_gmxRun = 0;
   }
   SetTitle(wxString::FromUTF8(("ECCE " + getTitle()).c_str()));

   // Verify sits beside Launch, as in the other editors.
   wxWindow *launch = FindWindow(ID_BUTTON_LAUNCH);
   if (launch != 0 && launch->GetContainingSizer() != 0) {
      wxSizer *row = launch->GetContainingSizer();
      ewxButton *verify = new ewxButton(launch->GetParent(), wxID_ANY, _("Verify..."));
      verify->Bind(wxEVT_BUTTON, &GmxPrepare::OnVerifyClick, this);
      size_t at = 0;
      for (wxSizerItemList::compatibility_iterator n = row->GetChildren().GetFirst();
           n; n = n->GetNext(), at++)
         if (n->GetData()->GetWindow() == launch) break;
      row->Insert(at, verify, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
   }
   // The base fitted the window to its content; it may shrink below that.
   SetMinSize(wxSize(420, 320));
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
      // The page is taller than a laptop screen with a mineral chosen: it
      // scrolls inside a compact window instead of forcing the window up.
      wxScrolledWindow *scroll = new wxScrolledWindow(p_notebook, wxID_ANY);
      scroll->SetScrollRate(10, 10);
      scroll->SetMinSize(wxSize(700, 420));
      p_panel = new GromacsPreparePanel(this, scroll);
      wxBoxSizer *fill = new wxBoxSizer(wxVERTICAL);
      fill->Add(p_panel, 1, wxEXPAND);
      scroll->SetSizer(fill);
      p_notebook->AddPage(scroll, _("System"));
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
string GmxPrepare::recipeText(MdTask *task)
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
   string err, recipe;
   if (istatus != 0) {
      ifstream ef(tmpErr->path().c_str());
      std::ostringstream es;
      es << ef.rdbuf();
      err = es.str();
   } else {
      ifstream ifs(tmpInput->path().c_str());
      std::ostringstream rs;
      rs << ifs.rdbuf();
      recipe = rs.str();
   }

   SFile *tmps[] = { tmpModel, tmpInput, tmpErr };
   for (int i = 0; i < 3; i++) { tmps[i]->remove(); delete tmps[i]; }

   if (istatus != 0) {
      throw IOException("Input file generation failed. " + err, WHERE);
   }
   return recipe;
}


void GmxPrepare::generateInputFile(MdTask *task)
{
   std::istringstream ifs(recipeText(task));
   if (task->putInputFile(RECIPE_NAME, &ifs)) {
      task->setState(ResourceDescriptor::STATE_READY);
      setRunState(ResourceDescriptor::STATE_READY);
   } else {
      p_feedback->setMessage("Failed to save input file", WxFeedback::ERROR);
   }
}


static string recipeValue(const string& recipe, const string& key,
                          const string& dflt)
{
   std::istringstream in(recipe);
   string l;
   while (std::getline(in, l))
      if (l.compare(0, key.size() + 1, key + "=") == 0) return l.substr(key.size() + 1);
   return dflt;
}

string GmxPrepare::verifyText()
{
   MdTask *task = dynamic_cast<MdTask*>(EDSIFactory::getResource(p_model->getUrl()));
   if (task == 0) return "No task is open.";
   string recipe;
   try {
      recipe = recipeText(task);
   } catch (IOException& e) {
      return e.what();
   }
   GromacsPrepareModel *m = prepModel();
   const bool minff = m->source() == "minff";
   const bool water = !minff || m->boxDistance() > 0;
   const string w = m->waterModel();
   string wbox = "spc216.gro";
   if (w == "tip4p" || w == "tip4pew" || w == "opc" || w == "tip4p-fb") wbox = "tip4p.gro";
   else if (w == "tip5p") wbox = "tip5p.gro";
   char num[64];
   std::ostringstream o;

   o << "The recipe stored with this task (gromacs.gmxprep):\n\n" << recipe
     << "\nThe job runs these commands, in its own directory, with the gmx "
        "it finds on the compute machine:\n\n";
   if (minff) {
      o << "1. (mineral) the topology and structure built here are used as "
           "they are: topol.top, step1.gro\n";
   } else {
      o << "1. gmx pdb2gmx -f <structure.pdb> -ff " << m->forceField()
        << " -water " << w << (m->ignoreHydrogens() ? " -ignh" : "")
        << (m->mergeChains() ? " -merge all" : "")
        << " -o step1.gro -p topol.top -i posre.itp\n";
   }
   if (minff) {
      snprintf(num, sizeof num, "%g", m->boxDistance());
      o << "2. the cell is kept; c is lengthened by the water layer ("
        << (water ? num : "none") << " nm)\n";
   } else {
      snprintf(num, sizeof num, "%g", m->boxDistance());
      o << "2. gmx editconf -f step1.gro -o step2.gro -c -bt " << m->boxType()
        << " -d " << num << "\n";
   }
   if (water) {
      o << "3. gmx solvate -cp step2.gro -cs " << wbox
        << " -o step3.gro -p topol.top\n";
   } else {
      o << "3. no water layer: step2.gro is used as it is\n";
   }

   // genion's cut-off, as gensub works it out: 1 nm, or for a mineral at
   // most 0.5 nm and what its cell holds.
   string rc = "1.0", periodic;
   if (minff) {
      double v[3][3];
      double lim = 0.5;
      if (p_panel != 0 && p_panel->cellVectors(v)) {
         for (int i = 0; i < 3; i++) {
            lim = std::min(lim, 0.5 * std::sqrt(v[i][0]*v[i][0] + v[i][1]*v[i][1] + v[i][2]*v[i][2]));
            lim = std::min(lim, v[i][i]);
         }
         snprintf(num, sizeof num, "%.3f", int(lim / 1.05 * 980) / 1000.0);
         rc = num;
      } else {
         rc = "0.5 (the mineral has not been built here yet)";
      }
      periodic = "periodic-molecules = yes\n";
   }
   o << "4. gmx grompp -f ions.mdp -c step3.gro -p topol.top -o ions.tpr\n";
   if (water && !m->neutralise() && m->ionConcentration() <= 0) {
      o << "   (neutralising is off and the salt concentration is 0: no ions are added;"
           " the net charge stays as it is)\n";
   } else if (water) {
      snprintf(num, sizeof num, "%g", m->ionConcentration());
      o << "   echo SOL | gmx genion -s ions.tpr -o ion.gro -p topol.top -pname "
        << m->positiveIon() << " -pq " << recipeValue(recipe, "PQ", "1")
        << " -nname " << m->negativeIon() << (m->neutralise() ? " -neutral" : "")
        << (m->ionConcentration() > 0 ? string(" -conc ") + num : string())
        << "\n";
   } else {
      o << "   (no solvent to replace with ions: the mineral's charge is left as it is)\n";
   }
   o << "5. gmx editconf -f <system>_<calculation>.gro -o chemsys.pdb\n"
     << "\nions.mdp, which grompp reads in step 4:\n\n"
     << "integrator = steep\nnsteps = 0\ncutoff-scheme = Verlet\n"
        "coulombtype = cutoff\nrcoulomb = " << rc << "\nrvdw = " << rc
     << "\npbc = xyz\n" << periodic;
   return o.str();
}


void GmxPrepare::OnVerifyClick(wxCommandEvent&)
{
   wxDialog dlg(this, wxID_ANY, _("Verify GROMACS Prepare"), wxDefaultPosition,
                wxSize(760, 560), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
   wxBoxSizer *col = new wxBoxSizer(wxVERTICAL);
   wxTextCtrl *text = new wxTextCtrl(&dlg, wxID_ANY,
       wxString::FromUTF8(verifyText().c_str()), wxDefaultPosition,
       wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP);
   text->SetFont(wxFont(wxFontInfo().Family(wxFONTFAMILY_TELETYPE)));
   col->Add(text, 1, wxEXPAND | wxALL, 8);
   col->Add(dlg.CreateStdDialogButtonSizer(wxCLOSE), 0, wxEXPAND | wxALL, 8);
   dlg.SetSizer(col);
   dlg.SetEscapeId(wxID_CLOSE);
   dlg.ShowModal();
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
   } else if (cmd == "size") {
      wxScrolledWindow *page = dynamic_cast<wxScrolledWindow*>(p_panel->GetParent());
      wxSize f = GetSize(), c = page ? page->GetClientSize() : wxSize(0, 0),
             v = page ? page->GetVirtualSize() : wxSize(0, 0);
      char b[200];
      snprintf(b, sizeof b, "frame %d %d page %d %d virtual %d %d", f.x, f.y,
               c.x, c.y, v.x, v.y);
      return b;
   } else if (cmd == "verify") {
      string text = verifyText(), out, l;
      std::istringstream in(text);
      while (std::getline(in, l)) out += (out.empty() ? "" : " | ") + l;
      return out;
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
