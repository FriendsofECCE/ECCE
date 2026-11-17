/**
 * @file
 *
 * One running ECCE per user and session context in one-program mode
 * (#1069).  The first start holds a lock and listens on a local socket; a
 * later start sends it "raise" (and file names to open) and quits.
 */
#ifndef ONEINSTANCE_HH
#define ONEINSTANCE_HH

#include <vector>

#include "wx/string.h"

class OneInstance
{
public:
  // True: this is the only copy (it is now listening).  False: another copy
  // has been asked to come to the front and open `files`; the caller must
  // end the process.  Also true when the lock is held but the other copy
  // cannot be reached, so a hung copy never locks the user out.
  static bool claim(const wxString& context,
                    const std::vector<wxString>& files);

  // Existing structure files among the command-line words, as absolute paths.
  static std::vector<wxString> filesIn(const std::vector<wxString>& words);
};

#endif
