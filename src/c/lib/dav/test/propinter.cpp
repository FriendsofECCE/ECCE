// include files ***********************************************************
#  include <iostream.h>
#  include <strstream.h>
#  include <fstream.h>

#include "util/EcceURL.H"

#include "tdat/TProperty.H"

#include "dsm/DavEDSI.H"
#include "dsm/PropertyInterpreter.H"
//#include "dsm/PropertyCacheEntry.H"
void test(istream& is);

void test(istream& is) {

}
void usage() {
  cout << "usage propinter url "<< endl;
  exit(-1);
}

int main(int argc, char** argv) {
//  if (argc != 2)
//    usage();
  ifstream calcresults("/files0/launch_data/parseOutD.20205");
  test(calcresults);
  string token;
  //  PropertyCacheEntry *entry = 0;

  //Process entire file 
  while (calcresults.good()) {
    calcresults >> token;

    // Look for key token
    while (calcresults.good() && (token.rfind("key:") != string::npos)) calcresults >>  token;
//    calcresults >> token;
    cout << "key: " << token << endl;
    //TODO:  is value really needed?  Right now I think not!
    entry = PropertyInterpreter::parse(calcresults, token);
    if (entry) {
      if (entry->getClassType() == TProperty::PROPVALUE)  {
        ostrstream val;
        string valString;
        vector<PropertyCacheEntry> p_entries;
        p_entries.push_back(*entry); 
        cout << "VECTOR VALUES---> " ;  
        val << p_entries[0].getStream()->rdbuf();
        valString = val.str(); 
        cout << valString ;
        cout <<  "<-- VECTOR VALUES" << endl;
        cout << "VALUE-->" << endl;
        cout << entry->getStream()->rdbuf();
        cout <<  "<--VALUE" << endl;
      }
      if (entry->getClassType() == TProperty::PROPVECTOR)  {
        cout << "VECTOR-->" << endl;
        cout << entry->getStream()->rdbuf();
        cout <<  "<--VECTOR" << endl;
      }
      if (entry->getClassType() == TProperty::PROPTABLE)  {
        cout << "TABLE-->" << endl;
        cout << entry->getStream()->rdbuf();
        cout <<  "<--TABLE" << endl;
      }
    }
  } 
  return 0;
}
