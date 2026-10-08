#include <stdlib.h>
#include "util/CommandWrapper.H"
#include "util/Ecce.H"

CommandWrapper::CommandWrapper()
{
}

CommandWrapper::CommandWrapper(const string& cmd)
{
   p_cmd = cmd;
}

CommandWrapper::CommandWrapper(const CommandWrapper& rhs)
{
   p_cmd = rhs.p_cmd;
}

string CommandWrapper::getCommand()
{
   return p_cmd;
}

void CommandWrapper::setCommand(const string& cmd)
{
   p_cmd = cmd;
}

void CommandWrapper::execute()
{
   string cmd = getCommand();
   // No console window on Windows, and the exit code rather than a
   // wait status there.
   int istatus = Ecce::runCommand(cmd);

   if (istatus != 0) 
      throw SystemCommandException(istatus, 
                                   string("Error executing: " + cmd).c_str(),
                                   WHERE);
   
}

