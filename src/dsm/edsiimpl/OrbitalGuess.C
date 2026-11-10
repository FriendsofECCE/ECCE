#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include "tdat/LatticeDef.H"
#include "tdat/Fragment.H"
#include "tdat/TAtm.H"
#include "tdat/GUIValues.H"
#include "tdat/PropTSVecTable.H"
#include <vector>

#include "util/SFile.H"
#include "util/TypedFile.H"
#include "util/TempStorage.H"
#include "dsm/ICalculation.H"
#include "dsm/JCode.H"
#include "dsm/OrbitalGuess.H"
#include "dsm/TaskJob.H"
#include "dsm/PropertyTask.H"

using std::string;
using std::vector;

namespace {

// The URL of the orbital file the code keeps from a run, or an empty URL.
EcceURL keptOrbitals(const TaskJob *calc)
{
  EcceURL none;
  const JCode *code = calc->application();
  if (code == 0) return none;
  vector<TypedFile> declared = code->getCodeFiles(JCode::AUXILIARY_OUTPUT);
  //  A code with several auxiliary outputs (Gaussian's fort.7 as well)
  //  names the one that holds the orbitals.
  string only;
  code->get_string("RetainOutputFile", only);
  vector<EcceURL> stored = calc->getDataFiles(JCode::AUXILIARY_OUTPUT);
  for (size_t i = 0; i < stored.size(); i++) {
    if (!only.empty()) {
      if (stored[i].getFilePathTail() == only) return stored[i];
      continue;
    }
    //  ORCA's .hess is kept beside its .gbw but is not the orbital file.
    if (stored[i].getFilePathTail().size() > 5 &&
        stored[i].getFilePathTail().compare(
            stored[i].getFilePathTail().size() - 5, 5, ".hess") == 0)
      continue;
    for (size_t d = 0; d < declared.size(); d++)
      if (stored[i].getFilePathTail() == declared[d].name()) return stored[i];
  }
  return none;
}

string atomSequence(const Fragment *fragment)
{
  std::ostringstream out;
  if (fragment != 0)
    for (int i = 0; i < fragment->numAtoms(); ++i)
      out << fragment->atomRef(i)->atomicNumber() << " ";
  return out.str();
}

string cp2kBasis(const GUIValues *settings)
{
  // CP2K chooses its basis in Theory Details, outside the Gaussian basis
  // model. Include per-element overrides and library files as well.
  std::ostringstream out;
  auto value = [&](const string& key, const string& fallback) {
    GUIValue *v = settings == 0 ? 0 : settings->get("ES.Theory.GPW." + key);
    string text = v == 0 ? string() : v->getValueAsString();
    return text.empty() ? fallback : text;
  };
  out << value("BasisLevel", "DZVP-MOLOPT-SR-GTH") << "|"
      << value("BasisFile", "BASIS_MOLOPT") << "|"
      << value("LibraryDir", "/usr/share/cp2k");
  for (int i = 1; i <= 6; ++i) {
    const string row = std::to_string(i);
    for (const char *key : {"KindElement", "KindBasis", "KindBasisFile"})
      out << "|" << value(string(key) + row, "");
  }
  return out.str();
}

string qeGeometry(const Fragment *fragment, bool cell)
{
  std::ostringstream out;
  out << std::setprecision(17);
  if (fragment == 0) return "";
  if (cell) {
    if (fragment->getLattice() == 0) return "";
    vector<MPoint> *vectors = fragment->getLattice()->toVectors();
    for (auto &v : *vectors) out << v.x() << " " << v.y() << " " << v.z() << " ";
    delete vectors;
  } else {
    for (int i = 0; i < fragment->numAtoms(); ++i) {
      const double *xyz = fragment->atomRef(i)->coordinates();
      out << xyz[0] << " " << xyz[1] << " " << xyz[2] << " ";
    }
  }
  return out.str();
}

bool sameGeometry(const string &before, const string &after)
{
  std::istringstream a(before), b(after);
  double x, y;
  while (a >> x) {
    if (!(b >> y) || std::abs(x - y) > 1.e-6) return false;
  }
  return !(b >> y);
}

string qeCutoffs(const GUIValues *settings)
{
  auto value = [&](const string &key, double fallback) {
    GUIValue *v = settings == 0 ? 0 : settings->get("ES.Theory.PW." + key);
    return v == 0 ? fallback : v->getValueAsDouble();
  };
  std::ostringstream out;
  out << std::setprecision(17) << value("EcutWfc", 30.) << " ";
  const bool explicitRho = value("UseEcutRho", 0.) != 0.;
  out << explicitRho << " " << (explicitRho ? value("EcutRho", 240.) : 0.);
  return out.str();
}

bool readsGuess(const TaskJob *calc)
{
  return !calc->orbitalGuessFile().empty();
}

}


