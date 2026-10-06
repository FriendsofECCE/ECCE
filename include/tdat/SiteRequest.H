// A change to a central server's site settings, made by an administrator
// from a -remote client: the processmachine form for Machines, Queues and
// <m>.Q, and the CONFIG.<m> edits.  The client writes it to a file that is
// copied to the server, and ecce-site-admin applies it there with the same
// writers a local "ecce -admin" uses, then publishes the site files to the
// clients.  Every name and value travels percent-encoded inside the file;
// no part of it is ever put on a command line or into a shell string.
#ifndef SITEREQUEST_HH
#define SITEREQUEST_HH

#include <string>
#include <vector>

#include "tdat/ConfigFile.H"

class SiteRequest
{
public:
  SiteRequest() : replaceText(false) {}

  std::string machine;              // the reference name
  std::string form;                 // processmachine form ("type=...&..."), or ""
  std::vector<ConfigEdit> edits;    // applied to siteconfig/CONFIG.<machine>
  bool replaceText;                 // a raw edit: text replaces the file...
  std::string text;
  std::string baseText;             // ...only if the file still reads this

  std::string encode() const;
  bool decode(const std::string& data, std::string& err);

  // A name that is safe as part of a file name in siteconfig.
  static bool validMachineName(const std::string& name);

  // Server side.  Checks that the caller may write ecceHome/siteconfig (its
  // group's write bit and membership are the authorisation), runs
  // processmachine, writes CONFIG.<machine> through ConfigFile, and runs
  // ecceHome/bin/ecce-site-publish.  Returns 0 on success; report says what
  // was done or why nothing (or only part) was.
  int applyOnServer(const std::string& ecceHome, std::string& report) const;

  // The value of one field of a processmachine form ("" if absent).
  static std::string formField(const std::string& form, const std::string& name);
};

#endif
