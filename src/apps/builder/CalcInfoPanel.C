#include <algorithm>
#include <iostream>
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
#include "util/TDateTime.H"
#include <ctime>
#include "dsm/LaunchInfoRows.H"
#include "comm/AllocationQuery.H"

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
  p_body->SetSizer(new wxBoxSizer(wxVERTICAL));
  wxBoxSizer *outer = new wxBoxSizer(wxVERTICAL);
  //  The standard border: labels touched the left edge, long values the right.
  outer->Add(p_body, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);
  SetSizer(outer);

  return true;
}


CalcInfoPanel::~CalcInfoPanel()
{
}



//  Cores x wall time, to the completion date or, while running, to now.
static std::string launchCost(TaskJob *job, long processors)
{
  const std::string start = job->startDate(), end = job->completionDate();
  if (start.empty()) return "";
  const bool running = end.empty() &&
      job->getState() == ResourceDescriptor::STATE_RUNNING;
  if (end.empty() && !running) return "";
  const long s = TDateTime(start).toSeconds();
  const long e = running ? (long)time(NULL) : TDateTime(end).toSeconds();
  return LaunchInfoRows::coreHoursText(processors, s, e, running);
}

void CalcInfoPanel::refresh()
{
  //  Clear(true), not DestroyChildren(): the spacers stay in the sizer
  //  otherwise, and a refresh (the allocation answer) shifts every row by
  //  one cell, labels into the value column.
  p_body->GetSizer()->Clear(true);
  //  Rows before and after the Launch Info block are separate grids; the
  //  block is two columns of its own.
  vector<wxFlexGridSizer*> grids;
  wxSizer *launchBlock = NULL;
  wxFlexGridSizer *grid = new wxFlexGridSizer(0, 2, 1, 5);
  grids.push_back(grid);
  p_body->GetSizer()->Add(grid);

  TaskJob *taskJob = dynamic_cast<TaskJob*>(getCalculation());
  ICalculation *escalc = dynamic_cast<ICalculation*>(getCalculation());
  // only get the fragment once - this could be expensive
  Fragment * frag = NULL;
  if (escalc) frag = escalc->fragment();

  // APPLICATION
  if (taskJob) {
    const JCode* app = taskJob->application() ;
    if (app && !app->name().empty()) {
      grid->Add(new ewxStaticText(p_body, wxID_ANY, "Application"));
      grid->Add(new ewxNonBoldLabel(p_body, wxID_ANY, app->name()));
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

    grid->Add(new ewxStaticText(p_body, wxID_ANY, "Theory/Runtype"));
    grid->Add(new ewxNonBoldLabel(p_body, wxID_ANY, trString));
  }

  // CHEMICAL SYSTEM
  if (escalc) {
    grid->Add(new ewxStaticText(p_body, wxID_ANY, "Chemical System"));
    grid->AddSpacer(0);

    if (frag) {
      if (!frag->name().empty()) {
        grid->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Name:"),
                   0, wxALIGN_RIGHT);
        grid->Add(new ewxStaticText(p_body, wxID_ANY, frag->name()));
      }

      grid->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Charge:"),
                 0, wxALIGN_RIGHT);
      grid->Add(new ewxStaticText(p_body, wxID_ANY,
                 wxString::Format("%d", frag->charge())));

      if (!frag->pointGroup().empty()) {
        grid->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Point Group:"),
                   0, wxALIGN_RIGHT);
        grid->Add(new ewxStaticText(p_body, wxID_ANY, frag->pointGroup()));
      }
    }

    grid->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Spin Multiplicity:"),
               0, wxALIGN_RIGHT);
    grid->Add(new ewxStaticText(p_body, wxID_ANY,
               SpinMult::toString(escalc->spinMultiplicity())));

    grid->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Open Shells:"),
               0, wxALIGN_RIGHT);
    grid->Add(new ewxStaticText(p_body, wxID_ANY,
               wxString::Format("%lu", escalc->openShells())));
  }

  // BASIS SET
  if (escalc) {
    TGBSConfig *config = escalc->gbsConfig();
    if (config && !config->empty()) {
      grid->Add(new ewxStaticText(p_body, wxID_ANY, "Basis Set"));
      grid->AddSpacer(0);

      if (!config->name().empty()) {
        grid->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Orbital:"),
                   0, wxALIGN_RIGHT);
        grid->Add(new ewxStaticText(p_body, wxID_ANY, config->name()));
      }

      if (frag) {
        TagCountMap* tcMap = frag->tagCountsSTL();
        if (tcMap) {
          grid->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "# Functions:"),
                     0, wxALIGN_RIGHT);
          grid->Add(new ewxStaticText(p_body, wxID_ANY,
                  wxString::Format("%d", config->num_functions(*tcMap))));

          grid->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "# Primitives:"),
                     0, wxALIGN_RIGHT);
          grid->Add(new ewxStaticText(p_body, wxID_ANY,
                  wxString::Format("%d", config->num_primitives(*tcMap))));

          delete tcMap;
        }
      }

      if (!config->ecpName().empty()) {
        grid->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "ECPs:"),
                   0, wxALIGN_RIGHT);
        grid->Add(new ewxStaticText(p_body, wxID_ANY, config->ecpName()));
      }

      if (!config->dftFittingName().empty()) {
        grid->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Exchange Fitting:"),
                   0, wxALIGN_RIGHT);
        grid->Add(new ewxStaticText(p_body, wxID_ANY,
                config->dftFittingName()));
      }

      if (!config->dftChargeFittingName().empty()) {
        grid->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Charge Fitting:"),
                   0, wxALIGN_RIGHT);
        grid->Add(new ewxStaticText(p_body, wxID_ANY,
                config->dftChargeFittingName()));
      }
    }
  }

  // LAUNCH INFO
  if (taskJob) {
    Jobdata job = taskJob->jobdata();
    Launchdata launch = taskJob->launchdata();

    grid->Add(new ewxStaticText(p_body, wxID_ANY, "Launch Info"));
    grid->AddSpacer(0);

    LaunchInfoRows::Input in;
    in.machine = launch.machine;
    in.processors = launch.totalprocs;
    in.queue = launch.queue;
    in.jobId = job.jobid;
    in.startDate = taskJob->startDate();
    in.completionDate = taskJob->completionDate();
    in.coreHours = launchCost(taskJob, launch.totalprocs);
    in.allocation = launch.machine.empty() ? string() :
                    AllocationQuery::account(launch);
    in.allocationRemaining = launch.allocationLeft;
    launchBlock = ewxLaunchInfoBlock(p_body, LaunchInfoRows::build(in));
    p_body->GetSizer()->Add(launchBlock, 0, wxTOP | wxBOTTOM, 2);

    grid = new wxFlexGridSizer(0, 2, 1, 5);
    grids.push_back(grid);
    p_body->GetSizer()->Add(grid);

    if (!job.jobpath.empty()) {
      grid->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Run Directory:"),
                 0, wxALIGN_RIGHT);
      grid->Add(new ewxStaticText(p_body, wxID_ANY, job.jobpath));
    }

    if (!launch.scratchdir.empty()) {
      grid->Add(new ewxNonBoldLabel(p_body, wxID_ANY, "Scratch Directory:"),
                 0, wxALIGN_RIGHT);
      grid->Add(new ewxStaticText(p_body, wxID_ANY, launch.scratchdir));
    }
  }

  // RUN STATS
  grid->Add(new ewxStaticText(p_body, wxID_ANY, "Run Statistics"));
  grid->AddSpacer(0);
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
      grid->Add(new ewxNonBoldLabel(p_body, wxID_ANY, key), 0, wxALIGN_RIGHT);
      grid->Add(new ewxStaticText(p_body, wxID_ANY, valueStr.c_str()));
    }
  }

  //  Width from the rows; height is left to the dock, the rows scroll.
  //  The value column can be selected and copied (#408).
  for (wxFlexGridSizer *g : grids) {
    wxSizerItemList &items = g->GetChildren();
    int col = 0;
    for (wxSizerItemList::compatibility_iterator n = items.GetFirst(); n;
         n = n->GetNext(), col = 1 - col)
      if (col == 1 && n->GetData()->GetWindow())
        ewxMakeSelectable(n->GetData()->GetWindow());
  }
  {
    //  One value column for the grids and the Launch Info between them.
    vector<wxSizer*> blocks(grids.begin(), grids.end());
    if (launchBlock) blocks.insert(blocks.begin() + 1, launchBlock);
    ewxAlignLabelColumns(blocks);
  }
  ewxWrapValueColumn(p_body);  // long values wrap within the panel (#490)
  p_body->FitInside();
  SetMinSize(wxSize(p_body->GetSizer()->GetMinSize().x, -1));
  Layout();
  ewxFitLaunchInfo(p_body);

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


