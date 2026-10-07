// DirectTransport on Windows (#133): CreateProcessW with pipes.  Scripts are
// POSIX sh, so they need an sh.exe (MSYS2 or Git for Windows) found at run
// time; without one every script call fails with a message saying so.
#ifdef _WIN32

#include "comm/DirectTransport.H"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <map>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include <tlhelp32.h>

namespace {

std::wstring widen(const std::string& s)
{
  if (s.empty()) return std::wstring();
  int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), 0, 0);
  std::wstring w(n, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &w[0], n);
  return w;
}

std::string narrow(const std::wstring& w)
{
  if (w.empty()) return std::string();
  int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), 0, 0, 0, 0);
  std::string s(n, '\0');
  WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), &s[0], n, 0, 0);
  return s;
}

std::string lastError(const char* what)
{
  DWORD e = GetLastError();
  wchar_t* msg = 0;
  FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                 FORMAT_MESSAGE_IGNORE_INSERTS, 0, e, 0, (LPWSTR)&msg, 0, 0);
  std::string m = msg ? narrow(msg) : std::string();
  if (msg) LocalFree(msg);
  while (!m.empty() && (m.back() == '\n' || m.back() == '\r' || m.back() == ' '))
    m.pop_back();
  return std::string(what) + ": " + m + " (" + std::to_string((unsigned long)e) + ")";
}

// One argument quoted so CommandLineToArgvW (and the MSVC/MSYS runtimes)
// read it back unchanged: backslashes only matter before a quote.
std::wstring quoteArg(const std::wstring& a)
{
  if (!a.empty() && a.find_first_of(L" \t\n\v\"") == std::wstring::npos) return a;
  std::wstring q = L"\"";
  size_t bs = 0;
  for (size_t i = 0; i < a.size(); i++) {
    wchar_t c = a[i];
    if (c == L'\\') { bs++; continue; }
    if (c == L'"') { q.append(bs * 2 + 1, L'\\'); q += L'"'; }
    else { q.append(bs, L'\\'); q += c; }
    bs = 0;
  }
  q.append(bs * 2, L'\\');
  q += L'"';
  return q;
}

bool exists(const std::wstring& p) { return GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES; }

