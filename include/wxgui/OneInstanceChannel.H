/**
 * @file
 *
 * The GUI-free half of the single-instance channel (#1069): where this
 * user's running ECCE listens and how to reach it.  The Organizer
 * (OneInstance) and ecce-one-instance share it, so the `ecce` launcher can
 * hand a start over before it starts a gateway that would sign in.
 */
#ifndef ONEINSTANCECHANNEL_HH
#define ONEINSTANCECHANNEL_HH

#include <vector>

#include "wx/string.h"

namespace OneInstanceChannel
{
  extern const char *TOPIC;

  struct Place {
    wxString dir;         // empty: no private run folder, checking is off
    wxString key, service, tokenFile;
  };

  // The user and where their data is, from the environment; never the
  // session id, which is new at every start.
  wxString context();

  Place place(const wxString& context);

  // Send the token and `files` to the copy listening at `where`.
  bool tell(const Place& where, const std::vector<wxString>& files);

  // Existing structure files among the command-line words, as absolute paths.
  std::vector<wxString> filesIn(const std::vector<wxString>& words);
}

#endif
