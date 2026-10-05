/**
 * @file
 *
 *
 */
#include <iostream>
 using std::cout;
 using std::endl;
 using std::flush;
#include <fstream>
   using std::ofstream;

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>  // stat
#include <sys/types.h>

#include <map>

#include "wx/utils.h"
#include "wx/process.h"
#include "wx/fswatcher.h"
#include "wx/filename.h"

#include "util/ErrMsg.H"
#include "util/UserEditor.H"
#include "util/TempStorage.H"
#include "util/SFile.H"
#include "util/EditListener.H"
#include "util/EditEvent.H"

#include "wxgui/WxEditSessionMgr.H"

static const int MAX_EDITOR_ARGS = 32;

// Modification time to the nanosecond plus size: two saves within one
// second must still register as two changes.
static bool fileStamp(const string& path, long long& stamp, long long& size)
{
   struct stat sb;
   if (stat(path.c_str(), &sb) != 0) return false;
#ifdef __APPLE__
   const struct timespec& mt = sb.st_mtimespec;
#else
   const struct timespec& mt = sb.st_mtim;
#endif
   stamp = (long long)mt.tv_sec * 1000000000LL + mt.tv_nsec;
   size = (long long)sb.st_size;
   return true;
}

static vector<EditSession>::iterator findSession(vector<EditSession>& v, int pid)
{
   vector<EditSession>::iterator it = v.begin();
   while (it != v.end() && (*it).pid != pid) it++;
   return it;
}


//////////////////////////////////////////////////////////////////////////////
// Helper class - WxEditProcess
//////////////////////////////////////////////////////////////////////////////

/**
 * Owns the editor child.  wx reaps only the processes it started, so this
 * cannot steal the children of system() calls on other threads the way a
 * process-wide SIGCHLD handler did.  On exit the listener is notified IFF
 * the file changed since the last notification, then the temp file goes.
 */
class WxEditProcess : public wxProcess
{
   public:
      WxEditProcess() : wxProcess(wxPROCESS_DEFAULT) {}

      void OnTerminate(int pid, int status)
      {
         vector<EditSession>::iterator it =
            findSession(WxEditSessionMgr::p_sessions, pid);
         if (it != WxEditSessionMgr::p_sessions.end()) {
            WxEditSessionMgr::finishSession(*it);
            it = findSession(WxEditSessionMgr::p_sessions, pid);
            if (it != WxEditSessionMgr::p_sessions.end()) {
               WxEditSessionMgr::p_sessions.erase(it);
            }
         }
         delete this;
      }
};


//////////////////////////////////////////////////////////////////////////////
// Helper class - WxEditWatcher
//////////////////////////////////////////////////////////////////////////////

/**
 * Watches the directories of files under edit, so a save is reported while
 * the editor is still open.  Editors that save by writing a new file and
 * renaming it show up as create/rename events in the directory, so the
 * directory is watched and each event just re-stats the session files.
 * If the watch cannot be set up, the change is still picked up when the
 * editor exits.
 */
class WxEditWatcher : public wxEvtHandler
{
   public:
      WxEditWatcher() : p_watcher(NULL)
      {
         Bind(wxEVT_FSWATCHER, &WxEditWatcher::onEvent, this);
      }

      ~WxEditWatcher() { delete p_watcher; }

      static WxEditWatcher& instance()
      {
         static WxEditWatcher* w = new WxEditWatcher();
         return *w;
      }

      void add(const string& dir)
      {
         if (p_dirs[dir]++ > 0) return;
         if (p_watcher == NULL) {
            p_watcher = new wxFileSystemWatcher();
            p_watcher->SetOwner(this);
         }
         p_watcher->Add(wxFileName::DirName(wxString::FromUTF8(dir.c_str())),
               wxFSW_EVENT_CREATE | wxFSW_EVENT_MODIFY | wxFSW_EVENT_RENAME);
      }

      void remove(const string& dir)
      {
         std::map<string,int>::iterator it = p_dirs.find(dir);
         if (it == p_dirs.end()) return;
         if (--(it->second) > 0) return;
         p_dirs.erase(it);
         if (p_watcher != NULL) {
            p_watcher->Remove(
               wxFileName::DirName(wxString::FromUTF8(dir.c_str())));
         }
      }

   private:
      void onEvent(wxFileSystemWatcherEvent& evt)
      {
         wxString path = evt.GetPath().GetFullPath();
         wxString newPath = evt.GetNewPath().GetFullPath();
         vector<int> pids;
         for (size_t i = 0; i < WxEditSessionMgr::p_sessions.size(); i++) {
            wxString f = wxString::FromUTF8(
                  WxEditSessionMgr::p_sessions[i].file.c_str());
            if (f == path || f == newPath) {
               pids.push_back(WxEditSessionMgr::p_sessions[i].pid);
            }
         }
         for (size_t i = 0; i < pids.size(); i++) {
            WxEditSessionMgr::checkSession(pids[i]);
         }
      }

      wxFileSystemWatcher *p_watcher;
      std::map<string,int> p_dirs;
};


//////////////////////////////////////////////////////////////////////////////
// WxEditSessionMgr
//////////////////////////////////////////////////////////////////////////////





