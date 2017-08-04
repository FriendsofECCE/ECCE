/**
 *  @file
 *  @author Ken Swanson
 *
 *  Utility class that encompasses and extends the functionality of thee
 *  MachConfig class of the earlier Ecce versions.  This class attempts
 *  to maintain backwards compatibility with the disk files produced
 *  by the older versions, but does not overwrite or change the information
 *  in those files.  Rather, it creates its own file that contains all
 *  registration information for the machines.  Thus, any changes to the
 *  files by Ecce version 4.0 will not be seen by earlier Ecce versions.
 *
 */


#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fstream>
    using std::ifstream;
    using std::ofstream;
#include <ios>
    using std::ios;

#include "util/KeyValueReader.H"
#include "util/STLUtil.H"
#include "util/Ecce.H"
#include "util/StringConverter.H"
#include "util/Preferences.H"
#include "util/PreferenceLabels.H"

#include "dsm/MachinePreferences.H"

#define RETAIN_VRSN3X_CONFIGFILES

#define VRSN4X_PREFS_FNAME "MachPrefs"
#define VRSN3X_CNFGS_FNAME "MachConfig"
#define VRSN3X_LOGIN_FNAME "MachLogin"

#define MAXLINE 256

bool MachinePreferences::p_prefsLoaded = false;
bool MachinePreferences::p_prefsChgd = false;
vector<MachinePreferences *> *MachinePreferences::p_prefsElmts = NULL;


MachinePreferences::MachinePreferences(string key)
{
    //  Assume that the key is the same as the refname of the machine to
    //  which this preferences object corresponds.

    p_itemKey = key;
    p_machRgstn = RefMachine::refLookup(p_itemKey);
    p_machOptns = NULL;

    //  The RefMachine lookup might come back as NULL.  This is perfectly fine.
    //  It indicates that there is no reference machine information for the entry
    //  in the preferences file.  There are two reasons why this can happen:
    //  (1) the reference machine once existed but no longer; (2) the entry
    //  is a placeholder for verifying user credentials.

    if (p_machRgstn != NULL)
        p_machOptns = new MachineOptions(p_machRgstn);

    p_username = Ecce::realUser();
    p_queueName = "";

    //  Actually should put default values in here from RefMachine, if any.
    p_remoteShell = "";
    p_remoteDir = "";
    p_scratchDir = "";
    p_allocAcct = "";

    p_processors = 1;
    p_nodes = 1;
    p_maxWallTime = 0;
    p_memoryLimit = 0;
    p_scratchSpace = 0;
    p_priority = 1;
}


MachinePreferences::~MachinePreferences()
{

    if (p_machRgstn != NULL)
    {
        delete p_machRgstn;
        delete p_machOptns;
    }
}


string MachinePreferences::getItemKey() const
{
    return p_itemKey;
}


RefMachine *MachinePreferences::getRegisteredMachine()
{
    return p_machRgstn;
}


string MachinePreferences::getRemoteShell() const
{
    return p_remoteShell;
}


void MachinePreferences::setRemoteShell(string s)
{
    p_remoteShell = s;
    p_prefsChgd = true;
}


string MachinePreferences::getRemoteDirectory() const
{
    return p_remoteDir;
}


void MachinePreferences::setRemoteDirectory(string s)
{
    p_remoteDir = s;
    p_prefsChgd = true;
}


string MachinePreferences::getScratchDirectory() const
{
    return p_scratchDir;
}


void MachinePreferences::setScratchDirectory(string s)
{
    p_scratchDir = s;
    p_prefsChgd = true;
}


string MachinePreferences::getQueueName() const
{
    return p_queueName;
}


void MachinePreferences::setQueueName(string s)
{
    p_queueName = s;
    p_prefsChgd = true;
}


string MachinePreferences::getAllocationAccount() const
{
    return p_allocAcct;
}


void MachinePreferences::setAllocationAccount(string s)
{
    p_allocAcct = s;
    p_prefsChgd = true;
}


string MachinePreferences::getUsername() const
{
    return p_username;
}


void MachinePreferences::setUsername(string s)
{
    p_username = s;
    p_prefsChgd = true;
}


int MachinePreferences::getProcessors() const
{
    return p_processors;
}


void MachinePreferences::setProcessors(int procs)
{
    p_processors = procs;
    p_prefsChgd = true;
}


int MachinePreferences::getNodes() const
{
    return p_nodes;
}


