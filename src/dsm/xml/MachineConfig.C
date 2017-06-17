///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: MachineConfig.C
//
//
// DESIGN:
//    The MachineConfig class is intended to hold persistent information
//    about the conditions under which a computation (job) was run.  It
//    tracks directories, remote user login information, and remote
//    job queue information.  In combination with Job and Machine objects,
//    MachineConfig objects will completely describe where to locate files.
//    MachineConfig objects are also crucial for describing user information
//    that is needed to remotely launch jobs and gather remote machine
//    statistics.
//
//    MachineConfig objects are also used to hold user preferences for
//    remote machines.  A collection of MachineConfig objects are created,
//    managed, and accessed through Machine class static functions.
//
//    Attributes of MachineConfig objects can be validated against the
//    known Machine parameters (read from Machines data file).  This keeps
//    users from accidently specifying an illegal scratch directory
//    or queue name.
//
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <iomanip>
  using std::ios;
#include <fstream>
    using std::ifstream;
    using std::ofstream;

#include "util/Ecce.H"
#include "util/Host.H"
#include "util/ErrMsg.H"
#include "util/KeyValueReader.H"
#include "util/EcceRegexp.H"

#include "tdat/RefMachine.H"

#include "dsm/MachineConfig.H"
#include "dsm/MachinePreferences.H"

// Public Member Functions
// -----------------------

// ---------- Constructors ------------

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Create a MachineConfig Object From Explicit Values.
//
//  Implementation
//    Since this is called by local functions (and only loadUserPrefs),
//    we don't need to validate the values.
//
///////////////////////////////////////////////////////////////////////////////

MachineConfig::MachineConfig(const string& machineRefName,
                            const string& remoteShell,
                            const string& remoteDir,
                            const string& scratchDir,
                            const string& queue,
                            const string& allocAcct,
                            const string& userName) : p_refname(machineRefName),
                                                      p_remoteShell(remoteShell),
                                                      p_remoteDir(remoteDir),
                                                      p_scratchDir(scratchDir),
                                                      p_queue(queue),
                                                      p_allocAcct(allocAcct),
                                                      p_userName(userName)
{
}


MachineConfig::MachineConfig(const string& refname) : p_refname(refname),
                                                    p_remoteShell(""),
                                                    p_remoteDir(""),
                                                    p_scratchDir(""),
                                                    p_queue(""),
                                                    p_allocAcct(""),
                                                    p_userName("")
{
}

// ---------- Virtual Destructor ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Destroy a MachineConfig Object.
//
//  Implementation
//    strings are destroyed by their destructors.
//
///////////////////////////////////////////////////////////////////////////////
MachineConfig::~MachineConfig(void)
{ }


///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Identity-based Equality and Inequality.
//
///////////////////////////////////////////////////////////////////////////////
bool MachineConfig::operator==(const MachineConfig& machineConfig) const
{  return (this == &machineConfig); }
bool MachineConfig::operator!=(const MachineConfig& machineConfig) const
{  return (this != &machineConfig); }

// ------------ Accessors ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Simple Accessors that Return Results by Value.
//
///////////////////////////////////////////////////////////////////////////////
string MachineConfig::machineRefName(void) const
{ return p_refname; }
string MachineConfig::remoteShell(void) const
{ return p_remoteShell; }
string MachineConfig::remoteDir(void) const
{ return p_remoteDir; }
string MachineConfig::scratchDir(void) const
{ return p_scratchDir; }
string MachineConfig::queue(void) const
{ return p_queue; }
string MachineConfig::allocAcct(void) const
{ return p_allocAcct; }
string MachineConfig::userName(void) const
{ return p_userName; }

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Reload Users Machine Configuration Preferences - Replacing Any
//    Changes Made Since the Last Save.
//
///////////////////////////////////////////////////////////////////////////////
const int bufferSize = 128;

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Write Users Machine Configuration Preferences to Preferences File.
//
//  Implementation
//    Clear out the old preferences.
//    Iterate through our dictionary and insert new preferences into
//    the Preference object.
//    Invoke "save_file" to write the new preferences.
//
//  TODO
//    Fix the preferences bug that adds an extra "\" (allow this to
//    be set explicitly).
//
///////////////////////////////////////////////////////////////////////////////
void MachineConfig::saveUserPrefs(void)
{
    MachinePreferences::saveChanges();
}


    ///////////////////////////////////////////////////////////////////////////////
    //  man
    //
    //  Description
    //    Lookup the Registered User Preferences for the Specified Machine.
    //    Caller Must Check for Null Return Value.
    //
    ///////////////////////////////////////////////////////////////////////////////

