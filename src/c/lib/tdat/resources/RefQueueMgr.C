///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: RefQueueManager.C
//
//
// DESIGN:
//   Objects of this class are read from configuration files 
//   Users are only allowed access
//   to attributes.  It is assumed that initialization of this class
//   occurs before it is used.  The objects are only intended to be used
//   transitively.  Inheritance from EXTENT(RefQueueManager) is only used
//   to track the transient objects that are created by
//   RefQueueManager::initialize();
//
///////////////////////////////////////////////////////////////////////////////

// system includes
// general includes
// library includes

// application includes
#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "util/Preferences.H"
#include "tdat/RefQueueMgr.H"

// #define DEBUG

vector<RefQueueManager*> *RefQueueManager::p_extent = 0;

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Virtual Destructor ------------
RefQueueManager::~RefQueueManager(void)
{
  vector<RefQueueManager*>::iterator it = p_extent->begin();
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
//    Value-Based Comparison.  Based on Name.
//
///////////////////////////////////////////////////////////////////////////////
bool RefQueueManager::operator==(const RefQueueManager& refQueueMgr) const
{
  return (name() == refQueueMgr.name());
}
bool RefQueueManager::operator!=(const RefQueueManager& refQueueMgr) const
{ return !((*this) == refQueueMgr); }


// ------------ Modifiers ------------

// ------------ Accessors ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Simple Attribute Accessors.
//
///////////////////////////////////////////////////////////////////////////////
string RefQueueManager::name(void) const
{ return p_name; }
string RefQueueManager::submitCommand(void) const
{ return p_submitCommand; }
string RefQueueManager::cancelCommand(void) const
{ return p_cancelCommand; }
string RefQueueManager::queryJobCommand(void) const
{ return p_queryJobCommand; }
string RefQueueManager::queryMachineCommand(void) const
{ return p_queryMachineCommand; }
string RefQueueManager::queryQueueCommand(void) const
{ return p_queryQueueCommand; }
string RefQueueManager::queryDiskUsageCommand(void) const
{ return p_queryDiskUsageCommand; }
string RefQueueManager::jobIdParseExpression(void) const
{ return p_jobIdParseExpression; }
string RefQueueManager::jobIdParseLeadingText(void) const
{ return p_jobIdParseLeadingText; }
string RefQueueManager::jobIdParseTrailingText(void) const
{ return p_jobIdParseTrailingText; }


// ------------ Extent Requirements and Overrides ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Lookup a RefQueueManager Object from Name.
//    Exact Matches Only are Supported.
//
//  Implementation
//    If only the name is specified then we can use use our inherited
//    lookup function.  
//
///////////////////////////////////////////////////////////////////////////////
const RefQueueManager* RefQueueManager::lookup(const string& name)
{
  initialize();
  RefQueueManager* result = (RefQueueManager*)0;
  RefQueueManager *it;
  for (int idx=p_extent->size()-1; idx>=0; idx--) {
    it = (*p_extent)[idx];
    if (it->name() == name) {
      result = it;
      break;
    }
  }
  return result;
}

// ---------- Initialization  ----------
void RefQueueManager::initialize(void)
{
  if (p_extent == 0) {
    p_extent = new vector<RefQueueManager*>();
    string path = Ecce::ecceHome();
    path.append("/siteconfig/");
    path.append(RefQueueManager::refQMgrLoadFile);
    Preferences prefs(path, true);
    EE_RT_ASSERT(prefs.isValid(), EE_FATAL,
	         "Error!  Must Have a Queue Manager Reference File!");
    vector<string> queueManagers;
    EE_RT_ASSERT(prefs.getStringList(RefQueueManager::refQMgrLoadFile,
				       queueManagers),
	         EE_FATAL,
	         "Illegally Structured Queue Manager Reference File!");
    RefQueueManager *newObject = (RefQueueManager*)0;
    for (unsigned int index = 0; index < queueManagers.size(); index++) {
      string& name = queueManagers[index];
	   newObject = new RefQueueManager(name);
      // Fetch and Fill Attributes
      newObject->fillAttributesFrom(prefs);
    }
  }
}



// --------------------------
// Protected Member Functions
// --------------------------

// ---------- Constructors ------------
RefQueueManager::RefQueueManager(const string& name) :
                                                        p_name(name)
{ 
  initialize();
  p_extent->push_back(this);
}

// ---------- Modifiers ------------
void RefQueueManager::submitCommand(const string& command)
{ p_submitCommand = command; }
void RefQueueManager::cancelCommand(const string& command)
{ p_cancelCommand = command; }
void RefQueueManager::queryJobCommand(const string& command)
{ p_queryJobCommand = command; }
void RefQueueManager::queryMachineCommand(const string& command)
{ p_queryMachineCommand = command; }
void RefQueueManager::queryQueueCommand(const string& command)
{ p_queryQueueCommand = command; }
void RefQueueManager::queryDiskUsageCommand(const string& command)
{ p_queryDiskUsageCommand = command; }
void RefQueueManager::jobIdParseExpression(const string& expression)
{ p_jobIdParseExpression = expression; }
void RefQueueManager::jobIdParseLeadingText(const string& text)
{ p_jobIdParseLeadingText = text; }
void RefQueueManager::jobIdParseTrailingText(const string& text)
{ p_jobIdParseTrailingText = text; }

// ------------------------
// Private Member Functions
// ------------------------

// ---------- Friends ----------
ostream& operator<<(ostream& os, const RefQueueManager& refQueueMgr)
{
  os << "[RefQueueManager<";
  os << refQueueMgr.name() << ",";
  os << refQueueMgr.submitCommand() << ",";
  os << refQueueMgr.cancelCommand() << ",";
  os << refQueueMgr.queryJobCommand() << ",";
  os << refQueueMgr.queryMachineCommand() << ",";
  os << refQueueMgr.queryQueueCommand() << ",";
  os << refQueueMgr.queryDiskUsageCommand() << ",";
  os << refQueueMgr.jobIdParseExpression() << ",";
  os << refQueueMgr.jobIdParseLeadingText() << ",";
  os << refQueueMgr.jobIdParseTrailingText() << ">]" << endl;;
  return os;
}

// ---------- Encapsulated Behavior ----------
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Fill Attributes for This Queue Manager From Preferences File
//
//  Implementation
//    Any missing attribute is a fatal error.
//    Preferences file keys are composites of the name,  and
//    attribute name (eg. LoadLeveler-2.0|submitCommand).
//
///////////////////////////////////////////////////////////////////////////////
void RefQueueManager::fillAttributesFrom(Preferences& prefs)
{
  string keyRoot(name());
  keyRoot += "|";
  string value;
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "submitCommand") << endl;
#endif
  EE_RT_ASSERT(prefs.getString(keyRoot + "submitCommand", value),
	       EE_FATAL, "Couldn't Find Mandatory Key submitCommand");
  submitCommand(value);
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "cancelCommand") << endl;
#endif
  EE_RT_ASSERT(prefs.getString(keyRoot + "cancelCommand", value),
	       EE_FATAL, "Couldn't Find Mandatory Key cancelCommand");
  cancelCommand(value);
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "queryJobCommand") << endl;
#endif
  EE_RT_ASSERT(prefs.getString(keyRoot + "queryJobCommand", value),
	       EE_FATAL, "Couldn't Find Mandatory Key queryJobCommand");
  queryJobCommand(value);
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "queryMachineCommand") << endl;
#endif
  EE_RT_ASSERT(prefs.getString(keyRoot + "queryMachineCommand", value),
	  EE_FATAL, "Couldn't Find Mandatory Key queryMachineCommand");
  queryMachineCommand(value);
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "queryQueueCommand") << endl;
#endif
  EE_RT_ASSERT(prefs.getString(keyRoot + "queryQueueCommand", value),
	       EE_FATAL, "Couldn't Find Mandatory Key queryQueueCommand");
  queryQueueCommand(value);
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "queryDiskUsageCommand") << endl;
#endif
  EE_RT_ASSERT(prefs.getString(keyRoot + "queryDiskUsageCommand", value),
	       EE_FATAL, "Couldn't Find Mandatory Key queryDiskUsageCommand");
  queryDiskUsageCommand(value);
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "jobIdParseExpression") << endl;
#endif
  EE_RT_ASSERT(prefs.getString(keyRoot + "jobIdParseExpression", value),
	       EE_FATAL, "Couldn't Find Mandatory Key jobIdParseExpression");
  jobIdParseExpression(value);
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "jobIdParseLeadingText") << endl;
#endif
  if (prefs.getString(keyRoot + "jobIdParseLeadingText", value))
    jobIdParseLeadingText(value);
  else
    jobIdParseLeadingText("");
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Looking Up " << (keyRoot + "jobIdParseTrailingText") << endl;
#endif
  if (prefs.getString(keyRoot + "jobIdParseTrailingText", value))
    jobIdParseTrailingText(value);
  else
    jobIdParseTrailingText("");
}

const char* RefQueueManager::refQMgrLoadFile = "QueueManagers";

// RefQueueManager for Function Headers
///////////////////////////////////////////////////////////////////////////////
///  man
//
//  Description
//	<description of semantics, uses>
//
//  Implementation
//	<implementation comments>
//
///////////////////////////////////////////////////////////////////////////////
//<return type> <class-name>::<function-name>(
//<type1 var1>,
//<type2 var2>,
//) const (or not)
//{
//}
