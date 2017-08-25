///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: RefQueueManager.H
//
//
// CLASS SYNOPSIS:
//    RefQueueManager Objects Contains Method for Using Different Queue
//    management Systems.
//
// EXPORTED TYPES:
//    Class RefQueueManager
//
// LOCAL TYPES:
//
// SEE ALSO:
//    Class QueueManager
//      
// DESCRIPTION:
//    Because multiple different job queue management systems may be used
//    to launch jobs, we create a common class that abstracts all
//    their common features.  Objects of this class contain sufficient
//    information to allow ECCE to submit, cancel, monitor, extract submitted
//    job id, and generally communicate with different queue management systems
//    through an abstract interface.
//
// EXAMPLES:
//    testRefQueueMgr.C
//
///////////////////////////////////////////////////////////////////////////////
#ifndef REFQUEUEMANAGER_HH
#define REFQUEUEMANAGER_HH

#include <iostream>
  using std::ostream;

#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include "util/EcceMap.H"

class RefQueueManager;
class Preferences;

typedef EcceMap ESSHDict;
typedef EcceMapIterator ESSHDictIter;

class RefQueueManager
{

public:
  // Constructors - No User Created Objects
    // Only initialization loads these objects

  // Virtual Destructor
    virtual ~RefQueueManager(void);

  // Operators
    bool operator==(const RefQueueManager& refQueueMgr) const;
    bool operator!=(const RefQueueManager& refQueueMgr) const;

  // Modifiers - No User Accessible Modifiers

  // Accessors
    string name(void) const;
    string submitCommand(void) const;
    string cancelCommand(void) const;
    string queryJobCommand(void) const;
    string queryMachineCommand(void) const;
    string queryQueueCommand(void) const;
    string queryDiskUsageCommand(void) const;
    string jobIdParseExpression(void) const;
    string jobIdParseLeadingText(void) const;
    string jobIdParseTrailingText(void) const;

    static const RefQueueManager* lookup(const string& name);

  // Initialization
    static void initialize(void);

protected:

  // Modifiers
    void submitCommand(const string& command);
    void cancelCommand(const string& command);
    void queryJobCommand(const string& command);
    void queryMachineCommand(const string& command);
    void queryQueueCommand(const string& command);
    void queryDiskUsageCommand(const string& command);
    void jobIdParseExpression(const string& command);
    void jobIdParseLeadingText(const string& command);
    void jobIdParseTrailingText(const string& command);

  // Derived Class Accessors
    RefQueueManager(const string& name);

private:

  // State
    string   p_name;               // "LoadLeveler"
    string   p_submitCommand;      // "llsubmit ##queue## ##script##" 
    string   p_cancelCommand;      // "llcancel ##id##"
    string   p_queryJobCommand;    // "llq ##id##"
    string   p_queryMachineCommand;// "llq ##machine##"
    string   p_queryQueueCommand;  // "llq ##queue##"
    string   p_queryDiskUsageCommand; // "df -k "
    string   p_jobIdParseExpression;  // ".*"
    string   p_jobIdParseLeadingText; // "The job \""
    string   p_jobIdParseTrailingText;// "\""

  // Encapsulated Behavior
    void fillAttributesFrom(Preferences& prefs);

  // Statics
    static const char* refQMgrLoadFile;
    static vector<RefQueueManager*> *p_extent;

  // Friends
    friend ostream& operator<<(ostream& os,
			       const RefQueueManager& refQueueMgr);

}; // RefQueueManager

#endif /* REFQUEUEMANAGER_HH */
