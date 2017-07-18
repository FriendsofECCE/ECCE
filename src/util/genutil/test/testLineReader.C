#include <iostream>
#include <strstream>
  using namespace std;

#include "util/Ecce.H"
#include "util/LineReader.H"

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

  LineReader kvr(str);
  string value;
  int idx = 0;
  while (kvr.getLine(value)) {
    cout << idx++ << " Line: " << value << "!" << endl;
  }


  return 0;
}
