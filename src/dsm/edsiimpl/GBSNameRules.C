///////////////////////////////////////////////////////////////////////////////
// FILENAME: GBSNameRules.C
//
// See GBSNameRules.H for why this is deliberately dependency free.
///////////////////////////////////////////////////////////////////////////////

#include "dsm/GBSNameRules.H"

#include <cctype>

using std::string;
using std::vector;


string GBSNameRules::lower(const string& s)
{
  string ret(s);
  for (string::size_type i = 0; i < ret.size(); i++) {
    ret[i] = (char)tolower((unsigned char)ret[i]);
  }
  return ret;
}


bool GBSNameRules::matchesFilter(const string& name, const string& filter)
{
  string lname = lower(name);

  //  Split the filter on whitespace; every term must appear.  An empty
  //  filter yields no terms, so everything matches -- that is how the
  //  unfiltered list stays reachable.
  string::size_type pos = 0;
  while (pos < filter.size()) {

    while (pos < filter.size() && isspace((unsigned char)filter[pos])) {
      pos++;
    }
    string::size_type start = pos;
    while (pos < filter.size() && !isspace((unsigned char)filter[pos])) {
      pos++;
    }
    if (pos == start) {
      break;
    }

    string term = lower(filter.substr(start, pos - start));
    if (lname.find(term) == string::npos) {
      return false;
    }
  }

  return true;
}


string GBSNameRules::pairedECP(const string& basisName,
                               const vector<string>& basisFiles,
                               const vector<string>& ecpNames,
                               const vector<string>& ecpFiles)
{
  string lbasis = lower(basisName);
  size_t i;

  //  1. the <basis>-ecp naming convention.  Only route to the def2 ECPs.
  string convention = lbasis + "-ecp";
  for (i = 0; i < ecpNames.size(); i++) {
    if (lower(ecpNames[i]) == convention) {
      return ecpNames[i];
    }
  }

  //  2. the identical name in both indexes.
  for (i = 0; i < ecpNames.size(); i++) {
    if (lower(ecpNames[i]) == lbasis) {
      return ecpNames[i];
    }
  }

  //  3. a shared .POT file.  Skip the orbital set's own .BAS: only the
  //     potential file can pair the two, and matching on any file would
  //     pair an orbital set with an ecp record that happens to carry the
  //     same .BAS name.
  for (i = 0; i < ecpNames.size() && i < ecpFiles.size(); i++) {
    if (ecpFiles[i].empty()) {
      continue;
    }
    string lecpFile = lower(ecpFiles[i]);
    if (lecpFile.size() < 4 ||
        lecpFile.compare(lecpFile.size() - 4, 4, ".pot") != 0) {
      continue;
    }
    for (size_t j = 0; j < basisFiles.size(); j++) {
      if (lower(basisFiles[j]) == lecpFile) {
        return ecpNames[i];
      }
    }
  }

  return "";
}
