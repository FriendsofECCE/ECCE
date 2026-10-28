//  A stored date as the user's preferences show it (date and time format of
//  Preferences); unreadable preferences leave the stored text. Shared by the
//  Organizer's and the Viewer's calculation summaries so they agree.
#ifndef PREFDATETIME_HH
#define PREFDATETIME_HH

#include <string>
#include "util/Preferences.H"
#include "util/PreferenceLabels.H"
#include "util/TDateTime.H"

inline std::string prefDateTime(const std::string& stored)
{
  if (stored.empty()) return stored;
  Preferences eccePref(PrefLabels::GLOBALPREFFILE);
  int dateFormat, timeFormat;
  if (!eccePref.getInt(PrefLabels::DATEFORMAT, dateFormat) ||
      !eccePref.getInt(PrefLabels::TIMEFORMAT, timeFormat))
    return stored;
  if ((TDateTime::DateFormat)dateFormat < TDateTime::AmericanLocale ||
      (TDateTime::DateFormat)dateFormat > TDateTime::TOTALDATE ||
      (TDateTime::TimeFormat)timeFormat < TDateTime::HM24 ||
      (TDateTime::TimeFormat)timeFormat > TDateTime::TOTALTIME)
    return stored;
  TDateTime datetime(stored);
  return datetime.toString((TDateTime::DateFormat)dateFormat,
           TDateTime::withSeconds((TDateTime::TimeFormat)timeFormat));
}

#endif
