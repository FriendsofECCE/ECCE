#include "util/UnitDisplay.H"
#include <algorithm>
  using std::copy;

#include <wx/link.h>
#include <wx/sizer.h>

#include "tdat/PropVector.H"
#include "tdat/TProperty.H"

#include "dsm/IPropCalculation.H"

#include "XYPlotCanvas.H"
#include "StepPlotAxes.H"
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
  p_plotCtrl = new XYPlotCanvas(this);
  GetSizer()->Add(p_plotCtrl, 1, wxEXPAND);

  vector<double> &vectorData = propVector->values();
  vector<double> stepData(vectorData.size());
  for (size_t i = 0; i < vectorData.size(); i++) stepData[i] = i+1;
  const StepPlotAxes::Shaped shaped = StepPlotAxes::shape(
      *(names.begin()), propVector->units(), vectorData);
  const int curve = p_plotCtrl->addCurve(
      stepData, shaped.y.empty() ? vectorData : shaped.y);
  for (size_t i = 0; i < shaped.hover.size(); i++)
    p_plotCtrl->setPointLabel(curve, (int)i, shaped.hover[i]);

  p_plotCtrl->setIntegerX(true);
  p_plotCtrl->setXAxisLabel(StepPlotAxes::xAxisName(propVector->rowLabel()));
  p_plotCtrl->setYAxisLabel(shaped.yLabel.empty()
                            ? StepPlotAxes::unitText(propVector->units())
                            : shaped.yLabel);

  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);

  return true;
}


VectorPlotPropertyPanel::~VectorPlotPropertyPanel()
{
}

