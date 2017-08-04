#include <iostream>
  using namespace std;

#include "util/CommandParameter.H"

int main(int argc, char** argv)
{

  cout << "Testing some simple object creates" << endl;

  CommandParameter p1("string","XYZ");
  cout << p1.getName() << " " << p1.getString() << endl;

  return 0;
}
