///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: CommandWrapper.H
//
//
// CLASS SYNOPSIS:
//    Wrapper for issuing simple shell commands.
//
// DESCRIPTION:
//
///////////////////////////////////////////////////////////////////////////////
#ifndef COMMANDWRAPPER_HH
#define COMMANDWRAPPER_HH

#include <string>
using std::string;

#include "util/SystemCommandException.H"

class CommandWrapper 
{

public:

    CommandWrapper();
    CommandWrapper(const string& cmd);
    CommandWrapper(const CommandWrapper& rhs);
    virtual ~CommandWrapper() {};

    virtual void setCommand(const string& cmd);
    virtual string getCommand();
    virtual void execute() throw(SystemCommandException);


protected:
    string p_cmd;


}; 

#endif 
