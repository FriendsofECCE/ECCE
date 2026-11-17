#include "tdat/Counterpoise.H"
#include "tdat/Fragment.H"
#include "tdat/TAtm.H"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
static int failures = 0;

static void check(bool ok, const std::string& what)
{
   printf("%s: %s\n", ok ? "PASS" : "FAIL", what.c_str());
   if (!ok) ++failures;
}

static Fragment waterDimer()
{
   Fragment f;
   const char* symbols[] = {"O", "H", "H", "O", "H", "H"};
   const double xyz[][3] = {{0, 0, 0}, {0.9584, 0, 0}, {-0.2392, 0.9278, 0},
                           {2.85, 0, 0}, {3.0892, 0.9278, 0}, {3.0892, -0.9278, 0}};
   for (int i = 0; i < 6; ++i) f.addAtom(symbols[i], xyz[i]);
   f.addBond(0, 1, 1.0); f.addBond(0, 2, 1.0);
   f.addBond(3, 4, 1.0); f.addBond(3, 5, 1.0);
   return f;
}

static std::vector<std::string> geometry(const std::string& text)
{
   std::istringstream in(text);
   std::string line;
   std::vector<std::string> result;
   bool xyz = false;
   while (std::getline(in, line)) {
      if (line.find("* xyz") != std::string::npos) { xyz = true; continue; }
      if (!xyz) continue;
      std::istringstream fields(line);
      std::string symbol;
      fields >> symbol;
      if (symbol == "*") break;
      if (!symbol.empty()) result.push_back(symbol);
   }
   return result;
}

static bool sameDefinition(const Fragment& a, const Fragment& b)
{
   if (a.numAtoms() != b.numAtoms() || a.fragmentStates().size() != b.fragmentStates().size())
      return false;
   for (size_t i = 0; i < a.numAtoms(); ++i)
      if (a.atomRef(i)->fragmentId() != b.atomRef(i)->fragmentId()) return false;
   for (const auto& item : a.fragmentStates()) {
      auto found = b.fragmentStates().find(item.first);
      if (found == b.fragmentStates().end() || found->second.charge != item.second.charge ||
          found->second.multiplicity != item.second.multiplicity) return false;
   }
   return true;
}

