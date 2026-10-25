#ifndef PLAYBACKLOOP__H
#define PLAYBACKLOOP__H

#include <vector>

// Playback speed and back-and-forth looping for the Relaxed Scan and IRC Path
// panels.  No wx here, so the order of the points is testable.
namespace playbackloop {

enum Speed { SLOW = 0, NORMAL = 1, FAST = 2 };

// Milliseconds between points; NORMAL is what the panels always used.
inline int delayMs(int speed)
{
  return speed == SLOW ? 1000 : speed == FAST ? 150 : 500;
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

// The points shown by <ticks> ticks of play from the first point.
inline std::vector<long> sequence(long n, long ticks)
{
  std::vector<long> out;
  Pos p{0, 1};
  if (n > 0) out.push_back(0);
  for (long t = 0; t < ticks && n > 0; t++) {
    p = next(p, n);
    out.push_back(p.cur);
  }
  return out;
}

}

#endif
