// glob() for plain wildcard patterns in one directory, over FindFirstFile.
#ifndef WIN32COMPAT_GLOB_H
#define WIN32COMPAT_GLOB_H
#include "util/PosixCompat.H"
#include <string>
#include <vector>
#define GLOB_APPEND 1
struct glob_t { size_t gl_pathc; char **gl_pathv; };
inline int glob(const char *pat, int flags, void *, glob_t *g)
{
  if (!(flags & GLOB_APPEND)) { g->gl_pathc = 0; g->gl_pathv = nullptr; }
  std::string p(pat);
  size_t slash = p.find_last_of("/\\");
  std::string dir = slash == std::string::npos ? "" : p.substr(0, slash + 1);
  std::vector<std::string> found;
  WIN32_FIND_DATAA fd;
  HANDLE h = FindFirstFileA(pat, &fd);
  if (h != INVALID_HANDLE_VALUE) {
    do { found.push_back(dir + fd.cFileName); } while (FindNextFileA(h, &fd));
    FindClose(h);
  }
  if (found.empty()) return 3; // GLOB_NOMATCH
  g->gl_pathv = (char **)realloc(g->gl_pathv, (g->gl_pathc + found.size() + 1) * sizeof(char *));
  for (size_t i = 0; i < found.size(); i++)
    g->gl_pathv[g->gl_pathc++] = _strdup(found[i].c_str());
  g->gl_pathv[g->gl_pathc] = nullptr;
  return 0;
}
inline void globfree(glob_t *g)
{
  for (size_t i = 0; i < g->gl_pathc; i++) free(g->gl_pathv[i]);
  free(g->gl_pathv);
  g->gl_pathc = 0; g->gl_pathv = nullptr;
}
#endif