MachineConfig *MachineConfig::userPref(const string& machineRefName)
{
    MachineConfig *cnfg = NULL;

    MachinePreferences::refresh(p_markUpdateFlag);
    MachinePreferences *prefs = MachinePreferences::lookup(machineRefName);

    if (prefs != NULL)
    {
        //  Need to create the config object, and initialize it from
        //  info in the MachinePreferences object just returne.  But
        //  cannot call its modifiers, because these will call
        //  analogous methods on the MachinePreferences.

        string remShell = prefs->getRemoteShell();
        string remDrcty = prefs->getRemoteDirectory();
        string scratchDrcty = prefs->getScratchDirectory();
        string queueName = prefs->getQueueName();
        string allocAcct = prefs->getAllocationAccount();
        string username = prefs->getUsername();

        cnfg = new MachineConfig(machineRefName, remShell, remDrcty,
                                 scratchDrcty, queueName, allocAcct,
                                 username);
    }

    return cnfg;
}


vector<string> MachineConfig::getConfiguredMachineNames()
{
    vector<string> names;

    MachinePreferences::refresh(p_markUpdateFlag);
    int numMachs = MachinePreferences::countItems();

    if (numMachs > 0)
    {
        //  Retrieve the user preferences for each registered machine
        for (int i = 0; i < numMachs; i++)
        {
            MachinePreferences *prefs = MachinePreferences::getItemAt(i);
            RefMachine *rgstn = prefs->getRegisteredMachine();

            if (rgstn != NULL)
            {
                names.push_back(rgstn->refname());
            }
        }
    }

    return names;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Add a New User Machine Configuration.
//    The Machine Configuration will be Copied from the Argument.
//    If the Machine is Already in the Preferences, the Previous Value
//    Will be Overwritten.
//
///////////////////////////////////////////////////////////////////////////////
void MachineConfig::addUserPref(const MachineConfig& machineConf)
{
    MachinePreferences::refresh(p_markUpdateFlag);

    MachinePreferences *prefs
        = MachinePreferences::create(machineConf.machineRefName());

    prefs->setRemoteShell(machineConf.remoteShell());
    prefs->setRemoteDirectory(machineConf.remoteDir());
    prefs->setScratchDirectory(machineConf.scratchDir());
    prefs->setQueueName(machineConf.queue());
    prefs->setAllocationAccount(machineConf.allocAcct());
    prefs->setUsername(machineConf.userName());

    MachinePreferences::saveChanges();
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Remove a User Machine Configuration for Specified Machine Name.
//
///////////////////////////////////////////////////////////////////////////////
void MachineConfig::removeUserPref(const string& machineRefName)
{
    MachinePreferences::refresh(p_markUpdateFlag);
    bool done = MachinePreferences::remove(machineRefName);

    if (done)
        MachinePreferences::saveChanges();
}

// --------------------------
// Protected Member Functions
// --------------------------

// ---------- Encapsulated Behavior ----------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Create a MachineConfig Object From Explicit Values.
//
//  Implementation
//    Since this is called by local functions (and only loadUserPrefs),
//    we don't need to validate the values.
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Copy The Entire State of the MachineConfig Argument.
//
//  Implementation
//    Use the assignment operator to do the copy.
//
///////////////////////////////////////////////////////////////////////////////
MachineConfig::MachineConfig(const MachineConfig& machineConfig)
{ (*this) = machineConfig; }

// ---------- Operators ----------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Value-Based Assignment Operator.
//
//  Implementation
//    We rely on modifiers to change attributes (which will validate
//    the scratchDir and queue arguments).
//
///////////////////////////////////////////////////////////////////////////////
MachineConfig& MachineConfig::operator=(const MachineConfig& machineConfig)
{
    machineRefName(machineConfig.machineRefName());
    remoteShell(machineConfig.remoteShell());
    remoteDir(machineConfig.remoteDir());
    scratchDir(machineConfig.scratchDir());
    queue(machineConfig.queue());
    allocAcct(machineConfig.allocAcct());
    userName(machineConfig.userName());
    return (*this);
}

// ------------ Modifiers ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Simple Attribute Modifiers.
//    Modification of scratchDir is Validated.
//    An Empty Scratch Directory Indicates the Job Should be Run in the
//    Remote (or Local Main Project) Directory.
//
//  Modifications for version 4.0 include synchronization with the
//  MachinePreferences object, if it exists with the same machineRefName
//  as this.
///////////////////////////////////////////////////////////////////////////////
void MachineConfig::machineRefName(const string& machineRefName)
{
    p_refname = machineRefName;
}

void MachineConfig::remoteShell(const string& remoteShell)
{
    p_remoteShell = remoteShell;

    MachinePreferences *prefs = MachinePreferences::lookup(p_refname);

    if (prefs != NULL) {
        prefs->setRemoteShell(remoteShell);
    }
}


void MachineConfig::remoteDir(const string& remoteDir)
{
    p_remoteDir = remoteDir;

    MachinePreferences *prefs = MachinePreferences::lookup(p_refname);

    if (prefs != NULL) {
        prefs->setRemoteDirectory(remoteDir);
    }
}


void MachineConfig::scratchDir(const string& scratchDir)
{
    p_scratchDir = scratchDir;

    MachinePreferences *prefs = MachinePreferences::lookup(p_refname);

    if (prefs != NULL) {
        prefs->setScratchDirectory(scratchDir);
    }
}


void MachineConfig::queue(const string& queue)
{
    p_queue = queue;

    MachinePreferences *prefs = MachinePreferences::lookup(p_refname);

    if (prefs != NULL) {
        prefs->setQueueName(queue);
    }
}


void MachineConfig::allocAcct(const string& allocAcct)
{
    p_allocAcct = allocAcct;

    MachinePreferences *prefs = MachinePreferences::lookup(p_refname);

    if (prefs != NULL) {
        prefs->setAllocationAccount(allocAcct);
    }
}


void MachineConfig::userName(const string& userName)
{
    p_userName = userName;

    MachinePreferences *prefs = MachinePreferences::lookup(p_refname);

    if (prefs != NULL) {
        prefs->setUsername(userName);
    }
}


// ------------------------
// Private Member Functions
// ------------------------

// ---------- Encapsulated Behavior ----------
void MachineConfig::markUpdateUserPrefs(void)
{
       p_markUpdateFlag = true;
}


// ---------- Friends ----------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Send a Formatted String to Specified Stream.
//    The Attached Machine Information Is Not Included.
//
///////////////////////////////////////////////////////////////////////////////
ostream& operator<<(ostream& os, const MachineConfig& machineConfig)
{
    os << "[MachineConfig<";
    os << machineConfig.machineRefName() << ",";
    os << machineConfig.remoteShell() << ",";
    os << machineConfig.remoteDir() << ",";
    os << machineConfig.scratchDir() << ",";
    os << machineConfig.queue() << ",";
    os << machineConfig.allocAcct() << ",";
    os << machineConfig.userName();
    os << ">]" << endl;
    return os;
}

map<string, MachineConfig*, less<string> >* MachineConfig::p_userPrefs=0;
bool MachineConfig::p_markUpdateFlag=false;
