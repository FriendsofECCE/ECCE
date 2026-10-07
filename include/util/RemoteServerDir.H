/**
 * @file
 * Where the central server's client configuration (DataServers, server.pem)
 * is read: ECCE_REMOTE_DIR when set -- a user's own choice made at first
 * start (#240), exported by the wrappers -- else the installation's
 * siteconfig/RemoteServer.  Header-only so that low-level code can use it.
 */
#ifndef REMOTESERVERDIR_HH
#define REMOTESERVERDIR_HH

#include <cstdlib>
#include <string>

inline std::string remoteServerDir()
{
  const char *d = getenv("ECCE_REMOTE_DIR");
  if (d && *d) return d;
  const char *home = getenv("ECCE_HOME");
  return std::string(home && *home ? home : "/opt/ecce") +
         "/siteconfig/RemoteServer";
}

#endif
