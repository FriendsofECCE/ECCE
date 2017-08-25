#include <algorithm>
  using std::copy;
  using std::fill;

#include <wx/link.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include "tdat/PropVector.H"

#include "dsm/IPropCalculation.H"
#include "dsm/PropFactory.H"

#include "wxgui/ewxChoice.H"
#include "wxgui/ewxIntClientData.H"
#include "wxgui/ewxPlotCtrl.H"
#include "wxgui/ewxStaticText.H"

#include "PlotAnyPropertyPanel.H"


wxFORCE_LINK_THIS_MODULE(PlotAnyPropertyPanel)


IMPLEMENT_DYNAMIC_CLASS(PlotAnyPropertyPanel, PropertyPanel)


BEGIN_EVENT_TABLE(PlotAnyPropertyPanel, PropertyPanel)
  EVT_CHOICE(wxID_ANY, PlotAnyPropertyPanel::OnAxisChoice)
END_EVENT_TABLE()


PlotAnyPropertyPanel::PlotAnyPropertyPanel()
  : PropertyPanel(),
    p_plotCtrl(NULL),
    p_xaxis(NULL),
    p_yaxis(NULL)
{
}


PlotAnyPropertyPanel::PlotAnyPropertyPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  : PropertyPanel(),
    p_plotCtrl(NULL),
    p_xaxis(NULL),
    p_yaxis(NULL)
{
  Create(calculation, parent, id, pos, size, style, name);
}


bool PlotAnyPropertyPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
  if (!PropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("PlotAnyPropertyPanel creation failed") );
    return false;
  }

  return true;
}


PlotAnyPropertyPanel::~PlotAnyPropertyPanel()
{
}


void PlotAnyPropertyPanel::refresh()
{
  doAxisChoice();
}


