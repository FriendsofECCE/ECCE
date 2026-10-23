#include "util/UnitDisplay.H"
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
#include "XYPlotCanvas.H"
#include "StepPlotAxes.H"
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
  p_plotCtrl = new XYPlotCanvas(this);
  // Small minimum: in a short pane the plot must give way before the
  // axis rows below it are clipped.
  p_plotCtrl->SetMinSize(wxSize(200,90));
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
  bool timeSeries = false;
  size_t steps = 0;

  names = getPropertyNames();
  for (name = names.begin(); name != names.end(); ++name) {
    prop = dynamic_cast<PropVector*>(getCalculation()->getProperty(*name));
    if (prop) {
      const Property_Ref *propRef = PropFactory::getPropRef(*name);
      if (propRef) {
        //  The time series is the step axis of an MD run, not a quantity.
        if (*name == "TIMEVEC" && prop->units() == "ps") {
          timeSeries = true;
          continue;
        }
        axisNames[StepPlotAxes::plotName(propRef->long_name)] = *name;
        if (dimension.IsEmpty()) {
          dimension = prop->rowLabel();
          steps = prop->rows();
        }
      }
    }
  }
  std::vector<double> t;
  wxString unit;
  if (timeSeries && StepPlotAxes::mdTime(getCalculation(), steps, t, unit))
    dimension = "Time";

  p_xaxis->Clear();
  p_yaxis->Clear();
  for (axisName = axisNames.begin(); axisName != axisNames.end(); ++axisName) {
    p_xaxis->Append(axisName->first, new wxStringClientData(axisName->second));
    p_yaxis->Append(axisName->first, new wxStringClientData(axisName->second));
  }
  // Our dimension is always last in the list
  const wxString shown = StepPlotAxes::xAxisName(dimension.ToStdString());
  p_xaxis->Append(shown);
  p_yaxis->Append(shown);

  // initialize the plot to some x and y axis
  // attemp to restore the last selections first
  if (!p_xaxis->SetStringSelection(lastX)) {
    p_xaxis->SetSelection(p_xaxis->GetCount()-1); // select dimension
  }
  //  Otherwise the energy: the first name in alphabetical order (often a
  //  noisy pressure) is not what one looks at first.
  if (!p_yaxis->SetStringSelection(lastY) &&
      !p_yaxis->SetStringSelection("Total Energy") &&
      !p_yaxis->SetStringSelection("Potential Energy")) {
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
  p_plotCtrl->clearCurves();

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
    // the shorter series decides: padding with zeros draws points that are
    // not in the data
    size = sizeX < sizeY ? sizeX : sizeY;
    // create and initialize the data arrays
    x = new double[size];
    y = new double[size];
    fill(x, x+size, 0);
    fill(y, y+size, 0);
    // fill the data arrays with their appropriate values
    std::vector<double> bx(sizeX), by(sizeY);
    getSelectionData(p_xaxis, &bx[0]);
    getSelectionData(p_yaxis, &by[0]);
    std::copy(bx.begin(), bx.begin() + size, x);
    std::copy(by.begin(), by.begin() + size, y);
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

  // The step axis of an MD run is its simulation time.
  p_timeUnit.clear();
  std::vector<double> t;
  if (size > 0 && (sizeX <= 0 || sizeY <= 0) &&
      StepPlotAxes::mdTime(getCalculation(), size, t, p_timeUnit)) {
    std::copy(t.begin(), t.end(), sizeX > 0 ? y : x);
  } else {
    p_timeUnit.clear();
  }

  // A series against the step: convergence quantities as log10, an energy
  // relative to step 1 (see StepPlotAxes::shape).
  StepPlotAxes::Shaped shaped;
  const bool stepAxis = (sizeX <= 0) != (sizeY <= 0);
  if (stepAxis && p_timeUnit.empty()) {
    const bool yIsSeries = sizeY > 0;
    shaped = StepPlotAxes::shape(
        getSelectionKey(yIsSeries ? p_yaxis : p_xaxis),
        getSelectionUnits(yIsSeries ? p_yaxis : p_xaxis),
        std::vector<double>(yIsSeries ? y : x, (yIsSeries ? y : x) + size));
    if (!shaped.y.empty()) std::copy(shaped.y.begin(), shaped.y.end(),
                                     yIsSeries ? y : x);
  }
  p_plotCtrl->setIntegerX(stepAxis && p_timeUnit.empty());

  // create our curve and add to plot control
  p_plotCtrl->setShowKey(false);
  const bool timedY = !p_timeUnit.empty() && sizeY > 0 && sizeX <= 0;
  const int curve = p_plotCtrl->addCurve(
      x, y, size, timedY ? p_yaxis->GetStringSelection() : wxString());
  for (size_t i = 0; i < shaped.hover.size() && (int)i < size; i++)
    p_plotCtrl->setPointLabel(curve, (int)i, shaped.hover[i]);
  if (timedY) {
    StepPlotAxes::addRunningAverage(p_plotCtrl,
                                    std::vector<double>(x, x + size),
                                    std::vector<double>(y, y + size));
  }
  delete[] x;
  delete[] y;

  // add and show the axes labels
  refreshAxisLabel(p_xaxis);
  refreshAxisLabel(p_yaxis);
  // Name the quantity (the axis labels only carry units), as the
  // Geometry Trace plot does.
  wxString title = p_yaxis->GetStringSelection();
  if (getSelectionSize(p_xaxis) > 0) {
    title += " vs " + p_xaxis->GetStringSelection();
  }
  if (!shaped.titleNote.empty()) title += " (" + shaped.titleNote + ")";
  p_plotCtrl->setTitle(title);
  if (!shaped.yLabel.empty())
    (sizeY > 0 ? p_plotCtrl->setYAxisLabel(shaped.yLabel)
               : p_plotCtrl->setXAxisLabel(shaped.yLabel));

  const bool timeX = !p_timeUnit.empty() && sizeX <= 0;
  const bool timeY = !p_timeUnit.empty() && sizeY <= 0;
  if (timeX || timeY) {
    p_plotCtrl->setReadout("Frame", timeX ? "Time" : "", timeX ? p_timeUnit : "",
                           timeY ? "Time" : "", timeY ? p_timeUnit : "");
  } else {
    p_plotCtrl->setReadout("Step", sizeX > 0 ? shaped.yName : wxString(),
                           sizeX > 0 ? shaped.yUnit : wxString(),
                           sizeY > 0 ? shaped.yName : wxString(),
                           sizeY > 0 ? shaped.yUnit : wxString());
  }
}


