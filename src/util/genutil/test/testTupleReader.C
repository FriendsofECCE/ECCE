#include <iostream>
#include <strstream>
  using namespace std;

#include "util/Ecce.H"
#include "util/TupleReader.H"

int main(int argc, char** argv)
{
  static const char *tests = 
    "key: value: x\n"
    "  key:emsl.pnl.gov: y\n"
    "#  key: comment line   \n"
    "  key:emsl.pnl.gov  #trailing comment \n"
    "  key:   emsl.pnl.gov: y   \n"
    "  key:   emsl.pnl.gov:    \\\n"
    "  adonis.pnl.gov: y   \n";

  istrstream str(tests);

  TupleReader kvr(str);
  string key, value, three;
  int idx = 0;
  while (kvr.gettuple(key,value, three)) {
    cout << idx++ << " ONE: " << key << " TWO: " << value << " THREE " << three <<  "!" << endl;
  }

  return 0;
}
