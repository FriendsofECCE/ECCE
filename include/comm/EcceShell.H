/**
 * @file 
 *
 *
 */
#ifndef ECCESHELL_HH
#define ECCESHELL_HH

#include <string>
#include <vector>
  using std::string;
  using std::vector;

class RCommand;
class RefMachine;

/**
 * Class to create loccal and remote shells (currently xterms) to
 * display information associated with a calculation.
 */
class EcceShell
{

   public:
      EcceShell(void);
      virtual ~EcceShell(void);

      string dirshell(const string& title, const string& machineName,
            const string& shell, const string& user, const string& password,
            const string& pathBase, const string& pathFull);

      string cmdshell(const string& title, const string& machineName,
            const string& shell, const string& user, const string& password,
            const string& cmd, const string& file);

      string topshell(const string& machineName, const string& shell,
            const string& user, const string& password);

      int lastStatus(void) const;   // 0=success; 1=warning, -1=error

      // Remote machine: the terminal runs locally and
      // the OpenSSH client inside it (libssh has no X11 forwarding).
      struct SshTerminal {
         string title, host, user;
         string frontend;       // front end host, "" if none
         string frontendMode;   // "forward" (-J) or "nested"; see RCommand::frontEndMode
         string geometry;       // xterm only, e.g. "80x40"
         string remoteCommand;  // as built by remoteCommand()
      };
      // The local argv (terminal, its options, ssh); false with error set.
      static bool terminalArgv(const SshTerminal& t, vector<string>& argv,
                               string& error);
      // One command line for the remote login shell (csh or sh syntax
      // alike): runs sourceFile and then cmd, or cd dir and an interactive
      // $SHELL, under the machine's own shell (mshell).
      static string remoteCommand(const string& mshell, const string& sourceFile,
                                  const string& dir, const string& cmd,
                                  string& error);
      // Starts argv detached from this process; false with error set.
      static bool spawnDetached(const vector<string>& argv, string& error);


   protected:
      string remoteShell(const string& machineName, const string& shell,
            const string& user, const string& password,
            const string& pathBase, const string& pathFull,
            const string& title, const string& cmd="");

      string shellCmd(const string& title);

      string sshTerminal(RefMachine* refMachine, const string& shell,
            const string& user, const string& password,
            const string& pathBase, const string& pathFull,
            const string& title, const string& cmd);

      EcceShell(const EcceShell&);

   private:

      int p_status;

};

#endif /* ECCESHELL_HH */
