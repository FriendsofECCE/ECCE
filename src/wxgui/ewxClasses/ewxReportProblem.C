#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#include <wx/dir.h>
#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/process.h>
#include <wx/progdlg.h>
#include <wx/utils.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

#include "util/Ecce.H"
#include "util/GLPath.H"
#include "wxgui/ewxReportProblem.H"

namespace {

const char *ISSUE_URL = "https://github.com/FriendsofECCE/ECCE/issues/new";

wxString userHome()
{
  wxString home;
  if (wxGetEnv("ECCE_REALUSERHOME", &home) && !home.empty()) return home;
  return wxFileName::GetHomeDir();
}

bool writeText(const wxString& path, const wxString& text)
{
  wxFFile f(path, "wb");
  const wxScopedCharBuffer u = text.utf8_str();
  return f.IsOpened() && f.Write(u.data(), u.length()) == u.length();
}

#ifdef __WXMSW__
// Last 'maxBytes' of a file, so an old, large log cannot bloat the archive.
void copyTail(const wxString& from, const wxString& to, wxFileOffset maxBytes)
{
  wxFFile in(from, "rb");
  if (!in.IsOpened()) return;
  wxFileOffset len = in.Length();
  if (len > maxBytes) in.Seek(len - maxBytes);
  wxFFile out(to, "wb");
  if (!out.IsOpened()) return;
  char buf[65536];
  size_t n;
  while ((n = in.Read(buf, sizeof buf)) > 0) out.Write(buf, n);
}

// No shell on a Windows PC: logs under ~/.ECCE (never credential files)
// and the last ECCE crash records of the Application event log.
void collectWindows(const wxString& dir)
{
  const wxString state = userHome() + "\\.ECCE";
  wxArrayString logs;
  wxDir::GetAllFiles(state, &logs, "*.log", wxDIR_FILES | wxDIR_DIRS);
  wxDir::GetAllFiles(state, &logs, "error_log", wxDIR_FILES | wxDIR_DIRS);
  wxFileName::Mkdir(dir + "\\logs", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
  int n = 0;
  for (const wxString& f : logs) {
    wxString low = f.Lower();
    if (low.Contains("auth") || low.Contains("login") || low.Contains("passw"))
      continue;
    copyTail(f, wxString::Format("%s\\logs\\%03d-%s", dir, n++,
                                 wxFileName(f).GetFullName()), 2 << 20);
  }

  wxArrayString out, err;
  wxExecute("wevtutil qe Application /q:\"*[System[(EventID=1000)]]\" "
            "/c:200 /rd:true /f:text", out, err,
            wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE);
  wxString crashes, block;
  int kept = 0;
  auto flush = [&]() {
    if (kept < 10 && block.Lower().Contains("ecce")) {
      crashes += block + "\n";
      ++kept;
    }
    block.clear();
  };
  for (const wxString& l : out) {
    if (l.StartsWith("Event[")) flush();
    block += l + "\n";
  }
  flush();
  if (crashes.empty())
    crashes = "No ECCE crash records among the last 200 application errors.\n";
  for (const wxString& l : err) crashes += l + "\n";
  writeText(dir + "\\crashes.txt", crashes);
}
#else
class Finished : public wxProcess
{
public:
  Finished() : wxProcess(wxPROCESS_DEFAULT), done(false) {}
  void OnTerminate(int, int) override { done = true; }
  bool done;
};

void drain(wxProcess *proc, wxString& text)
{
  wxInputStream *in = proc->GetInputStream();
  while (in && in->CanRead()) {
    char buf[4096];
    in->Read(buf, sizeof buf);
    if (in->LastRead() == 0) break;
    text += wxString::FromUTF8(buf, in->LastRead());
  }
}

// ecce-diagnose writes ecce-diagnostics-<host>-<time>.txt into 'dir'.
void collectPosix(const wxString& dir, wxWindow *parent)
{
  wxString diag = wxString::FromUTF8(Ecce::ecceHome()) + "/bin/ecce-diagnose";
  if (!wxFileExists(diag)) {
    writeText(dir + "/diagnose-stdout.txt", "ecce-diagnose not found\n");
    return;
  }
  Finished *proc = new Finished;
  proc->Redirect();
  wxString cmd = "/bin/bash \"" + diag + "\" -o \"" + dir + "\"";
  long pid = wxExecute(cmd, wxEXEC_ASYNC | wxEXEC_MAKE_GROUP_LEADER, proc);
  if (!pid) {
    delete proc;
    writeText(dir + "/diagnose-stdout.txt",
              "ecce-diagnose could not be started\n");
    return;
  }
  wxProgressDialog dlg("Report a Problem",
                       "Collecting diagnostics, about half a minute...",
                       100, parent, wxPD_APP_MODAL);
  wxString text;
  wxStopWatch watch;
  while (!proc->done && watch.Time() < 300000) {
    dlg.Pulse();
    drain(proc, text);   // so its pipe never fills
    wxMilliSleep(100);
    wxYield();
  }
  if (proc->done) {
    drain(proc, text);
    delete proc;
  } else {
    wxProcess::Kill(pid, wxSIGTERM, wxKILL_CHILDREN);
    text += "\n(stopped after 300 s)\n";
  }
  writeText(dir + "/diagnose-stdout.txt", text);
}
#endif

void addDir(wxZipOutputStream& zip, const wxString& root, const wxString& rel)
{
  wxDir d(rel.empty() ? root : root + wxFILE_SEP_PATH + rel);
  if (!d.IsOpened()) return;
  wxString name;
  for (bool ok = d.GetFirst(&name); ok; ok = d.GetNext(&name)) {
    wxString r = rel.empty() ? name : rel + "/" + name;
    wxString full = root + wxFILE_SEP_PATH + r;
    if (wxDirExists(full)) {
      addDir(zip, root, r);
    } else {
      wxFFileInputStream in(full);
      if (!in.IsOk()) continue;
      zip.PutNextEntry(wxFileName(root).GetFullName() + "/" + r);
      zip.Write(in);
      zip.CloseEntry();
    }
  }
}

}  // namespace


wxString ewxReportProblem::collect(wxString& error, wxWindow *parent)
{
  const wxString home = userHome();
  const wxString name = "ecce-bug-" + wxDateTime::Now().Format("%Y%m%d-%H%M%S");
  const wxString dir = home + wxFILE_SEP_PATH + name;
  const wxString archive = dir + ".zip";
  if (!wxFileName::Mkdir(dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
    error = "Cannot create " + dir;
    return "";
  }

  writeText(dir + wxFILE_SEP_PATH + "version.txt",
            wxString("ECCE ") + Ecce::ecceVersion() + "\n" +
            wxGetOsDescription() + "\n" +
            wxString::FromUTF8(GLPath::describeForDisplay().c_str()) + "\n");
#ifdef __WXMSW__
  collectWindows(dir);
#else
  collectPosix(dir, parent);
#endif

  wxFFileOutputStream out(archive);
  if (!out.IsOk()) {
    error = "Cannot write " + archive;
    return "";
  }
  wxZipOutputStream zip(out);
  addDir(zip, dir, "");
  if (!zip.Close() || !out.Close()) {
    error = "Cannot write " + archive;
    return "";
  }
  return archive;
}


void ewxReportProblem::run(wxWindow *parent)
{
  wxString error;
  wxString archive = collect(error, parent);
  if (archive.empty()) {
    wxMessageBox(error, "Report a Problem", wxOK | wxICON_ERROR, parent);
    return;
  }
  const wxString folder = wxFileName(archive).GetPath();
  wxString msg = "The diagnostics are collected in\n\n" + archive +
    "\n\nIt contains no passwords, but it does contain host names, user "
    "names and paths: look through it before sending it. Nothing has been "
    "uploaded. Open the issue page and attach the archive there.";
  for (;;) {
    wxMessageDialog dlg(parent, msg, "Report a Problem",
                        wxYES_NO | wxCANCEL | wxICON_INFORMATION);
    dlg.SetYesNoCancelLabels("Open folder", "Open issue page", "Close");
    int r = dlg.ShowModal();
    if (r == wxID_YES) wxLaunchDefaultApplication(folder);
    else if (r == wxID_NO) wxLaunchDefaultBrowser(ISSUE_URL);
    else break;
  }
}