bool isDir(const std::wstring& p)
{
  DWORD a = GetFileAttributesW(p.c_str());
  return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

std::wstring exeDir()
{
  wchar_t self[MAX_PATH];
  DWORD n = GetModuleFileNameW(0, self, MAX_PATH);
  if (n == 0 || n >= MAX_PATH) return L"";
  std::wstring d(self, n);
  size_t s = d.find_last_of(L"\\/");
  return s == std::wstring::npos ? L"" : d.substr(0, s);
}

// The POSIX shell scripts run under: $ECCE_SH, one shipped beside the
// program or under $ECCE_HOME, one on PATH, then the usual MSYS2/Git homes.
std::wstring findShell()
{
  std::vector<std::wstring> cand;
  if (const char* e = getenv("ECCE_SH")) cand.push_back(widen(e));
  std::wstring d = exeDir();
  if (!d.empty()) {
    cand.push_back(d + L"\\sh.exe");
    cand.push_back(d + L"\\..\\usr\\bin\\sh.exe");
  }
  if (const char* h = getenv("ECCE_HOME")) {
    std::wstring hh = widen(h);
    cand.push_back(hh + L"\\bin\\sh.exe");
    cand.push_back(hh + L"\\usr\\bin\\sh.exe");
  }
  for (size_t i = 0; i < cand.size(); i++)
    if (exists(cand[i])) return cand[i];
  wchar_t buf[MAX_PATH];
  DWORD n = SearchPathW(0, L"sh.exe", 0, MAX_PATH, buf, 0);
  if (n > 0 && n < MAX_PATH) return buf;
  static const wchar_t* const fixed[] = {
    L"C:\\msys64\\usr\\bin\\sh.exe", L"C:\\Program Files\\Git\\usr\\bin\\sh.exe",
    L"C:\\Program Files\\Git\\bin\\sh.exe" };
  for (size_t i = 0; i < sizeof fixed / sizeof *fixed; i++)
    if (exists(fixed[i])) return fixed[i];
  return L"";
}

const char* const kNoShell =
    "No POSIX shell (sh.exe) found: install MSYS2 or Git for Windows, or set "
    "ECCE_SH to the path of sh.exe";

std::wstring dirOf(const std::wstring& path)
{
  size_t s = path.find_last_of(L"\\/");
  return s == std::wstring::npos ? L"" : path.substr(0, s);
}

// The environment block: this process's variables, then `set`, minus `unset`
// (names compare case-insensitively on Windows), with pathFront put before PATH.
std::wstring envBlock(const std::map<std::string, std::string>& set,
                      const std::map<std::string, bool>& unset,
                      const std::wstring& pathFront)
{
  std::map<std::wstring, std::wstring> v;   // upper-case name -> NAME=value
  auto up = [](std::wstring n) {
    for (size_t i = 0; i < n.size(); i++) n[i] = towupper(n[i]);
    return n;
  };
  wchar_t* cur = GetEnvironmentStringsW();
  for (wchar_t* e = cur; cur && *e; e += wcslen(e) + 1) {
    std::wstring s(e);
    size_t eq = s.find(L'=', s[0] == L'=' ? 1 : 0);
    v[up(eq == std::wstring::npos ? s : s.substr(0, eq))] = s;
  }
  if (cur) FreeEnvironmentStringsW(cur);
  for (auto i = set.begin(); i != set.end(); ++i)
    v[up(widen(i->first))] = widen(i->first) + L"=" + widen(i->second);
  for (auto i = unset.begin(); i != unset.end(); ++i) v.erase(up(widen(i->first)));
  if (!pathFront.empty()) {
    auto p = v.find(L"PATH");
    if (p == v.end()) v[L"PATH"] = L"PATH=" + pathFront;
    else p->second = L"PATH=" + pathFront + L";" + p->second.substr(5);
  }
  std::wstring blk;
  for (auto i = v.begin(); i != v.end(); ++i) { blk += i->second; blk += L'\0'; }
  blk += L'\0';
  return blk;
}

SECURITY_ATTRIBUTES inheritSa()
{
  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof sa;
  sa.lpSecurityDescriptor = 0;
  sa.bInheritHandle = TRUE;
  return sa;
}

HANDLE openNul(bool write)
{
  SECURITY_ATTRIBUTES sa = inheritSa();
  return CreateFileW(L"NUL", write ? GENERIC_WRITE : GENERIC_READ,
                     FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, 0);
}

// A pipe whose child end is inheritable and whose parent end is not.
bool makePipe(bool childReads, HANDLE& parentEnd, HANDLE& childEnd)
{
  SECURITY_ATTRIBUTES sa = inheritSa();
  HANDLE r, w;
  if (!CreatePipe(&r, &w, &sa, 65536)) return false;
  parentEnd = childReads ? w : r;
  childEnd = childReads ? r : w;
  SetHandleInformation(parentEnd, HANDLE_FLAG_INHERIT, 0);
  return true;
}

void closeH(HANDLE& h)
{
  if (h && h != INVALID_HANDLE_VALUE) CloseHandle(h);
  h = 0;
}

// An inheritable copy of the handle behind a C runtime descriptor.
HANDLE dupFd(int fd)
{
  HANDLE src = (HANDLE)_get_osfhandle(fd), dst = 0;
  if (src == INVALID_HANDLE_VALUE) return 0;
  if (!DuplicateHandle(GetCurrentProcess(), src, GetCurrentProcess(), &dst, 0,
                       TRUE, DUPLICATE_SAME_ACCESS))
    return 0;
  return dst;
}

struct Spawn {
  Spawn() : in(0), out(0), err(0), flags(0) {}
  std::wstring exe, cmd, cwd;
  const std::wstring* env;
  HANDLE in, out, err;   // inheritable; a null one becomes NUL
  DWORD flags;
};

// Starts the process with only in/out/err inherited.  The caller closes its
// copies of the child ends.
bool startProc(Spawn& sp, PROCESS_INFORMATION& pi, std::string& error)
{
  HANDLE nulIn = 0, nulOut = 0;
  HANDLE in = sp.in, out = sp.out, err = sp.err;
  if (!in) in = nulIn = openNul(false);
  if (!out) out = nulOut = openNul(true);
  if (!err) err = out;
  std::vector<HANDLE> list;
  HANDLE all[3] = { in, out, err };
  for (int i = 0; i < 3; i++) {
    bool dup = false;
    for (size_t j = 0; j < list.size(); j++) if (list[j] == all[i]) dup = true;
    if (!dup) list.push_back(all[i]);
  }

  SIZE_T sz = 0;
  InitializeProcThreadAttributeList(0, 1, 0, &sz);
  std::vector<char> attrBuf(sz);
  LPPROC_THREAD_ATTRIBUTE_LIST al = (LPPROC_THREAD_ATTRIBUTE_LIST)&attrBuf[0];
  bool ok = InitializeProcThreadAttributeList(al, 1, 0, &sz) &&
            UpdateProcThreadAttribute(al, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                      &list[0], list.size() * sizeof(HANDLE), 0, 0);
  if (!ok) {
    error = lastError("handle list");
    closeH(nulIn); closeH(nulOut);
    return false;
  }

  STARTUPINFOEXW si;
  memset(&si, 0, sizeof si);
  si.StartupInfo.cb = sizeof si;
  si.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
  si.StartupInfo.hStdInput = in;
  si.StartupInfo.hStdOutput = out;
  si.StartupInfo.hStdError = err;
  si.lpAttributeList = al;

  std::vector<wchar_t> cmd(sp.cmd.begin(), sp.cmd.end());
  cmd.push_back(L'\0');
  DWORD fl = sp.flags | EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT;
  const wchar_t* exe = sp.exe.empty() ? 0 : sp.exe.c_str();
  const wchar_t* cwd = sp.cwd.empty() ? 0 : sp.cwd.c_str();
  void* env = sp.env ? (void*)sp.env->c_str() : 0;
  BOOL r = CreateProcessW(exe, &cmd[0], 0, 0, TRUE, fl | CREATE_BREAKAWAY_FROM_JOB,
                          env, cwd, &si.StartupInfo, &pi);
  if (!r)   // the job we are in does not allow breakaway
    r = CreateProcessW(exe, &cmd[0], 0, 0, TRUE, fl, env, cwd, &si.StartupInfo, &pi);
  if (!r) error = lastError("CreateProcess");
  DeleteProcThreadAttributeList(al);
  closeH(nulIn); closeH(nulOut);
  if (r) CloseHandle(pi.hThread);
  return r != 0;
}

ULONGLONG ctime64(DWORD pid)
{
  HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
  if (!h) return 0;
  FILETIME c, e, k, u;
  ULONGLONG t = 0;
  if (GetProcessTimes(h, &c, &e, &k, &u))
    t = ((ULONGLONG)c.dwHighDateTime << 32) | c.dwLowDateTime;
  CloseHandle(h);
  return t;
}

int exitStatus(DWORD code)
{
  // A crash code is not a shell status; report it as a signal-style 128+n.
  return code >= 0x80000000u ? 139 : (int)code;
}

ULONGLONG tick() { return GetTickCount64(); }

// The script as a file, so sh reads it from there and the script's own stdin
// stays free (a stdin script would be eaten by the commands it runs).
std::wstring writeScript(const std::string& script, std::string& error)
{
  static std::atomic<int> counter(0);
  wchar_t tmp[MAX_PATH];
  DWORD n = GetTempPathW(MAX_PATH, tmp);
  if (n == 0 || n >= MAX_PATH) { error = lastError("GetTempPath"); return L""; }
  std::wstring path = std::wstring(tmp, n) + L"ecce-sh-" +
    std::to_wstring(GetCurrentProcessId()) + L"-" +
    std::to_wstring(counter++) + L"-" + std::to_wstring(tick()) + L".sh";
  HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, 0, CREATE_NEW,
                         FILE_ATTRIBUTE_TEMPORARY, 0);
  if (h == INVALID_HANDLE_VALUE) { error = lastError("temporary script"); return L""; }
  size_t done = 0;
  bool ok = true;
  while (done < script.size() && ok) {
    DWORD w = 0;
    ok = WriteFile(h, script.data() + done, (DWORD)(script.size() - done), &w, 0) && w > 0;
    done += w;
  }
  CloseHandle(h);
  if (!ok) { error = lastError("temporary script"); DeleteFileW(path.c_str()); return L""; }
  for (size_t i = 0; i < path.size(); i++) if (path[i] == L'\\') path[i] = L'/';
  return path;
}

