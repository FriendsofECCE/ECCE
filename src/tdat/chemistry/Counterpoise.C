#include "tdat/Counterpoise.H"
#include "tdat/Fragment.H"
#include "tdat/TAtm.H"
#include "tdat/TBond.H"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>

namespace Counterpoise {
namespace {

bool realCentre(const TAtm& atom)
{
  return !atom.isNub() && atom.atomicNumber() > 0;
}

long long electrons(const Fragment& fragment, int id)
{
  long long count = 0;
  for (size_t i = 0; i < fragment.numAtoms(); ++i) {
    const TAtm& atom = *fragment.atomRef(i);
    if ((id == 0 || atom.fragmentId() == id) && realCentre(atom) &&
        atom.getBehaviorType() != TAtm::BQX)
      count += atom.atomicNumber();
  }
  return count;
}

bool spinValid(long long count, int multiplicity)
{
  return count >= 0 && multiplicity > 0 && multiplicity - 1 <= count &&
         (count - (multiplicity - 1)) % 2 == 0;
}

int defaultMultiplicity(const Fragment& fragment, int id, int charge)
{
  return (electrons(fragment, id) - charge) % 2 == 0 ? 1 : 2;
}

bool assignments(const Fragment& fragment, std::set<int>& ids, std::string& error)
{
  if (fragment.numAtoms() == 0) {
    error = "No atoms to assign to fragments";
    return false;
  }
  for (size_t i = 0; i < fragment.numAtoms(); ++i) {
    const TAtm& atom = *fragment.atomRef(i);
    if (!realCentre(atom)) continue;
    if (atom.fragmentId() <= 0) {
      error = "Atom " + std::to_string(i + 1) + " has no fragment; use fragments auto";
      return false;
    }
    ids.insert(atom.fragmentId());
  }
  if (ids.empty()) {
    error = "No real atoms to assign to fragments";
    return false;
  }
  return true;
}

bool supportedCentres(const Fragment& fragment, std::string& error)
{
  for (size_t i = 0; i < fragment.numAtoms(); ++i) {
    const TAtm& atom = *fragment.atomRef(i);
    if (!realCentre(atom) || atom.getBehaviorType() == TAtm::BQ ||
        atom.getBehaviorType() == TAtm::POINT) {
      error = "Counterpoise export requires element atoms without nubs, dummies or point charges";
      return false;
    }
    const double* xyz = atom.coordinates();
    if (!std::isfinite(xyz[0]) || !std::isfinite(xyz[1]) || !std::isfinite(xyz[2])) {
      error = "Counterpoise export requires finite coordinates";
      return false;
    }
  }
  return true;
}

}

bool autoAssign(Fragment& fragment, std::string& error)
{
  error.clear();
  if (fragment.numAtoms() == 0) { error = "No atoms to assign to fragments"; return false; }
  std::vector<int> ids(fragment.numAtoms(), 0);
  int id = 0;
  for (size_t start = 0; start < fragment.numAtoms(); ++start) {
    if (!realCentre(*fragment.atomRef(start)) || ids[start]) continue;
    ++id;
    std::vector<int> pending{static_cast<int>(start)};
    ids[start] = id;
    while (!pending.empty()) {
      int index = pending.back();
      pending.pop_back();
      TAtm* atom = fragment.atomRef(index);
      for (const TBond* bond : atom->bondList()) {
        if (bond->order() < 1.0) continue;
        TAtm* other = bond->atom1() == atom ? bond->atom2() : bond->atom1();
        int next = fragment.atomIndex(*other);
        if (next >= 0 && static_cast<size_t>(next) < ids.size() &&
            !ids[next] && realCentre(*other)) {
          ids[next] = id;
          pending.push_back(next);
        }
      }
    }
  }
  if (!id) { error = "No real atoms to assign to fragments"; return false; }
  for (size_t i = 0; i < ids.size(); ++i) fragment.atomRef(i)->fragmentId(ids[i]);
  States states;
  for (int n = 1; n <= id; ++n)
    states[n] = {0, defaultMultiplicity(fragment, n, 0)};
  fragment.fragmentStates(states);
  return true;
}

bool assignAtoms(Fragment& fragment, int id, const std::vector<int>& atoms,
                 std::string& error)
{
  error.clear();
  if (id <= 0 || atoms.empty()) { error = "Fragment ID and atom list must be positive and nonempty"; return false; }
  std::set<int> unique;
  for (int atom : atoms) {
    if (atom < 0 || static_cast<size_t>(atom) >= fragment.numAtoms() ||
        !realCentre(*fragment.atomRef(atom)) || !unique.insert(atom).second) {
      error = "Fragment atom list contains an invalid or repeated atom";
      return false;
    }
  }
  std::set<int> oldIds;
  if (!assignments(fragment, oldIds, error)) return false;
  States states = fragment.fragmentStates();
  long long oldCharge = 0;
  for (const auto& state : states) oldCharge += state.second.charge;
  std::vector<int> proposed;
  for (size_t i = 0; i < fragment.numAtoms(); ++i)
    proposed.push_back(fragment.atomRef(i)->fragmentId());
  for (int atom : atoms) proposed[atom] = id;
  std::set<int> ids;
  for (size_t i = 0; i < proposed.size(); ++i)
    if (realCentre(*fragment.atomRef(i))) ids.insert(proposed[i]);
  for (auto it = states.begin(); it != states.end();) {
    if (!ids.count(it->first)) it = states.erase(it);
    else ++it;
  }
  for (int n : ids) {
    if (states.count(n)) continue;
    long long count = 0;
    for (size_t i = 0; i < proposed.size(); ++i) {
      const TAtm& atom = *fragment.atomRef(i);
      if (proposed[i] == n && atom.getBehaviorType() != TAtm::BQX)
        count += atom.atomicNumber();
    }
    states[n] = {0, count % 2 == 0 ? 1 : 2};
  }
  long long newCharge = 0;
  for (const auto& state : states) newCharge += state.second.charge;
  if (oldCharge == fragment.charge() && newCharge != fragment.charge()) {
    error = "Moving these atoms would change the sum of fragment charges";
    return false;
  }
  for (size_t i = 0; i < proposed.size(); ++i)
    fragment.atomRef(i)->fragmentId(proposed[i]);
  fragment.fragmentStates(states);
  return true;
}

bool setState(Fragment& fragment, int id, int charge, int multiplicity,
              std::string& error)
{
  error.clear();
  std::set<int> ids;
  if (!assignments(fragment, ids, error)) return false;
  if (!ids.count(id)) { error = "Unknown fragment " + std::to_string(id); return false; }
  if (!spinValid(electrons(fragment, id) - charge, multiplicity)) {
    error = "Fragment charge/multiplicity is incompatible with its electron count";
    return false;
  }
  States states = fragment.fragmentStates();
  states[id] = {charge, multiplicity};
  long long total = 0;
  for (int n : ids) {
    if (!states.count(n)) { error = "Missing fragment charge/multiplicity"; return false; }
    total += states[n].charge;
  }
  if (total != fragment.charge()) {
    error = "Fragment charges sum to " + std::to_string(total) +
            "; total structure charge is " + std::to_string(fragment.charge());
    return false;
  }
  fragment.fragmentStates(states);
  return true;
}

bool validate(const Fragment& fragment, std::string& error)
{
  error.clear();
  std::set<int> ids;
  if (!assignments(fragment, ids, error)) return false;
  const States& states = fragment.fragmentStates();
  if (states.size() != ids.size()) { error = "Fragment states do not match the atom assignments; use fragments auto"; return false; }
  long long total = 0;
  for (int id : ids) {
    auto found = states.find(id);
    if (found == states.end()) { error = "Missing state for fragment " + std::to_string(id); return false; }
    const State& state = found->second;
    if (!spinValid(electrons(fragment, id) - state.charge, state.multiplicity)) {
      error = "Charge/multiplicity is incompatible with electron count for fragment " + std::to_string(id);
      return false;
    }
    total += state.charge;
  }
  if (total != fragment.charge()) {
    error = "Fragment charges sum to " + std::to_string(total) +
            "; total structure charge is " + std::to_string(fragment.charge());
    return false;
  }
  return true;
}

std::string query(const Fragment& fragment)
{
  std::ostringstream out;
  std::set<int> ids;
  for (size_t i = 0; i < fragment.numAtoms(); ++i)
    if (fragment.atomRef(i)->fragmentId() > 0) ids.insert(fragment.atomRef(i)->fragmentId());
  for (int id : ids) {
    out << "fragment " << id << " atoms";
    for (size_t i = 0; i < fragment.numAtoms(); ++i)
      if (fragment.atomRef(i)->fragmentId() == id) out << " " << i + 1;
    auto found = fragment.fragmentStates().find(id);
    if (found != fragment.fragmentStates().end())
      out << " charge " << found->second.charge << " mult " << found->second.multiplicity;
    else out << " state missing";
    out << "\n";
  }
  std::string error;
  if (!validate(fragment, error)) out << "invalid: " << error << "\n";
  return out.str();
}

bool writeOrcaXyz(std::ostream& out, const Fragment& fragment, int activeFragment,
                  int totalMultiplicity, std::string& error)
{
  if (!validate(fragment, error) || !supportedCentres(fragment, error)) return false;
  if (activeFragment < 0 || (activeFragment && !fragment.fragmentStates().count(activeFragment))) {
    error = "Unknown active fragment";
    return false;
  }
  const State state = activeFragment ? fragment.fragmentStates().at(activeFragment)
                                     : State{fragment.charge(), totalMultiplicity};
  if (!spinValid(electrons(fragment, activeFragment) - state.charge, state.multiplicity)) {
    error = "Total charge/multiplicity is incompatible with its electron count";
    return false;
  }
  std::ostringstream block;
  block << "* xyz " << state.charge << " " << state.multiplicity << "\n";
  block << std::setprecision(15);
  for (size_t i = 0; i < fragment.numAtoms(); ++i) {
    const TAtm& atom = *fragment.atomRef(i);
    const bool ghost = atom.getBehaviorType() == TAtm::BQX ||
                       (activeFragment && atom.fragmentId() != activeFragment);
    const double* xyz = atom.coordinates();
    block << atom.atomicSymbol() << (ghost ? ":" : "") << " "
          << xyz[0] << " " << xyz[1] << " " << xyz[2] << "\n";
  }
  block << "*\n";
  out << block.str();
  if (!out) { error = "Could not write ORCA coordinates"; return false; }
  return true;
}

bool writeOrcaInputs(const Fragment& fragment, const std::string& folder,
                     const std::string& route, int totalMultiplicity,
                     std::string& error)
{
  error.clear();
  if (!validate(fragment, error) || !supportedCentres(fragment, error)) return false;
  if (fragment.fragmentStates().size() < 2) { error = "Counterpoise needs at least two fragments"; return false; }
  if (folder.empty() || route.empty() || route.find_first_of("\r\n*!%") != std::string::npos) {
    error = "Provide an output folder and a single ORCA single-point keyword line without !";
    return false;
  }
  std::istringstream words(route);
  std::string word;
  while (words >> word) {
    std::transform(word.begin(), word.end(), word.begin(), [](unsigned char c) { return std::tolower(c); });
    if (word.find("opt") != std::string::npos || word.find("freq") != std::string::npos ||
        word == "irc" || word == "engrad" || word == "numgrad" || word == "md" ||
        word == "goat" || word.find("scan") != std::string::npos || word.find("neb") != std::string::npos) {
      error = "Counterpoise export supports single-point energies only";
      return false;
    }
  }
  std::map<std::string, std::string> inputs;
  std::vector<int> ids{0};
  for (const auto& state : fragment.fragmentStates()) ids.push_back(state.first);
  for (int id : ids) {
    std::ostringstream input;
    input << "! " << route << "\n\n";
    if (!writeOrcaXyz(input, fragment, id, totalMultiplicity, error)) return false;
    inputs[id ? "fragment-" + std::to_string(id) + ".inp" : "complex.inp"] = input.str();
  }
  namespace fs = std::filesystem;
  std::error_code ec;
  const fs::path directory(folder);
  if (fs::exists(directory, ec)) {
    if (!fs::is_directory(directory, ec)) { error = "Output folder is not a directory"; return false; }
  } else if (!fs::create_directories(directory, ec)) {
    error = "Could not create output folder: " + ec.message();
    return false;
  }
  for (const auto& input : inputs) {
    if (fs::exists(directory / input.first, ec) || ec) {
      error = "Refusing to overwrite " + input.first;
      return false;
    }
  }
  for (const auto& input : inputs) {
    std::ofstream out(directory / input.first);
    out << input.second;
    out.close();
    if (!out) { error = "Could not write " + input.first; return false; }
  }
  return true;
}

}
