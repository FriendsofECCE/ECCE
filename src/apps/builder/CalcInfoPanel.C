#include <iostream>
  using std::cout;
  using std::endl;

#include "wx/link.h"
#include "wx/log.h"
#include "wx/sizer.h"
#include "wx/scrolwin.h"

#include "tdat/TProperty.H"

#include "dsm/ICalculation.H"
#include "dsm/IPropCalculation.H"
#include "dsm/JCode.H"
#include "dsm/PropFactory.H"
#include "dsm/TaskJob.H"

#include "wxgui/ewxNonBoldLabel.H"
#include "wxgui/ewxStaticText.H"

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
  outer->Add(p_body, 1, wxEXPAND);
  SetSizer(outer);

  return true;
}


CalcInfoPanel::~CalcInfoPanel()
{
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
  set<Property_Ref>::iterator propRef;
  for (propRef = processKeys.begin(); propRef != processKeys.end(); ++propRef) {
    TProperty *tprop = getCalculation()->getProperty(propRef->short_name);
    if (tprop) {
      string key = propRef->long_name + ":";
      double scalar = tprop->scalarize();
      wxString valueStr;
      if (propRef->units == "Time") {
        valueStr = secondsToString((int)rint(scalar));
      } else {
        valueStr = wxString::Format("%f", scalar);
      }
      p_body->GetSizer()->Add(new ewxNonBoldLabel(p_body, wxID_ANY, key), 0, wxALIGN_RIGHT);
      p_body->GetSizer()->Add(new ewxStaticText(p_body, wxID_ANY, valueStr.c_str()));
    }
  }

  //  Width from the rows; height is left to the dock, the rows scroll.
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


