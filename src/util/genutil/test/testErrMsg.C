#include <iostream>
#include <strstream>
using namespace std;

#include "util/Ecce.H"
#include "util/ErrMsg.H"

int main(int argc, char** argv)
{
  ErrMsg errs;

  errs.message("EE_DB_FNF",EE_WARNING,WHERE,"junk");
  errs.dump();
  return 0;
}
