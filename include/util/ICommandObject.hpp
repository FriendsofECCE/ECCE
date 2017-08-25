///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: ICommandObject.H
//
//
// CLASS SYNOPSIS:
//   A java-style "interface" for any object that is the subject of "commands".
//
// DESCRIPTION:
//   This class exists to server as a superclass to all classes that will
//   be the subject of commands.
///////////////////////////////////////////////////////////////////////////////

#ifndef ICOMMANDOBJECT_HH
#define ICOMMANDOBJECT_HH

#include <string>
using std::string;

class ICommandObject
{
public:
  virtual string commandObjectType() const = 0;

};

#endif
