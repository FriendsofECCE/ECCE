#include <iostream.h>

#include "util/Receiver.H"
#include "util/StringReceiver.H"
#include "util/StringAppendEC.H"
#include "util/CommandParameter.H"

int main(int argc, char** argv)
{

   StringReceiver *sr = new StringReceiver();
   StringAppendEC *cmd1 = new StringAppendEC("StringAppendEC", 
                                            dynamic_cast<Receiver*>(sr));
   CommandParameter *param = cmd1->getParameter("append");
   param->setString("test");
   cmd1->execute();
   cout << "executed: " << sr->c_str() << endl;

   StringAppendEC *cmd2 = new StringAppendEC("StringAppendEC", 
                                            dynamic_cast<Receiver*>(sr));
   param = cmd2->getParameter("append");
   param->setString("this");
   cmd2->execute();
   cout << "executed: " << sr->c_str() << endl;

   cmd2->undo();
   cout << "undone: " << sr->c_str() << endl;
   cmd1->undo();
   cout << "undone: " << sr->c_str() << endl;

  return 0;
}
