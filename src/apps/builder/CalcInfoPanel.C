#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>
#include <set>
#include <thread>
  using std::cout;
  using std::endl;

#include "wx/link.h"
#include "wx/app.h"
#include "wx/weakref.h"
#include "wx/log.h"
#include "wx/sizer.h"
#include "wx/scrolwin.h"

#include "tdat/TProperty.H"

#include "dsm/ICalculation.H"
#include "dsm/IPropCalculation.H"
#include "dsm/JCode.H"
#include "dsm/PropFactory.H"
#include "dsm/TaskJob.H"
#include "dsm/AllocationLeft.H"
#include "dsm/MachinePreferences.H"
#include "tdat/QueueMgr.H"
#include "tdat/RefQueueMgr.H"
#include "comm/RCommand.H"
#include "util/Ecce.H"

#include "wxgui/ewxNonBoldLabel.H"
#include "wxgui/ewxSelectableText.H"
#include "wxgui/ewxStaticText.H"

#include "util/UnitDisplay.H"
#include "CalcInfoPanel.H"


// TODO 
// . sort properties - need to sort on long name; may be easiest to 
//   put the data into a grid and sort via the grid


wxFORCE_LINK_THIS_MODULE(CalcInfoPanel)


IMPLEMENT_DYNAMIC_CLASS(CalcInfoPanel, PropertyPanel)


CalcInfoPanel::CalcInfoPanel()
  : PropertyPanel(), p_body(0)
{
}


CalcInfoPanel::CalcInfoPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  : PropertyPanel(), p_body(0)
{
  Create(calculation, parent, id, pos, size, style, name);
}


bool CalcInfoPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
  if (!PropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("CalcInfoPanel creation failed") );
    return false;
  }

  //  The rows scroll: the pane shares the dock with the other property
  //  panes and may get less height than its rows need.
  p_body = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition,
                                wxDefaultSize, wxVSCROLL | wxNO_BORDER);
  p_body->SetScrollRate(0, 10);
  p_body->SetSizer(new wxFlexGridSizer(0,2,1,5));
  wxBoxSizer *outer = new wxBoxSizer(wxVERTICAL);
  //  The standard border: labels touched the left edge, long values the right.
  outer->Add(p_body, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);
  SetSizer(outer);

  return true;
}


CalcInfoPanel::~CalcInfoPanel()
{
}


//  How much of the allocation is left (#613, docs/claude/services/
//  allocation-left-613.md). Answers are kept 15 minutes per machine and
//  account, in memory and in AllocationCache; the command runs on a
//  background thread and only over a connection that is already logged in,
//  so showing the summary never asks for a password.
namespace {
std::mutex gAllocMu;
AllocationLeft::Cache gAllocCache;
bool gAllocLoaded = false;
std::set<std::string> gAllocBusy;

string allocCachePath()
{
  return string(Ecce::userRegistrationDir()) + "AllocationCache";
}

//  "" when nothing is to be shown. A stale answer is shown until the
//  refresh lands; a refresh that fails leaves "" cached for 15 minutes.
string allocationLeft(const Launchdata& l, CalcInfoPanel *panel)
{
  if (l.machine.empty() || !AllocationLeft::validAccount(l.account))
    return "";
  MachinePreferences *prefs = MachinePreferences::lookup(l.machine);
  RefMachine *rgstn = prefs ? prefs->getRegisteredMachine() : NULL;
  if (!rgstn || rgstn->noRemoteAccess())
    return "";
  const QueueManager *queueMngr = QueueManager::lookup(l.machine);
  const RefQueueManager *refQMngr = queueMngr ?
    RefQueueManager::lookup(queueMngr->queueMgrName()) : NULL;
  string command = RefQueueManager::allocationCommand(refQMngr, rgstn->allocationCommand());
  if (command.empty())
    return "";
  command = AllocationLeft::substitute(command, l.account);

  const string key = AllocationLeft::Cache::key(l.machine, l.account);
  const long now = (long)time(NULL);
  AllocationLeft::Cache::Entry e;
  bool have, fresh;
  {
    std::lock_guard<std::mutex> lock(gAllocMu);
    if (!gAllocLoaded) {
      gAllocLoaded = true;
      std::ifstream in(allocCachePath().c_str());
      std::stringstream all;
      all << in.rdbuf();
      gAllocCache.parse(all.str());
    }
    have = gAllocCache.find(key, e);
    fresh = gAllocCache.fresh(key, now);
    if (!fresh && gAllocBusy.count(key))
      fresh = true;                       // being read
    else if (!fresh)
      gAllocBusy.insert(key);
  }

  if (!fresh) {
    const string machine = l.machine, user = prefs->getUsername(),
                 front = rgstn->frontendMachine(), bypass = rgstn->frontendBypass(),
                 shellPath = rgstn->shellPath(), account = l.account;
    wxWeakRef<CalcInfoPanel> self(panel);
    std::thread([=]() {
      string out, text;
      if (RCommand::execOnLiveLogin(machine, user, front, bypass, shellPath,
                                    command, out))
        text = AllocationLeft::summarize(out, account);
      {
        std::lock_guard<std::mutex> lock(gAllocMu);
        gAllocBusy.erase(key);
        //  A failure is kept too: the refresh below would otherwise start
        //  the next attempt at once.
        gAllocCache.put(key, (long)time(NULL), text);
        std::ofstream o(allocCachePath().c_str());
        o << gAllocCache.serialise();
      }
      wxTheApp->CallAfter([self]() { if (self) self->refresh(); });
    }).detach();
  }
  if (!have || e.text.empty())
    return "";
  return e.text + " (checked " + AllocationLeft::checkedAt(e.when) + ")";
}
}


