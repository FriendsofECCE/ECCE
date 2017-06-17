///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: TDateTime.C
//
//
// DESIGN:
//   This class stores a "struct tm" structure for representing time.
//   The time is ALWAYS GMT and conversions must take this into account.
//
//   It would probably be better to keep this as as time_t to minimuze
//   the number of conversions that have to take place.
//
//   This is a pretty lame little class that originally was designed to
//   allow us to recover persistent data that was stored with rogue wave
//   dates.  Doing math etc on dates is something we can add if needed.
//
//
///////////////////////////////////////////////////////////////////////////////
#include <iostream> 
#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;

#include <stdio.h> // sprintf
#include <time.h>  // gmtime_r, localtime_r
#include <string.h>

#include "util/ErrMsg.H"
#include "util/TDateTime.H"

#ifndef __APPLE__
extern time_t timezone;
#endif

// Class statics
time_t TDateTime::p_tzseconds = 0;

// File static to keep out of schema.
static const unsigned char daysInMonth[12]={31,28,31,30,31,30,31,31,30,31,30,31};


// Data for parsing timezones.  The c api doesn't seem to support.
// I planned to add support for parsing dates that are not in GMT or the
// local time zone but didn't get it done.  See adjustZone().
typedef struct _tzoffset {
  const char *name;
  int hours;
} TZOffset;


const char* TDateTime::p_dateFormatName[TOTALDATE] =
  {"mm/dd/yy", "dd/mm/yy", "Day, dd Mon yyyy", "dd Mon yyyy", "No Date"}; 

const char* TDateTime::p_timeFormatName[TOTALTIME] = 
  {"hh:mm (24hr.)", "hh:mm AM/PM", "hh:mm:ss (24hr.)",
   "hh:mm:ss AM/PM", "hh:mm:ss timezone (24hr.)", "No time"};

static TZOffset v_zones[] = { 
  {"UT", 0},
  {"GMT", 0},
  {"EST", -5},
  {"EDT", -4},
  {"CST", -6},
  {"CDT", -5},
  {"MST", -7},
  {"MDT", -6},
  {"PST", -8},
  {"PDT", -7},
  // Military stuff
  {"Z", 0},
  {"A", -1},
  {"B", -2},
  {"C", -3},
  {"D", -4},
  {"E", -5},
  {"F", -6},
  {"G", -7},
  {"H", -8},
  {"I", -9},
  {"K", -10},
  {"L", -11},
  {"M", -12},
  {"N", 1},
  {"O", 2},
  {"P", 3},
  {"Q", 4},
  {"R", 5},
  {"S", 6},
  {"T", 7},
  {"U", 8},
  {"V", 9},
  {"W", 10},
  {"X", 11},
  {"Y", 12}
};
static int v_numZones = sizeof(v_zones) / sizeof(TZOffset);

// Must match enumeration.  Used for parsing and formatting.
// Had trouble with %T so be explicit
static const char *datestyles[] = {
  "%m/%d/%y",
  "%d/%m/%y",
  "%a, %d %b %Y",
  "%d %b %Y",
  "%d %b %Y %H:%M:%S"
};

static const char *timestyles[] = {
  " %H:%M",
  " %I:%M %p",
  " %H:%M:%S",
  " %I:%M:%S %p",
  " %H:%M:%S %Z",
  ""
};


///////////////////////////////////////////////////////////////////////////////
// Description
//   Construct object with current date/time.
///////////////////////////////////////////////////////////////////////////////
TDateTime::TDateTime(void)
{

  initTimeZone();

  time_t clk = time(0);
  gmtime_r(&clk,&p_tm); 


}

TDateTime::TDateTime(const TDateTime& rhs)
{
  initTimeZone();
  p_tm = rhs.p_tm;
}

TDateTime::TDateTime(const string& rhs)
{
  initTimeZone();
  time_t clk = time(0);
  gmtime_r(&clk,&p_tm); 
  *this = rhs;
}

 
TDateTime::~TDateTime(void)
{
}


TDateTime& TDateTime::operator=(const TDateTime& rhs)
{
  if (this != &rhs) {
    p_tm = rhs.p_tm; // rely on struct copy constructor
  }
  return *this;
}

