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
            // Loook up pid - if found notify listener
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

                  Start(CHECK_INTERVAL, true);
               }

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
      const string & name) throw(EcceException)
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
      const string& name) throw(EcceException)
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
 * Performs fork/execve on the specified command.  The process id
 * and other information is retained for later when a SIGCHLD is
 * received.
 *
 * The signal handler must be uninstalled prior to the fork/exec process
 * or else upon forking a second process, the parent will hang until
 * the child dies.  I found this out via experience.  Nothing in the
 * man pages explained why this is so.
 */
void WxEditSessionMgr::startSession (const string& app,
      /*const*/ char *args[],
      SFile* file,
      const string& id,
      EditListener *l)
{
   int pid;


   // temporarily suspend signal handler
   signal(SIGCHLD, SIG_DFL);

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

   // install signal handler for Citations/Annotations
   signal(SIGCHLD, WxEditSessionMgr::editSessionCompleted);

}


/**
 * Signal handler for SIGCHLD.  All processing is actually done
 * in a method schduled via WxEditTimer (sublcass WxTimer) to avoid
 * ANY Xlib calls in the handler.  This basic philosophy carried over from
 * old X implementation (See O'Reilly Chapter 20 for more information)
 * 
 * A timer of basically no time is used to schedule actual sigchild processing.
 * We go to all this trouble so that the application can post
 * messages to the feedback area if problems occurr.
 */
void WxEditSessionMgr::editSessionCompleted(int arg)
{
   int pid;
   int status;

   pid = wait3(&status,WNOHANG,NULL);

   if (pid == -1) {
      // man pages say: If there are no children, -1 is returned immediately
      // cerr << "No children to wait3() on." << endl;

   } else if (pid == 0) {
      // Got tired of seeing this warning which didn't seem related to any
      // problems so I commented it out  GDB 2/15/08
      // EE_ASSERT(false, EE_WARNING, "nothing to wait on");

   } else {
      // Add timer so we don't process any Xlib events in handler
      //XtAppAddTimeOut(XtDisplayToApplicationContext(XtDisplay(p_shell)),
      //      0,processDeadChild,(XtPointer)new int(pid));
      WxEditTimer *timer = new WxEditTimer(pid, true);
      timer->Start(1,true);
   }

   // From the man pages:
   // Before entering the signal-catching function, the value of func for the
   // caught signal will be set to SIG_DFL unless the signal is SIGILL,
   // SIGTRAP, or SIGPWR.  This means that before exiting the handler, a
   // signal call is necessary to again set the disposition to catch the signal
   signal(SIGCHLD, WxEditSessionMgr::editSessionCompleted);
}



void WxEditSessionMgr::editFile(SFile* file, 
      const string& id, 
      EditListener *l,
      bool readOnly,
      const string& name) throw(EcceException)
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




