/**
 * @file 
 *
 *
 */
#ifndef EDITSESSION_HH
#define EDITSESSION_HH

#include <string>
  using std::string;

class EditListener;

/**
 * An EditSession represents a single instance of a user editing
 * session where the editor is presumed to be external (vi/emacs...).
 * The session info includes the process id, timestamp info for
 * detected if the file changed, the file path/name, an id provided
 * by the invoking code, and a pointer to a listener to call when
 * the session ends or a change is detected.
 *
 * The data about a session is separate from methods such as start and
 * stop because these operations involve system features (exec, timer) that are
 * not currently abstracted out into helper classes.
 */
class EditSession
{
   public:
      int  pid;
      long modsec;
      string callerids;
      EditListener *l;
      string file;

};


#endif
