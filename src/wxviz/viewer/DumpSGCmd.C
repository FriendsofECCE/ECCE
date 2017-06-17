#include "inv/actions/SoWriteAction.H"
#include "inv/SoOutput.H"


#include "wxviz/SGViewer.H"
#include "wxviz/DumpSGCmd.H"

#include "viz/EcceAxis.H"

DumpSGCmd::DumpSGCmd(const string& name, Receiver *receiver)
       : Command(name, receiver)
{
  init();
}

DumpSGCmd::~DumpSGCmd()
{
}

void DumpSGCmd::init() 
{
   addParameter(new CommandParameter("file","sgdump.txt"));
}

bool DumpSGCmd::execute() throw(EcceException)
{
   SGViewer *viewer = dynamic_cast<SGViewer*>(getReceiver());

   string filename = getParameter("file")->getString();

   SoWriteAction myaction;

   myaction.getOutput()->openFile(filename.c_str());
   myaction.getOutput()->setBinary(false);
   myaction.apply(viewer->getTopNode());
   myaction.getOutput()->closeFile();


   return true;
}

bool DumpSGCmd::isUndoable() const
{
  return false;
}

void DumpSGCmd::undo()
{
}
