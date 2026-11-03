#include <cstdio>

#include "dsm/EDSIFactory.H"
#include "dsm/PropertyTask.H"
#include "dsm/IPropCalculation.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ScanPoint.H"
#include "tdat/PropTSVecTable.H"
#include "tdat/TAtm.H"
#include "tdat/Fragment.H"
#include "util/EcceURL.H"

#include "PointCalc.H"

bool applyPointStructure(Fragment *frag, const std::vector<double>& xyz,
                         std::string& why)
{
  return ScanPoint::apply(frag, xyz, why);
}

std::string newCalcFromPoint(IPropCalculation *calc, const std::string& key,
                             int frame, int point, std::string& why)
{
  if (!calc) { why = "No calculation."; return ""; }
  PropTSVecTable *geom = dynamic_cast<PropTSVecTable*>(calc->getProperty(key));
  if (!geom || frame < 0 || frame >= (int)geom->tables()) {
    why = "This point has no stored structure.";
    return "";
  }
  const std::vector<double> xyz = geom->values(frame);

  EcceURL url = calc->getURL();
  Resource *source = EDSIFactory::getResource(url);
  if (!source) { why = "The calculation cannot be opened."; return ""; }
  EcceURL copyUrl = url;
  if (!source->copy(copyUrl)) {
    why = source->messages();
    return "";
  }
  Resource *copy = EDSIFactory::getResource(copyUrl);
  PropertyTask *task = dynamic_cast<PropertyTask*>(copy);
  Fragment *frag = task ? task->fragment() : 0;
  if (!frag) { why = "The copy has no chemical system."; return ""; }
  if (!applyPointStructure(frag, xyz, why)) return "";
  task->fragment(frag);
  task->resetForRerun(ResourceDescriptor::STATE_CREATED);

  // The copy has a generated name; a failed rename keeps it.
  char suffix[32];
  snprintf(suffix, sizeof suffix, "_point_%d", point);
  EcceURL named = url.getParent().getChild(url.getFilePathTail() + suffix);
  if (copy->move(named)) copyUrl = named;
  EDSIFactory::notifyCreate(copyUrl.toString(), true);
  return copyUrl.toString();
}
