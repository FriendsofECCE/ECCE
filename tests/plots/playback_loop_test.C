// Playback loop and speed of the Relaxed Scan and IRC Path panels, without wx.
#include <cstdio>
#include <vector>
#include "PlaybackLoop.H"

static int fails = 0;
static void check(bool ok, const char *what)
{
  std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
  if (!ok) fails++;
}

int main()
{
  using namespace playbackloop;
  check(sequence(5, 10) == (std::vector<long>{0, 1, 2, 3, 4, 3, 2, 1, 0, 1, 2}),
        "5 points: forward, back, forward again; ends shown once");
  check(sequence(2, 5) == (std::vector<long>{0, 1, 0, 1, 0, 1}),
        "2 points alternate");
  check(sequence(1, 3) == (std::vector<long>{0, 0, 0, 0}), "1 point stays");
  check(sequence(0, 3).empty(), "no points, nothing shown");
  check(delayMs(SLOW) > delayMs(NORMAL) && delayMs(NORMAL) > delayMs(FAST),
        "slow > normal > fast delay");
  check(delayMs(NORMAL) == 500, "normal is the old 500 ms");
  std::printf("%s\n", fails ? "FAILED" : "all passed");
  return fails ? 1 : 0;
}