void MachinePreferences::setNodes(int nodes)
{
    p_nodes = nodes;
    p_prefsChgd = true;
}


long MachinePreferences::getMaxWallTime() const
{
    return p_maxWallTime;
}


void MachinePreferences::setMaxWallTime(long t)
{
    p_maxWallTime = t;
    p_prefsChgd = true;
}


long MachinePreferences::getMemoryLimit() const
{
    return p_memoryLimit;
}


void MachinePreferences::setMemoryLimit(long mbytes)
{
    p_memoryLimit = mbytes;
    p_prefsChgd = true;
}


long MachinePreferences::getScratchSpace() const
{
    return p_scratchSpace;
}


void MachinePreferences::setScratchSpace(long mbytes)
{
    p_scratchSpace = mbytes;
    p_prefsChgd = true;
}


int MachinePreferences::getPriority() const
{
    return p_priority;
}


void MachinePreferences::setPriority(int p)
{
    p_priority = p;
    p_prefsChgd = true;
}


bool MachinePreferences::isOptionSupported(string mnmc)
{
    return p_machOptns->isSupported(mnmc);
}


void MachinePreferences::loadConfigs(vector<string> fields, int idx0)
{
    //  Read at most six fields (i = 0 to 5), starting at idx0.

    int i = 0;
    string fval;
    int maxFields = 6;

    while ((i < maxFields) && (i + idx0) < (int)fields.size())
    {
        fval = STLUtil::trim(fields[i + idx0]);

        switch (i)
        {
            case  0:  p_username = fval;     break;
            case  1:  p_remoteShell = fval;  break;
            case  2:  p_queueName = fval;    break;
            case  3:  p_remoteDir = fval;    break;
            case  4:  p_scratchDir = fval;   break;
            case  5:  p_allocAcct = fval;    break;
            default:                         break;
        }

        i++;
    }
}


void MachinePreferences::initializeFrom(Launchdata ld)
{
    p_remoteShell = ld.remoteShell;
    p_queueName = ld.queue;
    p_memoryLimit = ld.maxmemory;
    p_scratchSpace = ld.minscratch;
    p_priority = (int)(atol(ld.priority.c_str()));
    p_username = ld.user;
    p_remoteDir = ld.rundir;
    p_scratchDir = ld.scratchdir;
    p_processors = (int)ld.totalprocs;
    p_nodes = (int)ld.nodes;

    p_prefsChgd = true;
}


vector<string> MachinePreferences::parseFields(string pvals, char delim)
{
    int startIdx = 0;
    int chpos = 0;
    int numChars = 0;
    int totalSize = pvals.size();
    vector<string> fields;

    string token;

    while (startIdx < totalSize)
    {
        chpos = pvals.find(delim, startIdx);
        numChars = ((chpos > 0) ? chpos : totalSize) - startIdx;
        fields.push_back(pvals.substr(startIdx, numChars));

        startIdx += numChars + 1;
    }

    return fields;
}



void MachinePreferences::loadSettings(vector<string> fields)
{
    this->loadConfigs(fields, 0);

    int i = 0;

    // Start at field 6 for new 12 field MachPrefs format.  Old format has
    // 14 fields and we want to maintain compatibility reading those.
    int idx0 = 6;
    if (fields.size() >= 14)
      idx0 = 8;

    int maxFields = 6;
    string fval;

    while ((i < maxFields) && (i + idx0) < (int)fields.size())
    {
        fval = STLUtil::trim(fields[i + idx0]);

        if (fval.size() > 0)
        {
            long v = atol(fval.c_str());

            switch (i)
            {
                case 0:  p_processors = (int)(v);  break;
                case 1:  p_nodes = (int)(v);       break;
                case 2:  p_maxWallTime = v;        break;
                case 3:  p_memoryLimit = v;        break;
                case 4:  p_scratchSpace = v;       break;
                case 5:  p_priority = (int)(v);    break;
                default:                           break;
            }
        }

        i++;
    }
}


string MachinePreferences::listValues(string delimiter)
{
    string prefVals = p_username + delimiter;
    prefVals += p_remoteShell + delimiter;
    prefVals += p_queueName + delimiter;
    prefVals += p_remoteDir + delimiter;
    prefVals += p_scratchDir + delimiter;
    prefVals += p_allocAcct + delimiter;
    prefVals += StringConverter::toString(p_processors) + delimiter;
    prefVals += StringConverter::toString(p_nodes) + delimiter;
    prefVals += StringConverter::toString((int)p_maxWallTime) + delimiter;
    prefVals += StringConverter::toString((int)p_memoryLimit) + delimiter;
    prefVals += StringConverter::toString((int)p_scratchSpace) + delimiter;
    prefVals += StringConverter::toString((int)p_priority);

    return prefVals;
}


