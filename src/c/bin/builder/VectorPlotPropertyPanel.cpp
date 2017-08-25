#include <algorithm>
  using std::copy;

#include <wx/link.h>
#include <wx/sizer.h>

#include "tdat/PropVector.H"
#include "tdat/TProperty.H"

#include "dsm/IPropCalculation.H"

#include "wxgui/ewxPlotCtrl.H"
#include "wxgui/ewxStaticText.H"

#include "VectorPlotPropertyPanel.H"


wxFORCE_LINK_THIS_MODULE(VectorPlotPropertyPanel)


IMPLEMENT_DYNAMIC_CLASS(VectorPlotPropertyPanel, PropertyPanel)


VectorPlotPropertyPanel::VectorPlotPropertyPanel()
  : PropertyPanel(),
    p_plotCtrl(NULL)
{
}


VectorPlotPropertyPanel::VectorPlotPropertyPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : PropertyPanel(),
    p_plotCtrl(NULL)
{
  Create(calculation, parent, id, pos, size, style, name);
}


bool VectorPlotPropertyPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
  if (!PropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("VectorPlotPropertyPanel creation failed") );
    return false;
  }

  if (!GetSizer()) {
    SetSizer(new wxBoxSizer(wxVERTICAL));
  }

  // there should be only one TProperty
  set<string> names = getPropertyNames();
  if (names.empty()) {
    GetSizer()->Add(new ewxStaticText(this, wxID_ANY,
            "No properties registered for VectorPlotPropertyPanel"));
    return true;
  } else if (names.size() > 1) {
    GetSizer()->Add(new ewxStaticText(this, wxID_ANY,
            "Too many properties registered for VectorPlotPropertyPanel"));
    return true;
  }

  TProperty *tprop = getCalculation()->getProperty(*(names.begin()));
  PropVector *propVector = dynamic_cast<PropVector*>(tprop);
  if (!propVector) {
    GetSizer()->Add(new ewxStaticText(this, wxID_ANY,
            "Property was not a PropVector"));
    return true;
  }

  // create our wxPlotCtrl and add to sizer
  p_plotCtrl = new ewxPlotCtrl(this, wxID_ANY);
  GetSizer()->Add(p_plotCtrl, 1, wxEXPAND);

  // wxPlotData expects either an ascii file or a pointer to an array
  vector<double> &vectorData = propVector->values();
  double *arrayData = new double[vectorData.size()];
  copy(vectorData.begin(), vectorData.end(), arrayData);
  
  // We also need our "step" data
  double *stepData = new double[vectorData.size()];
  for (size_t i = 0; i < vectorData.size(); i++) {
    stepData[i] = i+1;
  }

  // create our curve and add to plot control
  wxPlotData plotData(stepData, arrayData, vectorData.size());
  p_plotCtrl->AddCurve(plotData, true, true);

  // add and show the axes labels
  p_plotCtrl->SetXAxisLabel(propVector->rowLabel());
  p_plotCtrl->SetYAxisLabel(propVector->units());
  p_plotCtrl->SetShowXAxisLabel(true);
  p_plotCtrl->SetShowYAxisLabel(true);

  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);

  return true;
}


VectorPlotPropertyPanel::~VectorPlotPropertyPanel()
{
}