int PlotAnyPropertyPanel::getSelectionSize(ewxChoice* axis)
{
  int ret = -1;

  wxClientData *clientData = axis->GetClientObject(axis->GetSelection());
  wxStringClientData *stringData;
  if ((stringData = dynamic_cast<wxStringClientData*>(clientData))) {
    PropVector *prop = dynamic_cast<PropVector*>(
            getCalculation()->getProperty(stringData->GetData().ToStdString()));
    ret = prop->rows();
  } else {
  }

  return ret;
}


std::string PlotAnyPropertyPanel::getSelectionKey(ewxChoice* axis)
{
  wxStringClientData *d = dynamic_cast<wxStringClientData*>(
      axis->GetClientObject(axis->GetSelection()));
  return d ? d->GetData().ToStdString() : std::string();
}


std::string PlotAnyPropertyPanel::getSelectionUnits(ewxChoice* axis)
{
  TProperty *prop = getCalculation()->getProperty(getSelectionKey(axis));
  return prop ? prop->units() : std::string();
}


void PlotAnyPropertyPanel::getSelectionData(ewxChoice* axis, double *values)
{
  wxClientData *clientData = axis->GetClientObject(axis->GetSelection());
  wxStringClientData *stringData;
  if ((stringData = dynamic_cast<wxStringClientData*>(clientData))) {
    PropVector *prop = dynamic_cast<PropVector*>(
            getCalculation()->getProperty(stringData->GetData().ToStdString()));
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
            getCalculation()->getProperty(stringData->GetData().ToStdString());
    label << StepPlotAxes::unitText(prop->units());
  } else if (!p_timeUnit.empty()) {
    label << "Time (" << p_timeUnit << ")";
  } else {
    label << StepPlotAxes::xAxisName(axis->GetStringSelection().ToStdString());
  }

  if (axis == p_xaxis) {
    p_plotCtrl->setXAxisLabel(label);
  } else if (axis == p_yaxis) {
    p_plotCtrl->setYAxisLabel(label);
  }
}