bool OrbitalGuess::available(const TaskJob *source, string &why)
{
  why.clear();
  if (source == 0 || !readsGuess(source)) {
    why = "Only a calculation of a code that can start from earlier "
          "orbitals (ORCA, Gaussian 16, NWChem, ECCE-QM, MOPAC, CP2K, Quantum ESPRESSO) can be duplicated with its final "
          "orbitals.";
    return false;
  }
  if (keptOrbitals(source).toString().empty()) {
    why = "This calculation has no stored orbital file. It is kept for "
          "runs made with this version of ECCE, once the run has finished "
          "(Gaussian: with Create Checkpoint File on).";
    return false;
  }
  return true;
}


namespace {

EcceURL keptHessian(const TaskJob *calc)
{
  vector<EcceURL> files = calc->getDataFiles(JCode::AUXILIARY_OUTPUT);
  for (size_t i = 0; i < files.size(); i++) {
    const string tail = files[i].getFilePathTail();
    if (tail.size() > 5 && tail.compare(tail.size() - 5, 5, ".hess") == 0)
      return files[i];
  }
  return EcceURL();
}

}


bool OrbitalGuess::hessianAvailable(const TaskJob *source)
{
  return source != 0 && !keptHessian(source).toString().empty();
}


bool OrbitalGuess::attachHessian(TaskJob *target, TaskJob *source,
                                 string &why)
{
  if (target->hessianInputFile().empty()) {
    why = "The calculation's code cannot read a starting Hessian.";
    return false;
  }
  if (!hessianAvailable(source)) {
    why = "The source calculation has no stored Hessian.";
    return false;
  }
  SFile *temp = TempStorage::getTempFile();
  SFile *got = source->getAnyFile(keptHessian(source), temp);
  bool ok = got != 0 && temp->exists() &&
            attachHessianFile(target, temp->path(), source->getName(), why);
  temp->remove();
  delete temp;
  return ok;
}


bool OrbitalGuess::attachHessianFile(TaskJob *target, const string &path,
                                     const string &sourceName, string &why)
{
  if (target->hessianInputFile().empty()) {
    why = "The calculation's code cannot read a starting Hessian.";
    return false;
  }
  std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
  if (!in || !target->putInputFile(target->hessianInputFile(), &in)) {
    why = "The Hessian could not be stored in the calculation.";
    return false;
  }
  target->addProp(TaskJob::hessianFromSourceProp(), sourceName);
  return true;
}


