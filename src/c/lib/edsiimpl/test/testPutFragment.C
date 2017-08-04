#include <iostream>
#include <strstream>
  using namespace std;

#include "util/CmdLineAuthListener.H"

#include "dsm/EDSIFactory.H"
#include "dsm/ChemistryTask.H"

#include "tdat/Fragment.H"



/**
 * Test program that test adding a fragment to an existing calculation.
 * The url provided is assumed to be a calculation in good standing.
 */
int main(int argc, char** argv)
{

   if (argc < 2) {
      cout << "Usage: testPutFragment <url> " << endl;
      cout << "\t testPutFragment http://fletcher.pnl.gov:8080/sam/files/users/karen/mycalc" << endl;
      exit(-1);
   }

   EcceURL url = argv[1];


  static string xyz = "8\n";
  xyz +=  "Ethane\n";
  xyz +=  "C   -0.537090    0.148330   -0.000410\n";
  xyz +=  "C    0.965490    0.079930    0.000250\n";
  xyz +=  "H   -0.900450    1.054480   -0.502500\n";
  xyz +=  "H   -0.979670   -0.711650   -0.519840\n";
  xyz +=  "H   -0.942390    0.156680    1.019810\n";
  xyz +=  "H    1.409980    0.944880    0.509490\n";
  xyz +=  "H    1.329880   -0.820590    0.511890\n";
  xyz +=  "H    1.370610    0.061380   -1.019850\n";

  Fragment frag;

  istrstream istr(xyz.data());

  frag.restoreXYZ(istr,true);


  CmdLineAuthListener myauth;
  EDSIFactory::addAuthEventListener(&myauth);

  Task *task = EDSIFactory::getTask(url);

  ChemistryTask * chem = dynamic_cast<ChemistryTask*>(task);
  if (chem != 0) {
     chem->fragment(&frag);
  } else {
     cerr << "This url is not a chemistry task" << endl;
  }

  return 0;
}