void CalcInfoPanel::refresh()
{
  p_body->DestroyChildren();

  TaskJob *taskJob = dynamic_cast<TaskJob*>(getCalculation());
  ICalculation *escalc = dynamic_cast<ICalculation*>(getCalculation());
  // only get the fragment once - this could be expensive
  Fragment * frag = NULL;
  if (escalc) frag = escalc->fragment();

  // APPLICATION
  if (taskJob) {
    const JCode* app = taskJob->application() ;
    if (app && !app->name().empty()) {
      p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY, "Application"));
      p_body->GetSizer()->Add(new ewxNonBoldLabel(p_body, wxID_ANY, app->name()));
    }
  }

  // THEORY/RUNTYPE
  if (escalc) {
    string trString;
    TTheory *ttheory = escalc->theory();
    if (ttheory) {
      trString = ttheory->toConciseString();
      if (trString.empty()) trString = "NA";
      delete ttheory;
    } else {
      trString = "NA";
    }

    TRunType runtype = escalc->runtype();
    trString += "/";  // separates theory and runtype
    string tmp = runtype.name();
    if (!tmp.empty()) {
      trString += tmp;
    } else {
      trString += "NA";
    }

    p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY, "Theory/Runtype"));
    p_body->GetSizer()->Add(new ewxNonBoldLabel(p_body, wxID_ANY, trString));
  }

  // CHEMICAL SYSTEM
  if (escalc) {
    p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY, "Chemical System"));
    p_body->GetSizer()->AddSpacer(0);

    if (frag) {
      if (!frag->name().empty()) {
        p_body->GetSizer()->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Name:"),
                   0, wxALIGN_RIGHT);
        p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY, frag->name()));
      }

      p_body->GetSizer()->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Charge:"),
                 0, wxALIGN_RIGHT);
      p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY,
                 wxString::Format("%d", frag->charge())));

      if (!frag->pointGroup().empty()) {
        p_body->GetSizer()->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Point Group:"),
                   0, wxALIGN_RIGHT);
        p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY, frag->pointGroup()));
      }
    }

    p_body->GetSizer()->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Spin Multiplicity:"),
               0, wxALIGN_RIGHT);
    p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY,
               SpinMult::toString(escalc->spinMultiplicity())));

    p_body->GetSizer()->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Open Shells:"),
               0, wxALIGN_RIGHT);
    p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY,
               wxString::Format("%lu", escalc->openShells())));
  }

  // BASIS SET
  if (escalc) {
    TGBSConfig *config = escalc->gbsConfig();
    if (config && !config->empty()) {
      p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY, "Basis Set"));
      p_body->GetSizer()->AddSpacer(0);

      if (!config->name().empty()) {
        p_body->GetSizer()->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Orbital:"),
                   0, wxALIGN_RIGHT);
        p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY, config->name()));
      }

      if (frag) {
        TagCountMap* tcMap = frag->tagCountsSTL();
        if (tcMap) {
          p_body->GetSizer()->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "# Functions:"),
                     0, wxALIGN_RIGHT);
          p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY,
                  wxString::Format("%d", config->num_functions(*tcMap))));

          p_body->GetSizer()->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "# Primitives:"),
                     0, wxALIGN_RIGHT);
          p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY,
                  wxString::Format("%d", config->num_primitives(*tcMap))));

          delete tcMap;
        }
      }

      if (!config->ecpName().empty()) {
        p_body->GetSizer()->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "ECPs:"),
                   0, wxALIGN_RIGHT);
        p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY, config->ecpName()));
      }

      if (!config->dftFittingName().empty()) {
        p_body->GetSizer()->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Exchange Fitting:"),
                   0, wxALIGN_RIGHT);
        p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY,
                config->dftFittingName()));
      }

      if (!config->dftChargeFittingName().empty()) {
        p_body->GetSizer()->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Charge Fitting:"),
                   0, wxALIGN_RIGHT);
        p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY,
                config->dftChargeFittingName()));
      }
    }
  }

  // LAUNCH INFO
  if (taskJob) {
    Jobdata job = taskJob->jobdata();
    Launchdata launch = taskJob->launchdata();

    p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY, "Launch Info"));
    p_body->GetSizer()->AddSpacer(0);

    if (!launch.machine.empty()) {
      p_body->GetSizer()->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Machine:"),
                 0, wxALIGN_RIGHT);
      p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY, launch.machine));
    }

    if (!launch.queue.empty()) {
      p_body->GetSizer()->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Queue:"),
                 0, wxALIGN_RIGHT);
      p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY, launch.queue));
    }

    if (!launch.account.empty() && !launch.machine.empty()) {
      string left = allocationLeft(launch, this);
      p_body->GetSizer()->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Allocation:"),
                 0, wxALIGN_RIGHT);
      p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY, launch.account));
      if (!left.empty()) {
        p_body->GetSizer()->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Left:"),
                   0, wxALIGN_RIGHT);
        p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY, left));
      }
    }

    p_body->GetSizer()->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "# Processors:"),
               0, wxALIGN_RIGHT);
    p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY,
            wxString::Format("%ld", launch.totalprocs)));

    if (!job.jobpath.empty()) {
      p_body->GetSizer()->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Run Directory:"),
                 0, wxALIGN_RIGHT);
      p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY, job.jobpath));
    }

    if (!launch.scratchdir.empty()) {
      p_body->GetSizer()->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Scratch Directory:"),
                 0, wxALIGN_RIGHT);
      p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY, launch.scratchdir));
    }

    if (!taskJob->startDate().empty()) {
      p_body->GetSizer()->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Start Date:"),
                 0, wxALIGN_RIGHT);
      p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY, taskJob->startDate()));
    }
  }

  // RUN STATS
  p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY, "Run Statistics"));
  p_body->GetSizer()->AddSpacer(0);
  set<Property_Ref> processKeys = PropFactory::ofClassification("Process");
  //  In the properties file's order, so a code's statistics read as it lists them.
  vector<Property_Ref> processRefs(processKeys.begin(), processKeys.end());
  std::stable_sort(processRefs.begin(), processRefs.end(),
                   [](const Property_Ref& x, const Property_Ref& y) {
                     return x.order < y.order;
                   });
  for (const Property_Ref& propRef : processRefs) {
    TProperty *tprop = getCalculation()->getProperty(propRef.short_name);
    if (tprop) {
      string key = propRef.long_name + ":";
      double scalar = tprop->scalarize();
      wxString valueStr;
      if (propRef.units == "Time") {
        valueStr = secondsToString((int)rint(scalar));
      } else {
        // Counts are not "5913.000000".
        valueStr = scalar == rint(scalar) ? wxString::Format("%.0f", scalar)
                                          : wxString::Format("%g", scalar);
        const string units = tprop->units();
        if (units == "Degree") valueStr += wxString::FromUTF8(" \xc2\xb0");
        else if (!units.empty() && units != "NA") valueStr += wxString::FromUTF8((" " + unitLabel(units)).c_str());
      }
      p_body->GetSizer()->Add(new ewxNonBoldLabel(p_body, wxID_ANY, key), 0, wxALIGN_RIGHT);
      p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY, valueStr.c_str()));
    }
  }

  //  Width from the rows; height is left to the dock, the rows scroll.
  //  The value column can be selected and copied (#408).
  wxSizerItemList &items = p_body->GetSizer()->GetChildren();
  int col = 0;
  for (wxSizerItemList::compatibility_iterator n = items.GetFirst(); n;
       n = n->GetNext(), col = 1 - col)
    if (col == 1 && n->GetData()->GetWindow())
      ewxMakeSelectable(n->GetData()->GetWindow());
  ewxWrapValueColumn(p_body);  // long values wrap within the panel (#490)
  p_body->FitInside();
  SetMinSize(wxSize(p_body->GetSizer()->GetMinSize().x, -1));
  Layout();

  // we're done with the frag and we're supposed to clean it up
  // according to the documentation
  if (frag) delete frag;
}


void CalcInfoPanel::initialize()
{
  refresh();
}


/**
 * Given the number of seconds, conert it into our elapsed time format
 * DDDD HH:MM:SS
 */
wxString CalcInfoPanel::secondsToString(int sec)
{
  int min, hrs, days;
  min = sec / 60;  // calc. # of hrs and leftover min
  sec = sec % 60;

  hrs = min / 60;  // calc. # of hrs and leftover min
  min = min % 60;

  days = hrs / 24; // calc. # of days and leftover hrs
  hrs = hrs % 24;

  return wxString::Format("%dd %02d:%02d:%02d", days, hrs, min, sec);
}


