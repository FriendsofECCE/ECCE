///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: RCommand.H
//
// CLASS SYNOPSIS:
//    The RCommand Class Provides an API to Remote Execution of Commands.
//
// EXPORTED TYPES:
//    Class RCommand
//
// LOCAL TYPES:
//
// SEE ALSO:
//      
// DESCRIPTION:
//    
// EXAMPLES:
//
///////////////////////////////////////////////////////////////////////////////
#ifndef RCOMMAND_HH
#define RCOMMAND_HH

#include <string>
  using std::string;
#include <vector>
  using std::vector;

static const long RC_CONNECT_TIMEOUT = 60;  // Connect timeout seconds
static const long RC_EXEC_TIMEOUT = 30;     // Normal execute timeout seconds
static const long RC_COPY_TIMEOUT = 900;    // Copy timeout seconds
static const long RC_COMMAND_TIMEOUT = 180; // Command timeout seconds

class RCommand
{

public:
  // Constructor
    RCommand(const string& machine="", const string& remShell="",
             const string& locShell="csh", const string& userName="",
             const string& password="", const string& frontendMachine="",
             const string& frontendBypass="", const string& shellPath="",
             const string& libPath="", const string& sourceFile="");

  // Destructor
    ~RCommand(void);

  // Operations
    bool isOpen(void);    // Check connection status
    bool fileOp(const string& op, const string& filename);
    bool exists(const string& filename);
    bool directory(const string& filename);
    bool writable(const string& filename);
    bool executable(const string& filename);

    bool cd(const string& directory);
    bool which(const string& filename, string& path);
    bool exec(const string& command,
              const string& errorMessage="", const int& timeout=0);
    bool execout(const string& command, string& output,
                 const string& errorMessage="", const int& timeout=0);
    bool execbg(const string& command, string& output,
                const string& errorMessage="");

    bool shellput(const char** fromFiles, const string& toFile);
    bool shellget(const char** fromFiles, const string& toFile);
    bool shellget(const vector<string>& fromFiles, const string& toFile);

    bool hop(const string& hopMachine, const string& locShell="csh",
             const string& userName="", const string& password="",
             const string& shellPath="", const string& libPath="",
             const string& sourceFile="");

    bool expwrite(const string& command);
    bool expwrite(const char* command);
    bool expwritefull(const string& command);
    void exptimeout(const int& timeout=0);
    char* expout(void);
    void patalloc(int numPatterns, ...);
    void patfree(void);
    int patexpect(void);
    int expect(int numPatterns, ...);
    int expect1(const char* pat);
    int expect2(const char* pat1, const char* pat2);
    int expfid(void);

    string commError(void); // Return error message string

    static bool globusproxy(const string& password, string& errMessage);
    static bool userproxy(const string& proxyAuth, const string& machine,
                          const string& userName, const string& password,
                          string& errMessage);
    static bool globusrun(const string& command, string& output,
                          string& errMessage,
                          const string& machine, const string& contact,
                          const string& password, const string& queueRSL="");

    static bool get(string& errMessage,
                    const string& machine, const string& remShell,
                    const string& userName, const string& password,
                    int numFiles, ...);
    static bool get(string& errMessage,
                    const string& machine, const string& remShell,
                    const string& userName, const string& password,
                    const char** fromFiles, const string& toFile);
    static bool get(string& errMessage,
                    const string& machine, const string& remShell,
                    const string& userName, const string& password,
                    const vector<string>& fromFiles, const string& toFile);
    static bool put(string& errMessage,
                    const string& machine, const string& remShell,
                    const string& userName, const string& password,
                    int numFiles, ...);
    static bool put(string& errMessage,
                    const string& machine, const string& remShell,
                    const string& userName, const string& password,
                    const char** fromFiles, const string& toFile);
    static bool put(string& errMessage,
                    const string& machine, const string& remShell,
                    const string& userName, const string& password,
                    const vector<string>& fromFiles, const string& toFile);

    static bool command(const string& command, string& output,
                string& errMessage, const string& machine="",
                const string& remShell="", const string& locShell="csh",
                const string& userName="", const string& password="",
                const string& frontendMachine="",
                const string& frontendBypass="");

    static bool command(const string& command, const string& args,
                string& output, string& errMessage, const string& machine="",
                const string& remShell="", const string& locShell="csh",
                const string& userName="", const string& password="",
                const string& frontendMachine="",
                const string& frontendBypass="");

    static bool bgcommand(const string& command, string& errMessage,
                const string& machine="", const string& remShell="",
                const string& userName="", const string& password="");

    static bool bgcommand(const string& command, const string& args,
                string& errMessage,
                const string& machine="", const string& remShell="",
                const string& userName="", const string& password="");

    static string argsToCommand(const string& command, const string& args,
                                const string& remShell, const bool& isRemote,
                                string& commandWithArgs);

    static bool isRemote(const string& machine, const string& remShell,
                         const string& userName="");
    static bool isSameDomain(const string& machine);
    static string whereami(void);
    static string commandShell(const string& machine="",
                         const string& remShell="", const string& userName="");

private:
    static bool fidwrite(const int& fid, const string& command,
                         string& errMessage);
    static bool copy(string& errMessage,
                     const string& machine, const string& copyCmd,
                     const string& userName, const string& password,
                     char** argv);
    static string ftpTarget(const string& ftpIn);
    static bool ftp(string& errMessage, const string& machine,
                    const string& copyCmd, const string& userName,
                    const string& password, char** argv,
                    const char** fromFiles, const string& toFile,
                    const bool& putFlag);
    static bool globFiles(const char** inFiles, char**& outFiles,int& numFiles);
    static string globusRSL(const string& command, const string& queueRSL);
    static string shellCommand(const string& remShell, const string& machine,
                               const string& locShell, const string& userName,
                               const bool& hopFlag, string& proxyAuth,
                               char** argv);
    static string copyCommand(const string& remShell, const bool& isRemote,
                              const string& machine, const string& userName,
                              string& proxyAuth, int& argc, char** argv);
    static string copyToShell(const string& copyCmd);
    static string userCommand(const string& command,
                              const string& remShell, const string& machine,
                              const string& locShell, const string& userName,
                              string& proxyAuth, char** argv);
    static string userShellCommandArgs(const string& remShell,
                                       string& proxyAuth,
                                       int& argc, char** argv);
    static string userCopyCommandArgs(const string& remCopy, string& proxyAuth,
                                      int& argc, char** argv);
    static void expMungedOutputFix();

    static const bool getPassCache(const string& shell, const string& machine,
                                   const string& userName, string& passBuf);
    static void  setPassCache(const string& shell, const string& machine,
                              const string& userName, const string& password);

    int p_fid;
    int p_pid;
    struct exp_case* p_pats;
    bool p_connected;
    bool p_background;
    int p_hopCount;
    string p_shell;
    string p_machine;
    string p_errMessage;

}; // RCommand

#endif /* RCOMMAND_HH */
