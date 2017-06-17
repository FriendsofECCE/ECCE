#include <iostream>
  using namespace std;

#include <unistd.h>

#include "util/ETimer.H"

int main(int argc, char** argv)
{

  ETimer timer;
  timer.start();
  int junk = 0;
  for (int idx=0; idx<1000000; idx++) {
    junk = idx++; 
  }
  timer.stop();
  cout << "elapsed time " << timer.elapsedTime() << endl;
  cout << "cpu time " << timer.CPUTime() << endl;




  return 0;
}
