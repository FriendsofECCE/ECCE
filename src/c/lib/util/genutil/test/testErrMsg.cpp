#include <iostream>
#include <strstream>
using namespace std;

#include "util/Ecce.hpp"
#include "util/ErrMsg.hpp"

int main(int argc, char** argv)
{
  ErrMsg errs;

  errs.message("EE_DB_FNF",EE_WARNING,WHERE,"junk");
  errs.dump();
  return 0;
}
