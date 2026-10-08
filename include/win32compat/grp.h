// No Unix groups; lookups find nothing.
#ifndef WIN32COMPAT_GRP_H
#define WIN32COMPAT_GRP_H
#include "util/PosixCompat.H"
struct group { char *gr_name; char **gr_mem; };
inline struct group *getgrnam(const char *) { return nullptr; }
inline struct group *getgrgid(int) { return nullptr; }
#endif
