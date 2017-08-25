///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: TDateTime.H
//
//
// CLASS SYNOPSIS:
//   Simple Time/Date class to enforce consistent formats.
//
// DESCRIPTION:
//   Creates and restores predefined time/date strings. RFC 1123/822.
//
// TODO 
//   would be handy to be able to create an invalid date and check for
//   validity.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef TDATETIME_HH
#define TDATETIME_HH

#include <string>
using std::string;

#include <time.h>

class TDateTime 
{

public:

    enum DateFormat 
      { AmericanLocale, EuropeanLocale, FullLocale, ShortLocale, NODATE, TOTALDATE };

    enum TimeFormat
      { HM24, HM, HMS24, HMS, HMS24Z, NOTIME, TOTALTIME };

        TDateTime(void);
        TDateTime(const TDateTime& rhs);
        TDateTime(const string& rhs); // in RFC 1123/822 format
        virtual ~TDateTime(void);

        TDateTime& operator=(const TDateTime&);
        TDateTime& operator=(const string&);
        bool operator==(const TDateTime&) const;
        bool operator!=(const TDateTime&) const;
        bool operator>(const TDateTime& rhs) const;
        bool operator<(const TDateTime& rhs) const;
        int compare(const TDateTime& rhs) const;

        // What dav uses e.g. 2001-01-11T20:34:18Z
        bool parseMilFormat(const string& value);

        // What old ecce code uses mm/dd/yyyy
        bool parseUSDate(const string& value);

        // pass in the value as a time_t instance
        void setFromTimeT(const time_t& timet);

        long   toSeconds() const;
        string toString() const;   // GMT
        string toString(TDateTime::DateFormat dformat,
                        TDateTime::TimeFormat tformat) const;
        string toLocaleString() const;
	string asString(const char) const;

	bool isValid();

        static bool isLeapYear(unsigned year);

        // value must be in standard format
        static long toSeconds(const char *value,
                              const TDateTime::DateFormat& dformat=NODATE,
                              const TDateTime::TimeFormat& tformat=NOTIME);

  static string getDateFormatName(TDateTime::DateFormat id);
  static string getTimeFormatName(TDateTime::TimeFormat id);

protected:

        void initTimeZone();
        static unsigned long currentDate(void);
        static unsigned long jday(unsigned m, unsigned d, unsigned y);
        static bool dayWithinMonth(unsigned m, unsigned d, unsigned y);
        void gregMDY(unsigned& m, unsigned& d, unsigned& y) const;
        bool adjustZone(const char *zname);
        void debug() const;

private:

     struct tm p_tm;
     static time_t    p_tzseconds;  // locale offset from GMT
  static const char * p_dateFormatName[TOTALDATE];
  static const char * p_timeFormatName[TOTALTIME];

}; 

#endif 
