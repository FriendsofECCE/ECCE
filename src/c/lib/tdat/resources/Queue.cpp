///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: Queue.C
//
//
// DESIGN:
//    The Queue class has very little behavior other than accessors.
//    They are created by the QueueManager that manages them (during
//    QueueManager::initialize()).  Access is also primarily through
//    QueueManager::selectQueues().  Queue objects read themselves,
//    after having their "UNIQUE" name set, from a preferences file.
//    The only other non-structure like behavior is the ability to
//    "match" or "qualify" a queue against user-specified constraints.
//    Matching is accomplished by operator<() that takes a QueueSpec.
//
///////////////////////////////////////////////////////////////////////////////

// system includes
#include <limits.h>

#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "util/Preferences.H"
#include "util/StringTokenizer.H"
#include "tdat/Queue.H"

// #define DEBUG

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Initialize Structure  ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Initialize All Matching Values in Queue Constraint Specification.
//    A Newly Initialized QueueSpec Object Will Match ALL Queues.
//
///////////////////////////////////////////////////////////////////////////////
QueueSpec::QueueSpec(void) : maxJobCPU(0),
                             maxProcessCPU(0),
                             runLimit(0),
                             processors(0),
                             fileLimit(0),
                             coreLimit(0),
                             memLimit(0),
                             scratchLimit(0),
                             dataLimit(0),
                             stackLimit(0)
{ }

// ---------- Virtual Destructor ------------
Queue::~Queue(void)
{
}


// ---------- Operators ----------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Value-Based Comparison.  Based on Name.
//
///////////////////////////////////////////////////////////////////////////////
bool
Queue::operator==(const Queue& queue) const
{ return (name() == queue.name()); }
bool
Queue::operator!=(const Queue& queue) const
{ return !((*this) == queue); }

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Return TRUE if the Queue is Sufficient to Handle the Specification.
//
//  Implementation
//    Use a short circuit combination of ifs to fail ASAP.
//    Since most queues will be rejected solely based on the number of
//    processors, we evaluate that condition first.
//    This function is used in the QueueManager::selectQueue() function.
//
///////////////////////////////////////////////////////////////////////////////
bool
Queue::operator<(const QueueSpec& queueSpec) const
{
  if (queueSpec.processors != 0) {
    if ((queueSpec.processors < minProcessors()) ||
	(queueSpec.processors > maxProcessors())) return false;
  }
  if (queueSpec.runLimit != 0) {
    if (queueSpec.runLimit > runLimit()) return false;
  }
  if (queueSpec.maxJobCPU != 0) {
    if (queueSpec.maxJobCPU > maxJobCPU()) return false;
  }
  if (queueSpec.maxProcessCPU != 0) {
    if (queueSpec.maxProcessCPU > maxProcessCPU()) return false;
  }
  if (queueSpec.fileLimit != 0) {
    if (queueSpec.fileLimit > fileLimit()) return false;
  }
  if (queueSpec.coreLimit != 0) {
    if (queueSpec.coreLimit > coreLimit()) return false;
  }
  if (queueSpec.memLimit != 0) {
    if (queueSpec.memLimit > memLimit()) return false;
  }
  if (queueSpec.scratchLimit != 0) {
    if (queueSpec.scratchLimit > scratchLimit()) return false;
  }
  if (queueSpec.dataLimit != 0) {
    if (queueSpec.dataLimit > dataLimit()) return false;
  }
  if (queueSpec.stackLimit != 0) {
    if (queueSpec.stackLimit > stackLimit()) return false;
  }
  return true;
}


