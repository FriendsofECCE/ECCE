#ifndef RUNFOLDERCOPY_HH
#define RUNFOLDERCOPY_HH

#include <string>
#include <vector>

#include "util/EcceURL.H"

/**
 * A job's run folder on the compute host as a marked copy of the stored
 * calculation (#1083): the calculation is staged in a local folder through
 * TreeCopy, its sidecar gets the copy markers, and the caller uploads the
 * staged files with the launch's own copy primitives.  The original stays
 * authoritative.
 */
class RunFolderCopy {
public:
  // Copy markers on the copy's root (Ecce namespace).
  static const char* COPY_OF;          // original calculation URL
  static const char* COPY_ORIGIN;      // launching host
  static const char* COPY_ORIGIN_ROOT; // its Local data folder or server
  static const char* COPY_MADE_AT;

  // False for a calculation already stored on an SSH folder (its run folder
  // may be the calculation itself) or with ECCE_RUN_FOLDER_COPY=0.
  static bool applies(const EcceURL& calc);

  // Stage calc as <parent>/copy, keeping only the top-level folders named
  // in keepDirs, and mark it.  state, when not empty, replaces the state.
  // Returns the staged root, or "" with err set.
  static std::string stage(const EcceURL& calc, const std::string& parent,
                           const std::vector<std::string>& keepDirs,
                           const std::string& originHost,
                           const std::string& state, std::string& err);

  // The staged files grouped by folder, root first; dir is relative ("" for
  // the root) and files are full local paths.
  struct Batch { std::string dir; std::vector<std::string> files; };
  static std::vector<Batch> batches(const std::string& stagedRoot);

  // The Organizer's label suffix for a resource with these properties, or
  // "" when it is not a copy.
  static std::string copyNote(const std::string& copyOf,
                              const std::string& origin);
};

#endif
