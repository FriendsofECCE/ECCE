#include <iostream.h>
#include <fstream.h>
#include <unistd.h>

#include "util/CmdLineAuthListener.H"

#include "dsm/EDSIFactory.H"
#include "dsm/CCCreator.H"

int main(int argc, char** argv)
{

  string url = "http://eccetera.emsl.pnl.gov:8080/Ecce/users/d39506";
  if (argc >= 2) {
    url = argv[1];
  } else {
    cout << "You can specify the dest url on the command line" << endl;
  }

  // hack - have to init password prompter
  CmdLineAuthListener myauth;
  EDSIFactory::addAuthEventListener(&myauth);


  CCCreator creator(url,"G2MP2.xml");
  try {
    creator.execute();
    cout << "ok" << endl;
  } catch (EcceException& ex) {
    cout << "Oh Oh " << ex.what() << endl;
  }
  return 0;
}
