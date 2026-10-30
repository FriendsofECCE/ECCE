**ECCE 9.4.5**, a patch release of 9.4.4. Please report problems as [issues](https://github.com/FriendsofECCE/ECCE/issues).

**Platforms.** Linux (Debian 13; Ubuntu 24.04, RHEL/Rocky 9 and Fedora 44 packages are built on those systems but not tested). Windows 10 and 11. macOS, Apple silicon and Intel: experimental.

## Fixes
- **After a network interruption** (for example a VPN that disconnects) with jobs running on a cluster that uses a password, ECCE no longer opens a password window for every job, again after each Cancel (#684). At most one window per cluster asks for the login; Cancel, or a refused password, leaves the jobs waiting for login until Run Mgmt > Reconnect or the next start; Reconnect never starts a second monitor for a job that is still followed. A connection that is full (the cluster's limit of sessions per login) is reported as such instead of as a wrong password.

## Known issues
- **Density Map:** the full map of a large calculation still takes a while after zooming.
- **Windows:** the first start after installing takes about a minute on slower computers, with progress shown (#444).
- **HiDPI screens on Windows** (scaling 150 % or more): some icons and panels are not yet scaled (#670).
- **GROMACS, CP2K and Quantum ESPRESSO** remain experimental. **The MO diagram** is experimental.
