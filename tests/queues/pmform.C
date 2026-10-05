// Prints the form Machine Registration posts to processmachine for the
// given name/value argument pairs ("type" first), encoded by the GUI's
// own ProcessMachine::field, for tests/queues/config_test.py.
#include <iostream>

#include "util/ProcessMachine.H"

int main(int argc, char** argv)
{
  if (argc < 3 || argc % 2 != 1) {
    std::cerr << "usage: pmform name value [name value ...]" << std::endl;
    return 2;
  }
  std::string form;
  for (int i = 1; i < argc; i += 2)
    form += ProcessMachine::field(argv[i], argv[i + 1]);
  std::cout << form.substr(1);
  return 0;
}
