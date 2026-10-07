#ifndef TAILSOURCE_HH
#define TAILSOURCE_HH

#include <string>

class RCommand;

/**
 * A file followed as it grows (tail -F), read through ECCE's own connection
 * to the machine: one login for the check and the stream together, so the
 * user is asked for a password or one-time code at most once, by the same
 * AuthCache/passdialog path every other remote operation uses.  No wx.
 */
class TailSource
{
public:
  TailSource();
  ~TailSource();

  // Connects to a registered machine as RCommand does (AuthCache first, then
  // passdialog) and starts following path, beginning with its last `lines`
  // lines.  A file that does not exist yet is waited for; *missing says so.
  bool open(const std::string& machineName, const std::string& shell,
            const std::string& user, const std::string& path, int lines,
            std::string& error, bool* missing = 0);
  // The same on a connection already made; takes ownership of rcmd.
  bool start(RCommand* rcmd, const std::string& path, int lines,
             std::string& error, bool* missing = 0);

  // Appends whatever has arrived, without waiting.  False once the stream
  // has ended (tail stopped or the connection closed); out may still have
  // received the last data.
  bool read(std::string& out);
  // -1 when nothing is open.
  int fd() const;
  // Stops tail on the machine and logs out.
  void close();

  // "local", "libssh" or "openssh": how the stream reaches the machine.
  const std::string& backend() const { return p_backend; }

  // The sh script that follows path; ends tail when its stdin closes, so a
  // closed window or a dropped connection leaves no tail behind.
  static std::string script(const std::string& path, int lines);
  // path as one sh word; a leading ~ still expands.
  static std::string shPath(const std::string& path);

private:
  TailSource(const TailSource&);
  TailSource& operator=(const TailSource&);

  RCommand* p_rcmd;
  std::string p_backend;
  bool p_ended;
};

#endif
