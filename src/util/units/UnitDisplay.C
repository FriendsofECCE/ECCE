#include "util/UnitDisplay.H"

using std::string;

static void replaceAll(string& s, const string& from, const string& to)
{
  for (size_t p = s.find(from); p != string::npos; p = s.find(from, p + to.size()))
    s.replace(p, from.size(), to);
}

string unitLabel(const string& units)
{
  string s = units;
  replaceAll(s, "kJoule/Mole", "kJ/mol");
  replaceAll(s, "kcal/Mole", "kcal/mol");
  return s;
}
