#ifndef  SGCOMMAND_HH
#define  SGCOMMAND_HH


#include <string>
using std::string;

#include "viz/VCommand.H"

class SGFragment;


class ICommandObject;

class SGCommand : public VCommand
{

public:

    SGCommand(const string& name);
   ~SGCommand();

    bool  operator==(const SGCommand& rhs);

    // UComand Virtuals
    virtual bool record(ostream& os, ICommandObject *);
    virtual string getCommandClassId() const;

    // VComand Virtuals
    virtual bool replay(DOMElement *node, ICommandObject *);

protected:


private:

};


#endif // VCommand
