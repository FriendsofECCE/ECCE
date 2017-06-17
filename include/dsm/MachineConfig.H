///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: MachineConfig.H
//
//
// CLASS SYNOPSIS:
//    Class for Tracking User Configuration for Application Launch.
//
// EXPORTED TYPES:
//    Class MachineConfig
//
// LOCAL TYPES:
//
// SEE ALSO:
//    Class Machine
//    Class SingleJob
//      
// DESCRIPTION:
//    MachineConfig objects contain user specific information about
//    the conditions that an application (or job) is being run.  This
//    class compliments the SingleJob class by adding machine-specific
//    information to the process-specific information of Jobs.
//
//    See SingleJob.H for diagram describing the relationship between
//    MachineConfig, SingleJob, Machine, and Calculation.
//
// EXAMPLES:
//    testMachine.C
//    testSingleJob.C
//
///////////////////////////////////////////////////////////////////////////////
#ifndef MACHINECONFIG_HH
#define MACHINECONFIG_HH

#include <iostream>
    using std::ostream;

#include <string>
#include <map>
#include <vector>
    using std::string;
    using std::map;
    using std::vector;
    using std::less;

class MachineConfig
{
    public:
        // Constructors
        MachineConfig(const string& machineRefName,
                      const string& remoteShell,
                      const string& remoteDir,
                      const string& scratchDir,
                      const string& queue,
                      const string& allocAcct,
                      const string& userName);
                      
        MachineConfig(const string& refname);

        // Virtual Destructor
        virtual ~MachineConfig(void);

        // Operators
        bool operator==(const MachineConfig&) const;
        bool operator!=(const MachineConfig&) const;

        // Modifiers
        void remoteShell(const string& remoteShell);
        void remoteDir(const string& remoteDir);
        void scratchDir(const string& scratchDir);
        void queue(const string& queue);
        void allocAcct(const string& allocAcct);
        void userName(const string& userName);

        // Accessors
        string       machineRefName(void) const;
        string       remoteShell(void) const;
        string       remoteDir(void) const;
        string       scratchDir(void) const;
        string       queue(void) const;
        string       allocAcct(void) const;
        string       userName(void) const;

        // User Preferences Accessors - Modifiers - Savers - Readers
        static void                 saveUserPrefs();
        static MachineConfig *      userPref(const string& machineRefName);
        static void                 addUserPref(const MachineConfig& machineConf);
        static void                 removeUserPref(const string& machineRefName);
        static void                 markUpdateUserPrefs();
        static vector<string>       getConfiguredMachineNames();

    protected:
        // Subclass Member Functions
        MachineConfig(const MachineConfig& machineConfig);

        // Modifiers
        void machineRefName(const string& machineRefName);

        // Operators
        MachineConfig& operator=(const MachineConfig& machineConfig);

    private:
        // State
        string    p_refname;             // Machine Reference Name
        string    p_remoteShell;         // Remote Communications Shell
        string    p_remoteDir;           // Root of Remote Project Tree
        string    p_scratchDir;          // Root Scratch Directory on p_machine
        string    p_queue;               // Preferred Job Queue Name (May Be MT)
        string    p_allocAcct;           // Allocation Account to debit
        string    p_userName;            // User Login Name on p_machine

        // Current User Machine Preferences
        static map<string, MachineConfig*, less<string> >* p_userPrefs;

        // Lazy Update Flag For User Machine Preferences
        static bool p_markUpdateFlag;

        // Friends
        friend ostream& operator<<(ostream& os,const MachineConfig& machineConfig);

}; // MachineConfig

#endif /* MACHINECONFIG_HH */