int main(int argc, char** argv)
{
   std::string error;
   Fragment dimer = waterDimer();
   // A hydrogen bond must not join the two covalent molecules.
   dimer.addBond(1, 3, 0.1);
   check(Counterpoise::autoAssign(dimer, error), "water dimer auto assignment: " + error);
   check(dimer.fragmentStates().size() == 2 && dimer.atomRef(0)->fragmentId() == 1 &&
         dimer.atomRef(2)->fragmentId() == 1 && dimer.atomRef(3)->fragmentId() == 2 &&
         dimer.atomRef(5)->fragmentId() == 2, "water dimer has two ordered fragments");
   check(Counterpoise::validate(dimer, error), "neutral water fragment states validate: " + error);

   const std::vector<std::string> expected[] = {
      {"O", "H", "H", "O", "H", "H"},
      {"O", "H", "H", "O:", "H:", "H:"},
      {"O:", "H:", "H:", "O", "H", "H"}};
   for (int active = 0; active < 3; ++active) {
      std::ostringstream out;
      check(Counterpoise::writeOrcaXyz(out, dimer, active, 1, error),
            "ORCA xyz writes system " + std::to_string(active) + ": " + error);
      check(out.str().find("* xyz 0 1") != std::string::npos && geometry(out.str()) == expected[active],
            "ORCA xyz preserves six centres, charge/spin and ghost selection for system " + std::to_string(active));
   }

   Fragment before = dimer;
   check(!Counterpoise::setState(dimer, 1, 2, 1, error) && error.find("sum") != std::string::npos && sameDefinition(dimer, before),
         "fragment charge sum mismatch is refused without changing the model");
   check(!Counterpoise::setState(dimer, 1, 0, 0, error) && sameDefinition(dimer, before),
         "nonpositive multiplicity is refused without changing the model");
   check(!Counterpoise::assignAtoms(dimer, 3, {6}, error) && sameDefinition(dimer, before),
         "out-of-range atom override is refused without changing the model");
   check(Counterpoise::setState(dimer, 2, 0, 3, error), "a water-fragment triplet can be specified explicitly");
   check(Counterpoise::assignAtoms(dimer, 3, {1}, error) && dimer.atomRef(1)->fragmentId() == 3 &&
         dimer.fragmentStates().at(2).multiplicity == 3,
         "per-atom override moves the selected atom to its fragment");
   check(dimer.fragmentStates().at(2).multiplicity == 3, "atom override preserves an unaffected fragment's explicit triplet");
   check(Counterpoise::autoAssign(dimer, error) && sameDefinition(dimer, before),
         "auto assignment restores connected molecules after an override");

   Fragment ion;
   const double na[] = {0, 0, 0}, o[] = {3.2, 0, 0}, h1[] = {3.44, 0.93, 0}, h2[] = {3.44, -0.93, 0};
   ion.addAtom("Na", na); ion.addAtom("O", o); ion.addAtom("H", h1); ion.addAtom("H", h2);
   ion.addBond(1, 2, 1.0); ion.addBond(1, 3, 1.0);
   ion.charge(1);
   check(Counterpoise::autoAssign(ion, error) && ion.fragmentStates().size() == 2,
         "Na+(H2O) contains an ion fragment and a water fragment");
   check(!Counterpoise::validate(ion, error), "an ionic complex needs explicit fragment charges");
   check(Counterpoise::setState(ion, 1, 1, 1, error) && Counterpoise::validate(ion, error),
         "Na +1 and water 0 agree with total charge +1: " + error);
   Fragment ionBefore(ion);
   check(!Counterpoise::assignAtoms(ion, 2, {0}, error) && sameDefinition(ion, ionBefore),
         "merging away a charged fragment is refused without changing assignments");
   std::ostringstream sodium, water;
   check(Counterpoise::writeOrcaXyz(sodium, ion, 1, 1, error) &&
         sodium.str().find("* xyz 1 1") != std::string::npos &&
         geometry(sodium.str()) == std::vector<std::string>({"Na", "O:", "H:", "H:"}),
         "Na monomer uses charge +1, singlet and ghost water");
   check(Counterpoise::writeOrcaXyz(water, ion, 2, 1, error) &&
         water.str().find("* xyz 0 1") != std::string::npos,
         "water monomer in the ionic complex uses charge 0 and singlet");

   Fragment copy(ion), assigned;
   assigned = ion;
   check(sameDefinition(copy, ion) && sameDefinition(assigned, ion), "fragment definitions survive copy and assignment");
   std::stringstream mvm;
   check(ion.dumpMVM(mvm), "ionic fragment structure serializes");
   Fragment restored;
   check(restored.restoreMVM(mvm, 1.0, false) && sameDefinition(restored, ion) &&
         restored.charge() == 1 && Counterpoise::validate(restored, error),
         "MVM roundtrip retains atom IDs and fragment charge/multiplicity");
   std::stringstream firstMvm, secondMvm;
   dimer.dumpMVM(firstMvm); dimer.dumpMVM(secondMvm);
   Fragment appended;
   check(appended.restoreMVM(firstMvm, 1.0, false) && appended.restoreMVM(secondMvm, 1.0, false) &&
         appended.numAtoms() == 12 && appended.fragmentStates().size() == 4 &&
         appended.atomRef(6)->fragmentId() == 3 && appended.atomRef(9)->fragmentId() == 4 &&
         Counterpoise::validate(appended, error),
         "appending MVM structures remaps fragment IDs without collisions");
   Fragment merged(dimer);
   merged.merge(dimer);
   check(merged.numAtoms() == 12 && merged.atomRef(0)->fragmentId() == 1 &&
         merged.atomRef(6)->fragmentId() == 0 && !Counterpoise::validate(merged, error),
         "structural merge requires reassignment of incoming atom fragment IDs");
   copy.clear();
   check(copy.fragmentStates().empty(), "clearing a structure clears fragment states");

   const bool keepInputs = argc == 3 && std::string(argv[1]) == "--write-inputs";
   const fs::path folder = keepInputs ? fs::path(argv[2]) :
      fs::current_path() / ("counterpoise-test-" + std::to_string(
         std::chrono::steady_clock::now().time_since_epoch().count()));
   check(!Counterpoise::writeOrcaInputs(dimer, folder.string(), "HF def2-SVP", 2, error) &&
         !fs::exists(folder), "invalid total multiplicity is refused before creating files");
   check(Counterpoise::writeOrcaInputs(dimer, folder.string(), "HF def2-SVP", 1, error),
         "N+1 ORCA single-point inputs written: " + error);
   for (int i = 0; i < 3; ++i) {
      const std::string filename = i == 0 ? "complex.inp" : "fragment-" + std::to_string(i) + ".inp";
      std::ifstream file(folder / filename);
      std::stringstream text;
      text << file.rdbuf();
      check(text.str().find("! HF def2-SVP") != std::string::npos && geometry(text.str()) == expected[i],
            filename + " has the requested method and ghost centres");
   }
   check(!Counterpoise::writeOrcaInputs(dimer, folder.string(), "HF def2-SVP", 1, error),
         "existing counterpoise inputs cannot be overwritten");
   if (!keepInputs) fs::remove_all(folder);
   return failures ? 1 : 0;
}
