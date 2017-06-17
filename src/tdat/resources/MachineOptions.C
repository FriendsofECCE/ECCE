/**
 *  @file
 *  @author Ken Swanson
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



#include "tdat/MachineOptions.H"


string p_optionMnemonics[] = {  "Q", "TL", "MM", "SS", "MN",
                                     "AA", "UN", "PW", "RD", "SD", "P"  };

int p_defaultOptions[DEFAULT_OPTIONS] = { optnMachineName, optnUsername, optnPassword,
                                          optnPriorityReduction, optnRemoteDirectory,
                                          optnScratchDirectory };

///////////////////////////////////////////////////////////////////////
// Description
//   Parses the machine launch options token string.  Currently
//   if the list is null it defaults to standard workstations stuff.
///////////////////////////////////////////////////////////////////////
MachineOptions::MachineOptions(RefMachine *machine)
{
    //  Ensure that the optionSupported array is initialized to false
    for (int i = 0; i < TOTAL_OPTIONS; i++)
    {
        p_optionSupported[i] = false;
    }

    string options = machine->launchOptions();

    if (options.empty() || (options == "WS"))
    {
        this->loadDefaultOptions();
    }
    else
    {
        StringTokenizer tknzr(options);
        string mnmc;
        int optnIdx;

        mnmc = tknzr.next(":");

        while (!mnmc.empty())
        {
            optnIdx = locateOption(mnmc);

            if (optnIdx >= 0)
            {
                p_optionSupported[optnIdx] = true;
            }

            mnmc = tknzr.next(":");
        }
    }
}


MachineOptions::~MachineOptions()
{
}


void MachineOptions::loadDefaultOptions()
{
    for (int i = 0; i < DEFAULT_OPTIONS; i++)
    {
        p_optionSupported[p_defaultOptions[i]] = true;
    }
}


bool MachineOptions::isSupported(MachineOptionConstants c)
{
    return p_optionSupported[c];
}


bool MachineOptions::isSupported(string mnmc)
{
    bool result = false;
    int optnIdx = locateOption(mnmc);

    if (optnIdx >= 0)
        result = p_optionSupported[optnIdx];

    return result;
}


/**
 *  Returns the index of the option with the indicated mnemonic.
 *  Returns -1 if the mnemonic is not recognized.
 */
int MachineOptions::locateOption(string mnmc)
{
    int i;
    bool found;
    int optnIdx;

    i = 0;
    found = false;
    optnIdx = -1;

    while ((i < TOTAL_OPTIONS) && !found)
    {
        if (mnmc == p_optionMnemonics[i])
        {
            found = true;
            optnIdx = i;
        }

        i++;
    }

    return optnIdx;
}
