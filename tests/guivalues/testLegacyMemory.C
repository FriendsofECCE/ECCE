// Drives GUIValues::load() with memory sizes saved before every dialog's
// memory field became GB, and prints what a generator would be handed
// (dumpKeyVals) and what a dialog would be handed (dump).  run_tests.py
// checks the output.
#include <iostream>
#include <string>
#include "tdat/GUIValues.H"

int main()
{
  GUIValues values;
  values = std::string(
    "ES.Theory.SCF.Memory|1||1|0|toggle_input\n"
    "ES.Theory.SCF.MemorySize|1800|Megawords|1|1|integer_input\n"
    "ES.Theory.MemorySize|40|Megawords|1|1|integer_input\n"
    "ES.Theory.SCF.DiskSize|64|Megawords|1|1|integer_input\n");
  std::cout << "converted " << values.convertedLegacyUnits() << "\n";
  values.dump(std::cout);
  std::cout << "--keyvals\n";
  values.dumpKeyVals(std::cout);

  GUIValues orca;
  orca = std::string(
    "ES.Theory.SCF.MemorySize|1000|Megabytes / core|1|1|integer_input\n");
  std::cout << "--orca converted " << orca.convertedLegacyUnits() << "\n";
  orca.dump(std::cout);

  GUIValues current;
  current = std::string(
    "ES.Theory.SCF.MemorySize|6|Gigabytes|1|1|integer_input\n");
  std::cout << "--current converted " << current.convertedLegacyUnits()
            << "\n";
  current.dump(std::cout);

  GUIValues copy(values);
  std::cout << "--copy converted " << copy.convertedLegacyUnits() << "\n";
  return 0;
}
