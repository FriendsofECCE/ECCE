#include <iostream.h>

#include "util/Receiver.H"
#include "util/StringReceiver.H"
#include "util/StringAppendEC.H"
#include "util/CommandParameter.H"
#include "util/CommandManager.H"

int main(int argc, char** argv)
{
   static char *words[] = {"test", "out", "adding", "undo", "and", "redo"};

   CommandManager mgr("xxx");

   StringReceiver *sr = new StringReceiver();
   StringAppendEC *cmd1 =0;
   CommandParameter *param = 0;

   for (int idx=0; idx<sizeof(words)/sizeof(char*); idx++) {
      cmd1 = new StringAppendEC("StringAppendEC", sr);
      param = cmd1->getParameter("append");
      param->setString(words[idx]);
      cmd1->execute();
      mgr.addCommand(cmd1);
      cout << "executed: " << sr->c_str() << endl;
   }
   cout << "size " << mgr.getNumCommands() << endl;

   while (mgr.isUndoable()) {
      mgr.undo();
      sr->debugPrint();
   }

   cout << "resizing by -2 " << endl;
   mgr.resizeCommandStack(mgr.getNumCommands()-2);
   mgr.undo();
   cout << "undone: " << sr->c_str() << endl;

   mgr.undo();
   cout << "undone: " << sr->c_str() << endl;

   mgr.redo();
   cout << "redone: " << sr->c_str() << endl;

   mgr.redo();
   cout << "redone: " << sr->c_str() << endl;

   mgr.redo();
   cout << "redone: " << sr->c_str() << endl;

   mgr.redo();
   cout << "should do nothing: redone: " << sr->c_str() << endl;


  return 0;
}
