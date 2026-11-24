#ifndef PLAYBACKLOOP__H
#define PLAYBACKLOOP__H

#include <string>
#include <vector>

// What Play does at the last point (Repeat: off / from the start / back and
// forth).  No wx here, so the order of the points is testable.
namespace playbackloop {

enum Repeat { OFF, FROM_START, BACK_AND_FORTH };

// The names the Repeat choice shows, in enum order, and the script words.
inline const char *repeatLabel(Repeat r)
{
  return r == OFF ? "off" : r == FROM_START ? "from the start" : "back and forth";
}
inline const char *repeatWord(Repeat r)
{
  return r == OFF ? "off" : r == FROM_START ? "start" : "backforth";
}
inline bool parseRepeat(const std::string& w, Repeat& r)
{
  if (w == "off") r = OFF;
  else if (w == "start") r = FROM_START;
  else if (w == "backforth" || w == "on") r = BACK_AND_FORTH;
  else return false;
  return true;
}

struct Pos {
  long cur;
  long dir;   // +1 forward, -1 backward
};

// One tick of a looping play over n points.  Reaching an end turns round
// without showing that end point twice.
inline Pos next(Pos p, long n)
{
  if (n <= 1) return Pos{0, 1};
  long c = p.cur + p.dir;
  if (c >= n) return Pos{n - 2, -1};
  if (c < 0) return Pos{1, 1};
  return Pos{c, p.dir};
}

// The points shown by <ticks> ticks of play from the first point; with OFF
// play stops at the last point.
inline std::vector<long> sequence(long n, long ticks, Repeat r = BACK_AND_FORTH)
{
  std::vector<long> out;
  Pos p{0, 1};
  if (n > 0) out.push_back(0);
  for (long t = 0; t < ticks && n > 0; t++) {
    if (r == BACK_AND_FORTH) p = next(p, n);
    else if (p.cur < n - 1) p.cur++;
    else if (r == FROM_START) p.cur = 0;
    else break;
    out.push_back(p.cur);
  }
  return out;
}

}

#endif
