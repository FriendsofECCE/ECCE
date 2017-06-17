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

#include "RxnErrPlotPropertyPanel.H"


wxFORCE_LINK_THIS_MODULE(ReactionErrorPlotPropertyPanel)


IMPLEMENT_DYNAMIC_CLASS(ReactionErrorPlotPropertyPanel, PropertyPanel)


const wxWindowID ReactionErrorPlotPropertyPanel::ID_BUTTON_REFERENCE(wxNewId());


map<wxString,wxString> ReactionErrorPlotPropertyPanel::s_key_to_name;
map<wxString,wxString> ReactionErrorPlotPropertyPanel::s_name_to_key;


BEGIN_EVENT_TABLE(ReactionErrorPlotPropertyPanel, PropertyPanel)
  EVT_CHOICE(wxID_ANY, ReactionErrorPlotPropertyPanel::OnStdDevChoice)
  EVT_BUTTON(ID_BUTTON_REFERENCE,
             ReactionErrorPlotPropertyPanel::OnReferenceClick)
END_EVENT_TABLE()


ReactionErrorPlotPropertyPanel::ReactionErrorPlotPropertyPanel()
  : PropertyPanel(),
    p_plotCtrl(NULL)
{
}


ReactionErrorPlotPropertyPanel::ReactionErrorPlotPropertyPanel(
        IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  : PropertyPanel(),
    p_plotCtrl(NULL)
{
  Create(calculation, parent, id, pos, size, style, name);
}


bool ReactionErrorPlotPropertyPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
  if (!PropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("ReactionErrorPlotPropertyPanel creation failed") );
    return false;
  }

  if (s_key_to_name.empty()) {
    s_key_to_name["FORWARD_RATE1"] = "1 Standard Deviation";
    s_key_to_name["FORWARD_RATE2"] = "2 Standard Deviations";
    s_key_to_name["REVERSE_RATE1"] = "1 Standard Deviation";
    s_key_to_name["REVERSE_RATE2"] = "2 Standard Deviations";
    for (map<wxString,wxString>::iterator it = s_key_to_name.begin();
         it != s_key_to_name.end();
         ++it) {
      s_name_to_key[it->second] = it->first;
    }
  }

  return true;
}


ReactionErrorPlotPropertyPanel::~ReactionErrorPlotPropertyPanel()
{
}


