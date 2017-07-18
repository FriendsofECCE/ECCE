#include <iostream>
#include <strstream>
  using namespace std;

#include "util/Preferences.H"

int main(int argc, char** argv)
{
  Preferences prefs("CalcMgr");
  cout << "---Contents--" << endl;
  cout << prefs << endl;

  string strvalue;
  vector<string> strlist;
  int ivalue;
  float fvalue;
  bool bvalue;

  if (prefs.getString("CalcMgr.OrderedColumns",strvalue))
    cout << "ordered columns:" << strvalue << endl;
  else
    cout << "unable to get CalcMgr.OrderedColumns" << endl;

  if (prefs.getStringList("CalcMgr.OrderedColumns",strlist))
    cout << "ordered columns:" << strlist.size() << endl;
  else
    cout << "unable to get CalcMgr.OrderedColumns" << endl;

  if (prefs.getInt("CalcMgr.X",ivalue))
    cout << "X:" << ivalue << endl;
  else
    cout << "unable to get CalcMgr.X as int" << endl;

  if (prefs.getFloat("CalcMgr.X",fvalue))
    cout << "X:" << fvalue << endl;
  else
    cout << "unable to get CalcMgr.X as float" << endl;

  if (prefs.getBool("CalcMgr.ReverseSort",bvalue))
    cout << "ReverseSort:" << bvalue << endl;
  else
    cout << "unable to get CalcMgr.ReverseSort" << endl;

  vector<string> test;
  test.push_back("oky dokey");
  test.push_back("fred + wilma");
  prefs.setStringList("StringTest",test);
  cout << "Set string list in code" << endl;

  test.clear();
  if (prefs.getStringList("StringTest",test)) 
    cout << "test size " << test.size() << endl;
  else 
    cout << "Unable to get StringTest as string list" << endl;

  cout << "-- Final Prefs file--" << endl;
  cout << prefs << endl;




  Preferences p("/files0/ecce/trunk/siteconfig/Queues");
  cout << "--Queues--" << endl;
  cout << p << endl;

  return 0;
}
