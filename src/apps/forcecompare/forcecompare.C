#include <filesystem>
#include <iostream>
#include <string>

#include "CompareForces.H"
#include "dsm/EDSIServerCentral.H"

int main(int argc, char **argv)
{
  const bool json = argc == 4 && std::string(argv[3]) == "--json";
  if (argc != 3 && !json) {
    std::cerr << "usage: ecce-force-compare A B [--json]\n";
    return 2;
  }
  try {
    EDSIServerCentral central;
    std::string urls[2];
    for (int i = 0; i < 2; ++i) {
      urls[i] = argv[i+1];
      if (urls[i].find("://") == std::string::npos)
        urls[i] = "file://" + std::filesystem::absolute(urls[i]).generic_string();
    }
    const ForceComparison r = compareForces(gatherForceInput(urls[0]), gatherForceInput(urls[1]));
    std::cout << (json ? forceComparisonJson(r) : forceComparisonText(r));
    return r.ok ? 0 : 1;
  } catch (const std::exception& e) {
    std::cerr << "ecce-force-compare: " << e.what() << '\n';
    return 1;
  }
}
