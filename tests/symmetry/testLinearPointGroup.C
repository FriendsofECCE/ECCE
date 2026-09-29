// finitePointGroup(): every spelling a code reports for an infinite
// linear group must fold to the finite group autosym chooses, and
// nothing else may change.
#include <cstdio>
#include <string>
#include "tdat/LinearPointGroup.H"

static int failures = 0;

static void expect(const std::string& in, const std::string& want)
{
   std::string got = finitePointGroup(in);
   if (got != want) {
      std::printf("  FAIL  %-10s -> %s, expected %s\n",
                  in.c_str(), got.c_str(), want.c_str());
      failures++;
   }
}

int main()
{
   // Gaussian's "Full point group", ORCA's "Point Group:", autosym's
   // table names, the Unicode label the MO diagram draws.
   const char* cinf[] = { "C*V", "C*v", "c*v", "C(inf)v", "CINFV", "Cinfv",
                          "Coov", "C\xe2\x88\x9ev", " C*V " };
   const char* dinf[] = { "D*H", "D*h", "D(inf)h", "DINFH", "Dinfh",
                          "Dooh", "D\xe2\x88\x9eh" };
   for (const char* s : cinf) expect(s, "C4v");
   for (const char* s : dinf) expect(s, "D4h");

   const char* finite[] = { "C1", "C4v", "D4h", "C2v", "D2h", "Td", "Oh",
                            "Cs", "Ci", "C*", "" };
   for (const char* s : finite) expect(s, s);

   std::printf("  linear point groups: %s\n", failures ? "FAIL" : "PASS");
   return failures ? 1 : 0;
}
