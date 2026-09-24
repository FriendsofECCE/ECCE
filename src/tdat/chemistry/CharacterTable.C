#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include "tdat/CharacterTable.H"

using std::ifstream;
using std::istringstream;
using std::ostringstream;

map<string, CharacterTable> CharacterTable::p_tables;
bool CharacterTable::p_loaded = false;

namespace {

  string upcase(const string& s)
  {
    string out(s);
    for (size_t i = 0; i < out.size(); i++) out[i] = toupper(out[i]);
    return out;
  }

  string trim(const string& s)
  {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e-b+1);
  }
}


CharacterTable::CharacterTable()
  : p_order(0)
{
}


vector<string> CharacterTable::names(void)
{
  initialize();
  vector<string> out;
  for (map<string, CharacterTable>::const_iterator it = p_tables.begin();
       it != p_tables.end(); ++it) {
    out.push_back(it->first);
  }
  return out;
}


int CharacterTable::loadFile(const string& path)
{
  p_tables.clear();
  p_loaded = true;

  ifstream in(path.c_str());
  if (!in) return 0;

  CharacterTable current;
  bool haveCurrent = false;

  string raw;
  while (getline(in, raw)) {

    //  Comments run to end of line, and a line that is only a comment
    //  disappears entirely.
    const size_t hash = raw.find('#');
    if (hash != string::npos) raw = raw.substr(0, hash);

    const string line = trim(raw);
    if (line.empty()) continue;

    if (line[0] == '[') {
      if (haveCurrent) p_tables[upcase(current.p_name)] = current;

      current = CharacterTable();
      haveCurrent = true;

      const size_t close = line.find(']');
      if (close == string::npos) { haveCurrent = false; continue; }
      current.p_name = line.substr(1, close-1);

      //  "h=<order>" follows the bracket.
      const size_t eq = line.find('=', close);
      if (eq != string::npos) {
        current.p_order = atoi(line.substr(eq+1).c_str());
      }
      continue;
    }

    if (!haveCurrent) continue;

    const size_t colon = line.find(':');
    if (colon == string::npos) continue;

    const string key = trim(line.substr(0, colon));
    istringstream fields(line.substr(colon+1));

    if (key == "classes") {
      string token;
      while (fields >> token) current.p_classes.push_back(token);

    } else if (key == "counts") {
      int value;
      while (fields >> value) current.p_counts.push_back(value);

    } else if (key == "translations") {
      //  Recorded in the file as a check on the labelling; nothing here
      //  reads it, and tests/symmetry is what enforces it.
      continue;

    } else {
      vector<double> row;
      double value;
      while (fields >> value) row.push_back(value);
      if (!row.empty()) {
        current.p_irreps.push_back(key);
        current.p_characters[key] = row;
      }
    }
  }

  if (haveCurrent) p_tables[upcase(current.p_name)] = current;

  return (int)p_tables.size();
}


void CharacterTable::initialize(void)
{
  if (p_loaded) return;
  p_loaded = true;

  //  Resolved here rather than in loadFile so that the parsing stays
  //  usable without any of the ECCE runtime.
  const char* home = getenv("ECCE_HOME");
  if (home == 0) return;

  string path(home);
  path += "/data/client/config/CharacterTables";
  loadFile(path);
}


const CharacterTable* CharacterTable::lookup(const string& group)
{
  initialize();

  map<string, CharacterTable>::const_iterator it =
    p_tables.find(upcase(trim(group)));

  if (it == p_tables.end()) return (const CharacterTable*)0;
  return &(it->second);
}


const vector<double>* CharacterTable::characters(const string& irrep) const
{
  map<string, vector<double> >::const_iterator it = p_characters.find(irrep);
  if (it == p_characters.end()) return (const vector<double>*)0;
  return &(it->second);
}


int CharacterTable::dimension(const string& irrep) const
{
  const vector<double>* chi = characters(irrep);
  if (chi == 0 || chi->empty()) return 0;
  //  The identity is the first class, which tests/symmetry enforces.
  return (int)((*chi)[0] + 0.5);
}


bool CharacterTable::reduce(const vector<double>& chi,
                            vector<int>& multiplicity) const
{
  multiplicity.clear();

  const size_t nclass = p_classes.size();
  if (nclass == 0 || p_order <= 0) return false;
  if (chi.size() != nclass || p_counts.size() != nclass) return false;

  for (size_t i = 0; i < p_irreps.size(); i++) {
    const vector<double>* row = characters(p_irreps[i]);
    if (row == 0 || row->size() != nclass) return false;

    double total = 0.0;
    for (size_t c = 0; c < nclass; c++) {
      total += p_counts[c] * (*row)[c] * chi[c];
    }
    total /= p_order;

    //  A non-integral or negative multiplicity means what was handed in
    //  is not a representation of this group.  Reported rather than
    //  rounded: rounding turns a wrong frame, which is the likeliest
    //  fault here, into a plausible answer.
    const double rounded = floor(total + 0.5);
    if (fabs(total - rounded) > 1.0e-6 || rounded < 0.0) return false;

    multiplicity.push_back((int)rounded);
  }
  return true;
}


string CharacterTable::reduceToString(const vector<double>& chi) const
{
  vector<int> multiplicity;
  if (!reduce(chi, multiplicity)) return "";

  ostringstream out;
  bool first = true;

  for (size_t i = 0; i < multiplicity.size(); i++) {
    if (multiplicity[i] == 0) continue;
    if (!first) out << " + ";
    first = false;
    if (multiplicity[i] != 1) out << multiplicity[i];
    out << p_irreps[i];
  }
  return out.str();
}