Launchdata MachinePreferences::getLaunchData() const
{
    Launchdata ldat;

    ldat.remoteShell = p_remoteShell;
    ldat.nodes = p_nodes;
    ldat.totalprocs = p_processors;
    ldat.machine = p_machRgstn->refname();
    ldat.rundir = p_remoteDir;
    ldat.scratchdir = p_scratchDir;
    ldat.minscratch = (unsigned long)p_scratchSpace;
    ldat.maxmemory = (unsigned long)p_memoryLimit;
    ldat.user = p_username;
    ldat.queue = p_queueName;
    ldat.priority = StringConverter::toString(p_priority);
    ldat.maxwall = "";

    return ldat;
}




//** STATIC METHODS ***********************************************************//

/**
 *  Returns true or false, depending on whether an element corresponding
 *  with indicated key exists.
 */
bool MachinePreferences::itemExists(string key)
{
    return (MachinePreferences::findItem(key) != NULL);
}

/**
 *  Looks for, and returns, a MachinePreferences object indicated key; creates
 *  the object if it does not already exist.
 *
 *  This method is included to provide for functionality needed to supplant the
 *  MachineConfig class.
 */
MachinePreferences * MachinePreferences::create(string key)
{
    if (!p_prefsLoaded)
    {
        //  Load from disk
        MachinePreferences::loadSettings();
    }

    return ensureItemExists(key);
}

/**
 *  Removes the MachinePreferences element with the value 'key'.
 *
 *  Returns true or false depending on whether the removal was successful or not.
 *  This method is included to provide for functionality needed to supplant the
 *  MachineConfig class.
 */
bool MachinePreferences::remove(string key)
{
    MachinePreferences *prefs = NULL;

    if (!p_prefsLoaded)
        MachinePreferences::loadSettings();    //  Load from disk

    vector<MachinePreferences *>::iterator itr = p_prefsElmts->begin();
    bool done = false;

    while ((itr < p_prefsElmts->end()) && !done)
    {
        prefs = *itr;

        if (prefs->getItemKey() == key)
        {
            p_prefsElmts->erase(itr);
            done = true;
        }

        itr++;
    }

    return done;
}

/**
 *  Moves the element identified by the value of 'key' to the front of the list.
 *
 */
void MachinePreferences::moveItemToFront(string key)
{
    MachinePreferences *prefs = NULL;
    vector<MachinePreferences *>::iterator itr = p_prefsElmts->begin();
    bool done = false;

    while ((itr < p_prefsElmts->end()) && !done)
    {
        prefs = *itr;

        if (prefs->getItemKey() == key)
        {
            p_prefsElmts->erase(itr);
            p_prefsElmts->insert(p_prefsElmts->begin(), prefs);
            done = true;
        }

        itr++;
    }
}


/**
 *
 *  Return the total number of MachinePreferences items.  This will include
 *  any preferences that have a NULL-valued RefMachine.
 */
int MachinePreferences::countItems()
{
    if (!p_prefsLoaded)
    {
        MachinePreferences::loadSettings();
    }

    return (p_prefsElmts->size());
}


/**
 *  Obtain the MachinePreferences instance which corresponds to the supplied
 *  logical name.
 *
 *  This method ensures that the preferences have been loaded from disk; it will
 *  automatically invoke a load if it has not already happened.
 *
 *  @return
 *      Instance of MachinePreferences; NULL if not found.
 */
MachinePreferences *MachinePreferences::lookup(string key)
{
    if (!p_prefsLoaded)
    {
        //  Load from disk
        MachinePreferences::loadSettings();
    }

    return MachinePreferences::findItem(key);
}


MachinePreferences *MachinePreferences::getItemAt(int index)
{
    if (!p_prefsLoaded)
    {
        //  Load from disk
        MachinePreferences::loadSettings();
    }

    return ((*p_prefsElmts)[index]);
}


/**
 *  Returns the MachinePreferences instance that corresponds to indicated machine.
 *
 *  This method differs from getPreferences() in that it assumes that the preferences
 *  have already been loaded from disk.  Thus it should not be made publicly
 *  available.
 *
 *  @returns
 *      Instance of MachinePreferences; NULL if not found.
 */
