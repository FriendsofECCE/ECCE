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

#include <set>
#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include "comm/DirectTransport.H"

class Transport;
class RemoteTransport;
struct RemoteStream;

static const long RC_CONNECT_TIMEOUT = 60;  // Connect timeout seconds
static const long RC_EXEC_TIMEOUT = 30;     // Normal execute timeout seconds
static const long RC_COPY_TIMEOUT = 900;    // Copy timeout seconds
static const long RC_COMMAND_TIMEOUT = 180; // Command timeout seconds

class RCommand
{

public:
  // Constructor.  A local machine runs its commands through a
  // DirectTransport; an ssh machine goes through libssh (SshTransport), where
  // a front end is reached by a forwarded channel or a nested ssh
  // (SshTransport::FrontEnd) and hop() opens a new connection through it.
  // A host whose ~/.ssh/config shares connections (ControlMaster/ControlPath)
  // is served by the OpenSSH client instead (OpenSshTransport); see
  // ECCE_SSH_BACKEND in docs/ENVIRONMENT.md.
    RCommand(const string& machine="", const string& remShell="",
             const string& locShell="bash", const string& userName="",
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

    // "" unless libssh reaches this machine through a front end; then
    // "forward" (direct-tcpip channel) or "nested" (ssh run on the front end).
    string frontEndMode() const;

    // "libssh" or "openssh" when an ssh transport serves this connection
    // (OpenSSH for hosts whose ssh config shares connections), else "".
    string sshBackend() const;
    // ssh options that let another OpenSSH client (a terminal) reuse this
    // connection's shared login; empty when there is none to add.
    vector<string> terminalSshOptions() const;

    bool hop(const string& hopMachine, const string& locShell="bash",
             const string& userName="", const string& password="",
             const string& shellPath="", const string& libPath="",
             const string& sourceFile="");

    // The monitor stream.  startStream() runs the command on pipes (over
    // ssh, on a session and thread of its own); streamFd()/streamWrite() then
    // address it, and \003 sent through exec() becomes SIGINT to its process
    // group.  streamFd() is -1 when no stream is open.  onThisLogin runs an
    // ssh stream on this connection's own login instead (no second password
    // or one-time code); the connection then runs nothing else until
    // stopStream().
    bool startStream(const string& command, bool onThisLogin=false);
    // get()/put() (putFlag) over this connection's login instead of a new
    // one.  Paths are relative to the remote home, as there.  Fails while a
    // stream is open.
    bool copyOnLogin(bool putFlag, const vector<string>& files,
                     const string& toFile, string& errMessage);
    void stopStream(int graceMs=2000);

    bool streamWrite(const string& command);
    bool streamWrite(const char* command);
    // Timeout in seconds for execout() and friends; 0 restores the default,
    // negative means none.
    void streamTimeout(const int& timeout=0);
    int streamFd(void);

    string commError(void); // Return error message string

    // Asked when an ssh host key is not in known_hosts.
    // Return true to accept and record it.  Unset, the key is refused and
    // the user is told to run ssh once by hand.
    typedef bool (*HostKeyHook)(const string& host, const string& fingerprint);
    static HostKeyHook hostKeyHook;

    // Non-empty (a message fit to show the user) when remShell is anything
    // but the ssh family (ssh, and the old sshpass and ssh/ftp, which mean
    // ssh now): telnet, Globus, rsh and the shells remote_shells.site used to
    // define are all refused.
    static string removedShellMessage(const string& remShell);

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
                const string& remShell="", const string& locShell="bash",
                const string& userName="", const string& password="",
                const string& frontendMachine="",
                const string& frontendBypass="");

    static bool command(const string& command, const string& args,
                string& output, string& errMessage, const string& machine="",
                const string& remShell="", const string& locShell="bash",
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

    // A remote machine in the ssh family: the test the constructor applies.  A
    // build without libssh serves these machines with the OpenSSH client.
    static bool usesSsh(const string& machine, const string& remShell,
                        const string& userName);
    static bool isRemote(const string& machine, const string& remShell,
                         const string& userName="");
    // "" unless machine names THIS machine; otherwise a short statement of
    // what isRemote() will actually do with it for this shell and user
    // ("runs locally, as <you>" / "via <shell> to <machine> as <user>").
    static string localityNote(const string& machine, const string& remShell,
                               const string& userName="");
    static bool isSameDomain(const string& machine);
    static string whereami(void);

private:
    // get/put for every overload: local cp, or SFTP for an ssh machine.
    static bool copyFiles(bool putFlag, string& errMessage,
                          const string& machine, const string& remShell,
                          const string& userName, const string& password,
                          const vector<string>& files, const string& toFile);
    static bool globFiles(const char** inFiles, char**& outFiles,int& numFiles);

    static const bool getPassCache(const string& shell, const string& machine,
                                   const string& userName, string& passBuf);
    static void  setPassCache(const string& shell, const string& machine,
                              const string& userName, const string& password);

    bool p_connected;
    int p_timeout;           // seconds for execout(); <= 0 means none
    string p_shell;
    string p_machine;
    string p_errMessage;
    Transport* p_transport;  // null until a connection is made
    bool p_ssh;              // the Transport is a RemoteTransport
    string p_scriptPrefix;   // PATH/LD_LIBRARY_PATH setup for every script
    DirectTransport::Stream p_stream;
    RemoteStream* p_sshStream;

    bool sshConnect(const string& machine, const string& userName,
                    const string& password, const string& jumpHost = "");
    bool sshConnectLibssh(const string& machine, const string& userName,
                          const string& password, const string& jumpHost);
    bool sshConnectOpenssh(const string& machine, const string& userName,
                           const string& jumpHost);
    static bool useOpenssh(const string& machine, const string& userName,
                           const string& jumpHost);
    void adoptTransport(Transport* t);
    bool sshHop(const string& hopMachine, const string& locShell,
                const string& userName, const string& password,
                const string& shellPath, const string& libPath,
                const string& sourceFile);
    bool importSourceFile(const string& sourceFile, const string& locShell,
                          bool loginShell, std::set<std::string>& changed);
    bool transferFile(bool putFlag, const string& local, const string& remote);
    static bool sshCopy(bool putFlag, const string& machine,
                        const string& remShell, const string& userName,
                        const string& password, const vector<string>& files,
                        const string& toFile, string& errMessage);
    static bool transportCopy(RemoteTransport* t, bool putFlag,
                              const vector<string>& files,
                              const string& toFile, string& errMessage);

}; // RCommand

#endif /* RCOMMAND_HH */