struct Collected {
  std::string out, err;
  bool timedOut;
  DWORD code;
  bool started;
};

void drain(HANDLE& h, std::string& dst, bool& progress)
{
  while (h) {
    DWORD avail = 0;
    if (!PeekNamedPipe(h, 0, 0, 0, &avail, 0)) { closeH(h); return; }
    if (avail == 0) return;
    char buf[65536];
    DWORD got = 0;
    if (!ReadFile(h, buf, avail < sizeof buf ? avail : (DWORD)sizeof buf, &got, 0) ||
        got == 0) {
      closeH(h);
      return;
    }
    dst.append(buf, got);
    progress = true;
  }
}

// Runs a prepared process and collects its output.  Input is `input` followed
// by what is read from inFd; stdout goes to outFd when that is >= 0.
// timeoutSec is the idle limit, as in the POSIX version.
TransportResult runProc(Spawn& sp, const std::string& input, int inFd, int outFd,
                        int timeoutSec)
{
  TransportResult res;
  HANDLE inP = 0, inC = 0, outP = 0, outC = 0, errP = 0, errC = 0;
  bool ownOutC = true;
  if (!makePipe(true, inP, inC) || !makePipe(false, errP, errC) ||
      (outFd < 0 && !makePipe(false, outP, outC))) {
    res.error = lastError("pipe");
    closeH(inP); closeH(inC); closeH(errP); closeH(errC); closeH(outP); closeH(outC);
    return res;
  }
  if (outFd >= 0) { outC = dupFd(outFd); }
  sp.in = inC; sp.out = outC; sp.err = errC;
  PROCESS_INFORMATION pi;
  memset(&pi, 0, sizeof pi);
  std::string error;
  bool ok = startProc(sp, pi, error);
  closeH(inC); closeH(errC); if (ownOutC) closeH(outC);
  if (!ok) {
    res.error = error;
    closeH(inP); closeH(errP); closeH(outP);
    return res;
  }

  std::atomic<ULONGLONG> wrote(tick());
  HANDLE hin = inP;
  std::thread writer([&]() {
    size_t done = 0;
    bool ok2 = true;
    while (done < input.size() && ok2) {
      DWORD w = 0;
      DWORD chunk = (DWORD)std::min<size_t>(input.size() - done, 65536);
      ok2 = WriteFile(hin, input.data() + done, chunk, &w, 0) && w > 0;
      done += w;
      wrote = tick();
    }
    char buf[65536];
    while (ok2 && inFd >= 0) {
      int got = _read(inFd, buf, sizeof buf);
      if (got <= 0) break;
      for (int off = 0; off < got && ok2;) {
        DWORD w = 0;
        ok2 = WriteFile(hin, buf + off, got - off, &w, 0) && w > 0;
        off += w;
      }
      wrote = tick();
    }
    CloseHandle(hin);
  });

  ULONGLONG last = tick();
  bool expired = false;
  for (;;) {
    bool progress = false;
    drain(outP, res.out, progress);
    drain(errP, res.err, progress);
    if (progress) last = tick();
    if (wrote > last) last = wrote;
    if (!outP && !errP) break;
    if (timeoutSec > 0 && tick() - last >= (ULONGLONG)timeoutSec * 1000) {
      expired = true;
      break;
    }
    if (!progress) WaitForSingleObject(pi.hProcess, 5);
  }
  if (!expired) {
    // Output closed; the process may still be running.
    for (;;) {
      DWORD w = WaitForSingleObject(pi.hProcess, 20);
      if (w == WAIT_OBJECT_0) break;
      if (wrote > last) last = wrote;
      if (timeoutSec > 0 && tick() - last >= (ULONGLONG)timeoutSec * 1000) {
        expired = true;
        break;
      }
    }
  }
  if (expired) {
    res.timedOut = true;
    res.error = "timed out after " + std::to_string(timeoutSec) + " s";
    DirectTransport::killTree(pi.dwProcessId, 143);
    WaitForSingleObject(pi.hProcess, 5000);
    bool dummy = false;
    drain(outP, res.out, dummy);
    drain(errP, res.err, dummy);
  }
  closeH(outP); closeH(errP);
  // A writer stuck on a child that never reads must not outlive us.
  CancelIoEx(hin, 0);
  writer.join();

  DWORD code = 0;
  if (GetExitCodeProcess(pi.hProcess, &code) && code != STILL_ACTIVE)
    res.status = expired ? 143 : exitStatus(code);
  else
    res.status = -1;
  CloseHandle(pi.hProcess);
  return res;
}

