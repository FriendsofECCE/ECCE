// include files ***********************************************************
#  include <iostream>
#  include <strstream>
#  include <fstream>
#  include <string>
using namespace std;
#  include <map>
using std::less;
#include "util/EcceURL.H"

#include "tdat/TProperty.H"

#include "dsm/DavEDSI.H"
#include "dsm/IPropCache.H"
#include "dsm/EDSIFactory.H"
void test(istream& is);

void test(istream& is) {

}
void usage() {
  cout << "usage dpc debug-output(parseOutD.pid) calculation-url "<< endl;
  exit(-1);
}

int main(int argc, char** argv) {
  if (argc != 3)
    usage();
  string  debugOutputFile(argv[1]);
  EcceURL calcURL(argv[2]);
  map< pair <string, int> , string, less<pair < string, int > > > properties;
  IPropCache* pc = EDSIFactory::getIPropCache(calcURL);
  if (!pc) {
    cout << "Could not create Property Cache" << endl;
    exit(1);
  }
  ifstream calcresults(debugOutputFile.c_str());
  test(calcresults);
  string token;

  //Process entire file 
  string key, value;
  while (calcresults.good()) {
    calcresults >> token;
    string log;
    pc->put(calcresults, properties, log); 
    cout << log << endl;
    // Look for key token
  } 
  return 0;
}
