/**
 * @file
 *
 * Local data mode (#216): where this user's data folder is, if any, and
 * moving it.  wx-free; the Preferences page and the Gateway's start-up
 * question are its only GUI.
 *
 * Which mode a process is in: ECCE_LOCAL_DATA when it is set (empty means
 * a data server), else a data server under ECCE_REMOTE_SERVER, else the
 * LocalData preference.  The wrappers resolve this once per session and
 * export it, so a preference change takes effect at the next start and
 * never splits one session between two stores.
 */
#ifndef LOCALDATA_HH
#define LOCALDATA_HH

#include <string>
using std::string;

class LocalData
{
  public:
    // The data folder, without a trailing slash; "" with a data server.
    static string dir();
    // <dir>/users/<user>, the user's home in the store; "" with a server.
    static string userHome();
    // ~/.ECCE-local: outside ~/.ECCE, which people delete to reset ECCE.
    static string defaultDir();

    // The preference, as Edit > Preferences shows and sets it.
    static bool prefEnabled();
    static string prefFolder();          // never empty: defaultDir() if unset
    static string prefMoveTo();          // a move asked for, "" if none
    static void setPref(bool enabled, const string& folder,
                        const string& moveTo);

    // Moving a folder, before any process of a session has opened it.
    enum MoveResult { MOVED, SAME, TARGET_NOT_EMPTY, IN_USE, FAILED };
    static bool isEmptyOrMissing(const string& path);
    // True while a process holds the folder (it took dir() for it).
    static bool inUse(const string& path);
    // Rename on one file system; elsewhere copy, compare every file's
    // size, and only then delete the old folder.  On failure the old
    // folder is untouched and msg says why.
    static MoveResult move(const string& from, const string& to, string& msg);

  private:
    static void hold(const string& path);
};

#endif
