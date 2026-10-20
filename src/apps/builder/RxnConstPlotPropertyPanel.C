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

#include "RxnConstPlotPropertyPanel.H"


wxFORCE_LINK_THIS_MODULE(ReactionConstantPlotPropertyPanel)


IMPLEMENT_DYNAMIC_CLASS(ReactionConstantPlotPropertyPanel, PropertyPanel)


BEGIN_EVENT_TABLE(ReactionConstantPlotPropertyPanel, PropertyPanel)
END_EVENT_TABLE()


ReactionConstantPlotPropertyPanel::ReactionConstantPlotPropertyPanel()
  : PropertyPanel(),
    p_plotCtrl(NULL)
{
}


ReactionConstantPlotPropertyPanel::ReactionConstantPlotPropertyPanel(
        IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  : PropertyPanel(),
    p_plotCtrl(NULL)
{
  Create(calculation, parent, id, pos, size, style, name);
}


bool ReactionConstantPlotPropertyPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
  if (!PropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("ReactionConstantPlotPropertyPanel creation failed") );
    return false;
  }

  return true;
}


ReactionConstantPlotPropertyPanel::~ReactionConstantPlotPropertyPanel()
{
}


void ReactionConstantPlotPropertyPanel::refresh()
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

  property_name = hasError() ? *(++props.begin()) : *props.begin();
  property = calculation->getProperty(property_name);
  table = dynamic_cast<PropTable*>(property);
  if (table) {
    int col_1000_T = 1;
    int col_data = 0;
    int count = table->rows();
    double *a = new double[count];
    double *b = new double[count];
    double *c = new double[count];
    double *d = new double[count];

    p_plotCtrl->setXAxisLabel("1000/(T[K])");
    p_plotCtrl->setYAxisLabel("ln(k[1/sec])");
    p_plotCtrl->setTitle(getName());

    // figure out which columns to use, first TST, then CVT
    // (w/error)     "T 1000/T ln(k_TST) low high ln(k_CVT) low high"
    // (w/out error) "T 1000/T ln(k_TST) ln(k_CVT)"
    // ... first, TST
    col_data = 2;
    for (i=0; i<count; ++i) {
      a[i] = table->value(i, col_1000_T);
      b[i] = table->value(i, col_data);
    }
    { int curve = p_plotCtrl->addCurve(a, b, count, "TST", 0, XYPlotCanvas::LINE_MARKERS);
      p_plotCtrl->setCurveMarker(curve, 1); }
    delete[] a;
    delete[] b;

    // ... next, CVT
    col_data = hasError() ? 5 : 3;
    for (i=0; i<count; ++i) {
      c[i] = table->value(i, col_1000_T);
      d[i] = table->value(i, col_data);
    }
    { int curve = p_plotCtrl->addCurve(c, d, count, "CVT", 1, XYPlotCanvas::LINE_MARKERS);
      p_plotCtrl->setCurveMarker(curve, 2); }
    delete[] c;
    delete[] d;
  } else {
    INTERNALEXCEPTION(table, "Incorrect cast to a table");
  }
}


bool
ReactionConstantPlotPropertyPanel::isRelevant(IPropCalculation *propCalc) const
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


void ReactionConstantPlotPropertyPanel::initialize()
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
  p_plotCtrl->setShowKey(true);
  GetSizer()->Add(p_plotCtrl, 1, wxEXPAND);

  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);

  refresh();
}


bool ReactionConstantPlotPropertyPanel::hasError()
{
  set<string> names = getPropertyNames();
  if (!names.empty()) {
    string name = *names.begin();
    return getCalculation()->getProperty(name) == NULL;
  }

  return false;
}

