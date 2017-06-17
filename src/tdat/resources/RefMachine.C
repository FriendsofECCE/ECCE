///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: RefMachine.C
//
//
// DESIGN:
//    This class is designed to be a holder for all known EMSL
//    machines.  The information maintained can be used to setup
//    and launch calculations.
//
//    The RefMachine class has two purposes: 1. Transiently load the list of
//    known machines from a text file and 2. Allow creation of validated,
//    storable RefMachine objects for calculations.
//
///////////////////////////////////////////////////////////////////////////////

#include <unistd.h>    // access
#include <sys/utsname.h> // uname
#include <netdb.h>    // gethostbyname
#include <string.h>

#include <algorithm>
using std::find;

    
#include "util/Ecce.H"
#include "util/SFile.H"
#include "util/EcceMap.H"
#include "util/KeyValueReader.H"
#include "util/ErrMsg.H"
#include "util/LineReader.H"
#include "util/StringTokenizer.H"
#include "util/StringConverter.H"

#include "tdat/RefMachine.H"
#include "tdat/QueueMgr.H"
#include "tdat/Queue.H"

static vector<string>       *s_refname_list=0;
static vector<string>       *s_fullname_list=0;
static vector<string>       *s_vendor_list=0;
static vector<string>       *s_model_list=0;
static vector<string>       *s_proctype_list=0;
static vector<unsigned int> *s_nodes_list=0;
static vector<unsigned int> *s_proccount_list=0;
static vector<string>       *s_remshells_list=0;
static vector<string>       *s_codes_list=0;
static vector<string>       *s_options_list=0;

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
    // This is A Reference Data Class - No Public Constructors!

// ---------- Virtual Destructor ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Destroy a RefMachine Object
//
//  Implementation
//    strings are destroyed by their destructors.
//
///////////////////////////////////////////////////////////////////////////////
RefMachine::~RefMachine(void)
{ }

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//  Clean Up Transient Memory Created by RefMachine::initialize()
//
//  Implementation
//
///////////////////////////////////////////////////////////////////////////////
void RefMachine::finalize(void)
{
  if (s_refname_list != 0) {
    s_refname_list->clear();
    delete s_refname_list;
  }
  s_refname_list = 0;

  if (s_fullname_list != 0) {
    s_fullname_list->clear();
    delete s_fullname_list;
  }
  s_fullname_list = 0;

  if (s_vendor_list != 0) {
    s_vendor_list->clear();
    delete s_vendor_list;
  }
  s_vendor_list = 0;

  if (s_model_list != 0) {
    s_model_list->clear();
    delete s_model_list;
  }
  s_model_list = 0;

  if (s_proctype_list != 0) {
    s_proctype_list->clear();
    delete s_proctype_list;
  }
  s_proctype_list = 0;

  if (s_nodes_list != 0) {
    s_nodes_list->clear();
    delete s_nodes_list;
  }
  s_nodes_list = 0;

  if (s_proccount_list != 0) {
    s_proccount_list->clear();
    delete s_proccount_list;
  }
  s_proccount_list = 0;

  if (s_remshells_list != 0) {
    s_remshells_list->clear();
    delete s_remshells_list;
  }
  s_remshells_list = 0;

  if (s_codes_list != 0) {
    s_codes_list->clear();
    delete s_codes_list;
  }
  s_codes_list = 0;

  if (s_options_list != 0) {
    s_options_list->clear();
    delete s_options_list;
  }
  s_options_list = 0;
}

// ---------- Operators ----------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Identity-based Equality and Inequality
//
///////////////////////////////////////////////////////////////////////////////
bool RefMachine::operator==(const RefMachine& refMachine) const
{ return (this == &refMachine); }
bool RefMachine::operator!=(const RefMachine& refMachine) const
{ return (this != &refMachine); }

// ------------ Modifiers ------------
    // This is A Reference Data Class - No Public Modifiers!

// ------------ Accessors ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Simple Accessors that Return Results by Value
//
///////////////////////////////////////////////////////////////////////////////
string RefMachine::refname(void) const
{ return p_refname; }
string RefMachine::fullname(void) const
{ return p_fullname; }
string RefMachine::vendor(void) const
{ return p_vendor; }
string RefMachine::model(void) const
{ return p_model; }
string RefMachine::proctype(void) const
{ return p_proctype; }
unsigned int RefMachine::nodes(void) const
{ return p_nodes; }
unsigned int RefMachine::proccount(void) const
{ return p_proccount; }
string RefMachine::remshellsString(void) const
{ return p_remshells; }
string RefMachine::codesString(void) const
{ return p_codes; }


