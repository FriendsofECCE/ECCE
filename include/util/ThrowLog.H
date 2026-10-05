/**
 * @file
 *
 * The "Throw Log" an exception prints to stderr where it is thrown.
 */
#ifndef THROWLOG_HH
#define THROWLOG_HH

class EcceException;

// Exceptions that mark a programming fault (InternalException,
// NullPointerException, ...) print it always.  The rest are mostly
// messages for the user, caught and shown, so they print it only with
// ECCE_THROW_LOG set; a build-tree path is noise for a user.
namespace ThrowLog {
  bool all();                               // ECCE_THROW_LOG is set
  void print(const EcceException& ex);
  inline void fault(const EcceException& ex) { if (!all()) print(ex); }
}

#endif
