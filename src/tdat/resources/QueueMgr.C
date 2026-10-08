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

#include <algorithm>
#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "util/Preferences.H"
#include "util/SFile.H"
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
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Locate a queue configuration file, preferring the user's own copy.
//
//    Queue definitions used to be read only from $ECCE_HOME/siteconfig,
//    which on a packaged install is root-owned, so describing your own
//    cluster's queues meant editing files as root.  If
//    $ECCE_REALUSERHOME/.ECCE/<name> exists it wins, otherwise the site-wide
//    file is used.  CONFIG.<machine> follows a different rule (a per-key
//    merge, RefMachine::config()); these files are not CONFIG files.
//
//    Note this is an override, not a merge: a user copy of the "Queues"
//    registry replaces the site one wholesale rather than adding to it.
//    That is the simple, predictable behaviour; a proper editor and an
//    additive merge are a separate piece of work.
//
///////////////////////////////////////////////////////////////////////////////
string QueueManager::queueConfigFile(const string& name,
                                     const string& machine)
{
  std::vector<Ecce::SiteLayer> layers = Ecce::siteConfigLayers(machine);
  for (size_t i = 0; i < layers.size(); i++) {
    string path = layers[i].dir + name;
    SFile file(path.c_str());
    if (file.exists())
      return path;
  }
  return layers.back().dir + name;     // the install's, which may not exist
}


// The Queues registries of the configuration layers, highest priority first.
struct QLayers {
  std::vector<Ecce::SiteLayer> layers;
  std::vector<Preferences*> prefs;         // 0: no such file
  std::vector<std::vector<string> > lists;  // the machines each lists

  void load(const string& machine, const string& regFile)
  {
    layers = Ecce::siteConfigLayers(machine);
    for (size_t i = 0; i < layers.size(); i++) {
      string path = layers[i].dir + regFile;
      SFile file(path.c_str());
      Preferences* p = 0;
      if (file.exists()) {
        p = new Preferences(path, true, 0);
        if (!p->isValid()) { delete p; p = 0; }
      }
      prefs.push_back(p);
      lists.push_back(std::vector<string>());
      if (p)
        p->getStringList(regFile, lists.back());
    }
  }
  ~QLayers()
  {
    for (size_t i = 0; i < prefs.size(); i++) delete prefs[i];
  }
  bool anyValid() const
  {
    for (size_t i = 0; i < prefs.size(); i++) if (prefs[i]) return true;
    return false;
  }
  bool get(const string& machine, const string& key, string& value) const
  {
    bool site = false;
    for (size_t i = 0; i < layers.size(); i++) {
      if (!prefs[i]) continue;
      if (!layers[i].user) {
        // one site layer supplies the machine: the first that lists it
        if (site) continue;
        if (find(lists[i].begin(), lists[i].end(), machine) == lists[i].end())
          continue;
        site = true;
      }
      if (prefs[i]->getString(machine + "|" + key, value)) return true;
    }
    return false;
  }
};

void QueueManager::initialize(void)
{
  if (p_extent == (vector<QueueManager*> *)0) {
    p_extent = new vector<QueueManager*>();
    // The registry of every layer (user, server site, install), with the
    // machines of all of them listed.  For one machine the user's entries win
    // per key, so a site machine can be adjusted without copying it across;
    // the site keys all come from the one site layer that lists the machine.
    // localhost has its own layers: the client's alone (#192).
    QLayers normal, client;
    normal.load("", QueueManager::queueMgrLoadFile);
    client.load("localhost", QueueManager::queueMgrLoadFile);

    EE_RT_ASSERT(normal.anyValid(), EE_FATAL,
                 "Error!  Must Have a Queues File!");

    vector<string> machines;
    for (int i = (int)normal.layers.size() - 1; i >= 0; i--) {
      for (unsigned int u = 0; u < normal.lists[i].size(); u++) {
        const string& m = normal.lists[i][u];
        if (m == "localhost" && normal.layers[i].fromServer) continue;
        if (find(machines.begin(), machines.end(), m) == machines.end())
          machines.push_back(m);
      }
    }

    QueueManager *newObject = (QueueManager*)0;
    string queueMgrName;
    for (unsigned int index = 0; index < machines.size(); index++) {
      string& name = machines[index];
      // User file wins per key, so a site machine can be adjusted without
      // copying it across.
      QLayers& q = (name == "localhost") ? client : normal;
      bool found = q.get(name, "queueMgrName", queueMgrName);
      EE_RT_ASSERT(found, EE_FATAL, name + "|queueMgrName: not found!");
#ifdef DEBUG
      cout << name << ", " << queueMgrName << endl;
#endif
      newObject = new QueueManager(queueMgrName,  name);
      if (queueMgrName != "Shell") {
        // Fetch and Fill Attributes
        string prefFile;
        bool gotPref = q.get(name, "prefFile", prefFile);
        EE_RT_ASSERT(gotPref, EE_FATAL,
                     "No Queue Preferences File Specified!");
        newObject->fillQueuesFrom(
            QueueManager::queueConfigFile(prefFile, name));
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
