//////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: QueueManager.C
//
//
// DESIGN:
//   Objects of this class are read from configuration files 
//   Users are only allowed access
//   to attributes.  It is assumed that initialization of this class
//   occurs before it is used.  The objects are only intended to be used
//   transitively.
//
///////////////////////////////////////////////////////////////////////////////

// system includes
#include <iostream>
  using std::cout;
  using std::endl;

// general includes
// library includes

#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "util/Preferences.H"
#include "tdat/RefQueueMgr.H"
#include "tdat/QueueMgr.H"
#include "tdat/RefMachine.H"
#include "tdat/Queue.H"

// #ifndef DEBUG
// #define DEBUG
// #endif

vector<QueueManager*> *QueueManager::p_extent = 0;

// -----------------------
// Public Member Functions
// -----------------------
// ---------- Virtual Destructor ------------
QueueManager::~QueueManager(void)
{
  vector<QueueManager*>::iterator it = p_extent->begin();
  while (it != p_extent->end()) {
    if (*it == this) {
//      delete this;
      p_extent->erase(it);
      return;
    }
    it++;
  }
}


// ---------- Operators ----------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Identity-Based Comparison.  Based on Name and Version.
//
///////////////////////////////////////////////////////////////////////////////
bool QueueManager::operator==(const QueueManager& queueMgr) const
{ return (this == &queueMgr); }
bool QueueManager::operator!=(const QueueManager& queueMgr) const
{ return (this != &queueMgr); }


// ------------ Modifiers ------------

// ------------ Accessors ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Simple Attribute Accessors.
//
///////////////////////////////////////////////////////////////////////////////
string QueueManager::queueMgrName(void) const
{ return p_queueMgrName; }
string QueueManager::machineRefName(void) const
{ return p_machineRefName; }


// ------------ Extent Requirements and Overrides ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    The Machine Reference Name is the Lookup Key.
//
///////////////////////////////////////////////////////////////////////////////
string QueueManager::key(void) const
{ return machineRefName(); }

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Lookup a QueueManager Object by Machine it is Associated With.
//    Exact Matches Only are Supported.
//
//  Implementation
//    The purpose of this function is to overide the Extent provided
//    lookup so that dbName, match, and persistence arguments are
//    defaulted.
//
///////////////////////////////////////////////////////////////////////////////
const QueueManager* QueueManager::lookup(const string& refname)
{
  const QueueManager *ret = 0;
  initialize();
  for (int idx=p_extent->size()-1; idx>=0; idx--) {
    if ((*p_extent)[idx]->p_machineRefName == refname) {
      ret = (*p_extent)[idx];
      break;
    }
  }
  return ret;
}

