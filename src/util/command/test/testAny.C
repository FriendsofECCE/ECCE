#include <iostream>
  using namespace std;

#include "util/Any.H"

int main(int argc, char** argv)
{

  cout << "Testing some simple object creates" << endl;

  Any *bany = new Any(true);
  cout << "bany value is " << bany->getBoolean() << endl;

  Any *iany = new Any((int)5);
  cout << "iany value is " << iany->getInteger() << endl;

  Any *dany = new Any(99.99);
  cout << "dany value is " << dany->getDouble() << endl;

  Any *sany = new Any("a string");
  cout << "sany value is " << sany->getString() << sany->toString() << endl;

  try { 
     string s = bany->getString();
  } catch (MismatchedTypeException& ex) {
     cout << "I knew this would happen: can't get bool as string" << endl;
  }

  return 0;
}
