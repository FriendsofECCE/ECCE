// Start and end times a job script wrote into its output's
// "-----ECCE Log Information-----" block (scripts/gensub), and the texts
// for scheduler end states.  Header-only, no ECCE deps (tests/jobstore).
#ifndef JOBLOGDATES_H
#define JOBLOGDATES_H

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cctype>
#include <cstdio>
#include <cstdlib>

namespace JobLogDates {

// Days from 1970-01-01 to y-m-d (proleptic Gregorian); avoids timegm.
inline long daysFromCivil(long y, unsigned m, unsigned d)
{
  y -= m <= 2;
  const long era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (long)doe - 719468;
}

inline std::string lower(std::string s)
{
  for (size_t i = 0; i < s.size(); i++)
    s[i] = (char)std::tolower((unsigned char)s[i]);
  return s;
}

// Month from an English (or common Swedish/German/Spanish) name or its
// abbreviation, 0 if the token is no month.
inline int monthOf(const std::string& tok)
{
  static const char* names[] = {"jan","feb","mar","apr","may","jun","jul",
                                "aug","sep","oct","nov","dec"};
  static const char* other[] = {"maj","mai","okt","dez","ene","abr","ago",
                                "dic"};
  static const int otherMonth[] = {5,5,10,12,1,4,8,12};
  std::string t = lower(tok);
  while (!t.empty() && t[t.size()-1] == '.') t.erase(t.size()-1);
  if (t.size() < 3) return 0;
  t = t.substr(0, 3);
  for (int i = 0; i < 12; i++) if (t == names[i]) return i + 1;
  for (int i = 0; i < 8; i++) if (t == other[i]) return otherMonth[i];
  return 0;
}

// Offset east of UTC in seconds for a zone abbreviation or +hhmm; false
// if unknown.  "IST" is ambiguous (India/Israel/Ireland) and left unknown.
inline bool zoneOffset(const std::string& tok, long& off)
{
  struct Z { const char* n; int h; };
  static const Z zones[] = {
    {"utc",0},{"gmt",0},{"z",0},{"wet",0},{"bst",1},{"cet",1},{"cest",2},
    {"met",1},{"mest",2},{"eet",2},{"eest",3},{"msk",3},
    {"est",-5},{"edt",-4},{"cst",-6},{"cdt",-5},{"mst",-7},{"mdt",-6},
    {"pst",-8},{"pdt",-7},{"jst",9},{"kst",9},{"aest",10},{"aedt",11}};
  const std::string t = lower(tok);
  if (t.empty()) return false;
  if ((t[0] == '+' || t[0] == '-') && t.size() >= 5) {
    std::string d;
    for (size_t i = 1; i < t.size(); i++) {
      if (t[i] == ':') continue;
      if (!std::isdigit((unsigned char)t[i])) return false;
      d += t[i];
    }
    if (d.size() != 4) return false;
    off = (t[0] == '-' ? -1 : 1) *
          (atoi(d.substr(0, 2).c_str()) * 3600L +
           atoi(d.substr(2).c_str()) * 60L);
    return true;
  }
  for (size_t i = 0; i < sizeof(zones) / sizeof(zones[0]); i++)
    if (t == zones[i].n) {
      off = zones[i].h * 3600L;
      return true;
    }
  return false;
}

// Seconds since the epoch (UTC) from the text after "Completed Job: ":
//   2026-10-28T07:12:50Z                 (the job script's own)
//   Tue Oct 28 07:12:50 CET 2026         (date, C locale)
//   Tue 28 Oct 2026 07:12:50 AM CET      (date, en_US; sv_SE etc. likewise)
// -1 when it is none of these or the zone is unknown: a local time without
// a known zone cannot be placed.
inline long parse(const std::string& text)
{
  int Y, M, D, h, m, s;
  if (sscanf(text.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d", &Y, &M, &D, &h, &m, &s)
      == 6) {
    return text.find('Z') == std::string::npos
           ? -1
           : daysFromCivil(Y, M, D) * 86400L + h * 3600L + m * 60L + s;
  }

  std::istringstream is(text);
  std::string tok;
  int month = 0, day = 0, year = 0, hh = -1, mm = 0, ss = 0;
  bool pm = false, am = false, haveZone = false;
  long off = 0;
  std::vector<std::string> nums;
  while (is >> tok) {
    int a, b, c;
    if (tok.find(':') != std::string::npos &&
        sscanf(tok.c_str(), "%d:%d:%d", &a, &b, &c) == 3) {
      hh = a; mm = b; ss = c;
      continue;
    }
    bool digits = true;
    for (size_t i = 0; i < tok.size(); i++)
      if (!std::isdigit((unsigned char)tok[i])) digits = false;
    if (digits) { nums.push_back(tok); continue; }
    const std::string t = lower(tok);
    if (t == "pm") { pm = true; continue; }
    if (t == "am") { am = true; continue; }
    long o;
    if (zoneOffset(tok, o)) { off = o; haveZone = true; continue; }
    const int mo = monthOf(tok);
    if (mo) month = mo;                 // last match wins (Spanish "mar")
  }
  for (size_t i = 0; i < nums.size(); i++) {
    if (nums[i].size() == 4) year = atoi(nums[i].c_str());
    else if (nums[i].size() <= 2 && !day) day = atoi(nums[i].c_str());
  }
  if (!month || !day || !year || hh < 0 || !haveZone) return -1;
  if (pm && hh < 12) hh += 12;
  if (am && hh == 12) hh = 0;
  return daysFromCivil(year, month, day) * 86400L + hh * 3600L + mm * 60L +
         ss - off;
}

// Epoch seconds of a "<label>" line ("Starting Job", "Completed Job") in
// the output's log block, -1 if missing or unreadable.  A newer job
// script also writes "<label> (UTC): <ISO>", which is preferred; the last
// such line wins when a restart appended another block.
inline long logBlockTime(const std::string& path, const std::string& label)
{
  std::ifstream ifs(path.c_str());
  if (!ifs) return -1;
  std::string line, plain, utc;
  bool in = false;
  const std::string a = label + " (UTC): ", b = label + ": ";
  while (std::getline(ifs, line)) {
    if (line.find("-----ECCE Log Information-----") != std::string::npos)
      in = true;
    else if (in) {
      if (line.compare(0, a.size(), a) == 0) utc = line.substr(a.size());
      else if (line.compare(0, b.size(), b) == 0) plain = line.substr(b.size());
    }
  }
  long t = utc.empty() ? -1 : parse(utc);
  if (t < 0 && !plain.empty()) t = parse(plain);
  return t;
}

// Text for a scheduler end state, "" if the state is not one we name.
inline std::string schedulerEndReason(const std::string& state)
{
  if (state == "OUT_OF_MEMORY")
    return "Out of memory: the scheduler killed the job (OUT_OF_MEMORY)";
  if (state == "TIMEOUT")
    return "Wall time reached: the scheduler stopped the job (TIMEOUT)";
  if (state == "NODE_FAIL")
    return "Node failure: the scheduler lost the job's node (NODE_FAIL)";
  if (state == "PREEMPTED")
    return "Preempted: the scheduler stopped the job for another (PREEMPTED)";
  return "";
}

} // namespace JobLogDates

#endif
