/**
 * @file 
 *
 *
 */
#ifndef ECCESHELL_HH
#define ECCESHELL_HH

#include <string>
  using std::string;

class RCommand;

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
            const string& cmd, const string& file, bool xtermFlag=true);

      string topshell(const string& machineName, const string& shell,
            const string& user, const string& password);

      int lastStatus(void) const;   // 0=success; 1=warning, -1=error


   protected:
      string remoteShell(const string& machineName, const string& shell,
            const string& user, const string& password,
            const string& pathBase, const string& pathFull,
            const string& title, const string& cmd="",
            bool xtermFlag=true);

      string shellCmd(const string& title);

      EcceShell(const EcceShell&);

   private:

      int p_status;

};

#endif /* ECCESHELL_HH */