// ---------- Queues Access  ----------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Return a Possibly Empty Pointer to All Queues for this Manager.
//
///////////////////////////////////////////////////////////////////////////////
const ESQHDict* QueueManager::queues(void) const
{ return p_queues; }

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Return a Newly Allocated Dictionary of Queues that Match the Specified 
//     Constraint.
//    Caller Must Delete the Dictionary BUT NOT the Queue Objects.
//
///////////////////////////////////////////////////////////////////////////////
ESQHDict* QueueManager::selectQueues(const QueueSpec& queueSpec) const
{
  ESQHDict* result = new ESQHDict;
  if (queues() != (ESQHDict*)0) {
    ESQHDictIter iter = ((ESQHDict*)queues())->begin();
    while (iter != ((ESQHDict*)queues())->end()) {
      if ((*(*iter).second) < queueSpec) {
        (*result)[(*iter).first] = (*iter).second;
      }
      iter++;
    }
  }
  return result;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Return a Newly Allocated Dictionary of Queues that Match the Specified 
//     Constraint.
//    Caller Must Delete the Dictionary BUT NOT the Queue Objects.
//
///////////////////////////////////////////////////////////////////////////////
const Queue* QueueManager::queue(const string& queueName) const
{
  const Queue* result = (Queue*)0;
  if ((queues() != (ESQHDict*)0) &&
      (queues()->find(queueName) != queues()->end())) {
    result = (*((ESQHDict*)queues()))[queueName];
  }
  return result;
}

// ---------- Initialization  ----------
// Note that destructor automatically removes from extent
void QueueManager::finalize(void)
{
  if (p_extent != (vector<QueueManager*> *)0) {
    for (int idx=p_extent->size()-1; idx>=0; idx--) {
      delete (*p_extent)[idx];
    }
    delete p_extent;
    p_extent = 0;
  }
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Populate all Queue Managers for Subsequent Access and Querying.
//
//  Implementation
//    We turn off persistent extent management and turn on transient tracking
//    because all of these objects will be transient.  The preferences
//    file specified by QueueManager::queueMgrLoadFile is parsed.  For
//    each machine with a QueueManager, a QueueManager object is created
//    and filled in from the preferences file.  Queues configurations are
//    then read from a file specified in the QueueManager::queueMgrLoadFile
//    preference file.  Any missing files are a fatal error.
//
///////////////////////////////////////////////////////////////////////////////
void QueueManager::initialize(void)
{
  if (p_extent == (vector<QueueManager*> *)0) {
    p_extent = new vector<QueueManager*>();
    string qfile = Ecce::ecceHome();
    qfile += "/siteconfig/";
    qfile += QueueManager::queueMgrLoadFile;

    Preferences prefs(qfile, true, 0 /*create mode -must exist*/);
    EE_RT_ASSERT(prefs.isValid(), EE_FATAL, "Error!  Must Have a Queues File!");

    vector<string> machines;
    prefs.getStringList(QueueManager::queueMgrLoadFile, machines);

    QueueManager *newObject = (QueueManager*)0;
    string queueMgrName;
    for (unsigned int index = 0; index < machines.size(); index++) {
      string& name = machines[index];
      EE_RT_ASSERT(prefs.getString(name + "|queueMgrName", queueMgrName),
                   EE_FATAL, name + "|queueMgrName: not found!");
#ifdef DEBUG
      cout << name << ", " << queueMgrName << endl;
#endif
      newObject = new QueueManager(queueMgrName,  name);
      if (queueMgrName != "Shell") {
        // Fetch and Fill Attributes
        string prefFile;
        EE_RT_ASSERT(prefs.getString(name + "|prefFile", prefFile),
                     EE_FATAL, "No Queue Preferences File Specified!");
        qfile = Ecce::ecceHome();
        qfile += "/siteconfig/";
        newObject->fillQueuesFrom(qfile + prefFile);
      }
#ifdef DEBUG
      cout << *newObject << endl;
#endif
    }
  }
}



// --------------------------
// Protected Member Functions
// --------------------------

// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Create and Validate a new Query Manager.
//    The Specified RefQueueManager Must Exist as Well as the Specified
//    Machine.
//
//  Implementation
//    Arguments are validated through other Extent() lookup functionality.
//    Note, if RefQueueManager hasn't been initialized yet, this call
//    will do it.
//
///////////////////////////////////////////////////////////////////////////////
QueueManager::QueueManager(const string& queueMgrName,
                           const string& machineRefName) :
                                            p_queueMgrName(queueMgrName),
                                            p_machineRefName(machineRefName),
                                            p_queues((ESQHDict*)0)
{
  initialize();

  // Validate the Values - These MUST Be Registered
  EE_RT_ASSERT((RefQueueManager::lookup(queueMgrName) !=
               (const RefQueueManager*)0), EE_FATAL,
               "Unknown Queue Manager Specified!");
  bool result = false;
  result = RefMachine::refLookup(machineRefName) != (RefMachine*)0;
  EE_RT_ASSERT(result, EE_FATAL, machineRefName);

  p_extent->push_back(this);
}

// ------------------------
// Private Member Functions
// ------------------------

// ---------- Friends ----------
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Send the QueueManager and Queue Information to a Stream.
//
///////////////////////////////////////////////////////////////////////////////
ostream& operator<<(ostream& os, const QueueManager& queueMgr)
{
  os << "[QueueManager<";
  os << queueMgr.queueMgrName() << ",";
  os << queueMgr.machineRefName() << endl;
  if (queueMgr.p_queues != (ESQHDict*)0) {
    ESQHDictIter iter = queueMgr.p_queues->begin();
    while (iter != queueMgr.p_queues->end()) {
      os << (*iter).first << " = ";
      if ((*iter).second) os << *((*iter).second) << endl;
      iter++;
    }
  }
  os << ">]" << endl;
  return os;
}

// ---------- Encapsulated Behavior ----------
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Fetch the Queues managed by this Queue Manager from Supplied
//    Preference File.
//
///////////////////////////////////////////////////////////////////////////////
void QueueManager::fillQueuesFrom(const string& prefFile)
{
  Preferences prefs(prefFile, true, 0/*createMode -> file must exist*/);
  EE_RT_ASSERT(prefs.isValid(), EE_FATAL,
               prefFile + ": Illegal or Non-Existent Queues File!");
  vector<string> queues;
  EE_RT_ASSERT(prefs.getStringList(QueueManager::queueMgrLoadFile, queues),
               EE_FATAL, prefFile + ": Illegally Structured Queues File!");
  Queue *newObject = (Queue*)0;
  p_queues = new ESQHDict;
  for (unsigned int index = 0; index < queues.size(); index++) {
    string& queueName = queues[index];
#ifdef DEBUG
    cout << queueName << endl;
#endif
    newObject = new Queue(queueName);
    newObject->fillAttributesFrom(prefs);
    (*p_queues)[queueName] = newObject;
#ifdef DEBUG
    cout << *newObject << endl;
#endif
  }
}

const char* QueueManager::queueMgrLoadFile = "Queues";
