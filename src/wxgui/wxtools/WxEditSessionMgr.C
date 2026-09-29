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
#include <sys/wait.h>
#include <sys/stat.h>  // stat
#include <sys/types.h>

#include "wx/timer.h"

#include "util/ErrMsg.H"
#include "util/UserEditor.H"
#include "util/TempStorage.H"
#include "util/TDateTime.H"
#include "util/SFile.H"
#include "util/EditListener.H"
#include "util/EditEvent.H"

#include "wxgui/WxEditSessionMgr.H"

extern char **environ;

static const int CHECK_INTERVAL = 2000;

//////////////////////////////////////////////////////////////////////////////
// Helper class - WxEditTimer
//////////////////////////////////////////////////////////////////////////////

/**
 * Handles timer events for WxEditFile facade.
 * This is an internal support class for WxEditSessionMgr.
 * This class is a timer that can be used for either sigchild handling
 * or to handle the case where the user has updated the file they are
 * editing.  It needs access to the EditSessions kept by WxEditSessionMgr
 * and is thus a friend.
 * 
 * Note that instances are created on the stack and not cleaned up.
 * This could be fixed by keeping a vector of these, marking them when they
 * are expired and periodically clean up expired timers.
 * I didn't bother now because its a fairly small memory problem.
 *
 * The Notify method is called to handle when the timer expires.
 * The behavior will depend on whether or not the instance was created
 * to handle sigchild or to check on file changes which is defined
 * at object construction time.
 * In the case of sigchild handling (destroy=true), notify the listener
 * only if there were writes.
 */
class WxEditTimer : public wxTimer
{
   public:

      WxEditTimer( int pid, bool destroy)
         : p_pid(pid), p_destroy(destroy) {;}

      ~WxEditTimer() {;}

      void Notify()
      {
         vector<EditSession>::iterator it = 
            WxEditSessionMgr::p_sessions.begin();
         while (it != WxEditSessionMgr::p_sessions.end()) {
            if ( (*it).pid == p_pid) {
               break;
            }
            it++;
         }

         if (it != WxEditSessionMgr::p_sessions.end()) {
            // Reap only our own editor.  A process-wide SIGCHLD handler
            // calling wait3() reaped whatever child exited, including the
            // one system() was waiting for on another thread.
            if (!p_destroy) {
               pid_t waited = waitpid(p_pid, NULL, WNOHANG);
               if (waited == p_pid || (waited == -1 && errno == ECHILD)) {
                  p_destroy = true;
               }
            }

            if (p_destroy) {
               // sigchild handler

               // Notify the client IFF file changed
               SFile file((*it).file);
               if (file.exists()) {
                  long modSec = file.lastModified().toSeconds();
                  if (modSec > (*it).modsec) {
                     EditEvent ee;
                     ee.id = (*it).callerids;
                     ee.filename = (*it).file;
                     (*it).l->processEditCompletion(ee);
                  }

                  // Delete the file - AFTER notifying client
                  file.remove();
               }

               WxEditSessionMgr::p_sessions.erase(it);


            } else {
               // update handler
               SFile file((*it).file);
               if (file.exists()) {
                  long modSec = file.lastModified().toSeconds();
                  if (modSec > (*it).modsec) {
                     // Notify the client IFF file changed
                     EditEvent ee;
                     ee.id = (*it).callerids;
                     ee.filename = (*it).file;
                     (*it).l->processEditCompletion(ee);
                     (*it).modsec = file.lastModified().toSeconds();
                  }
               }
               Start(CHECK_INTERVAL, true);

            }
         }
      }

   protected:
      int p_pid;
      bool p_destroy;
      
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
 * Performs fork/execve on the specified command.  The session's timer
 * polls for writes and reaps the editor when it exits.
 */
void WxEditSessionMgr::startSession (const string& app,
      /*const*/ char *args[],
      SFile* file,
      const string& id,
      EditListener *l)
{
   int pid;


   if ((pid = fork()) == 0) {
      /* child */
      if (execve((char*)app.c_str(),args,environ) < 0) {
         string msg = "Failed to execute execve - ";
         msg += app;
         EE_RT_ASSERT(false, EE_FATAL, msg);
      }
   } else if (pid == -1) {
      EE_RT_ASSERT(false, EE_WARNING, "fork failure");
   } else {
      /* parent */

      EditSession session;
      session.callerids = id;
      session.file = file->path();
      session.l = l;
      session.modsec = file->lastModified().toSeconds();
      session.pid = pid;
      p_sessions.push_back(session);

      // Add timer to check for writes
      WxEditTimer *timer = new WxEditTimer(pid, false);
      timer->Start(CHECK_INTERVAL,true);
   }
}





void WxEditSessionMgr::editFile(SFile* file, 
      const string& id, 
      EditListener *l,
      bool readOnly,
      const string& name)
{

   UserEditor editor;

   static const int MAX_ARGS = 16;   // gotta be plenty bug
   string exe;
   char *args[MAX_ARGS];
   editor.getEditCommand(*file, exe, args, MAX_ARGS, name, readOnly);

   // If we got here, no problems getting editor command
   startSession(exe, args, file, id, l);

   editor.freeArguments(args);

}




