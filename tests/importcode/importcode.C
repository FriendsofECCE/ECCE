// Import Calculation from Output File: checks the code JobParser::importCode
// picks for each output file, with the EDML descriptors of ECCE_HOME.
// Usage: importcode [-v] <file>=<code>...   ("-" for code: must be refused)
#include <cstring>
#include <iostream>
#include <string>

#include "dsm/CodeFactory.H"
#include "dsm/JCode.H"
#include "comm/JobParser.H"

using namespace std;

int main(int argc, char **argv)
{
  int first = 1, failures = 0;
  if (argc > 1 && strcmp(argv[1], "-v") == 0) {
    first = 2;
    for (const string& c : CodeFactory::getImportCodes()) {
      const JCode *j = CodeFactory::lookup(c.c_str());
      string importer;
      j->get_string("Importer", importer);
      cout << "import code " << c << ": importer '" << importer
           << "', pattern '" << j->getParseVerifyPattern() << "'" << endl;
    }
  }
  for (int i = first; i < argc; i++) {
    string arg = argv[i];
    size_t eq = arg.rfind('=');
    string file = arg.substr(0, eq);
    string want = eq == string::npos ? "" : arg.substr(eq + 1);
    string message;
    string code = JobParser::importCode(file, message);
    string got = code.empty() ? "-" : code;
    bool ok = want.empty() || got == want;
    if (!ok) failures++;
    cout << (ok ? "ok   " : "FAIL ") << file << ": "
         << (code.empty() ? "refused: " + message : code)
         << (ok || want.empty() ? "" : " (expected " + want + ")") << endl;
  }
  return failures ? 1 : 0;
}
