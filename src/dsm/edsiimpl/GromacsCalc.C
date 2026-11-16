#include <cstdio>
#include <sstream>
#include <vector>

#include "dsm/GromacsCalc.H"
#include "dsm/ChemistryTask.H"
#include "dsm/EDSI.H"
#include "dsm/EDSIFactory.H"
#include "dsm/JCode.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/TaskJob.H"
#include "tdat/Fragment.H"
#include "tdat/TAtm.H"

using std::string;
using std::vector;

namespace {

bool readUrl(const EcceURL& url, string& text)
{
  EDSI* edsi = EDSIFactory::getEDSI(url);
  if (!edsi) return false;
  std::ostringstream os;
  bool ok = edsi->getDataSet(os);
  delete edsi;
  text = os.str();
  return ok && !text.empty();
}

// Stored as the Inputs tab stores an attached file: same name replaces.
bool putInput(TaskJob* run, const string& name, const string& text)
{
  std::istringstream in(text);
  run->putInputFile(name, 0);
  return run->putInputFile(name, &in);
}

bool isOutput(const EcceURL& u)
{
  return u.toString().find("/Outputs/") != string::npos;
}

// The latest .gro of the run among its inputs or among its outputs.
EcceURL latestGro(TaskJob* run, bool outputs)
{
  vector<EcceURL> all = run->getDataFiles(JCode::RESTART_OUTPUT), pick;
  for (size_t i = 0; i < all.size(); i++)
    if (isOutput(all[i]) == outputs) pick.push_back(all[i]);
  return pick.empty() ? EcceURL() : run->getLatestDataFile(pick);
}

Fragment* groFragment(const string& text)
{
  if (text.empty()) return 0;
  std::istringstream in(text);
  Fragment* f = new Fragment;
  if (f->restoreGRO(in, true) && f->numAtoms() > 0) return f;
  delete f;
  return 0;
}

}  // namespace

namespace GromacsCalc {

bool isRun(const Resource* r)
{
  return r && r->getContentType() == ResourceDescriptor::CT_MDRUN;
}

EcceURL startGro(TaskJob* run)
{
  return run ? latestGro(run, false) : EcceURL();
}

string finalGro(TaskJob* run)
{
  string text;
  if (!run) return text;
  EcceURL u = latestGro(run, true);
  if (u.toString().empty() || !readUrl(u, text)) text.clear();
  return text;
}

Fragment* finalFragment(TaskJob* run)
{
  if (!run) return 0;
  if (Fragment* f = groFragment(finalGro(run))) return f;
  string text;
  EcceURL u = startGro(run);
  if (!u.toString().empty() && readUrl(u, text))
    if (Fragment* f = groFragment(text)) return f;
  ChemistryTask* ct = dynamic_cast<ChemistryTask*>(run);
  return ct ? ct->fragment() : 0;
}

bool replaceCoordinates(const string& gro, const Fragment& frag, string& out,
                        string& why)
{
  Fragment* old = groFragment(gro);
  if (!old) {
    why = "the run's .gro file cannot be read";
    return false;
  }
  const int n = old->numAtoms();
  Fragment& f = const_cast<Fragment&>(frag);
  if ((int)f.numAtoms() != n) {
    why = "the structure has " + std::to_string(f.numAtoms()) +
          " atoms and the run's .gro " + std::to_string(n) +
          "; the coordinates can only replace those of the same atoms in "
          "the same order";
    delete old;
    return false;
  }
  for (int i = 0; i < n; i++) {
    const string a = old->atomRef(i)->atomicSymbol();
    const string b = f.atomRef(i)->atomicSymbol();
    // "X": a .gro atom name the element rules do not know; trust the order.
    if (a != "X" && a != b) {
      why = "atom " + std::to_string(i + 1) + " is " + a +
            " in the run's .gro and " + b + " in the structure; the atoms "
            "must be the same and in the same order";
      delete old;
      return false;
    }
  }
  delete old;

  std::istringstream in(gro);
  std::ostringstream os;
  string line;
  std::getline(in, line);
  os << line << "\n";
  std::getline(in, line);
  os << line << "\n";
  for (int i = 0; i < n; i++) {
    std::getline(in, line);
    // Same field width and precision as the file (restoreGRO's rule).
    size_t d1 = line.find('.', 20);
    size_t d2 = line.find('.', d1 + 1);
    const int w = (int)(d2 - d1);
    const int prec = w > 5 ? w - 5 : 3;
    const double* xyz = f.atomRef(i)->coordinates();
    char buf[64];
    os << line.substr(0, 20);
    for (int k = 0; k < 3; k++) {
      snprintf(buf, sizeof buf, "%*.*f", w, prec, xyz[k] / 10.0);
      os << buf;
    }
    os << "\n";
  }
  if (std::getline(in, line)) os << line << "\n";
  out = os.str();
  return true;
}

bool setStructure(TaskJob* run, const Fragment& frag, string& why)
{
  EcceURL u = startGro(run);
  string gro, made;
  if (u.toString().empty() || !readUrl(u, gro)) {
    why = "the GROMACS calculation has no .gro file to take the coordinates "
          "(a run made from a ready .tpr cannot take a new structure)";
    return false;
  }
  if (!replaceCoordinates(gro, frag, made, why)) return false;
  if (!putInput(run, u.getFilePathTail(), made)) {
    why = "the new .gro could not be stored";
    return false;
  }
  if (ChemistryTask* ct = dynamic_cast<ChemistryTask*>(run))
    if (Fragment* f = groFragment(made)) {
      ct->fragment(f);
      delete f;
    }
  return true;
}

bool continueFromFinal(TaskJob* copy, string& why)
{
  EcceURL u = startGro(copy);
  if (u.toString().empty()) {
    why = "a run made from a ready .tpr has no .gro to take the final "
          "frame; the copy runs the same .tpr again";
    return false;
  }
  const string last = finalGro(copy);
  if (last.empty()) {
    why = "the run wrote no final frame; the copy keeps the starting "
          "structure";
    return false;
  }
  if (!putInput(copy, u.getFilePathTail(), last)) {
    why = "the final frame could not be stored";
    return false;
  }
  if (ChemistryTask* ct = dynamic_cast<ChemistryTask*>(copy))
    if (Fragment* f = groFragment(last)) {
      ct->fragment(f);
      delete f;
    }
  return true;
}

}  // namespace GromacsCalc
