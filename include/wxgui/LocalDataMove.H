#ifndef LOCALDATAMOVE_H
#define LOCALDATAMOVE_H

// Offers the move of the local data folder asked for in Preferences, at the
// start of a session: by the gateway, and on Windows, where ecce.cmd starts
// the Organizer without one, by the Organizer.  Sets ECCE_LOCAL_DATA to the
// folder the session uses.
void offerLocalDataMove();

#endif