string RefMachine::configFile(const string& refname)
{
  string configName = Ecce::ecceHome();
  configName += "/siteconfig/CONFIG." + refname;

  SFile testfile1(configName.c_str());
  if (!testfile1.exists()) {
    configName = Ecce::realUserPrefPath();
    configName += "CONFIG." + refname;

    SFile testfile2(configName.c_str());
    if (!testfile2.exists())
      configName = "";
  }
  return configName;
}

string RefMachine::exePath(const string& code, const string& refname,
                           const string& vendor, const string& model)
{
  string ret = "";
  KeyValueReader reader1(RefMachine::configFile(refname));
  string key, value;
  while (reader1.getpair(key,value)) {
    if (key == code) {
      ret = value;
      break;
    }
  }

  if (ret=="" && vendor!="") {
    string tryAgain = vendor;
    if (model != "")
      tryAgain += "." + model;

    // because of how full domain names are separated by dots the same
    // as vendors and models of machines the same method can be used to
    // return an config file for the vendor and model 
    string tryUpper;
    (void)StringConverter::toUpper(tryAgain, tryUpper);
    KeyValueReader reader2(RefMachine::configFile(tryUpper));
    while (reader2.getpair(key,value)) {
      if (key == code) {
        ret = value;
        break;
      }
    }
  }

  return ret;
}

string RefMachine::shellPath(void) const
{
  string path = "/bin:/usr/sbin:/sbin:/usr/X11R6/bin:/usr/bin/X11";

  string configName = RefMachine::configFile(refname());

  EcceMap *config = EcceMap::load(configName);

  string xdir, pdir, qdir;
  if (config->findValue("xappsPath", xdir))
    path.insert(0, xdir + ":");

  if (config->findValue("perlPath", pdir))
    path.insert(0, pdir + ":");

  if (config->findValue("qmgrPath", qdir))
    path.insert(0, qdir + ":");

  delete config;

  return path;
}

string RefMachine::libPath(void) const
{
  string path = "";

  string configName = RefMachine::configFile(refname());

  EcceMap *config = EcceMap::load(configName);

  string libdir;
  if (config->findValue("libPath", libdir))
    path = libdir;
  delete config;

  return path;
}

string RefMachine::sourceFile(void) const
{
  string file = "";

  string configName = RefMachine::configFile(refname());

  EcceMap *config = EcceMap::load(configName);

  string source;
  if (config->findValue("sourceFile", source))
    file = source;
  delete config;

  return file;
}

string RefMachine::shell(void) const
{
  string shell = "csh";

  string configName = RefMachine::configFile(refname());

  EcceMap *config = EcceMap::load(configName);

  string tmp;
  if (config->findValue("shell", tmp))
    shell = tmp;
  delete config;

  return shell;
}

string RefMachine::globusContact(void) const
{
  string contact = "";

  string configName = RefMachine::configFile(refname());

  EcceMap *config = EcceMap::load(configName);

  string tmp;
  if (config->findValue("globusContact", tmp))
    contact = tmp;
  delete config;

  return contact;
}

string RefMachine::frontendMachine(void) const
{
  string machine = "";

  string configName = RefMachine::configFile(refname());

  EcceMap *config = EcceMap::load(configName);

  string front;
  if (config->findValue("frontendMachine", front))
    machine = front;
  delete config;

  return machine;
}

string RefMachine::frontendBypass(void) const
{
  string bypass = "";

  string configName = RefMachine::configFile(refname());

  EcceMap *config = EcceMap::load(configName);

  string front;
  if (config->findValue("frontendBypass", front))
    bypass = front;
  delete config;

  return bypass;
}