// Class statics
vector<EditSession> WxEditSessionMgr::p_sessions;


WxEditSessionMgr::WxEditSessionMgr( )
{
}



WxEditSessionMgr::~WxEditSessionMgr( )
{
}


/**
 * Send kill signal to terminate all edit sessions. 
 * Probably not wise to use except when closing down.
 */
void WxEditSessionMgr::stopAll()
{
   vector<EditSession>::iterator it = p_sessions.begin();

   while (it != WxEditSessionMgr::p_sessions.end()) {
      (void)kill((*it).pid, SIGTERM);
      it++;
   }
}


bool WxEditSessionMgr::isEditSessionInProgress()
{
   return p_sessions.size() != 0;
}



/**
 * Initiate an edit session.
 * @throw EcceException if unable to find external editor.
 */
void WxEditSessionMgr::edit(SFile* file, 
      const string& id, 
      EditListener *l,
      bool readOnly, 
      const string & name)
{

   if (file != (SFile*)0) {

      if (file->exists()) {
         editFile(file,id,l,readOnly,name);

      } else {
         throw EcceException("File not found.", WHERE);
      }
   } else {
      throw EcceException("Null file object.", WHERE);
   }

}



void WxEditSessionMgr::edit(const string& text, 
      const string& id, 
      EditListener *l,
      bool readOnly, 
      const string& name)
{

   SFile* file = makeTemporaryFile(text);

   editFile(file,id,l,readOnly,name);

   delete file;

}


/**
 * Create a temporary file for the data.
 * @see TempStorage.
 */
SFile* WxEditSessionMgr::makeTemporaryFile(const string& data)
{
   SFile* file = TempStorage::getTempFile();
   ofstream os(file->path(true).c_str());

   if (os) {
      os << data << "\n" << flush;
      os.close();
   }

   return file;
}


/**
 * Starts the editor asynchronously.  A WxEditProcess ends the session
 * when the editor exits and a directory watch reports saves meanwhile;
 * neither needs a signal handler or a timer.
 */
void WxEditSessionMgr::startSession (const string& app,
      /*const*/ char *args[],
      SFile* file,
      const string& id,
      EditListener *l)
{
   WxEditProcess *proc = new WxEditProcess();
   long pid = wxExecute(args, wxEXEC_ASYNC, proc);

   if (pid <= 0) {
      delete proc;
      EE_RT_ASSERT(false, EE_WARNING, "Failed to start editor " + app);
      return;
   }

   EditSession session;
   session.callerids = id;
   session.file = file->path();
   session.l = l;
   session.pid = (int)pid;
   if (!fileStamp(session.file, session.stamp, session.size)) {
      session.stamp = 0;
      session.size = 0;
   }
   p_sessions.push_back(session);

   size_t slash = session.file.rfind('/');
   WxEditWatcher::instance().add(
         slash == string::npos ? "." : session.file.substr(0, slash));
}


/**
 * Notify the listener if the file differs from what it last saw.
 * @return true if the file exists.
 */
bool WxEditSessionMgr::notifyIfChanged(EditSession& s)
{
   long long stamp, size;
   if (!fileStamp(s.file, stamp, size)) return false;

   if (stamp != s.stamp || size != s.size) {
      s.stamp = stamp;
      s.size = size;
      EditEvent ee;
      ee.id = s.callerids;
      ee.filename = s.file;
      s.l->processEditCompletion(ee);
   }
   return true;
}


/**
 * Save-while-open: called by the directory watch.
 */
void WxEditSessionMgr::checkSession(int pid)
{
   vector<EditSession>::iterator it = findSession(p_sessions, pid);
   if (it != p_sessions.end()) {
      EditSession copy = *it;   // the listener may start another session
      notifyIfChanged(copy);
      it = findSession(p_sessions, pid);
      if (it != p_sessions.end()) {
         it->stamp = copy.stamp;
         it->size = copy.size;
      }
   }
}


/**
 * The editor has exited: last change check, then remove the temp file
 * AFTER notifying the client.
 */
void WxEditSessionMgr::finishSession(EditSession& s)
{
   EditSession copy = s;
   if (notifyIfChanged(copy)) {
      SFile file(copy.file);
      file.remove();
   }
   size_t slash = copy.file.rfind('/');
   WxEditWatcher::instance().remove(
         slash == string::npos ? "." : copy.file.substr(0, slash));
}





void WxEditSessionMgr::editFile(SFile* file, 
      const string& id, 
      EditListener *l,
      bool readOnly,
      const string& name)
{

   // Most GUI editors have no read-only flag; a read-only file makes them
   // refuse the save instead of writing to a copy that is then deleted.
   if (readOnly) {
      (void)chmod(file->path().c_str(), S_IRUSR);
   }

   UserEditor editor;

   string exe;
   char *args[MAX_EDITOR_ARGS];
   editor.getEditCommand(*file, exe, args, MAX_EDITOR_ARGS, name, readOnly);

   // If we got here, no problems getting editor command
   startSession(exe, args, file, id, l);

   editor.freeArguments(args);

}