bool OrbitalGuess::attach(TaskJob *target, TaskJob *source, string &why)
{
  if (!available(source, why)) return false;
  if (!readsGuess(target)) {
    why = "The duplicate's code cannot read starting orbitals.";
    return false;
  }

  SFile *temp = TempStorage::getTempFile();
  SFile *got = source->getAnyFile(keptOrbitals(source), temp);
  if (got == 0 || !temp->exists()) {
    delete temp;
    why = "The orbital file could not be copied from the data server.";
    return false;
  }
  std::ifstream in(temp->path().c_str(), std::ios::in | std::ios::binary);
  bool ok = target->putInputFile(target->orbitalGuessFile(), &in);
  in.close();
  temp->remove();
  delete temp;
  if (!ok) {
    why = "The orbital file could not be stored in the duplicate.";
    return false;
  }

  //  The Hessian is optional: a run that computed none leaves no .hess.
  if (!target->hessianInputFile().empty() && hessianAvailable(source)) {
    string ignored;
    attachHessian(target, source, ignored);
  }

  string basis;
  const ICalculation *calc = dynamic_cast<const ICalculation *>(source);
  if (calc != 0) {
    TGBSConfig *config = calc->gbsConfig();
    if (config != 0) {
      basis = config->name();
      delete config;
    }
  }
  const string code = source->application()->getCodeName();
  if (calc != 0 && (code == "MOPAC" || code == "CP2K" || code == "QuantumESPRESSO")) {
    Fragment *frag = calc->fragment();
    if (code == "QuantumESPRESSO" && frag != 0) {
      // The stored input structure can precede a relaxation; the density
      // belongs to the final geometry and, for vc-relax, the final cell.
      PropertyTask *properties = dynamic_cast<PropertyTask*>(source);
      if (properties != 0) {
        properties->getFragmentStep(frag, -1);
        PropTSVecTable *cells = dynamic_cast<PropTSVecTable*>(properties->getProperty("LATTICEVEC"));
        if (cells != 0 && cells->tables() > 0) {
          const vector<double> &v = cells->values(cells->tables() - 1);
          if (v.size() == 9) {
            LatticeDef *cell = new LatticeDef(1, 1, 1, 90, 90, 90);
            cell->assign(MPoint(v[0], v[1], v[2]), MPoint(v[3], v[4], v[5]),
                         MPoint(v[6], v[7], v[8]));
            frag->setLattice(cell);
          }
        }
      }
    }
    target->addProp(TaskJob::orbitalGuessSourceProp() + "atoms", atomSequence(frag));
    if (code == "QuantumESPRESSO") {
      target->addProp(TaskJob::orbitalGuessSourceProp() + "positions", qeGeometry(frag, false));
      target->addProp(TaskJob::orbitalGuessSourceProp() + "cell", qeGeometry(frag, true));
      GUIValues *settings = calc->guiparams();
      target->addProp(TaskJob::orbitalGuessBasisProp() + "settings", qeCutoffs(settings));
      delete settings;
    }
    delete frag;
    if (code == "CP2K") {
      GUIValues *settings = calc->guiparams();
      target->addProp(TaskJob::orbitalGuessBasisProp() + "settings", cp2kBasis(settings));
      delete settings;
    }
  }
  target->addProp(TaskJob::orbitalGuessSourceProp(), source->getName());
  target->addProp(TaskJob::orbitalGuessBasisProp(), basis);
  return true;
}


string OrbitalGuess::sourceName(const TaskJob *calc)
{
  return calc == 0 ? string() : calc->getProp(TaskJob::orbitalGuessSourceProp());
}


string OrbitalGuess::sourceBasis(const TaskJob *calc)
{
  return calc == 0 ? string() : calc->getProp(TaskJob::orbitalGuessBasisProp());
}


string OrbitalGuess::compatibilityWarning(const TaskJob *calc,
                                          const Fragment *fragment,
                                          const GUIValues *settings)
{
  if (calc == 0 || calc->application() == 0) return "";
  const string code = calc->application()->getCodeName();
  if (code != "MOPAC" && code != "CP2K" && code != "QuantumESPRESSO") return "";
  string note;
  const string atoms = calc->getProp(TaskJob::orbitalGuessSourceProp() + "atoms");
  if (!atoms.empty() && atoms != atomSequence(fragment))
    note += "; " + code + " needs the same atoms in the same order: "
            "restore them or use the default guess";
  if (code == "QuantumESPRESSO") {
    const string positions = calc->getProp(TaskJob::orbitalGuessSourceProp() + "positions");
    const string cell = calc->getProp(TaskJob::orbitalGuessSourceProp() + "cell");
    const string cutoffs = calc->getProp(TaskJob::orbitalGuessBasisProp() + "settings");
    if (!sameGeometry(positions, qeGeometry(fragment, false)))
      note += "; Quantum ESPRESSO needs the same atoms at the same positions: "
              "restore the structure or use the default guess";
    if (!sameGeometry(cell, qeGeometry(fragment, true)))
      note += "; Quantum ESPRESSO needs the same cell: restore it or use the default guess";
    if (!sameGeometry(cutoffs, qeCutoffs(settings)))
      note += "; Quantum ESPRESSO needs the same cutoffs: restore the Theory Details "
              "cutoff settings or use the default guess";
  }
  if (code == "CP2K") {
    const string basis = calc->getProp(TaskJob::orbitalGuessBasisProp() + "settings");
    if (!basis.empty() && basis != cp2kBasis(settings))
      note += "; CP2K needs the same basis set: restore the Theory Details "
              "basis settings or use the default guess";
  }
  return note;
}