//////////////////////////////////////////////////////////////////////////////
// operator=
//   Assign date by parsing string.  String must be in format ala
//   toString or the results are undefined.
//
//   This is a bit tricky.
//     . strip off day of week since it can only get in the way.
//     . %T gave me trouble though the equivalent of %H:%M:%S worked.
//     . reading timezone is not workable.  Assumed to be GMT ONLY at this time.
//
// NOTE NOTE NOTE
//   Currently the time string is assumed to be GMT
//////////////////////////////////////////////////////////////////////////////
TDateTime& TDateTime::operator=(const string& rhs)
{
  // find first numeric
  const char *rhsdata = rhs.c_str();
  const char *start = strstr(rhsdata,", ");
  if (start != NULL) {
    start++;

    // EXAMPLE:  31 Jan 2002 15:30:10
    char *loc = strptime(start,"%d %b %Y %H:%M:%S",&p_tm);
    if (loc == NULL) {
      string msg = "bad format ";
      msg += rhs;
      EE_RT_ASSERT(false, EE_WARNING, msg);
    } else if (strcmp(rhsdata,"INVALID") == 0) {
      time_t timet = 0;
      setFromTimeT(timet);
    } else {
      while (loc && (*loc == ' ' || *loc == '\t')) loc++;
      adjustZone(loc);
    }
  } else {
    // This can happen on copy constructor.
    //cerr << "Warning: bad date format" << endl;
  }
  return *this;
}

/**
 * Make system call to initialize the time zone.
 * Also now force LC_TIME to C.
 */
void TDateTime::initTimeZone() 
{
  static bool firstTime = true;

  if (firstTime == true) {
    // Use tzset to initialize external global system variables.
    // Its not clear to me that daylight time is accounted for
    // Alternatively the code below seems to work also.
    // timezone is a globally defined value of seconds from UTC.
    tzset();   

#ifdef __APPLE__
    time_t a;
    time(&a);
    tm * lt = localtime(&a);
    p_tzseconds = -lt->tm_gmtoff+(lt->tm_isdst?3600:0);
#else
    p_tzseconds = timezone;
#endif
    firstTime = false;
  }
}

string TDateTime::toString(TDateTime::DateFormat dformat,
                           TDateTime::TimeFormat tformat) const
{
  string ret;

  if (dformat == TDateTime::NODATE) {
    ret = toString();
  } else  {
    string format = datestyles[(int)dformat];
    format += timestyles[(int)tformat];
    char buf[256];
    struct tm tmp = p_tm;  // copy because mktime doesn't want const
    time_t cur = mktime(&tmp);
    cur -= p_tzseconds;
    tmp = *localtime(&cur);
    strftime(buf,255,format.c_str(),&tmp);
    ret = buf;
  }

  return ret;
}

///////////////////////////////////////////////////////////////////////////////
// toSeconds
//   This method expects "value" to be in the ecce standard format and in
//   GMT.  If its not, results are undefined.
///////////////////////////////////////////////////////////////////////////////
long TDateTime::toSeconds(const char *value,
                          const TDateTime::DateFormat& dformat,
                          const TDateTime::TimeFormat& tformat)
{
  long ret = 0;  // the beginning of time...

  if (value == NULL) return ret;

  const char* start = NULL;

  // find first numeric for RFC1123.  Skip that day of week nonsense.
  if (dformat==TDateTime::NODATE && (start=strstr(value, ", "))!=NULL)
    start++;
  else
    start = value;

  string format = datestyles[(int)dformat];
  format += timestyles[(int)tformat];

  // You have to initialize each of these - if you don't and you don't ask
  // for a value in your format, you'll get junk.
  struct tm tmp;
  tmp.tm_year = 0;
  tmp.tm_mon = 0;
  tmp.tm_mday = 0;
  tmp.tm_hour = 0;
  tmp.tm_min = 0;
  tmp.tm_sec = 0;
  tmp.tm_isdst = 0;

  char* loc = strptime(start,format.c_str(),&tmp);
  // strptime tends to be inconsistent about the following
  // two fields so just to make sure mktime doesn't get confused set them
  // explicitly to values indicating that they need to be determined.
  tmp.tm_yday = -1;
  tmp.tm_isdst = -1;

  if (loc == NULL) {
    string msg = "bad format ";
    msg += value;
    EE_RT_ASSERT(false, EE_WARNING, msg);
  } else {
    //while (loc && *loc == ' ' || *loc == '/t') loc++;
    //adjustZone(loc);
    ret = mktime(&tmp);
  }

  return ret;
}


long TDateTime::toSeconds() const
{
  long ret = 0;  // the beginning of time...
  struct tm tmp = p_tm;  // copy because mktime doesn't want const
  ret = mktime(&tmp);
  return ret;
}


bool TDateTime::operator==(const TDateTime& rhs) const
{
  return (toSeconds() == rhs.toSeconds());
}


bool TDateTime::operator!=(const TDateTime& rhs) const
{
  return !(*this == rhs);
}


bool TDateTime::operator>(const TDateTime& rhs) const
{
  return (toSeconds() > rhs.toSeconds());
}


