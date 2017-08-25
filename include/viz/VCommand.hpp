#ifndef  VCOMMAND_HH
#define  VCOMMAND_HH


#include <string>
using std::string;

#include <xercesc/dom/DOMElement.hpp>
using namespace xercesc;

#include "util/UCommand.H"

class SGFragment;


class ICommandObject;

class VCommand : public UCommand
{
public:
    enum UndoType { UndoClone, UndoAssign, UndoStyle };

    VCommand(const string& name);
   ~VCommand();

    bool  operator==(const VCommand& rhs);

    // UCommand Virtual
    virtual string getCommandClassId() const;
    virtual bool record(ostream& os, ICommandObject *);

    // VCommand Virtual
    virtual bool replay(DOMElement *node, ICommandObject *);

    // Set this to true if a command changes the selection
    virtual bool selectionChanged() const;
    virtual bool geometryDataChanged() const;
    virtual bool residueDataChanged() const;
    virtual bool save() const;
    virtual bool clearSelection() const;

    static void pushUndo(UndoType type, SGFragment *frag, 
        const string& label);


protected:
    // Helpers for parsing for replay
    string getValue(DOMElement *node);
    bool   parseAndSetParameters(DOMElement *el);

    bool p_selChanged;
    bool p_geometryDataChanged;
    bool p_residueDataChanged;
    bool p_save;
    bool p_clearSelection;
    static int  p_undoThreshold;


private:

    // Friends
    friend ostream& operator<< (ostream& os, const VCommand& rhs);
};


#endif // VCommand
