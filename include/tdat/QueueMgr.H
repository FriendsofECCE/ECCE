///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: QueueManager.H
//
//
// CLASS SYNOPSIS:
//    QueueManager Objects Manage Queue Information for a Single Machine.
//
// EXPORTED TYPES:
//    Class QueueManager
//
// LOCAL TYPES:
//
// SEE ALSO:
//    Class RefQueueManager
//    Class Queue
//      
// DESCRIPTION:
//    QueueManager manages the connections (logical or physical) between
//    RefMachine, RefQueueManager, and Queue classes as shown below.
//
//    --------------       ----------------        -------------------
//    | RefMachine |-------| QueueManager |*-------| RefQueueManager |
//    --------------       ----------------        -------------------
//                                ^
//                                v
//                                |
//                                *
//                            ---------
//                            | Queue |
//                            ---------
//   Access to individual Queue objects and RefQueueManager objects
//   should be made through this class.  The entire list of queues
//   are available through the QueueManager::queues() accessor and
//   queue selection can be done with QueueManager::sleectQueue().
//   QueueManager also gathers constraint information from the Queue
//   objects that it manages.
//   This class, and related classes, are NOT to be used persistently
//   (it would be hard anyway because they aren't marked).
//
// EXAMPLES:
//    const QueueManager* queueMgr =
//                                QueueManager::lookup("mpp2.emsl.pnl.gov");
//    if (queueMgr != (const QueueManager*)0) {
//      QueueSpec spec;
//      spec.processors = 64;
//      ESQHDict* matches = queueMgr->selectQueues(spec);
//      ESQHDictIter iter(*matches);
//      while (++iter) {
//        cout << iter.key() << endl;
//      }
//      delete matches; matches = (ESQHDict*)0;
//    }
//    testQueueMgr.C
//
///////////////////////////////////////////////////////////////////////////////
#ifndef QUEUEMANAGER_HH
#define QUEUEMANAGER_HH

#include <iostream>
  using std::ostream;

#include <string>
#include <vector>
#include <map>
  using std::string;
  using std::vector;
  using std::map;
  using std::less;

class QueueManager;
class Preferences;
class QueueSpec;
class Queue;

typedef map<string, Queue*, less<string> > ESQHDict;
typedef map<string, Queue*, less<string> >::iterator ESQHDictIter;


class QueueManager
{

public:
  // Constructors - No User Created Objects
    // Only initialization loads these objects

  // Virtual Destructor
    virtual ~QueueManager(void);

  // Operators
    bool operator==(const QueueManager& queueMgr) const;
    bool operator!=(const QueueManager& queueMgr) const;

  // Modifiers - No User Accessible Modifiers

  // Accessors
    string queueMgrName(void) const;
    string machineRefName(void) const;

  // Extent Requirements and Overrides
    virtual string key(void) const;
    static const QueueManager* lookup(const string& name);

  // Queues Access
    const ESQHDict* queues(void) const;
    ESQHDict*       selectQueues(const QueueSpec& queueSpec) const;
    const Queue*    queue(const string& queuename) const;

  // Initialization
    static void initialize(void);
    static void finalize(void);

protected:

  // Modifiers

  // Derived Class Accessors
    QueueManager(const string& queueMgrName, const string& machineRefName);

private:

  // State
    string     p_queueMgrName;       // "LoadLeveler"
    string     p_machineRefName;     // "columbo"
    ESQHDict*  p_queues;

  // Encapsulated Behavior
    void fillQueuesFrom(const string& prefFile);

  // Statics
    static const char* queueMgrLoadFile;
    static vector<QueueManager*> *p_extent;

  // Friends
    friend ostream& operator<<(ostream& os,
			       const QueueManager& queueMgr);

}; // QueueManager

#endif /* QUEUEMANAGER_HH */