bool RefMachine::isSameDomain(void) const
{
  bool sameDomain = false;

  string whereami = "";
  struct utsname _uname;
  if (uname(&_uname) != -1)
    whereami = _uname.nodename;

  struct hostent* host = gethostbyname(whereami.c_str());
  if (host != NULL) {
    string mymachine = host->h_name;

    char* mymachinestr = strdup((char*)mymachine.c_str());
    char* machinestr = strdup((char*)fullname().c_str());

    // strange little bit of logic to compare the last two dot-separated
    // parts of machine and mymachine
    char* domstr = strrchr(machinestr, '.');
    if (domstr != NULL) {
      domstr--;
      while (domstr!=NULL && *domstr!='.' && domstr!=machinestr)
        domstr--;
      if (domstr != NULL) {
        char* mydomstr = strrchr(mymachinestr, '.');
        if (mydomstr != NULL) {
          mydomstr--;
          while (mydomstr!=NULL && *mydomstr!='.' && mydomstr!=mymachinestr)
            mydomstr--;
          if (mydomstr != NULL)
            sameDomain = strcmp(domstr, mydomstr)==0;
        }
      }
    }

    free(mymachinestr);
    free(machinestr);
  }

  return sameDomain;
}


bool RefMachine::singleConnect(void) const
{
  bool single = false;

  string configName = RefMachine::configFile(refname());
  EcceMap *config = EcceMap::load(configName);

  string connect;
  if (config->findValue("singleConnect", connect)) {
    if (connect=="true" || connect=="TRUE" || connect=="True" ||
        connect=="yes" || connect=="YES" || connect=="Yes")
      single = true;
    else if (connect=="false" || connect=="FALSE" || connect=="False" ||
        connect=="no" || connect=="NO" || connect=="No")
      single = false;
    else {
      single = !RefMachine::isSameDomain();
    }
  }
  delete config;

  return single;
}


bool RefMachine::checkScratch(void) const
{
  bool check = true;

  string configName = RefMachine::configFile(refname());

  EcceMap *config = EcceMap::load(configName);

  string scratch;
  if (config->findValue("checkScratch", scratch)) {
    if (scratch=="false" || scratch=="FALSE" || scratch=="False" ||
        scratch=="no" || scratch=="NO" || scratch=="No")
      check = false;
  }
  delete config;

  return check;
}