// argv[0] as a path: itself when it names a directory part, else PATH.
std::wstring findProgram(const std::string& name)
{
  std::wstring w = widen(name);
  if (w.find_first_of(L"/\\:") != std::wstring::npos) {
    for (size_t i = 0; i < w.size(); i++) if (w[i] == L'/') w[i] = L'\\';
    return exists(w) ? w : (exists(w + L".exe") ? w + L".exe" : L"");
  }
  wchar_t buf[MAX_PATH];
  DWORD n = SearchPathW(0, w.c_str(), L".exe", MAX_PATH, buf, 0);
  return (n > 0 && n < MAX_PATH) ? std::wstring(buf) : std::wstring();
}

std::wstring cmdLine(const std::vector<std::string>& args)
{
  std::wstring c;
  for (size_t i = 0; i < args.size(); i++) {
    if (i) c += L' ';
    c += quoteArg(widen(args[i]));
  }
  return c;
}

// "sh <script>" in `dir`; empty error on success.
bool shellSpawn(const DirectTransport& t, const std::wstring& shell,
                const std::wstring& scriptArgs, const std::map<std::string, std::string>& set,
                const std::map<std::string, bool>& unset, const std::string& dir,
                Spawn& sp, std::wstring& envStore, TransportResult& res)
{
  if (!dir.empty() && !isDir(widen(dir))) {
    res.status = 97;
    res.err = "sh: cd: " + dir + ": No such file or directory\n";
    return false;
  }
  envStore = envBlock(set, unset, dirOf(shell));
  sp.exe = shell;
  sp.cmd = quoteArg(shell) + L" " + scriptArgs;
  sp.cwd = widen(dir);
  sp.env = &envStore;
  sp.flags = CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP;
  (void)t;
  return true;
}

}  // namespace