MachinePreferences *MachinePreferences::findItem(string key)
{
    MachinePreferences *prefs = NULL;

    int i = 0;
    int n = p_prefsElmts->size();
    bool found = false;

    while ((i < n) && !found)
    {
        prefs = (*p_prefsElmts)[i];
        found = (prefs->getItemKey() == key);
        i++;
    }

    if (!found)
        prefs = NULL;

    return prefs;
}


/**
 *  Looks for an instance of MachinePreferences that corresponds to the indicated
 *  RefMachine; creates an instance if the instance cannot be found.
 *
 *  This method assumes that the p_prefsElmts vector has been instantiated, but
 *  (through necessity) does not check whether the preferences have actually
 *  been loaded.
 *
 *  @returns
 *      An existing MachinePrefences instance, newly created as needed.
 */
MachinePreferences *MachinePreferences::ensureItemExists(string key)
{
    MachinePreferences *prefs = MachinePreferences::findItem(key);

    if (prefs == NULL)
    {
        prefs = new MachinePreferences(key);
        p_prefsElmts->push_back(prefs);
        p_prefsChgd = true;
    }

    return prefs;
}


void MachinePreferences::loadSettings()
{
    MachinePreferences *prefs = NULL;
    string key;

    p_prefsElmts = new vector<MachinePreferences *>;

    string prefsDrcty(Ecce::realUserPrefPath());

    //  The Version 4.x prefs file contains all information needed for
    //  specifying user preferences per machine.  The ordering of the
    //  elements in this file reflects the history of launches that the
    //  user has executed.  If it exists, read from it.
    string fpathPrefs(prefsDrcty);
    fpathPrefs += VRSN4X_PREFS_FNAME;
    ifstream *isPrefs = new ifstream(fpathPrefs.c_str());

    if (isPrefs->is_open())
    {
        string prefVals;
        KeyValueReader kvrPrefs(*isPrefs);
        kvrPrefs.stripTrailingComments(false);

        while (kvrPrefs.getpair(key, prefVals))
        {
            prefs = MachinePreferences::ensureItemExists(key);
            prefs->loadSettings(parseFields(prefVals, '|'));
        }

        isPrefs->close();
        delete isPrefs;
    }

    //  Lastly, ensure that a MachinePreferences element exists for each
    //  registered machine.  This must be done last in order to preserve
    //  the ordering of preferences as found on the disk file.
    vector<string> *refnames = RefMachine::referenceNames();
    int numRgstns = refnames->size();

    for (int i = 0; i < numRgstns; i++)
    {
        MachinePreferences::ensureItemExists((*refnames)[i]);
    }

    p_prefsLoaded = true;
    p_prefsChgd = false;

}


void MachinePreferences::saveChanges()
{
    string fpathPrefs((char *)Ecce::realUserPrefPath());
    fpathPrefs += VRSN4X_PREFS_FNAME;

    ofstream *osPrefs = new ofstream(fpathPrefs.c_str(), ios::out);

    if (osPrefs->is_open())
    {
        string header("### DO NOT EDIT ### - File automatically generated!\n");
        header.append("#\n");
        header.append("# Created by ECCE version ");
        header.append(Ecce::ecceVersion());
        header.append("\n");
        header.append("#\n");

        *osPrefs << header;

        vector<MachinePreferences *>::iterator itCnfgs = p_prefsElmts->begin();

        while (itCnfgs != p_prefsElmts->end())
        {
            MachinePreferences *prefs = *itCnfgs;

            *osPrefs << prefs->getItemKey() << ":\t";
            *osPrefs << prefs->listValues("|") << endl;

            itCnfgs++;
        }

        osPrefs->close();
        delete osPrefs;

        chmod(fpathPrefs.c_str(), 0600);
        p_prefsChgd = false;
    }
}



void MachinePreferences::refresh(bool force)
{
    if (p_prefsChgd || force)
    {
        // Delete any existing instances of MachinePreferences
        vector<MachinePreferences * >::iterator it;

        for (it = p_prefsElmts->begin(); it < p_prefsElmts->end(); it++)
        {
            if ((*it) != NULL)
            {
                delete (*it);
            }
        }

        p_prefsElmts->clear();
        delete p_prefsElmts;

        p_prefsLoaded = false;
        MachinePreferences::loadSettings();
    }
}
