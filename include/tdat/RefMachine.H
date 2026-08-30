///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: RefMachine.H
//
//
// CLASS SYNOPSIS:
//    Reference Class for Tracking Registered Machines
//
// EXPORTED TYPES:
//    Class RefMachine
//
// LOCAL TYPES:
//    <None>
//
// SEE ALSO:
//    Class Machine
//      
// DESCRIPTION:
//    RefMachine objects are reference objects containing name-type
//    information about various computational machines (computers).
//
// EXAMPLES:
//    testRefMachine.C
//
///////////////////////////////////////////////////////////////////////////////
#ifndef REFMACHINE_HH
#define REFMACHINE_HH

#include <iostream>
  using std::ostream;
#include <fstream>
  using std::ifstream;

#include <string>
#include <vector>
  using std::string;
  using std::vector;

class RefMachine
{

public:

    enum machineContextEnum { allMachines, siteMachines, userMachines };

  // Constructors
    // This is A Reference Data Class - No Public Constructors!
    RefMachine(void);
    RefMachine(const string& refname);
    RefMachine(const RefMachine& refMachine);

  // Virtual Destructor
    virtual ~RefMachine(void);
    static void finalize(void);

  // Operators
    RefMachine& operator=(const RefMachine& refMachine);
    bool operator==(const RefMachine& refMachine) const;
    bool operator!=(const RefMachine& refMachine) const;

  // Accessors
    string        refname(void) const;
    string        fullname(void) const;
    string        vendor(void) const;
    string        model(void) const;
    string        proctype(void) const;
    unsigned int  nodes(void) const;
    unsigned int  proccount(void) const;
    string        remshellsString(void) const;
    string        codesString(void) const;
    string        launchOptions(void) const;
    string        shellPath(void) const;
    string        libPath(void) const;
    string        sourceFile(void) const;
    string        shell(void) const;
    string        globusContact(void) const;
    string        frontendMachine(void) const;
    string        frontendBypass(void) const;
    bool          singleConnect(void) const;
    bool          checkScratch(void) const;
    bool          isSameDomain(void) const;
    bool          userSubmit(void) const;
    static string exePath(const string& code, const string& refname,
                          const string& vendor="", const string& model="");

    vector<string*>* queues(void) const;
    vector<string>*  remshells(void) const;
    vector<string>*  codes(void) const;

  // Query Operations
    bool hasRemShell(const string& remShell) const;
    bool hasQueue(const string& queue) const;
    bool hasCode(const string& code) const;

    static RefMachine* refLookup(const string& refname);
    static RefMachine* fullLookup(const string& fullname);
    static RefMachine* abbrevLookup(const string& abbrevname);
    static vector<string>* referenceNames(
                               const machineContextEnum& context = allMachines);

    // name with path of CONFIG file
    static string configFile(const string& refname);

    static void markUpdateUserPrefs(void);

protected:

  // Derived Class Accessors
    static bool initialize(const machineContextEnum& context = allMachines);
    static void parseFile(ifstream& inFile);
    bool validate_and_assign(const string& refname);

private:

  // State
    string       p_refname;          // Reference name - unique
    string       p_fullname;         // Including Domain - columbo.emsl.pnl.gov
    string       p_vendor;           // Vendor name
    string       p_model;            // Model name
    string       p_proctype;         // Processor type name
    string       p_remshells;        // Remote Shell commands supported
    string       p_codes;            // Registered Codes with delimiters
    unsigned int p_nodes;            // Number of Physical Nodes
    unsigned int p_proccount;        // Total Number of Processors
    string       p_options;          // Applicable launch options

  // Lazy Update Flag For User Registered Machine Preferences
    static bool  p_markUpdateFlag;


  // Modifiers
    void refname(const char* refname);
    void fullname(const char* fullname);
    void vendor(const char* vendor);
    void model(const char* model);
    void proctype(const char* proctype);
    void nodes(unsigned int nodes);
    void proccount(unsigned int value);
    void remshells(const char* remshells);
    void codes(const char* codes);

  // Encapsulated Behavior
    static void enforce_initialization(
                        const machineContextEnum& context = allMachines);

  // Friends
    friend ostream& operator<< (ostream& os, const RefMachine& refMachine);

}; // RefMachine

#endif /* REFMACHINE_HH */