bool DirectTransport::killTree(long pid, unsigned exitCode)
{
  HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  std::vector<DWORD> order;
  if (snap != INVALID_HANDLE_VALUE) {
    std::multimap<DWORD, DWORD> kids;   // parent -> child
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof pe;
    for (BOOL ok = Process32FirstW(snap, &pe); ok; ok = Process32NextW(snap, &pe))
      kids.insert(std::make_pair(pe.th32ParentProcessID, pe.th32ProcessID));
    CloseHandle(snap);
    // Parents first, so nothing respawns what is about to be killed.  A child
    // created before its parent is a recycled pid, not a descendant.
    std::vector<DWORD> todo(1, (DWORD)pid);
    while (!todo.empty()) {
      DWORD p = todo.back();
      todo.pop_back();
      order.push_back(p);
      ULONGLONG pt = ctime64(p);
      auto range = kids.equal_range(p);
      for (auto i = range.first; i != range.second; ++i) {
        if (i->second == p || i->second == GetCurrentProcessId()) continue;
        if (std::find(order.begin(), order.end(), i->second) != order.end()) continue;
        ULONGLONG ct = ctime64(i->second);
        if (pt && ct && ct < pt) continue;
        todo.push_back(i->second);
      }
    }
  } else {
    order.push_back((DWORD)pid);
  }
  bool any = false;
  for (size_t i = 0; i < order.size(); i++) {
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, order[i]);
    if (!h) continue;
    if (TerminateProcess(h, exitCode)) any = true;
    CloseHandle(h);
  }
  return any;
}

