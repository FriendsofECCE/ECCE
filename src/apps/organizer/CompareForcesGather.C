#include "CompareForces.H"
#include "CompareEnergies.H"

#include <memory>
#include "dsm/EDSIFactory.H"
#include "dsm/PropertyTask.H"
#include "tdat/Fragment.H"
#include "tdat/PropTable.H"
#include "tdat/PropTSVecTable.H"
#include "tdat/TAtm.H"
#include "util/EcceURL.H"

ForceInput gatherForceInput(const std::string& url)
{
  ForceInput r;
  PropertyTask *task = EDSIFactory::getPropertyTask(EcceURL(url));
  if (!task) {
    r.problem = "The calculation cannot be read.";
    return r;
  }
  std::unique_ptr<Fragment> frag(task->fragment());
  if (!frag || !frag->numAtoms()) {
    r.problem = "The calculation has no final geometry.";
    return r;
  }
  for (int i = 0; i < frag->numAtoms(); ++i) {
    const TAtm *atom = frag->atomRef(i);
    r.elements.push_back(atom->atomicSymbol());
    const double *xyz = atom->coordinates();
    r.coordinates.insert(r.coordinates.end(), xyz, xyz+3);
  }
  // Read the final stored frame directly: a viewer's selected scan point
  // and display wrapping must not change a convergence comparison.
  PropTSVecTable *geometry = dynamic_cast<PropTSVecTable*>(task->getProperty("GEOMTRACE"));
  if (geometry) {
    if (!geometry->tables() || geometry->columns() != 3 ||
        geometry->values(geometry->tables()-1).size() != r.coordinates.size()) {
      r.problem = "The final geometry trace does not match the atoms.";
      return r;
    }
    r.coordinates = geometry->values(geometry->tables()-1);
    if (geometry->units() == "Bohr")
      for (double& x : r.coordinates) x *= 0.529177210903;
    else if (geometry->units() != "Angstrom") {
      r.problem = "Unsupported geometry units: " + geometry->units() + ".";
      return r;
    }
  }
  TProperty *property = task->getProperty("EGRADVEC");
  // EGRADVEC is normally a write-once table: later parsed blocks replace it.
  // Also accept a step table, taking only its last geometry step.
  PropTable *table = dynamic_cast<PropTable*>(property);
  PropTSVecTable *steps = dynamic_cast<PropTSVecTable*>(property);
  if (table && table->columns() == 3) r.gradient = table->values();
  else if (steps && steps->columns() == 3 && steps->tables())
    r.gradient = steps->values(steps->tables()-1);
  else {
    r.problem = "The calculation has no stored EGRADVEC gradient with three components per atom.";
    return r;
  }
  r.units = property->units();
  r.energy = gatherEnergyTerm(url, 1);
  return r;
}
