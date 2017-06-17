#include <iostream.h>
#include <strstream.h>
#include <unistd.h>

#include "tdat/GUIValues.H"

int main(int argc, char** argv)
{
  static const char *tests = {
    "keyone: fred 1 1\n"
    "#junk: ignore 1 0\n"
    "keytwo: barney 0 1\n"
    "nocolonisok barney 0 1\n"
    "missingone barney  1\n"
    "one barney 0 1\n"
    "two barney 0 1\n"
    "three barney 0 1\n"
    "four barney 0 1\n"
    "five barney 0 1\n"
    "six 6 0 1\n"
    "seven barney 0 1\n"
  };

  istrstream istr(tests);

  GUIValues details;
  int cnt = details.load(istr);
  cout << "loaded cnt " << cnt << endl;

  GUIValue *obj = details.get("six");
  if (obj) {
    cout << "able to find six" << obj->m_key << " ";
    cout << obj->getValueAsString() << " " ;
    cout << obj->getValueAsInt() << " " ;
    cout << endl;
  } else {
    cout << "no six" << endl;
  }

  details += "dino astro 1 1";

  details.dump(cout);

  return 0;
}
