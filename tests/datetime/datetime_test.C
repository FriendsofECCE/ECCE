#include <cstdio>
#include <locale.h>

#include "util/TDateTime.H"

int main()
{
  // Honor the test runner's locale. HTTP/RFC dates must still parse their
  // English month name even when LC_TIME is not the C locale.
  setlocale(LC_TIME, "");

  const char *rfcDate = "Thu, 22 Oct 2026 04:17:19 GMT";
  TDateTime parsed(rfcDate);
  int failures = 0;

  if (parsed.asString('Y') != "2026") {
    std::fprintf(stderr, "year did not parse from RFC date\n");
    failures++;
  }
  if (parsed.asString('b') != "Oct" || parsed.asString('d') != "22") {
    std::fprintf(stderr, "month/day did not parse from RFC date\n");
    failures++;
  }
  if (parsed.asString('X') != "4:17:19") {
    std::fprintf(stderr, "time did not parse from RFC date\n");
    failures++;
  }
  long withWeekday = TDateTime::toSeconds(rfcDate);
  long withoutWeekday = TDateTime::toSeconds("22 Oct 2026 04:17:19");
  if (withWeekday == 0 || withoutWeekday != withWeekday) {
    std::fprintf(stderr, "date without weekday did not parse\n");
    failures++;
  }

  return failures == 0 ? 0 : 1;
}