void PlotAnyPropertyPanel::initialize()
{
  set<string> names;
  wxSizer * axisSizer;

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

  // create the axis choice boxes
  // lay them out in a flex grid sizer although two horizontal sizers
  // would have sufficed
  // NOTE: using wxCB_SORT does not work with wxChoice that uses wxClientData
  axisSizer = new wxFlexGridSizer(2); // 2 columns
  GetSizer()->Add(axisSizer, 0, wxEXPAND);
  axisSizer->Add(new wxStaticText(this, wxID_STATIC, "X-Axis"),
          0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
  //p_xaxis = new ewxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
          //wxArrayString(), wxCB_SORT);
  p_xaxis = new ewxChoice(this, wxID_ANY);
  axisSizer->Add(p_xaxis, 0, wxALL, 5);
  axisSizer->Add(new wxStaticText(this, wxID_STATIC, "Y-Axis"),
          0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
  //p_yaxis = new ewxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
          //wxArrayString(), wxCB_SORT);
  p_yaxis = new ewxChoice(this, wxID_ANY);
  axisSizer->Add(p_yaxis, 0, wxALL, 5);

  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);

  refreshChoices(wxEmptyString, wxEmptyString);
  doAxisChoice();
}


/**
 * Populate the axis choice boxes.
 *
 * We're expecting each property to be a PropVector (or PropTSVector).
 * PropVectors are one dimensional, so we take the "rowLabel" to be the 
 * name of the dimension e.g. Geometry Step, Atom, Normal Mode.
 * This panel only handles a single "rowLabel" set of vectors, so no
 * extra care is needed to handle multiple ones.
 *
 * We first populate a map<> so that the axis names get sorted.  Relying
 * on wxWidgets to sort the choiceboxes was causing bad behavior.
 *
 * @param lastX last choice for x axis, so we can restore the selection
 * @param lastY last choice for y axis, so we can restore the selection
 */
void PlotAnyPropertyPanel::refreshChoices(const wxString& lastX,
                                          const wxString& lastY)
{
  map<wxString,wxString> axisNames;
  map<wxString,wxString>::iterator axisName;
  wxString dimension;
  set<string> names;
  set<string>::iterator name;
  PropVector *prop = NULL;

  names = getPropertyNames();
  for (name = names.begin(); name != names.end(); ++name) {
    prop = dynamic_cast<PropVector*>(getCalculation()->getProperty(*name));
    if (prop) {
      const Property_Ref *propRef = PropFactory::getPropRef(*name);
      if (propRef) {
        wxString longName = propRef->long_name;
        axisNames[longName] = *name;
        if (dimension.IsEmpty()) dimension = prop->rowLabel();
      }
    }
  }

  p_xaxis->Clear();
  p_yaxis->Clear();
  for (axisName = axisNames.begin(); axisName != axisNames.end(); ++axisName) {
    p_xaxis->Append(axisName->first, new wxStringClientData(axisName->second));
    p_yaxis->Append(axisName->first, new wxStringClientData(axisName->second));
  }
  // Our dimension is always last in the list
  p_xaxis->Append(dimension);
  p_yaxis->Append(dimension);

  // initialize the plot to some x and y axis
  // attemp to restore the last selections first
  if (!p_xaxis->SetStringSelection(lastX)) {
    p_xaxis->SetSelection(p_xaxis->GetCount()-1); // select dimension
  }
  if (!p_yaxis->SetStringSelection(lastY)) {
    p_yaxis->SetSelection(0); // select first prop vec
  }
}


void PlotAnyPropertyPanel::OnAxisChoice(wxCommandEvent &event)
{
  doAxisChoice();
}


void PlotAnyPropertyPanel::doAxisChoice()
{
  // clear any plot(s) that might exist
  p_plotCtrl->DeleteCurve(-1, true); // -1 deletes all, true emits event

  // now we need to get the data to be plotted
  // its a problem if the data we're trying to plot does not contain the same
  // number of elements x and y
  // first, determine max size of each array and set both to largest
  int sizeX = getSelectionSize(p_xaxis);
  int sizeY = getSelectionSize(p_yaxis);

  int size;
  double *x, *y;
  if (sizeX > 0 && sizeY > 0) {
    // user selected prop vecs for both axes
    // make sure they're the same length
    size = sizeX > sizeY ? sizeX : sizeY;
    // create and initialize the data arrays
    x = new double[size];
    y = new double[size];
    fill(x, x+size, 0);
    fill(y, y+size, 0);
    // fill the data arrays with their appropriate values
    getSelectionData(p_xaxis, x);
    getSelectionData(p_yaxis, y);
  } else if (sizeX > 0 || sizeY > 0) {
    // user selected prop vec for one axis only
    size = sizeX > 0 ? sizeX : sizeY;
    x = new double[size];
    y = new double[size];
    fill(x, x+size, 0);
    fill(y, y+size, 0);
    // fill the data arrays with their appropriate values
    if (sizeX > 0) {
      getSelectionData(p_xaxis, x);
      for (int i = 0; i < size; ++i) { y[i] = i+1; }
    } else {
      getSelectionData(p_yaxis, y);
      for (int i = 0; i < size; ++i) { x[i] = i+1; }
    }
  } else {
    // user selected dimension for both axis - dumb user!
    size = 0;
    x = new double[size];
    y = new double[size];
  }

  // create our curve and add to plot control
  wxPlotData *plotData = new wxPlotData(x, y, size);
  p_plotCtrl->AddCurve(plotData, true, true);

  // add and show the axes labels
  refreshAxisLabel(p_xaxis);
  refreshAxisLabel(p_yaxis);
  p_plotCtrl->SetPlotTitle(p_xaxis->GetStringSelection()
                           + " vs " + p_yaxis->GetStringSelection());
}


int PlotAnyPropertyPanel::getSelectionSize(ewxChoice* axis)
{
  int ret = -1;

  wxClientData *clientData = axis->GetClientObject(axis->GetSelection());
  wxStringClientData *stringData;
  if ((stringData = dynamic_cast<wxStringClientData*>(clientData))) {
    PropVector *prop = dynamic_cast<PropVector*>(
            getCalculation()->getProperty(stringData->GetData().c_str()));
    ret = prop->rows();
  } else {
  }

  return ret;
}


void PlotAnyPropertyPanel::getSelectionData(ewxChoice* axis, double *values)
{
  wxClientData *clientData = axis->GetClientObject(axis->GetSelection());
  wxStringClientData *stringData;
  if ((stringData = dynamic_cast<wxStringClientData*>(clientData))) {
    PropVector *prop = dynamic_cast<PropVector*>(
            getCalculation()->getProperty(stringData->GetData().c_str()));
    vector<double> &vectorData = prop->values();
    copy(vectorData.begin(), vectorData.end(), values);
  } else {
  }
}


void PlotAnyPropertyPanel::refreshAxisLabel(ewxChoice *axis)
{
  wxString label;

  wxClientData *clientData = axis->GetClientObject(axis->GetSelection());
  wxStringClientData *stringData;
  if ((stringData = dynamic_cast<wxStringClientData*>(clientData))) {
    TProperty *prop =
            getCalculation()->getProperty(stringData->GetData().c_str());
    label << prop->units();
  } else {
    label << axis->GetStringSelection();
  }

  if (axis == p_xaxis) {
    p_plotCtrl->SetXAxisLabel(label);
  } else if (axis == p_yaxis) {
    p_plotCtrl->SetYAxisLabel(label);
  }
}

