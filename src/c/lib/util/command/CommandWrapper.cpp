#include <stdlib.h>
#include "util/CommandWrapper.H"

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

void CommandWrapper::execute() throw(SystemCommandException)
{
   string cmd = getCommand();
   int istatus = system(cmd.c_str());
   istatus = ( istatus >> 8 ) ;

   if (istatus != 0) 
      throw SystemCommandException(istatus, 
                                   string("Error executing: " + cmd).c_str(),
                                   WHERE);
   
}

