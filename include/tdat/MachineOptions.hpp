/**
 * @file
 * @author Ken Swanson
 *
 *  Utility object for containing the various options for a machine registration.
 *  Specific codes and their meanings are as follows
 *              Q   --  Queue
 *              TL  --  Time-Limit (i.e., Wall time)
 *              MM  --  Maximum Memory
 *              SS  --  Scratch Space
 *              MN  --  Machine Name
 *              AA  --  Allocation Account
 *              UN  --  Username
 *              PW  --  Password
 *              RD  --  Run (or remote, or calculation) directory
 *              SD  --  Scratch directory
 *              P   --  Priority
 *  The appearance of the code for a given option within the Machines file
 *  indicates that the option is supported.
 *
 */


#ifndef MACHINEOPTIONS_HH
#define MACHINEOPTIONS_HH

#include <string>
    using std::string;

#include "util/StringTokenizer.H"

#include "tdat/RefMachine.H"


enum MachineOptionConstants
{
    optnQueue               = 0,
    optnWallTimeLimit       = 1,
    optnMaximumMemory       = 2,
    optnScratchSpace        = 3,
    optnMachineName         = 4,
    optnAllocationAccount   = 5,
    optnUsername            = 6,
    optnPassword            = 7,
    optnRemoteDirectory     = 8,
    optnScratchDirectory    = 9,
    optnPriorityReduction   = 10
};

#define TOTAL_OPTIONS       11
#define DEFAULT_OPTIONS      6

class MachineOptions
{
    public:
        //  Constructor(s), Destructor
        MachineOptions(RefMachine *machine);
        ~MachineOptions();

        bool isSupported(MachineOptionConstants c);
        bool isSupported(string mnmc);

    protected:

    private:
        static int locateOption(string mnmc);

        void loadDefaultOptions();

        bool                 p_optionSupported[TOTAL_OPTIONS];

};
#endif      //  MACHINEOPTIONS_HH