bool TDateTime::operator<(const TDateTime& rhs) const
{
  return (toSeconds() < rhs.toSeconds());
}


int TDateTime::compare(const TDateTime& rhs) const
{
  return toSeconds() - rhs.toSeconds();
}


//////////////////////////////////////////////////////////////////////////////
// parseMilFormat
//   Parses dates that we get on dav live properties
//////////////////////////////////////////////////////////////////////////////
bool TDateTime::parseMilFormat(const string& value)
{
  bool ret = true;
  char *loc = strptime(value.c_str(),"%Y-%m-%dT%H:%M:%S",&p_tm);
  if (loc == NULL) {
    ret = false;
  } else {
    while (loc && (*loc == ' ' || *loc == '\t')) loc++;
    adjustZone(loc);
  }
  return ret;
}

//////////////////////////////////////////////////////////////////////////////
// parseUSDate
//   Parses old ecce formated dates in US form mm/dd/yy.  SHould be ok to
//   include time in hh:mm:ss also.  From the man pages:
//     %D     Equivalent to %m/%d/%y. (This is the American style
//            date,  very  confusing to non-Americans, especially
//            since %d/%m/%y is widely used in Europe.   The ISO
//            8601 standard format is %Y-%m-%d.)
//////////////////////////////////////////////////////////////////////////////
bool TDateTime::parseUSDate(const string& value)
{
  bool ret = true;
  char *loc = strptime(value.c_str(),"%D",&p_tm);
  if (loc == NULL) {
    ret = false;
  } else {
    while (loc && (*loc == ' ' || *loc == '\t')) loc++;
    adjustZone(loc);
  }
  return ret;
}
//////////////////////////////////////////////////////////////////////////////
// adjustZone
//   Using known zones (see top of file) and their delta hours from GMT,
//   adjust the given date.
//
//   I couldn't get this working correctly and we don't need it for now
//   so just move on to bigger and better things.  Somehow I think that
//   mktime is already compensating for the zone.  Not sure what is going
//   on.
//////////////////////////////////////////////////////////////////////////////
bool TDateTime::adjustZone(const char *zname)
{
  bool ret = false;

  return ret;  // NOTE this is in here intentionally now since it doesn't work

  for (int idx=0; idx<v_numZones; idx++) {
    if (strcmp(v_zones[idx].name,zname)==0) {
      if (v_zones[idx].hours != 0) {
        struct tm tmp = p_tm;  // copy because mktime doesn't want const
        time_t cur = mktime(&tmp);
        debug();
        //cur = cur - (v_zones[idx].hours * 3600 );  // 3600 sec per hour
        gmtime_r(&cur,&p_tm);
        debug();
      }
      ret = true;
      break;
    }
  }
  return ret;
}

//////////////////////////////////////////////////////////////////////////////
// debug()
//   Just dump out the crucial struct tm members for debugging.
//////////////////////////////////////////////////////////////////////////////
void TDateTime::debug() const
{
  std::cout << "year " << p_tm.tm_year << std::endl;
  std::cout << "month " << p_tm.tm_mon << std::endl;
  std::cout << "day " << p_tm.tm_mday << std::endl;
  std::cout << "hour " << p_tm.tm_hour << std::endl;
  std::cout << "min " << p_tm.tm_min << std::endl;
  std::cout << "sec " << p_tm.tm_sec << std::endl;
  std::cout << "isdst " << p_tm.tm_isdst << std::endl;
}

//////////////////////////////////////////////////////////////////////////////
// setFromTimeT
//   initialize time/date from time_t structure.  This is actually a
//   degenerate operation since gmtime_r does a direct conversion from
//   time_t to struct tm which is the underly TDateTime data structure.
//////////////////////////////////////////////////////////////////////////////
void TDateTime::setFromTimeT(const time_t& timet) 
{
  gmtime_r(&timet,&p_tm);
}

bool TDateTime::isValid()
{
  return ((unsigned long)mktime(&p_tm)) > 28800;
}

//////////////////////////////////////////////////////////////////////////////
//   returns formatted string according to RFC 1123/822 format.  This time
//   will be in GMT.  Use toLocalString if you want the correct locale
//   adjusted time.
//////////////////////////////////////////////////////////////////////////////
string TDateTime::toString() const
{
  char buf[256];
  strftime(buf,255,"%a, %d %b %Y %T GMT",&p_tm);
  return buf;
}

