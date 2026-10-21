#include <cstdio>

#include "dsm/EDSIFactory.H"
#include "dsm/PropertyTask.H"
#include "dsm/IPropCalculation.H"
#include "dsm/ResourceDescriptor.H"
#include "tdat/PropTSVecTable.H"
#include "tdat/TAtm.H"
#include "tdat/Fragment.H"
#include "util/EcceURL.H"

#include "PointCalc.H"

bool applyPointStructure(Fragment *frag, const std::vector<double>& xyz,
                         std::string& why)
{
  const int n = frag->numAtoms();
  std::vector<int> target;
  for (int i = 0; i < n; i++)
    if (frag->atomRef(i)->atomicSymbol() != "X") target.push_back(i);
  if (3 * (size_t)n == xyz.size()) {
    target.clear();
    for (int i = 0; i < n; i++) target.push_back(i);
  } else if (3 * target.size() != xyz.size()) {
    char buf[128];
    snprintf(buf, sizeof buf, "The structure has %d atoms; the calculation has %d.",
             (int)(xyz.size() / 3), n);
    why = buf;
    return false;
  }
  for (size_t k = 0; k < target.size(); k++)
    frag->atomRef(target[k])->coordinates(xyz[3*k], xyz[3*k+1], xyz[3*k+2]);
  return true;
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
