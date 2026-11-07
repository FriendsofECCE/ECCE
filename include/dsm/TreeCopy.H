#ifndef TREECOPY_HH
#define TREECOPY_HH

#include <functional>
#include <string>
#include <vector>

#include "util/EcceURL.H"

/**
 * Copies a resource (file, folder, project or calculation) between any two
 * data trees through their EDSIs: Local data, SSH folders and ECCE servers.
 * Files travel through a temporary file and every property comes along, so
 * the copy is usable on the other side as it is.
 */
class TreeCopy {
public:
  struct Entry {
    std::string path;     // relative to the copied resource, "" for the root
    bool dir;
    unsigned long size;
  };

  // The item being copied and the running totals; false cancels.
  typedef std::function<bool(const std::string&, long, unsigned long)> Progress;

  struct Result {
    Result() : ok(false), cancelled(false), files(0), bytes(0) {}
    bool ok;
    bool cancelled;
    std::string error;
    EcceURL target;                 // the new resource
    long files;
    unsigned long bytes;
    std::vector<Entry> manifest;    // what was written, for verify()
  };

  // Copy src into the folder destFolder under its own name.  With
  // uniqueName a clash gets "-1", "-2", ...; without, a clash fails.  A
  // failed or cancelled copy removes what it made (result.error says
  // whether that worked too).
  static Result copy(const EcceURL& src, const EcceURL& destFolder,
                     bool uniqueName, const Progress& progress);

  // "" if the copy at target holds exactly the manifest's files with the
  // same sizes, else what differs.
  static std::string verify(const EcceURL& target, const std::vector<Entry>& m);

  // Remove a resource and everything below it through its EDSI.
  static bool removeTree(const EcceURL& url);
};

#endif
