// First-start registration of this computer (RunMgmt::registerLocalMachine):
// prints the message, for localcodes_test.py, which sets PATH and HOME.
#include <iostream>
#include <string>
#include "comm/RunMgmt.H"

int main()
{
  std::string msg;
  bool ok = RunMgmt::registerLocalMachine(msg);
  std::cout << (ok ? "OK" : "FAILED") << "\n" << msg << std::endl;
  return ok ? 0 : 1;
}