// ------------ Accessors ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Simple Attribute Accessors.
//
///////////////////////////////////////////////////////////////////////////////
string Queue::name(void) const
{ return p_name; }
string Queue::description(void) const
{ return p_description; }
string Queue::administrator(void) const
{ return p_administrator; }
unsigned int Queue::maxSlots(void) const
{ return p_maxSlots; }
string Queue::priority(void) const
{ return p_priority; }
int Queue::nice(void) const
{ return p_nice; }
unsigned int Queue::maxJobCPU(void) const
{ return p_maxJobCPU; }
unsigned int Queue::maxProcessCPU(void) const
{ return p_maxProcessCPU; }
unsigned int Queue::runLimit(void) const
{ return p_runLimit; }
unsigned int Queue::defRun(void) const
{ return p_defRun; }
unsigned int Queue::minProcessors(void) const
{ return p_minProcessors; }
unsigned int Queue::maxProcessors(void) const
{ return p_maxProcessors; }
unsigned int Queue::defProcessors(void) const
{ return p_defProcessors; }
unsigned int Queue::fileLimit(void) const
{ return p_fileLimit; }
unsigned int Queue::coreLimit(void) const
{ return p_coreLimit; }
unsigned int Queue::memLimit(void) const
{ return p_memLimit; }
unsigned int Queue::defMemory(void) const
{ return p_defMemory; }
string Queue::memUnits(void) const
{ return p_memUnits; }
unsigned int Queue::scratchLimit(void) const
{ return p_scratchLimit; }
unsigned int Queue::defScratch(void) const
{ return p_defScratch; }
unsigned int Queue::dataLimit(void) const
{ return p_dataLimit; }
unsigned int Queue::stackLimit(void) const
{ return p_stackLimit; }


// --------------------------
// Protected Member Functions
// --------------------------

// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Set All Values to Unlimited and Default System Values.
//
///////////////////////////////////////////////////////////////////////////////
Queue::Queue(const string& name) : p_name(name),
                                      p_maxSlots(INT_MAX),
                                      p_nice(0),
                                      p_maxJobCPU(INT_MAX),
                                      p_maxProcessCPU(INT_MAX),
                                      p_runLimit(INT_MAX),
                                      p_defRun(0),
                                      p_minProcessors(1),
                                      p_maxProcessors(INT_MAX),
                                      p_defProcessors(0),
                                      p_fileLimit(INT_MAX),
                                      p_coreLimit(INT_MAX),
                                      p_memLimit(INT_MAX),
                                      p_defMemory(0),
                                      p_memUnits(""),
                                      p_scratchLimit(INT_MAX),
                                      p_defScratch(0),
                                      p_dataLimit(INT_MAX),
                                      p_stackLimit(INT_MAX)
{ }

// ---------- Modifiers ------------
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Simple Modifiers Without Validation.
//
//  TODO
//    Validate the administrator.
//
///////////////////////////////////////////////////////////////////////////////
void Queue::description(const string& description)
{ p_description = description; }
void Queue::administrator(const string& administrator)
{ p_administrator = administrator; }
void Queue::maxSlots(unsigned int maxSlots)
{ p_maxSlots = maxSlots; }
void Queue::priority(const string& priority)
{ p_priority = priority; }
void Queue::nice(int nice)
{ p_nice = nice; }
void Queue::maxJobCPU(unsigned int maxJobCPU)
{ p_maxJobCPU = maxJobCPU; }
void Queue::maxProcessCPU(unsigned int maxProcessCPU)
{ p_maxProcessCPU = maxProcessCPU; }
void Queue::runLimit(unsigned int runLimit)
{ p_runLimit = runLimit; }
void Queue::defRun(unsigned int defRun)
{ p_defRun = defRun; }
void Queue::minProcessors(unsigned int minProcessors)
{ p_minProcessors = minProcessors; }
void Queue::maxProcessors(unsigned int maxProcessors)
{ p_maxProcessors = maxProcessors; }
void Queue::defProcessors(unsigned int defProcessors)
{ p_defProcessors = defProcessors; }
void Queue::fileLimit(unsigned int fileLimit)
{ p_fileLimit = fileLimit; }
void Queue::coreLimit(unsigned int coreLimit)
{ p_coreLimit = coreLimit; }
void Queue::memLimit(unsigned int memLimit)
{ p_memLimit = memLimit; }
void Queue::defMemory(unsigned int defMemory)
{ p_defMemory = defMemory; }
void Queue::memUnits(const string& memUnits)
{ p_memUnits = memUnits; }
void Queue::scratchLimit(unsigned int scratchLimit)
{ p_scratchLimit = scratchLimit; }
void Queue::defScratch(unsigned int defScratch)
{ p_defScratch = defScratch; }
void Queue::dataLimit(unsigned int dataLimit)
{ p_dataLimit = dataLimit; }
void Queue::stackLimit(unsigned int stackLimit)
{ p_stackLimit = stackLimit; }

// ------------------------
// Private Member Functions
// ------------------------