////////////////////////////////////////////////////////////////////////////// 
// asString
//   designed to match the RWTime.asString method.
////////////////////////////////////////////////////////////////////////////// 
string TDateTime::asString(const char request) const
{
  string ret;
  char buf[100];
  if (request == 'X') {
    int seconds = p_tm.tm_sec;
    int minutes = p_tm.tm_min;
    int hours = p_tm.tm_hour;
    sprintf(buf, "%d:%d:%d",hours, minutes, seconds);
    ret = buf;
  } else if (request == 'H') {
    sprintf(buf,"%d",p_tm.tm_hour);
    ret = buf;
  } else if (request == 'M') {
    sprintf(buf,"%d",p_tm.tm_min);
    ret = buf;
  } else if (request == 'b') {
    int month = p_tm.tm_mon+1;
    if (month == 1) ret = "Jan";
    else if (month == 2) ret = "Feb";
    else if (month == 3) ret = "Mar";
    else if (month == 4) ret = "Apr";
    else if (month == 5) ret = "May";
    else if (month == 6) ret = "Jun";
    else if (month == 7) ret = "Jul";
    else if (month == 8) ret = "Aug";
    else if (month == 9) ret = "Sep";
    else if (month == 10) ret = "Oct";
    else if (month == 11) ret = "Nov";
    else if (month == 12) ret = "Dec";
  } else if (request == 'd') {
    sprintf(buf,"%d",p_tm.tm_mday);
    ret = buf;
  } else if (request == 'Y') {
    sprintf(buf,"%d",p_tm.tm_year+1900);
    ret = buf;
  } else if (request == 'm') {
    int month = p_tm.tm_mon+1;
    if (month<10) {
      sprintf(buf,"0%d",month);
    } else {
      sprintf(buf,"%d",month);
    }
    ret = buf;
  } 
  return ret;
}

////////////////////////////////////////////////////////////////////////////// 
// toLocalString
//   returns formatted string according to RFC 1123 format in GMT.
//////////////////////////////////////////////////////////////////////////////
string TDateTime::toLocaleString() const
{
  char buf[256];
  struct tm tmp = p_tm;  // copy because mktime doesn't want const
  time_t cur = mktime(&tmp);
  cur -= p_tzseconds;
  tmp = *localtime(&cur);
  strftime(buf,255,"%a, %d %b %Y %T %Z",&tmp);
  return buf;
}

/*
 * Convert Gregorian calendar date to the corresponding Julian day
 * number j.  Algorithm 199 from Communications of the ACM, Volume 6, No.
 * 8, (Aug. 1963), p. 444.  Gregorian calendar started on Sep. 14, 1752.
 * This function not valid before that.
 * Returns 0 if the date is invalid.
 */
unsigned long TDateTime::jday(unsigned m, unsigned d, unsigned y)
{
  unsigned long c, ya;
  if( y <= 99 ) y += 1900;
  if( !dayWithinMonth(m, d, y) ) return (unsigned long)0;

  if (m > 2) { m -= 3; }  // wash out the leap day
  else       { m += 9;  y--; }
  c = y / 100;
  ya = y - 100*c;
  return ((146097*c)>>2) + ((1461*ya)>>2) + (153*m + 2)/5 + d + 1721119;
}

/*
 * Convert a Julian day number to its corresponding Gregorian calendar
 * date.  Algorithm 199 from Communications of the ACM, Volume 6, No. 8,
 * (Aug. 1963), p. 444.  Gregorian calendar started on Sep. 14, 1752.
 * This function not valid before that.
 */

/*
void TDateTime::gregMDY(unsigned& m, unsigned& D, unsigned& y) const
{
  unsigned long d;
  unsigned long j = p_date - 1721119;
  y = (unsigned) (((j<<2) - 1) / 146097);
  j = (j<<2) - 1 - 146097*y;
  d = (j>>2);
  j = ((d<<2) + 3) / 1461;
  d = (d<<2) + 3 - 1461*j;
  d = (d + 4)>>2;
  m = (unsigned)(5*d - 3)/153;
  d = 5*d - 3 - 153*m;
  D = (unsigned)((d + 5)/5);
  y = (unsigned)(100*y + j);
  if (m < 10)
        m += 3;
  else {
        m -= 9;
        y++;
  }
}
*/


// Is a day (1-31) within a given month?
bool TDateTime::dayWithinMonth(unsigned month, unsigned day, unsigned year)
{
  if (day <= 0 || month > 12) return false;
  unsigned d = daysInMonth[month-1];
  if (isLeapYear(year) && month == 2) d++;
  return day <= d;
}

// Algorithm from K & R, "The C Programming Language", 1st ed.
bool TDateTime::isLeapYear(unsigned year)
{
  return (year&3) == 0 && (year%100 != 0 || year % 400 == 0);
}


string TDateTime::getDateFormatName(TDateTime::DateFormat id)
{
  return p_dateFormatName[id];
}


string TDateTime::getTimeFormatName(TDateTime::TimeFormat id)
{
  return p_timeFormatName[id];
}

