#include <iostream>
#include <strstream>

  using namespace std;

#include "util/Ecce.H"
#include "util/KeyValueReader.H"

int main(int argc, char** argv)
{
  static const char *tests = 
    "key: value\n"
    "  key:emsl.pnl.gov\n"
    "#  key: comment line   \n"
    "  key:emsl.pnl.gov  #trailing comment \n"
    "  key:   emsl.pnl.gov   \n"
    "  key:   emsl.pnl.gov   \\\n"
    "  adonis.pnl.gov   \n";

  istrstream str(tests);

  KeyValueReader kvr(str);
//  kvr.stripTrailingComments(false);
  string key, value;
  int idx = 0;
  while (kvr.getpair(key,value)) {
    cout << idx++ << " KEY: " << key << " VALUE: " << value << "!" << endl;
  }

  string file = Ecce::ecceDataPrefPath();
  file += "/errmsg";

  cout << "file " << file << endl;
  KeyValueReader fkvr(file);
  fkvr.setSeparator(',');
  idx = 0;
  while (fkvr.getpair(key,value)) {
    cout << idx++ << " KEY: " << key << " VALUE: " << value << "!" << endl;
  }

  KeyValueReader reader("/files0/ecce/trunk/siteconfig/Queues");
  while (reader.getpair(key,value)) {
    cout << " KEY: " << key << " VALUE: " << value << "!" << endl;
  }


  return 0;
}
