#include <iostream>
  using namespace std;
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "util/JMSPublisher.H"


int main(int argc, char** argv) {

  JMSPublisher pub("testFork");
  int status = system("execer &");

  cerr << "return value of system call = " << endl;
  cerr << "exiting original program" << endl;

  return 0;
}