// ---------- Friends ----------
///////////////////////////////////////////////////////////////////////////////
// man
//
//  Description
//    Print Only Constraints Specified in the Requested Queue.
//
///////////////////////////////////////////////////////////////////////////////
ostream& operator<<(ostream& os, const Queue& queue)
{
  os << "[Queue<";
  os << "Name = " << queue.name() << ",";
  if (!queue.description().empty()) {
    os << "Description = " << queue.description() << ",";
  }
  if (!queue.administrator().empty()) {
    os << "Administrator = " << queue.administrator() << ",";
  }
  if (queue.maxSlots() != INT_MAX) {
    os << "MaxSlots = " << queue.maxSlots() << ",";
  }
  if (!queue.priority().empty()) {
    os << "Priority = " << queue.priority() << ",";
  }
  os << "Nice = " << queue.nice() << ",";
  if (queue.maxJobCPU() != INT_MAX) {
    os << "MaxJobCPU = " << queue.maxJobCPU() << " seconds,";
  }
  if (queue.maxProcessCPU() != INT_MAX) {
    os << "MaxProcessCPU = " << queue.maxProcessCPU() << "seconds,";
  }
  if (queue.runLimit() != INT_MAX) {
    os << "RunLimit = " << queue.runLimit() << " minutes,";
  }
  if (queue.defRun() != 0) {
    os << "DefRun = " << queue.defRun() << " minutes,";
  }
  os << "MinProcessors = " << queue.minProcessors() << ",";
  if (queue.maxProcessors() != INT_MAX) {
    os << "MaxProcessors = " << queue.maxProcessors() << ",";
  }
  if (queue.defProcessors() != 0) {
    os << "DefProcessors = " << queue.defProcessors() << ",";
  }
  if (queue.fileLimit() != INT_MAX) {
    os << "FileLimit = " << queue.fileLimit() << " K,";
  }
  if (queue.coreLimit() != INT_MAX) {
    os << "CoreLimit = " << queue.coreLimit() << " K,";
  }
  if (queue.memLimit() != INT_MAX) {
    os << "MemLimit = " << queue.memLimit() << ",";
  }
  if (queue.defMemory() != 0) {
    os << "DefMemory = " << queue.defMemory() << ",";
  }
  if (queue.memUnits() != "") {
    os << "MemUnits = " << queue.memUnits() << ",";
  }
  if (queue.scratchLimit() != INT_MAX) {
    os << "ScratchLimit = " << queue.scratchLimit() << ",";
  }
  if (queue.defScratch() != 0) {
    os << "DefScratch = " << queue.defScratch() << ",";
  }
  if (queue.dataLimit() != INT_MAX) {
    os << "DataLimit = " << queue.dataLimit() << " MB,";
  }
  if (queue.stackLimit() != INT_MAX) {
    os << "StackLimit = " << queue.stackLimit() << " MB" << endl;
  }
  os << ">]" << endl;
  return os;
}

