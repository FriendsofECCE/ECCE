#include <fstream>
#include <string>
#include <vector>

#include "util/SFile.H"
#include "util/TypedFile.H"
#include "util/TempStorage.H"
#include "dsm/ICalculation.H"
#include "dsm/JCode.H"
#include "dsm/OrbitalGuess.H"
#include "dsm/TaskJob.H"

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
          "orbitals (ORCA, Gaussian 16, NWChem, ECCE-QM) can be duplicated with its final "
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
