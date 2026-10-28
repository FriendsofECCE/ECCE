// JobLogDates: the date forms `date` prints on a compute host in the
// locales a job script may run under, and gensub's own ISO line.
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
using namespace std;

#include "comm/JobLogDates.H"

static int failures = 0;

static void expect(const string& text, long want)
{
  long got = JobLogDates::parse(text);
  if (got != want) {
    cerr << "FAIL \"" << text << "\": got " << got << ", want " << want << endl;
    failures++;
  } else {
    cout << "PASS " << text << endl;
  }
}

int main()
{
  const long t = 1793171570L;   // 2026-10-28 07:12:50 UTC
  expect("2026-10-28T07:12:50Z", t);
  expect("Wed Oct 28 08:12:50 CET 2026", t);
  expect("Wed Oct 28 07:12:50 UTC 2026", t);
  expect("Wed Oct 28 09:12:50 CEST 2026", t);
  expect("Wed 28 Oct 2026 08:12:50 AM CET", t);
  expect("Wed 28 Oct 2026 12:12:50 AM EDT", t - 3 * 3600);
  expect("Wed Oct 28 03:12:50 PM PDT 2026", t + 15 * 3600);
  expect("ons 28 okt 2026 08:12:50 CET", t);
  expect("Wed Oct 28 08:12:50 +0100 2026", t);
  expect("Wed Oct 28 07:12:50 2026", -1);   // no zone: cannot be placed
  expect("2026-10-28T07:12:50", -1);        // ISO without Z
  expect("28/10/2026 07:12:50", -1);
  expect("", -1);
  return failures ? 1 : 0;
}
