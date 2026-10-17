/**
 * Parse-script output on stdin -> ECCE property documents, through the
 * real PropertyInterpreter (as eccejobstore's DavPropCache does).
 *
 *   propinterp [outdir]
 *
 * Prints one line per property: "<key> rows <r> columns <c> values <n>",
 * and with outdir writes each document to outdir/<key>, the form a local
 * calculation's Props directory holds.  Needs ECCE_HOME with
 * data/client/config/properties.  Built and run by propinterp_test.py.
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <strstream>
#include <string>
#include <cstdlib>

#include "tdat/TProperty.H"
#include "dsm/PropFactory.H"
#include "dsm/PropertyDoc.H"
#include "dsm/PropertyInterpreter.H"
#include "dsm/BasicDOMWriter.H"

using namespace std;

static string text(char *p)
{
  //  Xerces-allocated; a one-shot driver leaves it.
  return p ? string(p) : string();
}

int main(int argc, char **argv)
{
  const string outdir = argc > 1 ? argv[1] : "";
  PropertyInterpreter pi;
  string token, log;
  int count = 0;
  while (cin.good()) {
    while (cin.good() && token.rfind("key:") == string::npos) cin >> token;
    if (!cin.good()) break;
    cin >> token;
    if (!PropFactory::getPropRef(token)) {
      cout << "UNKNOWN " << token << endl;
      continue;
    }
    const string key = token;
    PropertyDoc *doc = pi.parse(cin, key, log, 0);
    if (!doc) {
      cout << "FAILED " << key << endl;
      break;
    }
    //  The token the interpreter stopped on (END) is consumed; look for
    //  the next key from here.
    token = "";
    istringstream vals(text(doc->values()));
    size_t n = 0;
    string v;
    while (vals >> v) n++;
    cout << key << " rows " << text(doc->rows()) << " columns "
         << text(doc->columns()) << " values " << n << endl;
    if (!outdir.empty()) {
      ofstream out((outdir + "/" + key).c_str());
      ostrstream os;
      BasicDOMWriter::write(*(doc->container()), os, false, true);
      os << ends;
      out << os.str();
      os.freeze(false);
    }
    delete doc;
    count++;
  }
  cerr << log;
  return count > 0 ? 0 : 1;
}
