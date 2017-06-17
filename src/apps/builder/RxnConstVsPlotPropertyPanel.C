#include <algorithm>
  using std::copy;
  using std::fill;

#include <wx/link.h>
#include <wx/sizer.h>

#include "util/InternalException.H"

#include "tdat/PropTable.H"
#include "tdat/PropVector.H"
#include "tdat/TProperty.H"

#include "dsm/IPropCalculation.H"
#include "dsm/PropFactory.H"

#include "wxgui/ewxButton.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxPlotCtrl.H"
#include "wxgui/ewxStaticText.H"

#include "RxnConstVsPlotPropertyPanel.H"


wxFORCE_LINK_THIS_MODULE(ReactionConstantVsPlotPropertyPanel)


IMPLEMENT_DYNAMIC_CLASS(ReactionConstantVsPlotPropertyPanel, PropertyPanel)


BEGIN_EVENT_TABLE(ReactionConstantVsPlotPropertyPanel, PropertyPanel)
END_EVENT_TABLE()


ReactionConstantVsPlotPropertyPanel::ReactionConstantVsPlotPropertyPanel()
  : PropertyPanel(),
    p_plotCtrl(NULL)
{
}


ReactionConstantVsPlotPropertyPanel::ReactionConstantVsPlotPropertyPanel(
        IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  : PropertyPanel(),
    p_plotCtrl(NULL)
{
  Create(calculation, parent, id, pos, size, style, name);
}


bool ReactionConstantVsPlotPropertyPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
  if (!PropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("ReactionConstantVsPlotPropertyPanel creation failed") );
    return false;
  }

  return true;
}


ReactionConstantVsPlotPropertyPanel::~ReactionConstantVsPlotPropertyPanel()
{
}


void ReactionConstantVsPlotPropertyPanel::refresh()
{
  IPropCalculation *calculation = NULL;
  set<string> props;
  int i = 0;
  string property_name = "";
  TProperty *property = NULL;
  PropTable *table = NULL;

  calculation = getCalculation();

  // Clear the plot
  p_plotCtrl->DeleteCurve(-1, true); // -1 deletes all, true emits event

  props = getPropertyNames();
  if (props.size() <= 0) {
    return;
  }

  property_name = hasError() ? *(++props.begin()) : *props.begin();
  property = calculation->getProperty(property_name);
  table = dynamic_cast<PropTable*>(property);
  if (table) {
    int col_1000_T = 1;
    int col_data = 0;
    int count = table->rows();
    double *a = new double[count];
    double *b = new double[count];
    wxPlotData *plotData = NULL;

    p_plotCtrl->SetXAxisLabel("1000/(T[K])");
    p_plotCtrl->SetYAxisLabel("ln(k_TST/k_CVT)");
    p_plotCtrl->SetPlotTitle(getName());

    col_data = hasError() ? 8 : 4;
    for (i=0; i<count; ++i) {
      a[i] = table->value(i, col_1000_T);
      b[i] = table->value(i, col_data);
    }
    plotData = new wxPlotData(a,b,count);
    p_plotCtrl->AddCurve(plotData, true, true);
  } else {
    INTERNALEXCEPTION(table, "Incorrect cast to a table");
  }
}


bool ReactionConstantVsPlotPropertyPanel::isRelevant(IPropCalculation *propCalc) const
{
  if (getName().find("CVT") != string::npos) {
    set<string> props;
    set<string>::iterator prop;
    string property_name = "";
    TProperty *property = NULL;
    PropTable *table = NULL;

    props = getPropertyNames();
    for (prop = props.begin(); prop != props.end(); ++prop) {
      property = propCalc->getProperty(*prop);
      table = dynamic_cast<PropTable*>(property);
      if (table) {
        vector<string> *labels = table->colLabels();
        vector<string>::iterator it;
        for (it = labels->begin(); it != labels->end(); ++it) {
          if (it->find("CVT") != string::npos) {
            return true;
          }
        }
      }
    }
  } else {
    return true;
  }

  return false;
}


void ReactionConstantVsPlotPropertyPanel::initialize()
{
  set<string> names;

  if (!GetSizer()) {
    SetSizer(new wxBoxSizer(wxVERTICAL));
  }

  // Catch the case where we goofed the registry.
  names = getPropertyNames();
  if (names.empty()) {
    GetSizer()->Add(new ewxStaticText(this, wxID_ANY,
                    "No properties registered for VectorPlotPropertyPanel"));
    return;
  }
               
  // create our wxPlotCtrl and add to sizer
  p_plotCtrl = new ewxPlotCtrl(this);
  p_plotCtrl->SetMinSize(wxSize(300,200));
  p_plotCtrl->SetShowXAxisLabel(true);
  p_plotCtrl->SetShowYAxisLabel(true);
  GetSizer()->Add(p_plotCtrl, 1, wxEXPAND);

  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);

  refresh();
}


bool ReactionConstantVsPlotPropertyPanel::hasError()
{
  set<string> names = getPropertyNames();
  if (!names.empty()) {
    string name = *names.begin();
    return getCalculation()->getProperty(name) == NULL;
  }

  return false;
}

