/**
 * @file
 *
 * Which published release, if any, a running ECCE should be told about
 * (#273).  No wx and no network here: the caller fetches the GitHub
 * releases JSON and passes the text in.
 */
#ifndef RELEASECHECK_HH
#define RELEASECHECK_HH

#include <string>
using std::string;

namespace ReleaseCheck
{
  /// A release a user can be pointed at.
  struct Release
  {
    string version;   ///< without the leading "v", e.g. "9.0.0-beta.2"
    string url;       ///< the release page
  };

  /// Order of two version strings as the packages order them: numeric
  /// core first, a pre-release before its release, pre-release
  /// identifiers compared one by one (beta.9 < beta.10, alpha < beta).
  /// Returns <0, 0 or >0.  A leading "v" is ignored.
  int compareVersions(const string& a, const string& b);

  /// True when the version carries a pre-release part (alpha, beta, ...).
  bool isPreview(const string& version);

  /// The newest release in `json` (the GitHub releases list) that
  /// belongs to the running version's line and is newer than it.  The
  /// line is major.minor for 8.x (patch releases only) and the major
  /// version from 9 on.  An installed preview, or `includePreviews`,
  /// also hears about previews; otherwise only stable releases count.
  /// Drafts are skipped.  False when there is none or the JSON is
  /// unusable.
  bool newerRelease(const string& current, const string& json,
                    Release& found, bool includePreviews = false);

  /// The releases URL, or ECCE_UPDATE_CHECK_URL when set (a test hook;
  /// an http(s) URL or a file:// path to a JSON file).
  string releasesUrl();

  /// ECCE_NO_UPDATE_CHECK set to anything but empty or 0.
  bool disabledByEnvironment();

  /// The site switch: ECCE_NO_UPDATE_CHECK yes/true in
  /// siteconfig/site_runtime (the server's copy under -remote).
  bool disabledBySite();

  /// Either switch.
  inline bool disabled() { return disabledByEnvironment() || disabledBySite(); }

  /// Whether `now` is a day or more after `lastCheck` (both seconds
  /// since the epoch; a lastCheck in the future counts as due).
  bool checkDue(long lastCheck, long now);
}

#endif
