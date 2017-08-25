/**
 * @file 
 *
 *
 */
#ifndef EDITEVENT_HH
#define EDITEVENT_HH

#include <string>
  using std::string;

/**
 * An EditEvent signals that a user has finished and editing session
 * (using and external editor such as vi/emacs and that changes
 * are available.
 * @see EditListener
 *
 * EditEvent is sent to the listener when an edit session
 * completes.  The listener gets the id and the name of the file where
 * contents can be retrieved.
 */
class EditEvent
{
   public:
      string id;
      string filename;
};


#endif
