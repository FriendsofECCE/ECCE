#include <algorithm>
  using std::copy;
  using std::reverse;

#include <wx/link.h>
#include <wx/log.h>
#include <wx/sizer.h>

#include "tdat/PropTSVecTable.H"
#include "tdat/PropVector.H"
#include "tdat/TProperty.H"

#include "dsm/IPropCalculation.H"
#include "dsm/PropFactory.H"

#include "wxgui/ewxNonBoldLabel.H"
#include "wxgui/ewxPlotCtrl.H"
#include "wxgui/ewxStaticText.H"

#include "viz/GTStepCmd.H"

#include "RxnTracePropertyPanel.H"


wxFORCE_LINK_THIS_MODULE(ReactionTracePropertyPanel)


IMPLEMENT_DYNAMIC_CLASS(ReactionTracePropertyPanel, GeomTracePropertyPanel)


BEGIN_EVENT_TABLE(ReactionTracePropertyPanel, GeomTracePropertyPanel)
END_EVENT_TABLE()


ReactionTracePropertyPanel::ReactionTracePropertyPanel()
  : GeomTracePropertyPanel(),
    p_index(0)
{
}


ReactionTracePropertyPanel::ReactionTracePropertyPanel(
        IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : GeomTracePropertyPanel(),
    p_index(0)
{
  Create(calculation, parent, id, pos, size, style, name);
}


bool ReactionTracePropertyPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
  if (!GeomTracePropertyPanel::Create(calculation, parent, id, pos, size, style,
                                name)) {
    wxFAIL_MSG( wxT("ReactionTracePropertyPanel creation failed") );
    return false;
  }

  return true;
}


ReactionTracePropertyPanel::~ReactionTracePropertyPanel()
{
}


void ReactionTracePropertyPanel::restoreSettings()
{
  // find first important prop that's not our single key nor the rx coord
  // and fill plot
  map<string,wxWindowID>::iterator it;
  for (it = p_keyToId.begin(); it != p_keyToId.end(); ++it) {
    if (it->first != "RXTRACE" && it->first != "RXCOORD"
            && getCalculation()->getProperty(it->first)) {
      p_currentProp = it->first;
      break;
    }
  }
}


void ReactionTracePropertyPanel::fillPlot()
{
  // clear any plots that might exist, don't send event
  p_plotCtrl->DeleteCurve(-1, false);

  if (!getCalculation()) return;

  PropVector *prop = dynamic_cast<PropVector*>(
          getCalculation()->getProperty(p_currentProp));
  if (!prop) return; // bail if bad prop
  vector<double> prop_vector = prop->values();

  PropVector *rxcoord_prop = dynamic_cast<PropVector*>(
          getCalculation()->getProperty("RXCOORD"));
  if (!rxcoord_prop) return; // bail if bad prop
  vector<double> rxcoord_prop_vector = rxcoord_prop->values();

  if (prop_vector.empty() || rxcoord_prop_vector.empty()
          || prop_vector.size() != rxcoord_prop_vector.size()) {
    return;
  }
  size_t size = prop_vector.size();

  // where does the calculation start over?
  p_index = 1;
  double last_val = rxcoord_prop_vector[0];
  for (p_index = 1; p_index < size; ++p_index) {
    if (rxcoord_prop_vector[p_index] > last_val) {
      break;
    }
    last_val = rxcoord_prop_vector[p_index];
  }
  
  // create array copies from the prop vectors
  double * rxcoord_prop_array = new double[size];
  copy(rxcoord_prop_vector.begin(), rxcoord_prop_vector.end(),
          rxcoord_prop_array);
  double * prop_array = new double[size];
  copy(prop_vector.begin(), prop_vector.end(), prop_array);

  // reverse the first section of the reaction
  reverse(rxcoord_prop_array, rxcoord_prop_array + p_index);
  reverse(prop_array, prop_array + p_index);
  
  // create our curve and add to plot control
  wxPlotData plotData(rxcoord_prop_array, prop_array, size);
  p_plotCtrl->AddCurve(plotData, true, true);

  // add and show the axes labels
  p_plotCtrl->SetXAxisLabel(prop->rowLabel());
  p_plotCtrl->SetYAxisLabel(prop->units());
  p_plotCtrl->SetShowXAxisLabel(true);
  p_plotCtrl->SetShowYAxisLabel(true);

  // add and show the plot title
  const Property_Ref *propref = PropFactory::getPropRef(p_currentProp);
  if (propref) {
    p_plotCtrl->SetPlotTitle(propref->long_name);
    p_plotCtrl->SetShowPlotTitle(true);
  }
}


void ReactionTracePropertyPanel::processStep(int step)
{
  p_plotCtrl->SetCursorDataIndex(0, step);

  WxVizToolFW& fw = getFW();
  IPropCalculation *expt = getCalculation();
  SGContainer& sg = fw.getSceneGraph();

  set<string> names = getPropertyNames();

  int index = p_index;
  if (step < index) {
    step = index - step - 1;
  }

  Command *cmd = new GTStepCmd("Trace Step", &sg, expt);
  cmd->getParameter("Index")->setInteger(step);
  cmd->getParameter("PropKey")->setString(getTracePropKey());

  fw.execute(cmd);
}

