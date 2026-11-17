#ifndef COUNTERPOISE_H
#define COUNTERPOISE_H

#include <iosfwd>
#include <map>
#include <string>
#include <vector>

class Fragment;

namespace Counterpoise {

struct State {
  int charge = 0;
  int multiplicity = 1;
};
using States = std::map<int, State>;

bool autoAssign(Fragment& fragment, std::string& error);
bool assignAtoms(Fragment& fragment, int id, const std::vector<int>& atoms,
                 std::string& error);
bool setState(Fragment& fragment, int id, int charge, int multiplicity,
              std::string& error);
bool validate(const Fragment& fragment, std::string& error);
std::string query(const Fragment& fragment);

// activeFragment == 0 writes the complex; other IDs retain only that nucleus set.
bool writeOrcaXyz(std::ostream& out, const Fragment& fragment, int activeFragment,
                  int totalMultiplicity, std::string& error);
bool writeOrcaInputs(const Fragment& fragment, const std::string& folder,
                     const std::string& route, int totalMultiplicity,
                     std::string& error);

}

#endif
