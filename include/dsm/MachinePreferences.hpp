/**
 * @file
 * @author Ken Swanson
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


#ifndef MACHINEPREFERENCES_HH
#define MACHINEPREFERENCES_HH

#include "util/StringTokenizer.H"
#include "tdat/RefMachine.H"
#include "tdat/MachineOptions.H"

#include "dsm/TaskJob.H"        //  for the Launchdata structure


class MachinePreferences
{
    public:
        //  Constructor(s), Destructor
        ~MachinePreferences();

        RefMachine              * getRegisteredMachine();
        MachineOptions          * getMachineOptions();

        string getItemKey() const;

        string getRemoteShell() const;
        string getRemoteDirectory() const;
        string getScratchDirectory() const;
        string getQueueName() const;
        string getAllocationAccount() const;
        string getUsername() const;
        int getProcessors() const;
        int getNodes() const;
        long getMaxWallTime() const;
        long getMemoryLimit() const;
        long getScratchSpace() const;
        int getPriority() const;

        void setRemoteShell(string s);
        void setRemoteDirectory(string s);
        void setScratchDirectory(string s);
        void setQueueName(string s);
        void setAllocationAccount(string s);
        void setUsername(string s);
        void setProcessors(int procs);
        void setNodes(int nodes);
        void setMaxWallTime(long t);
        void setMemoryLimit(long mbytes);
        void setScratchSpace(long mbytes);
        void setPriority(int p);

        bool isOptionSupported(string mnmc);

        string listValues(string delimiter);
        Launchdata getLaunchData() const;
        void initializeFrom(Launchdata ld);

        static MachinePreferences     *lookup(string key);
        static MachinePreferences     *getItemAt(int index);
        static int                     countItems();
        static void                    moveItemToFront(string key);
        static void                    saveChanges();

        static MachinePreferences     *create(string key);
        static bool                    remove(string key);
        static void                    refresh(bool force = false);

    protected:
        void loadSettings(vector<string>);
        void loadConfigs(vector<string>, int idx0 = 0);

        static vector<MachinePreferences *> *p_prefsElmts;

    private:
        MachinePreferences(string key);

        static void         loadSettings();

        static bool                  p_prefsLoaded;
        static bool                  p_prefsChgd;
        static bool                  itemExists(string key);
        static MachinePreferences   *ensureItemExists(string key);
        static MachinePreferences   *findItem(string key);
        static vector<string>        parseFields(string pvals, char delim);

        //  Attributes
        RefMachine      *p_machRgstn;
        MachineOptions  *p_machOptns;
        string           p_itemKey;

        //  Fields
        string       p_remoteShell;        //  Remote Communications Shell
        string       p_remoteDir;          //  Root of Remote Project Tree
        string       p_scratchDir;         //  Root Scratch Directory on p_machine
        string       p_queueName;          //  Preferred Job Queue Name (May be empty)
        string       p_allocAcct;          //  Allocation Account to debit
        string       p_username;           //  User Login Name on p_machine

        int          p_processors;         //  Number of processors to use
        int          p_nodes;              //  Number of nodes to use
        long         p_maxWallTime;        //  Maximum Wall time, in minutes
        long         p_memoryLimit;        //  Memory Limit, in MBytes
        long         p_scratchSpace;       //  Scratch Space, in MBytes
        int          p_priority;           //  Task priority (1 -> 19; highest: 1)

};

#endif  //  MACHINEPREFERENCES_HH
