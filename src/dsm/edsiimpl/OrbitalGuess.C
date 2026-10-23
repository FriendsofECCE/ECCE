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
          "orbitals (ORCA, Gaussian 16) can be duplicated with its final "
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