bool RefMachine::userSubmit(void) const
{
  bool user = false;

  string configName = RefMachine::configFile(refname());

  EcceMap *config = EcceMap::load(configName);

  string submit;
  if (config->findValue("userSubmit", submit)) {
    if (submit=="true" || submit=="TRUE" || submit=="True" ||
        submit=="yes" || submit=="YES" || submit=="Yes")
      user = true;
  }
  delete config;

  return user;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Accessor for list of queues.
//    Caller is responsible for deleting memory
//    Caller Must Check for Null Return Value.
//  Implementation
//    Request the queue list of names from the QueueManager class based
//    upon the machine name for the RefMachine instance.
//
///////////////////////////////////////////////////////////////////////////////
vector<string*>* RefMachine::queues(void) const
{
  vector<string*>* result = (vector<string*>*)0;
  const QueueManager* queueMgr = QueueManager::lookup(p_refname.c_str());
  if (queueMgr != (const QueueManager*)0) {
    ESQHDict* queues = (ESQHDict*)queueMgr->queues();
    if (queues!=(ESQHDict*)0 && queues->size()>0) {
      result =  new vector<string*>();
      ESQHDictIter iter = queues->begin();
      while (iter != queues->end()) {
        result->push_back(new string((*iter).second->name()));
        iter++;
      }
    }
  }
  return result;
}

string RefMachine::launchOptions(void) const
{
  return p_options;
}

vector<string>* RefMachine::remshells(void) const
{
  vector<string>* result = 
                       new vector<string>();
  string remshells = p_remshells;
  string token;
  StringTokenizer next(remshells);
  while (!(token = next.next(":")).empty())
    result->push_back(token);

  string siteShellFile = Ecce::ecceHome();
  siteShellFile += "/siteconfig/remote_shells.site";

  if (access(siteShellFile.c_str(), F_OK) == 0) {
    ifstream is(siteShellFile.c_str());
    char buf[256];
    char* tokptr;
    while (!is.eof()) {
      is.getline(buf, 255);
      if (buf[0]!='\0' && buf[0]!='#') {
        tokptr = strtok(buf, ":");
        if (tokptr != NULL)
          result->push_back(tokptr);
      }
    }
    is.close();
  }

  return result;
}

vector<string>* RefMachine::codes(void) const
{
  vector<string>* result = new vector<string>();
  string codes = p_codes;
  string token;
  StringTokenizer next(codes, ":");
  while (!(token = next.next()).empty())
    result->push_back(token);

  return result;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Check to See that the Values Are Part of the Known RefMachine.
//
///////////////////////////////////////////////////////////////////////////////
bool RefMachine::hasRemShell(const string& remShell) const
{
  bool result = false;
  vector<string>* remoteShells = remshells();
  if (remoteShells != (vector<string>*)0) {
    vector<string>::iterator it;
    it = find(remoteShells->begin(), remoteShells->end(), remShell);
    if (it != remoteShells->end()) result = true;
  }
  return result;
}

bool RefMachine::hasCode(const string& code) const
{
  bool result = false;
  vector<string>* codies = codes();
  if (codies != (vector<string>*)0) {
    vector<string>::iterator it;
    it = find(codies->begin(), codies->end(), code);
    if (it != codies->end()) result = true;
  }
  return result;
}

bool RefMachine::hasQueue(const string& queue) const
{
  bool result = false;
  vector<string*>* refQueues= queues();
  if (refQueues != (vector<string*>*)0) {
    vector<string*>::iterator it;
    it = find(refQueues->begin(), refQueues->end(), (string*)&queue);
    if (it != refQueues->end()) result = true;
  }
  return result;
}

// --------------------------
// Protected Member Functions
// --------------------------

// ---------- Encapsulated Behavior ----------
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Private Default Constructor Used During Loading.
//
///////////////////////////////////////////////////////////////////////////////
RefMachine::RefMachine(void)
{ }

RefMachine::RefMachine(const string& refname)
{
  enforce_initialization();
  validate_and_assign(refname);
}

RefMachine::RefMachine(const RefMachine& refMachine)
{
  enforce_initialization();
  validate_and_assign(refMachine.refname());
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Value-Based Assignment Operator
//
///////////////////////////////////////////////////////////////////////////////
RefMachine& RefMachine::operator=(const RefMachine& refMachine)
{
  refname(refMachine.refname().c_str());
  fullname(refMachine.fullname().c_str());
  vendor(refMachine.vendor().c_str());
  model(refMachine.model().c_str());
  proctype(refMachine.proctype().c_str());
  nodes(refMachine.nodes());
  proccount(refMachine.proccount());
  remshells(refMachine.remshellsString().c_str());
  codes(refMachine.codesString().c_str());

  return (*this);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Private Modifiers Used During Loading.
//
///////////////////////////////////////////////////////////////////////////////
void RefMachine::refname(const char* refname)
{ p_refname = refname; }
void RefMachine::fullname(const char* fullname)
{ p_fullname = fullname; }
void RefMachine::vendor(const char* vendor)
{ p_vendor = vendor; }
void RefMachine::model(const char* model)
{ p_model = model; }
void RefMachine::proctype(const char* proctype)
{ p_proctype = proctype; }
void RefMachine::nodes(unsigned int nodes)
{ p_nodes = nodes; }
void RefMachine::proccount(unsigned int value)
{ p_proccount = value; }
void RefMachine::remshells(const char* remshells)
{
  if (strcmp(remshells, "na") != 0)
    p_remshells = remshells;
}
void RefMachine::codes(const char* codes)
{
  if (strcmp(codes, "na") != 0)
    p_codes = codes;
}


RefMachine* RefMachine::fullLookup(const string& fullname)
{
  enforce_initialization();

  int idx;
  for (idx=0; idx<s_fullname_list->size() &&
              (*s_fullname_list)[idx]!=fullname; idx++);

  RefMachine *tempRefMachine = (RefMachine*)0;
  if (idx < s_fullname_list->size()) {
    tempRefMachine = new RefMachine((*s_refname_list)[idx]);
  }

  return tempRefMachine;
}


RefMachine* RefMachine::abbrevLookup(const string& abbrevname)
{
  enforce_initialization();

  int idx = 0;
  for (; idx<s_fullname_list->size() && !((*s_fullname_list)[idx]==abbrevname ||
         ((*s_fullname_list)[idx].length()>abbrevname.length() &&
          (*s_fullname_list)[idx].substr(0,abbrevname.length())==abbrevname &&
          (*s_fullname_list)[idx][abbrevname.length()]=='.')); idx++);

  RefMachine *tempRefMachine = (RefMachine*)0;
  if (idx < s_fullname_list->size())
    tempRefMachine = new RefMachine((*s_refname_list)[idx]);

  return tempRefMachine;
}


RefMachine* RefMachine::refLookup(const string& refname)
{
  enforce_initialization();

  int idx;
  for (idx=0; idx<s_refname_list->size() &&
              (*s_refname_list)[idx]!=refname; idx++);

  RefMachine *tempRefMachine = (RefMachine*)0;
  if (idx < s_refname_list->size()) {
    tempRefMachine = new RefMachine(refname);
  } else {
    for (idx=0; idx<s_fullname_list->size() &&
                (*s_fullname_list)[idx]!=refname; idx++);

    if (idx < s_fullname_list->size()) {
      tempRefMachine = new RefMachine((*s_refname_list)[idx]);
    }
  }

  return tempRefMachine;
}

vector<string>* RefMachine::referenceNames(const machineContextEnum& context)
{
  vector<string> *result;

  enforce_initialization(context);

  result = new vector<string>();
  *result = *s_refname_list;

  return result;
}

// ------------------------
// Private Member Functions
// ------------------------

void RefMachine::parseFile(ifstream& inFile)
{
  LineReader reader(inFile);
  string rwline, token;
  bool found;
  int pos;

  while (reader.getLine(rwline)) {
    StringTokenizer next(rwline,"\t");

    // Name
    token = next.next();
    EE_RT_ASSERT(!token.empty(),EE_FATAL,rwline);
    found = false;
    if (s_refname_list != (vector<string>*)0) {
      vector<string>::iterator it;
      it = find(s_refname_list->begin(), s_refname_list->end(), token);
      if (it != s_refname_list->end()) found = true;
    }

    if (!found) {
      s_refname_list->push_back(token);

      // Machine
      token = next.next();
      EE_RT_ASSERT(!token.empty(),EE_FATAL,rwline);
      s_fullname_list->push_back(token);

      // Platform (vendor, model, processor)
      token = next.next();
      EE_RT_ASSERT(!token.empty(),EE_FATAL,rwline);
      s_vendor_list->push_back(token);

      token = next.next();
      EE_RT_ASSERT(!token.empty(),EE_FATAL,rwline);
      s_model_list->push_back(token);

      token = next.next();
      EE_RT_ASSERT(!token.empty(),EE_FATAL,rwline);
      s_proctype_list->push_back(token);

    //  The field within the Machines (or MyMachines) file is of the form
    //  procs:nodes.  The value of procs per node is calculated as needed
    //  (by seperate program entities) from procs and nodes.

      token = next.next();
      EE_RT_ASSERT(!token.empty(),EE_FATAL,rwline);
      pos = token.find(":");

        if (pos != string::npos)
        {
            string s1 = token.substr(0, pos);
            string s2 = token.substr(pos + 1);

            s_proccount_list->push_back((unsigned int)atoi(s1.c_str()));
            s_nodes_list->push_back((unsigned int)atoi(s2.c_str()));
        }
        else
        {
            s_proccount_list->push_back((unsigned int)atoi(token.c_str()));
            s_nodes_list->push_back(1);
        }


      // Remote Shell Support
      token = next.next();
      EE_RT_ASSERT(!token.empty(),EE_FATAL,rwline);
      s_remshells_list->push_back(token);

      // Computational Codes Available on Machine
      token = next.next();
      EE_RT_ASSERT(!token.empty(),EE_FATAL,rwline);
      s_codes_list->push_back(token);

      // Applicable launch options
      token = next.next();
      EE_RT_ASSERT(!token.empty(),EE_WARNING,rwline);
      s_options_list->push_back(token);
    }
  }

  inFile.close();
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//  Creates a Single Transient Instance of the RefMachine Class
//  May produce errors and returns status
//
//  Implementation
//  The reference machine data is loaded from a text file.  It is not
//  harmful to initalize more than once.
//
///////////////////////////////////////////////////////////////////////////////
bool RefMachine::initialize(const machineContextEnum& context)
{
  bool ret = false;

  if (s_refname_list != (vector<string>*)0) {
    s_refname_list->clear(); delete s_refname_list;
    s_fullname_list->clear(); delete s_fullname_list;
    s_vendor_list->clear(); delete s_vendor_list;
    s_model_list->clear(); delete s_model_list;
    s_proctype_list->clear(); delete s_proctype_list;
    s_nodes_list->clear(); delete s_nodes_list;
    s_proccount_list->clear(); delete s_proccount_list;
    s_remshells_list->clear(); delete s_remshells_list;
    s_codes_list->clear(); delete s_codes_list;
    s_options_list->clear(); delete s_options_list;
  }

  s_refname_list = new vector<string>();
  s_fullname_list = new vector<string>();
  s_vendor_list = new vector<string>();
  s_model_list = new vector<string>();
  s_proctype_list = new vector<string>();
  s_nodes_list = new vector<unsigned int>();
  s_proccount_list =new vector<unsigned int>();
  s_remshells_list = new vector<string>();
  s_codes_list = new vector<string>();
  s_options_list = new vector<string>();

  string fullFileName;
  if (context != RefMachine::siteMachines) {
    fullFileName = string(Ecce::realUserPrefPath()) + "MyMachines";
    ifstream userFile(fullFileName.c_str());

    if (userFile) {
      ret = true;
      parseFile(userFile);
    }
  }

  if (context != RefMachine::userMachines) {
    fullFileName = string(Ecce::ecceHome()) + "/siteconfig/Machines";
    ifstream siteFile(fullFileName.c_str());
    EE_RT_ASSERT(siteFile, EE_FATAL, "Unable to load Machine information");

    if (siteFile) {
      ret = true;
      parseFile(siteFile);
    }
  }

  return ret;
}


///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//  Validates the new machine against the reference data set.  If
//  the name is not valid, EE_INVALID_MACHINE is put on the queue
//  and the state is not changed.
//
///////////////////////////////////////////////////////////////////////////////
bool RefMachine::validate_and_assign(const string& refname)
{
  bool ret = false;

  int idx;
  for (idx=0; idx<s_refname_list->size() &&
              (*s_refname_list)[idx]!=refname;idx++);

  if (idx < s_refname_list->size()) {
    p_refname = refname;
    p_fullname = (*s_fullname_list)[idx];
    p_vendor = (*s_vendor_list)[idx];
    p_model = (*s_model_list)[idx];
    p_proctype = (*s_proctype_list)[idx];
    p_nodes = (*s_nodes_list)[idx];
    p_proccount = (*s_proccount_list)[idx];
    p_remshells = (*s_remshells_list)[idx];
    p_codes = (*s_codes_list)[idx];
    p_options = (*s_options_list)[idx];

    ret = true;
  }
  else {
    ErrMsg().message("EE_UNKNOWN_MACHINE", EE_WARNING, WHERE, refname.c_str());
  }

  return ret;
}


///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Guarantee the Initialization of Our Transient Static Dictionary
//
///////////////////////////////////////////////////////////////////////////////
void RefMachine::enforce_initialization(const machineContextEnum& context)
{
  if (s_refname_list==(vector<string>*)0 ||
      p_markUpdateFlag) {
    RefMachine::initialize(context);
    p_markUpdateFlag = false;
  }
}

void RefMachine::markUpdateUserPrefs(void)
{
  p_markUpdateFlag = true;
}


// ---------- Friends ----------
ostream& operator<<(ostream& os, const RefMachine& refMachine)
{
  os << "[RefMachine<";
  os << refMachine.refname() << ",";
  os << "," << refMachine.refname();
  os << "," << refMachine.fullname();
  os << "," << refMachine.vendor();
  os << "," << refMachine.model();
  os << "," << refMachine.proctype();
  os << "," << refMachine.nodes();
  os << "," << refMachine.proccount();

  vector<string>* list = refMachine.remshells();
  unsigned int index;
  os << endl << "<";
  for (index = 0; index < list->size(); index++) {
    os << (*list)[index] << ",";
  }
  os << ">";

  const vector<string*>* queuelist = refMachine.queues();
  if (queuelist != 0) {
    os << endl << "<";
    for (index = 0; index < queuelist->size(); index++) {
      os << *(*queuelist)[index] << ",";
    }
    os << ">";
  }

  vector<string>* list2 = refMachine.codes();
  os << endl << "<";
  for (index = 0; index < list2->size(); index++) {
    os << (*list2)[index] << ",";
  }
  os << ">";

  os << ">]" << endl;
  return os;
}

bool RefMachine::p_markUpdateFlag=false;
