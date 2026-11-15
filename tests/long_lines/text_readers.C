// Readers that once used a fixed char buffer stopped at the first line
// longer than the buffer.  Each reader must give the same result for a
// line with 2000 characters of trailing spaces or comment as for the
// short line.
#include <iostream>
#include <sstream>
#include <string>
#include "tdat/Fragment.H"
#include "tdat/GUIValues.H"
#include "util/KeyValueReader.H"
#include "util/LineReader.H"
#include "util/TupleReader.H"

using std::string;

namespace {

int fail(const char* what)
{
  std::cerr << "FAIL " << what << "\n";
  return 1;
}

// PDB: an ATOM line with a long tail, read by restorePDB (1024-byte buffer
// before).  The tail is spaces; the two atoms must come out the same.
bool pdbDump(const string& tail, string& dump, size_t& natoms)
{
  const string atom1 = "ATOM      1  C   ALA A   1    1.000   2.000   3.000  1.00  0.00           C";
  const string atom2 = "ATOM      2  O   ALA A   1    4.000   5.000   6.000  1.00  0.00           O";
  std::istringstream in(atom1 + tail + "\n" + atom2 + "\nEND\n");
  Fragment frag;
  if (!frag.restorePDB(in, 1.0, false)) return false;
  natoms = frag.numAtoms();
  std::ostringstream out;
  frag.dumpPDB(out, false);
  dump = out.str();
  return true;
}

}  // namespace

int main()
{
  const string pad(2000, ' ');
  const string comment = "# " + string(2000, 'x');

  // PDB reader.
  string shortDump, longDump;
  size_t shortAtoms = 0, longAtoms = 0;
  if (!pdbDump("", shortDump, shortAtoms) || shortAtoms != 2) return fail("PDB short line");
  if (!pdbDump(pad, longDump, longAtoms) || longAtoms != 2 || longDump != shortDump)
    return fail("PDB long line");

  // GUIValues: a value line over MAXLINE, then a line that must still load.
  GUIValues shortGui;
  shortGui = string("ES.Theory.Note|42|Gigabytes|1|1|integer_input\n");
  GUIValues longGui;
  longGui = string("ES.Theory.Note|42|Gigabytes|1|1|integer_input" + pad + "\n" +
                   "# " + string(5000, 'x') + "\n" +
                   "ES.Theory.Two|7|Gigabytes|1|1|integer_input\n");
  if (shortGui.find("ES.Theory.Note") == shortGui.end() ||
      shortGui["ES.Theory.Note"]->getValueAsString() != "42")
    return fail("GUIValues short line");
  if (longGui.find("ES.Theory.Note") == longGui.end() ||
      longGui["ES.Theory.Note"]->getValueAsString() != "42")
    return fail("GUIValues long line");
  if (longGui.find("ES.Theory.Two") == longGui.end() ||
      longGui["ES.Theory.Two"]->getValueAsString() != "7")
    return fail("GUIValues line after a long line");

  // KeyValueReader, TupleReader and LineReader: short and long lines.
  auto keyValue = [](const string& text, string& out) {
    std::istringstream in(text);
    KeyValueReader reader(in);
    reader.stripTrailingComments(true);
    string key, value;
    if (!reader.getpair(key, value)) return false;
    out = key + "=" + value;
    return true;
  };
  string kvShort, kvLong;
  if (!keyValue("name:value\n", kvShort) || kvShort != "name=value")
    return fail("KeyValueReader short line");
  if (!keyValue("name:value" + pad + comment + "\n", kvLong) || kvLong != kvShort)
    return fail("KeyValueReader long line");

  auto tuple = [](const string& text, string& out) {
    std::istringstream in(text);
    TupleReader reader(in);
    string one, two, three;
    if (!reader.gettuple(one, two, three)) return false;
    out = one + "|" + two + "|" + three;
    return true;
  };
  string tupShort, tupLong;
  if (!tuple("a:b:c\n", tupShort) || tupShort != "a|b|c") return fail("TupleReader short line");
  if (!tuple("a:b:c" + pad + "\n", tupLong) || tupLong != tupShort)
    return fail("TupleReader long line");

  auto lines = [](const string& text, string& out) {
    std::istringstream in(text);
    LineReader reader(in);
    reader.stripTrailingComments(true);
    string line;
    if (!reader.getLine(line)) return false;
    out = line;
    return true;
  };
  string lineShort, lineLong;
  if (!lines("alpha beta\n", lineShort) || lineShort.empty()) return fail("LineReader short line");
  if (!lines("alpha beta" + pad + comment + "\n", lineLong) || lineLong != lineShort)
    return fail("LineReader long line");

  std::cout << "PASS PDB, GUIValues, KeyValueReader, TupleReader and LineReader long lines\n";
  return 0;
}
