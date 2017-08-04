#include <iostream>
#include <strstream>
  using namespace std;

#include "util/StringConverter.H"

int main(int argc, char** argv)
{
  bool status;
  int ival;

  status = StringConverter::toInt("5",ival);
  cout << "Status: " << (int)status << " ival(5) " << ival << endl;

  status = StringConverter::toInt("-5",ival);
  cout << "Status: " << (int)status << " ival(-5) " << ival << endl;
  
  status = StringConverter::toInt("-5x",ival);
  cout << "Status: " << (int)status << " FAIL " << ival << endl;


  status = StringConverter::toInt("-540.x",ival);
  cout << "Status: " << (int)status << " FAIL " << ival << endl;

  status = StringConverter::toInt("-0 yyy",ival);
  cout << "Status: " << (int)status << " " << ival << endl;

  status = StringConverter::toInt("    666 ",ival);
  cout << "Status: " << (int)status << " ival(666) " << ival << endl;


  double dval=0;
  status = StringConverter::toDouble("    6.66 ",dval);
  cout << "Status: " << (int)status << " dval(6.66) " << dval << endl;


  bool bval=0;
  status = StringConverter::toBoolean("    1",bval);
  cout << "Status: " << (int)status << " bval(true) " << (int)bval << endl;

  status = StringConverter::toBoolean("0",bval);
  cout << "Status: " << (int)status << " bval(false) " << (int)bval << endl;

  status = StringConverter::toBoolean(" false",bval);
  cout << "Status: " << (int)status << " bval(false) " << (int)bval << endl;

             // list conversions
  int idx;


  /*
  StringConverter::_TOKENIZER = " \t";

  vector<int> myints;
  status = StringConverter::toIntList(" 1 2 3 9 8 6",myints);
  cout << "Status: " << (int)status << endl;
  for (idx=0; idx<myints.size(); idx++) {
    cout << "\t" << myints[idx] << endl;
  }

  myints.clear();
  status = StringConverter::toIntList(" 1 2 3.0 9 8 6",myints);
  cout << "Status: " << (int)status << endl;
  for (idx=0; idx<myints.size(); idx++) {
    cout << "\t" << myints[idx] << endl;
  }
  */

  return 0;
}
