#include <iostream.h>
#include <fstream.h>
#include <unistd.h>

#include "dsm/EDSIMessage.H"

int main(int argc, char** argv)
{

  EDSIMessage *x = new EDSIMessage();
  cout << "Adding NOT_COLLECTION" << endl;
  x->add("NOT_COLLECTION","junk");

  cout << "!" << x->getMessage() << "!" << endl;

  //x->add("EE_MISC","yadayada");
  //cout << "!" << x->getMessage() << "!" << endl;

  delete x;

  return 0;
}