TransportResult DirectTransport::run(const std::string& script, int timeoutSec)
{
  TransportResult res;
  std::wstring shell = findShell();
  if (shell.empty()) { res.error = kNoShell; return res; }
  std::string error;
  std::wstring file = writeScript(script, error);
  if (file.empty()) { res.error = error; return res; }
  Spawn sp;
  std::wstring env;
  if (shellSpawn(*this, shell, quoteArg(file), p_env, p_unset, p_dir, sp, env, res))
    res = runProc(sp, "", -1, -1, timeoutSec);
  if (!getenv("ECCE_KEEP_SCRIPTS")) DeleteFileW(file.c_str());
  return res;
}

TransportResult DirectTransport::runProcess(const std::vector<std::string>& args,
                                            const std::string& input, int inFd,
                                            int outFd, int timeoutSec)
{
  TransportResult res;
  if (args.empty()) { res.error = "no command"; return res; }
  std::wstring exe = findProgram(args[0]);
  if (exe.empty()) { res.error = args[0] + ": command not found"; return res; }
  std::map<std::string, std::string> none;
  std::map<std::string, bool> noneUnset;
  std::wstring env = envBlock(none, noneUnset, L"");
  Spawn sp;
  sp.exe = exe;
  sp.cmd = cmdLine(args);
  sp.env = &env;
  sp.flags = CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP;
  return runProc(sp, input, inFd, outFd, timeoutSec);
}

long DirectTransport::spawnProcess(const std::vector<std::string>& args, int in,
                                   int out, int err, std::string& error)
{
  if (args.empty()) { error = "no command"; return -1; }
  std::wstring exe = findProgram(args[0]);
  if (exe.empty()) { error = args[0] + ": command not found"; return -1; }
  std::map<std::string, std::string> none;
  std::map<std::string, bool> noneUnset;
  std::wstring env = envBlock(none, noneUnset, L"");
  Spawn sp;
  sp.exe = exe;
  sp.cmd = cmdLine(args);
  sp.env = &env;
  sp.flags = CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP;
  sp.in = dupFd(in); sp.out = dupFd(out); sp.err = dupFd(err);
  PROCESS_INFORMATION pi;
  memset(&pi, 0, sizeof pi);
  bool ok = startProc(sp, pi, error);
  closeH(sp.in); closeH(sp.out); closeH(sp.err);
  if (!ok) return -1;
  CloseHandle(pi.hProcess);
  return (long)pi.dwProcessId;
}

