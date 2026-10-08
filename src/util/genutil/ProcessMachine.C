#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include "util/Ecce.H"
#include "util/ProcessMachine.H"

extern char** environ;


std::string ProcessMachine::encode(const std::string& text)
{
  static const char hex[] = "0123456789ABCDEF";
  std::string out;
  for (unsigned char c : text) {
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      out += (char)c;
    } else {
      out += '%';
      out += hex[c >> 4];
      out += hex[c & 15];
    }
  }
  return out;
}


std::string ProcessMachine::field(const std::string& name,
                                  const std::string& value)
{
  return "&" + encode(name) + "=" + encode(value);
}


#ifdef _WIN32
// No fork on Windows, and a perl script is not executable by itself: perl
// (the bundled Strawberry one, on PATH) runs it with the form on stdin.
int ProcessMachine::run(const std::string& form)
{
  std::string script = std::string(Ecce::ecceHome()) + "/scripts/processmachine";
  if (access(script.c_str(), 0) != 0)
    return -1;

  SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
  HANDLE rd, wr;
  if (!CreatePipe(&rd, &wr, &sa, 0))
    return -1;
  SetHandleInformation(wr, HANDLE_FLAG_INHERIT, 0);

  STARTUPINFOA si;
  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdInput = rd;
  si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
  si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
  PROCESS_INFORMATION pi;
  std::string cmd = "perl \"" + script + "\"";
  _putenv_s("CONTENT_LENGTH", std::to_string(form.size()).c_str());
  BOOL ok = CreateProcessA(nullptr, &cmd[0], nullptr, nullptr, TRUE,
                           CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
  _putenv_s("CONTENT_LENGTH", "");
  CloseHandle(rd);
  if (!ok) {
    CloseHandle(wr);
    return -1;
  }
  CloseHandle(pi.hThread);

  size_t done = 0;
  while (done < form.size()) {
    DWORD n = 0;
    if (!WriteFile(wr, form.data() + done, (DWORD)(form.size() - done), &n,
                   nullptr) || n == 0)
      break;
    done += n;
  }
  CloseHandle(wr);

  DWORD code = (DWORD)-1;
  WaitForSingleObject(pi.hProcess, INFINITE);
  GetExitCodeProcess(pi.hProcess, &code);
  CloseHandle(pi.hProcess);
  return code == 0 ? 0 : (code < 256 ? (int)code : -1);
}

#else

int ProcessMachine::run(const std::string& form)
{
  std::string script = std::string(Ecce::ecceHome()) + "/scripts/processmachine";
  if (access(script.c_str(), X_OK) != 0)
    return -1;

  // The child's environment is built before fork: the GUI is multithreaded,
  // and only async-signal-safe calls are allowed between fork and exec.
  std::vector<std::string> env;
  for (char** e = environ; e && *e; e++)
    if (strncmp(*e, "CONTENT_LENGTH=", 15) != 0)
      env.push_back(*e);
  env.push_back("CONTENT_LENGTH=" + std::to_string(form.size()));
  std::vector<char*> envp;
  for (auto& s : env)
    envp.push_back(&s[0]);
  envp.push_back(nullptr);
  char* argv[] = { &script[0], nullptr };

  int fds[2];
  if (pipe(fds) != 0)
    return -1;
  pid_t pid = fork();
  if (pid < 0) {
    close(fds[0]);
    close(fds[1]);
    return -1;
  }
  if (pid == 0) {
    dup2(fds[0], 0);
    close(fds[0]);
    close(fds[1]);
    execve(argv[0], argv, envp.data());
    _exit(127);
  }
  close(fds[0]);

  // A script that exits early must not take the GUI down with SIGPIPE.
  struct sigaction ignore, old;
  memset(&ignore, 0, sizeof(ignore));
  ignore.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &ignore, &old);
  size_t done = 0;
  while (done < form.size()) {
    ssize_t n = write(fds[1], form.data() + done, form.size() - done);
    if (n < 0 && errno == EINTR)
      continue;
    if (n <= 0)
      break;
    done += n;
  }
  close(fds[1]);
  sigaction(SIGPIPE, &old, nullptr);

  int status = 0;
  while (waitpid(pid, &status, 0) < 0) {
    if (errno != EINTR)
      return -1;
  }
  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}
#endif