// ---------- Encapsulated Behavior ----------
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Fill Attributes for This Queue From Preferences File
//
//  Implementation
//    Preferences file keys are composites of the queue name and
//    attribute name (eg. small|description).
//
///////////////////////////////////////////////////////////////////////////////
void Queue::fillAttributesFrom(Preferences& prefs)
{
  string keyRoot(name());
  keyRoot += "|";
  string stringVal;
  int       intVal;
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "description") << endl;
#endif
  if (prefs.getString(keyRoot + "description", stringVal)) {
    description(stringVal);
  }
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "administrator") << endl;
#endif
  if (prefs.getString(keyRoot + "administrator", stringVal)) {
    administrator(stringVal);
  }
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "maxSlots") << endl;
#endif
  if (prefs.getInt(keyRoot + "maxSlots", intVal)) {
    maxSlots(intVal);
  }
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "priority") << endl;
#endif
  if (prefs.getString(keyRoot + "priority", stringVal)) {
    if (stringVal[stringVal.size()-1] == '~')
      stringVal.erase(stringVal.size()-1,1);
    priority(stringVal);
  }
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "nice") << endl;
#endif
  if (prefs.getInt(keyRoot + "nice", intVal)) {
    nice(intVal);
  }
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "maxJobCPU") << endl;
#endif
  if (prefs.getInt(keyRoot + "maxJobCPU", intVal)) {
    maxJobCPU(intVal);
  } else if (prefs.getString(keyRoot + "maxJobCPU", stringVal)) {
    maxJobCPU(secondsFromString(stringVal));
  }
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "maxProcessCPU") << endl;
#endif
  if (prefs.getInt(keyRoot + "maxProcessCPU", intVal)) {
    maxProcessCPU(intVal);
  } else if (prefs.getString(keyRoot + "maxProcessCPU", stringVal)) {
    maxProcessCPU(secondsFromString(stringVal));
  }
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "runLimit") << endl;
#endif
  if (prefs.getInt(keyRoot + "runLimit", intVal)) {
    runLimit(intVal);
  } else if (prefs.getString(keyRoot + "runLimit", stringVal)) {
    runLimit(minutesFromString(stringVal));
  }
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "defRun") << endl;
#endif
  if (prefs.getInt(keyRoot + "defRun", intVal)) {
    defRun(intVal);
  } else if (prefs.getString(keyRoot + "defRun", stringVal)) {
    defRun(minutesFromString(stringVal));
  }
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "minProcessors") << endl;
#endif
  if (prefs.getInt(keyRoot + "minProcessors", intVal)) {
    minProcessors(intVal);
  }
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "maxProcessors") << endl;
#endif
  if (prefs.getInt(keyRoot + "maxProcessors", intVal)) {
    maxProcessors(intVal);
  }
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "defProcessors") << endl;
#endif
  if (prefs.getInt(keyRoot + "defProcessors", intVal)) {
    defProcessors(intVal);
  }
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "fileLimit") << endl;
#endif
  if (prefs.getInt(keyRoot + "fileLimit", intVal)) {
    fileLimit(intVal);
  }
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "coreLimit") << endl;
#endif
  if (prefs.getInt(keyRoot + "coreLimit", intVal)) {
    coreLimit(intVal);
  }
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "memLimit") << endl;
#endif
  if (prefs.getInt(keyRoot + "memLimit", intVal)) {
    memLimit(intVal);
  }
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "defMemory") << endl;
#endif
  if (prefs.getInt(keyRoot + "defMemory", intVal)) {
    defMemory(intVal);
  }
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "memUnits") << endl;
#endif
  if (prefs.getString(keyRoot + "memUnits", stringVal)) {
    memUnits(stringVal);
  }
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "scratchLimit") << endl;
#endif
  if (prefs.getInt(keyRoot + "scratchLimit", intVal)) {
    scratchLimit(intVal);
  }
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "defScratch") << endl;
#endif
  if (prefs.getInt(keyRoot + "defScratch", intVal)) {
    defScratch(intVal);
  }
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "dataLimit") << endl;
#endif
  if (prefs.getInt(keyRoot + "dataLimit", intVal)) {
    dataLimit(intVal);
  }
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "stackLimit") << endl;
#endif
  if (prefs.getInt(keyRoot + "stackLimit", intVal)) {
    stackLimit(intVal);
  }
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Helper to Convert HHHH:MM:SS to Seconds and Minutes
//
///////////////////////////////////////////////////////////////////////////////
unsigned int Queue::secondsFromString(const string& time) const
{
  int overflow = 0;
  int hours = 0;
  int minutes = 0;
  int seconds = 0;
  StringTokenizer next(time,":");
  string token;
  while (!(token = next.next()).empty()) {
    overflow = hours; hours = minutes; minutes = seconds;
    seconds = atoi(token.c_str());
  }
  EE_RT_ASSERT((overflow == 0) &&
	       ((seconds >= 0) && (seconds < 60)) &&
	       ((minutes >= 0) && (minutes < 60)) &&
	       (hours >= 0), EE_FATAL, "Incorrectly Formatted Time");
  return (seconds + (60 * minutes) + (3600 * hours));
}

//////////////////////////////////////////////////////////////////////////////////////
// Description
//   Format is still hhhh:mm:ss but in this case seconds are just tossed.
//////////////////////////////////////////////////////////////////////////////////////
unsigned int Queue::minutesFromString(const string& time) const
{
  int overflow = 0;
  int hours = 0;
  int minutes = 0;
  int seconds = 0;
  StringTokenizer next(time,":");
  string token;
  while (!(token = next.next()).empty()) {
    overflow = hours; hours = minutes; minutes=seconds;
    seconds = atoi(token.c_str());
  }
  EE_RT_ASSERT((overflow == 0) &&
	       ((minutes >= 0) && (minutes < 60)) &&
	       (hours >= 0), EE_WARNING, time+" - Incorrectly Formatted Time");
  return (minutes + (60 * hours));
}