void ReactionErrorPlotPropertyPanel::refresh()
{
  IPropCalculation *calculation = NULL;
  set<string> props;
  set<string>::iterator prop;
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

  property_name = *props.begin();

  if (hasError()) {
    int lastSelection = p_choice->GetSelection();
    p_choice->Clear();
    for (prop = ++props.begin(); prop != props.end(); ++prop) {
      p_choice->Append(s_key_to_name[*prop]);
    }
    if (lastSelection != wxNOT_FOUND) {
      p_choice->SetSelection(lastSelection);
    } else {
      p_choice->SetSelection(0);
    }
    property_name = s_name_to_key[p_choice->GetStringSelection()];
  }

  property = calculation->getProperty(property_name);
  table = dynamic_cast<PropTable*>(property);
  if (table) {
    //bool isForward;
    bool isTST;
    int col_1000_T = 1;
    int col_data = 0;
    double *x,*y;
    int count = 0;
    wxPlotData *plotData = NULL;
    // figure out which columns to use
    // always have at least the columns "T 1000/T ln(k_TST) ln(k_CVT)"
    //isForward = property_name.find("FORWARD") != string::npos;
    isTST = getName().find("TST") != string::npos;
    if (hasError())
      col_data = isTST ? 2 : 5;
    else
      col_data = isTST ? 2 : 3;
    x = new double[table->rows()];
    y = new double[table->rows()];
    // get the data for the 1000/T column and the "data" column
    count = table->rows();
    for (i=0; i<count; ++i) {
      x[i] = table->value(i, col_1000_T);
      y[i] = table->value(i, col_data);
    }
    plotData = new wxPlotData(x,y,count);
    plotData->SetFilename(isTST ? "TST" : "CVT");
    plotData->SetPen(wxPLOTPEN_NORMAL, wxPen(*wxBLUE));
    plotData->SetPen(wxPLOTPEN_ACTIVE, wxPen(*wxBLUE));
    plotData->SetPen(wxPLOTPEN_SELECTED, wxPen(*wxBLUE));
    p_plotCtrl->AddCurve(plotData, true, true);

    p_plotCtrl->SetXAxisLabel("1000/(T[K])");
    p_plotCtrl->SetYAxisLabel("ln(k[1/sec])");
    p_plotCtrl->SetPlotTitle(getName());

    if (hasError()) {
      double *a,*b,*c,*d;
      a = new double[table->rows()];
      b = new double[table->rows()];
      c = new double[table->rows()];
      d = new double[table->rows()];
      // add error lines since they exist
      col_data = isTST ? 3 : 6;
      for (int i=0; i<count; ++i) {
        a[i] = table->value(i, col_1000_T);
        b[i] = table->value(i, col_data);
      }
      plotData = new wxPlotData(a,b,count);
      plotData->SetFilename(isTST? "TST_lower": "CVT_lower");
      plotData->SetPen(wxPLOTPEN_NORMAL, wxPen(*wxRED));
      plotData->SetPen(wxPLOTPEN_ACTIVE, wxPen(*wxRED));
      plotData->SetPen(wxPLOTPEN_SELECTED, wxPen(*wxRED));
      p_plotCtrl->AddCurve(plotData, true, true);
      col_data = isTST ? 4 : 7;
      for (int i=0; i<count; ++i) {
        c[i] = table->value(i, col_1000_T);
        d[i] = table->value(i, col_data);
      }
      plotData = new wxPlotData(c,d,count);
      plotData->SetFilename(isTST? "TST_upper": "CVT_upper");
      plotData->SetPen(wxPLOTPEN_NORMAL, wxPen(*wxBLACK));
      plotData->SetPen(wxPLOTPEN_ACTIVE, wxPen(*wxBLACK));
      plotData->SetPen(wxPLOTPEN_SELECTED, wxPen(*wxBLACK));
      p_plotCtrl->AddCurve(plotData, true, true);
    }
  } else {
    INTERNALEXCEPTION(table, "Incorrect cast to a table");
  }
}


bool
ReactionErrorPlotPropertyPanel::isRelevant(IPropCalculation *propCalc) const
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


void ReactionErrorPlotPropertyPanel::initialize()
{
  set<string> names;
  wxSizer * sizer;

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
  p_plotCtrl->SetShowKey(true);
  p_plotCtrl->SetKeyPosition(wxPoint(0,0));
  GetSizer()->Add(p_plotCtrl, 1, wxEXPAND);

  if (hasError()) {
    sizer = new wxBoxSizer(wxHORIZONTAL);
    p_choice = new ewxChoice(this);
    sizer->Add(p_choice);
    sizer->AddStretchSpacer();
    sizer->Add(new ewxButton(this, ID_BUTTON_REFERENCE, "Reference..."));
    GetSizer()->Add(sizer, 0, wxGROW);
  }

  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);

  refresh();
}


bool ReactionErrorPlotPropertyPanel::hasError()
{
  set<string> names = getPropertyNames();
  if (!names.empty()) {
    string name = *names.begin();
    return getCalculation()->getProperty(name) == NULL;
  }

  return false;
}


void ReactionErrorPlotPropertyPanel::OnStdDevChoice(wxCommandEvent &event)
{
  refresh();
}


void ReactionErrorPlotPropertyPanel::OnReferenceClick(wxCommandEvent &event)
{
  string caption = "Confidence Limits Reference";
  string message = "Lower and upper confidence limits for rate constants were calculated based on the errors reported in:\n\n\"Representative Benchmark Suites for Barrier Heights of Diverse Reaction Types and Assessment of Electronic Structure Methods for Thermochemical Kinetics,\" J. Zheng, Y. Zhao, and D. G. Truhlar, Journal of Chemical Theory and Computation 3, 569-582 (2007).";

  ewxMessageDialog dialog(this, message, caption, wxOK);
  dialog.ShowModal();
}

