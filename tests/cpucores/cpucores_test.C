// CpuCores (#436): sane counts, and on Linux the same physical count as lscpu.
#include "util/CpuCores.H"
#include <cstdio>
#include <set>
#include <string>
#include <utility>

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

int main()
{
  unsigned l = CpuCores::logical(), p = CpuCores::physical();
  printf("physical=%u logical=%u\n", p, l);
  CHECK(l >= 1);
  CHECK(p >= 1 && p <= l);
#if !defined(_WIN32) && !defined(__APPLE__)
  FILE* f = popen("lscpu -p=CORE,SOCKET 2>/dev/null", "r");
  std::set<std::pair<int,int> > cores;
  char line[256];
  while (f && fgets(line, sizeof line, f)) {
    int c, s;
    if (line[0] != '#' && sscanf(line, "%d,%d", &c, &s) == 2)
      cores.insert(std::make_pair(s, c));
  }
  if (f) pclose(f);
  if (cores.empty()) printf("lscpu not available: comparison skipped\n");
  else CHECK(cores.size() == p);
#endif
  return fails ? 1 : 0;
}
