#include <iostream.h>
#include <fstream.h>
#include <unistd.h>


#include <xercesc/util/PlatformUtils.hpp>

#include "util/ETimer.H"
#include "dsm/MimeTypes.H"

int main(int argc, char** argv)
{
  ETimer timer;
  timer.start();
  MimeTypes& blarg = MimeTypes::getMimeTypes();
  timer.stop();
  cout << "elapsed/cpu " << timer.elapsedTime() << " " << timer.CPUTime() << endl;
  string fname = blarg.fileName("text/xml");
  cout << "filename for text/xml: " << fname.c_str() << endl;
  for (int idx=0; idx<100; idx++) {
    timer.start();
    vector<string> junk = blarg.contentTypes("unordered-collection");
    timer.stop();
  cout << "length " << junk.size() << " elapsed/cpu " << timer.elapsedTime() << " " << timer.CPUTime() << endl;
  }
  timer.stop();

  return 0;
}
