#include "util/UnitDisplay.H"

#include <cstdio>

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
  if (s == "Kelvin") s = "K";
  return s;
}


string valueLabel(double value, int decimals)
{
  char buf[64];
  snprintf(buf, sizeof buf, "%.*f", decimals, value);
  string s = buf;
  const size_t dot = s.find('.');
  if (dot == string::npos) return s;
  size_t end = s.size();
  while (end > dot + 2 && s[end - 1] == '0') --end;
  return s.substr(0, end);
}
