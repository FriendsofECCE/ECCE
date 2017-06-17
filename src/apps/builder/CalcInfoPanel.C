#include <iostream>
  using std::cout;
  using std::endl;

#include "wx/link.h"
#include "wx/log.h"
#include "wx/sizer.h"

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
  : PropertyPanel()
{
}


CalcInfoPanel::CalcInfoPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  : PropertyPanel()
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

  if (!GetSizer())
    SetSizer(new wxFlexGridSizer(0,2,1,5));

  return true;
}


CalcInfoPanel::~CalcInfoPanel()
{
}


void CalcInfoPanel::refresh()
{
  DestroyChildren();

  TaskJob *taskJob = dynamic_cast<TaskJob*>(getCalculation());
  ICalculation *escalc = dynamic_cast<ICalculation*>(getCalculation());
  // only get the fragment once - this could be expensive
  Fragment * frag = NULL;
  if (escalc) frag = escalc->fragment();

  // APPLICATION
  if (taskJob) {
    const JCode* app = taskJob->application() ;
    if (app && !app->name().empty()) {
      GetSizer()->Add(new ewxStaticText(this, wxID_ANY, "Application"));
      GetSizer()->Add(new ewxNonBoldLabel(this, wxID_ANY, app->name()));
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

    GetSizer()->Add(new ewxStaticText(this, wxID_ANY, "Theory/Runtype"));
    GetSizer()->Add(new ewxNonBoldLabel(this, wxID_ANY, trString));
  }

  // CHEMICAL SYSTEM
  if (escalc) {
    GetSizer()->Add(new ewxStaticText(this, wxID_ANY, "Chemical System"));
    GetSizer()->AddSpacer(0);

    if (frag) {
      if (!frag->name().empty()) {
        GetSizer()->Add(new ewxNonBoldLabel(this, wxID_ANY, "Name:"),
                   0, wxALIGN_RIGHT);
        GetSizer()->Add(new ewxStaticText(this, wxID_ANY, frag->name()));
      }

      GetSizer()->Add(new ewxNonBoldLabel(this, wxID_ANY, "Charge:"),
                 0, wxALIGN_RIGHT);
      GetSizer()->Add(new ewxStaticText(this, wxID_ANY,
                 wxString::Format("%d", frag->charge())));

      if (!frag->pointGroup().empty()) {
        GetSizer()->Add(new ewxNonBoldLabel(this, wxID_ANY, "Point Group:"),
                   0, wxALIGN_RIGHT);
        GetSizer()->Add(new ewxStaticText(this, wxID_ANY, frag->pointGroup()));
      }
    }

    GetSizer()->Add(new ewxNonBoldLabel(this, wxID_ANY, "Spin Multiplicity:"),
               0, wxALIGN_RIGHT);
    GetSizer()->Add(new ewxStaticText(this, wxID_ANY,
               SpinMult::toString(escalc->spinMultiplicity())));

    GetSizer()->Add(new ewxNonBoldLabel(this, wxID_ANY, "Open Shells:"),
               0, wxALIGN_RIGHT);
    GetSizer()->Add(new ewxStaticText(this, wxID_ANY,
               wxString::Format("%lu", escalc->openShells())));
  }

  // BASIS SET
  if (escalc) {
    TGBSConfig *config = escalc->gbsConfig();
    if (config && !config->empty()) {
      GetSizer()->Add(new ewxStaticText(this, wxID_ANY, "Basis Set"));
      GetSizer()->AddSpacer(0);

      if (!config->name().empty()) {
        GetSizer()->Add(new ewxNonBoldLabel(this, wxID_ANY, "Orbital:"),
                   0, wxALIGN_RIGHT);
        GetSizer()->Add(new ewxStaticText(this, wxID_ANY, config->name()));
      }

      if (frag) {
        TagCountMap* tcMap = frag->tagCountsSTL();
        if (tcMap) {
          GetSizer()->Add(new ewxNonBoldLabel(this, wxID_ANY, "# Functions:"),
                     0, wxALIGN_RIGHT);
          GetSizer()->Add(new ewxStaticText(this, wxID_ANY,
                  wxString::Format("%d", config->num_functions(*tcMap))));

          GetSizer()->Add(new ewxNonBoldLabel(this, wxID_ANY, "# Primitives:"),
                     0, wxALIGN_RIGHT);
          GetSizer()->Add(new ewxStaticText(this, wxID_ANY,
                  wxString::Format("%d", config->num_primitives(*tcMap))));

          delete tcMap;
        }
      }

      if (!config->ecpName().empty()) {
        GetSizer()->Add(new ewxNonBoldLabel(this, wxID_ANY, "ECPs:"),
                   0, wxALIGN_RIGHT);
        GetSizer()->Add(new ewxStaticText(this, wxID_ANY, config->ecpName()));
      }

      if (!config->dftFittingName().empty()) {
        GetSizer()->Add(new ewxNonBoldLabel(this, wxID_ANY, "Exchange Fitting:"),
                   0, wxALIGN_RIGHT);
        GetSizer()->Add(new ewxStaticText(this, wxID_ANY,
                config->dftFittingName()));
      }

      if (!config->dftChargeFittingName().empty()) {
        GetSizer()->Add(new ewxNonBoldLabel(this, wxID_ANY, "Charge Fitting:"),
                   0, wxALIGN_RIGHT);
        GetSizer()->Add(new ewxStaticText(this, wxID_ANY,
                config->dftChargeFittingName()));
      }
    }
  }

  // LAUNCH INFO
  if (taskJob) {
    Jobdata job = taskJob->jobdata();
    Launchdata launch = taskJob->launchdata();

    GetSizer()->Add(new ewxStaticText(this, wxID_ANY, "Launch Info"));
    GetSizer()->AddSpacer(0);

    if (!launch.machine.empty()) {
      GetSizer()->Add(new ewxNonBoldLabel(this, wxID_ANY, "Machine:"),
                 0, wxALIGN_RIGHT);
      GetSizer()->Add(new ewxStaticText(this, wxID_ANY, launch.machine));
    }

    if (!launch.queue.empty()) {
      GetSizer()->Add(new ewxNonBoldLabel(this, wxID_ANY, "Queue:"),
                 0, wxALIGN_RIGHT);
      GetSizer()->Add(new ewxStaticText(this, wxID_ANY, launch.queue));
    }

    GetSizer()->Add(new ewxNonBoldLabel(this, wxID_ANY, "# Processors:"),
               0, wxALIGN_RIGHT);
    GetSizer()->Add(new ewxStaticText(this, wxID_ANY,
            wxString::Format("%ld", launch.totalprocs)));

    if (!job.jobpath.empty()) {
      GetSizer()->Add(new ewxNonBoldLabel(this, wxID_ANY, "Run Directory:"),
                 0, wxALIGN_RIGHT);
      GetSizer()->Add(new ewxStaticText(this, wxID_ANY, job.jobpath));
    }

    if (!launch.scratchdir.empty()) {
      GetSizer()->Add(new ewxNonBoldLabel(this, wxID_ANY, "Scratch Directory:"),
                 0, wxALIGN_RIGHT);
      GetSizer()->Add(new ewxStaticText(this, wxID_ANY, launch.scratchdir));
    }

    if (!taskJob->startDate().empty()) {
      GetSizer()->Add(new ewxNonBoldLabel(this, wxID_ANY, "Start Date:"),
                 0, wxALIGN_RIGHT);
      GetSizer()->Add(new ewxStaticText(this, wxID_ANY, taskJob->startDate()));
    }
  }

  // RUN STATS
  GetSizer()->Add(new ewxStaticText(this, wxID_ANY, "Run Statistics"));
  GetSizer()->AddSpacer(0);
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
      GetSizer()->Add(new ewxNonBoldLabel(this, wxID_ANY, key), 0, wxALIGN_RIGHT);
      GetSizer()->Add(new ewxStaticText(this, wxID_ANY, valueStr.c_str()));
    }
  }

  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);

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


