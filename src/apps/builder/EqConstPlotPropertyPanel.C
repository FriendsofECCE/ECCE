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
#include "XYPlotCanvas.H"
#include "wxgui/ewxStaticText.H"

#include "EqConstPlotPropertyPanel.H"


wxFORCE_LINK_THIS_MODULE(EquilibriumConstantPlotPropertyPanel)


IMPLEMENT_DYNAMIC_CLASS(EquilibriumConstantPlotPropertyPanel, PropertyPanel)


const wxWindowID EquilibriumConstantPlotPropertyPanel::ID_BUTTON_REFERENCE(wxNewId());


BEGIN_EVENT_TABLE(EquilibriumConstantPlotPropertyPanel, PropertyPanel)
END_EVENT_TABLE()


EquilibriumConstantPlotPropertyPanel::EquilibriumConstantPlotPropertyPanel()
  : PropertyPanel(),
    p_plotCtrl(NULL)
{
}


EquilibriumConstantPlotPropertyPanel::EquilibriumConstantPlotPropertyPanel(
        IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  : PropertyPanel(),
    p_plotCtrl(NULL)
{
  Create(calculation, parent, id, pos, size, style, name);
}


bool EquilibriumConstantPlotPropertyPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
  if (!PropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("EquilibriumConstantPlotPropertyPanel creation failed") );
    return false;
  }

  return true;
}


EquilibriumConstantPlotPropertyPanel::~EquilibriumConstantPlotPropertyPanel()
{
}


void EquilibriumConstantPlotPropertyPanel::refresh()
{
  IPropCalculation *calculation = NULL;
  set<string> props;
  int i = 0;
  string property_name = "";
  TProperty *property = NULL;
  PropTable *table = NULL;

  calculation = getCalculation();

  // Clear the plot
  p_plotCtrl->clearCurves();

  props = getPropertyNames();
  if (props.size() <= 0) {
    return;
  }

  property_name = *props.begin();

  property = calculation->getProperty(property_name);
  table = dynamic_cast<PropTable*>(property);
  if (table) {
    int count = table->rows();
    double *x = new double[count];
    double *y = new double[count];

    int col_x = 3;
    int col_y = 2;
    for (i=0; i<count; ++i) {
      x[i] = table->value(i, col_x);
      y[i] = table->value(i, col_y);
    }
    { int curve = p_plotCtrl->addCurve(x, y, count, wxString(), 0, XYPlotCanvas::LINE_MARKERS);
      p_plotCtrl->setCurveMarker(curve, 0); }
    delete[] x;
    delete[] y;

    p_plotCtrl->setXAxisLabel(table->colLabels()->at(col_x));
    p_plotCtrl->setYAxisLabel(table->colLabels()->at(col_y));
    p_plotCtrl->setTitle(getName());
  } else {
    INTERNALEXCEPTION(table, "Incorrect cast to a table");
  }
}


void EquilibriumConstantPlotPropertyPanel::initialize()
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
  p_plotCtrl = new XYPlotCanvas(this);
  p_plotCtrl->SetMinSize(wxSize(300,200));
  GetSizer()->Add(p_plotCtrl, 1, wxEXPAND);

  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);

  refresh();
}

