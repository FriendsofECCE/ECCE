#include "util/CpuCores.H"

#include <thread>

#if defined(_WIN32)
#include <windows.h>
#include <vector>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#else
#include <fstream>
#include <set>
#include <string>
#include <utility>
#include <unistd.h>
#endif

unsigned CpuCores::logical()
{
  unsigned n = std::thread::hardware_concurrency();
  return n > 0 ? n : 1;
}

unsigned CpuCores::physical()
{
  unsigned n = 0;
#if defined(_WIN32)
  DWORD len = 0;
  GetLogicalProcessorInformationEx(RelationProcessorCore, NULL, &len);
  if (len > 0) {
    std::vector<char> buf(len);
    if (GetLogicalProcessorInformationEx(RelationProcessorCore,
          (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buf.data(), &len)) {
      for (DWORD off = 0; off < len; ) {
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX p =
          (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(buf.data() + off);
        n++;
        off += p->Size;
      }
    }
  }
#elif defined(__APPLE__)
  int v = 0;
  size_t sz = sizeof v;
  if (sysctlbyname("hw.physicalcpu", &v, &sz, NULL, 0) == 0 && v > 0)
    n = (unsigned)v;
#else
  // One (package, core) pair per physical core; a missing topology
  // directory (some VMs, containers) ends the scan.
  std::set<std::pair<int,int> > cores;
  for (int cpu = 0; cpu < 4096; cpu++) {
    std::string d = "/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/topology/";
    std::ifstream core(d + "core_id");
    if (!core) {
      if (access(("/sys/devices/system/cpu/cpu" + std::to_string(cpu)).c_str(), F_OK) != 0 && cpu > 0)
        break;   // no such cpu; offline cpus have a dir but no topology
      continue;
    }
    std::ifstream pkg(d + "physical_package_id");
    int c = -1, p = 0;
    core >> c;
    pkg >> p;
    if (c >= 0) cores.insert(std::make_pair(p, c));
  }
  n = (unsigned)cores.size();
#endif
  unsigned l = logical();
  return (n == 0 || n > l) ? l : n;
}