long DirectTransport::spawnDetached(const std::string& script, std::string& error,
                                    const std::string& logFile)
{
  std::wstring shell = findShell();
  if (shell.empty()) { error = kNoShell; return -1; }
  // The script is one command-line argument; CreateProcess caps that near
  // 32 KiB.
  if (script.size() > 30000) {
    error = "script too long to spawn detached";
    return -1;
  }
  TransportResult tr;
  Spawn sp;
  std::wstring env;
  if (!shellSpawn(*this, shell, L"-c " + quoteArg(widen(script)), p_env, p_unset,
                  p_dir, sp, env, tr)) {
    error = tr.err;
    return -1;
  }
  HANDLE log = 0;
  if (!logFile.empty()) {
    SECURITY_ATTRIBUTES sa = inheritSa();
    log = CreateFileW(widen(logFile).c_str(), GENERIC_WRITE,
                      FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_ALWAYS, 0, 0);
    if (log == INVALID_HANDLE_VALUE) {
      error = "cannot open " + logFile + ": " + lastError("CreateFile");
      return -1;
    }
    // msys programs lose output written through an append-only handle.
    SetFilePointer(log, 0, 0, FILE_END);
    sp.out = sp.err = log;
  }
  PROCESS_INFORMATION pi;
  memset(&pi, 0, sizeof pi);
  bool ok = startProc(sp, pi, error);
  closeH(log);
  if (!ok) return -1;
  CloseHandle(pi.hProcess);
  return (long)pi.dwProcessId;
}

bool DirectTransport::openStream(const std::string& script, Stream& s,
                                 std::string& error)
{
  std::wstring shell = findShell();
  if (shell.empty()) { error = kNoShell; return false; }
  if (script.size() > 30000) { error = "script too long to stream"; return false; }
  TransportResult tr;
  Spawn sp;
  std::wstring env;
  if (!shellSpawn(*this, shell, L"-c " + quoteArg(widen(script)), p_env, p_unset,
                  p_dir, sp, env, tr)) {
    error = tr.err;
    return false;
  }
  HANDLE inP = 0, inC = 0, outP = 0, outC = 0;
  if (!makePipe(true, inP, inC)) { error = lastError("pipe"); return false; }
  if (!makePipe(false, outP, outC)) {
    error = lastError("pipe");
    closeH(inP); closeH(inC);
    return false;
  }
  sp.in = inC; sp.out = outC; sp.err = outC;
  PROCESS_INFORMATION pi;
  memset(&pi, 0, sizeof pi);
  bool ok = startProc(sp, pi, error);
  closeH(inC); closeH(outC);
  if (!ok) { closeH(inP); closeH(outP); return false; }
  CloseHandle(pi.hProcess);
  s.pid = (long)pi.dwProcessId;
  s.wfd = _open_osfhandle((intptr_t)inP, _O_WRONLY | _O_BINARY | _O_NOINHERIT);
  s.rfd = _open_osfhandle((intptr_t)outP, _O_RDONLY | _O_BINARY | _O_NOINHERIT);
  return true;
}

bool DirectTransport::writeStream(Stream& s, const std::string& data)
{
  if (s.wfd < 0) return false;
  size_t done = 0;
  while (done < data.size()) {
    int w = _write(s.wfd, data.data() + done, (unsigned)(data.size() - done));
    if (w <= 0) return false;
    done += w;
  }
  return true;
}

// There is no SIGINT to send a process group without a console, so an
// interrupt ends the whole stream process tree.
void DirectTransport::interruptStream(Stream& s)
{
  if (s.pid > 0) killTree(s.pid, 130);
}

int DirectTransport::closeStream(Stream& s, int graceMs)
{
  if (s.wfd >= 0) { _close(s.wfd); s.wfd = -1; }
  if (s.rfd >= 0) { _close(s.rfd); s.rfd = -1; }
  if (s.pid <= 0) return -1;
  DWORD pid = (DWORD)s.pid;
  s.pid = -1;
  HANDLE h = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION |
                         PROCESS_TERMINATE, FALSE, pid);
  if (!h) return -1;
  if (WaitForSingleObject(h, graceMs > 0 ? graceMs : 0) != WAIT_OBJECT_0) {
    killTree(pid, 143);
    WaitForSingleObject(h, 5000);
  }
  DWORD code = 0;
  int st = GetExitCodeProcess(h, &code) && code != STILL_ACTIVE ? exitStatus(code) : -1;
  CloseHandle(h);
  return st;
}

#endif  // _WIN32
