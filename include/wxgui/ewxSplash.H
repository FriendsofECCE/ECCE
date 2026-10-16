#ifndef EWXSPLASH_H
#define EWXSPLASH_H

// The title card shown while ECCE starts: by the gateway, and on Windows,
// where ecce.cmd starts the Organizer without one, by the Organizer.
// It closes itself after timeoutMs, or earlier when clicked.
void ewxShowSplash(int timeoutMs = 3000);

#endif
