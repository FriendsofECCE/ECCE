// The checks the job store makes on the outputs it copies back from the
// compute host (#732).  Header only, no ECCE dependencies, so
// tests/jobstore compiles it with plain g++.
#ifndef OUTPUTCOPYCHECK_H
#define OUTPUTCOPYCHECK_H

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <string>
#include <system_error>

namespace OutputCopyCheck {

// The line a code prints only when it ends normally.  An output copied back
// without it is not a finished result, whatever the job's exit status said.
inline std::string normalTerminationMarker(const std::string& code)
{
  std::string lc;
  for (size_t i = 0; i < code.size(); i++)
    lc += (char)tolower((unsigned char)code[i]);
  if (lc == "orca") return "ORCA TERMINATED NORMALLY";
  if (lc == "cp2k") return "PROGRAM ENDED AT";
  if (lc == "ecce-qm") return "end_of_output";
  if (lc == "xtb") return "normal termination of xtb";
  if (lc == "crest") return "CREST terminated normally";
  if (lc == "mlp") return "ecce-mlp terminated normally";
  return "";
}

// The monitor's file message carries the count as "1" or, since #732,
// "1:<size on the host>".  An older store reads the count with atoi and
// ignores the size; an older monitor gives no size (-1 here).
inline int parseCountToken(const char* tok, long long& hostSize)
{
  hostSize = -1;
  if (tok == 0) return 0;
  int count = atoi(tok);
  const char* colon = strchr(tok, ':');
  if (colon && colon[1] >= '0' && colon[1] <= '9')
    hostSize = atoll(colon + 1);
  return count;
}

// -1 when the file does not exist.
// std::filesystem: plain stat() has a 32-bit size on MinGW.
inline long long localSize(const std::string& path)
{
  std::error_code ec;
  const auto n = std::filesystem::file_size(std::filesystem::path(path), ec);
  return ec ? -1 : (long long)n;
}

// True when the copy is missing or shorter than the file the host named.
// An unknown host size (-1) only fails a missing copy.
inline bool copyIsShort(long long local, long long host)
{
  if (local < 0) return true;
  return host >= 0 && local < host;
}

// Whether the marker occurs in the file's last 4 MB (the codes print it at
// the end; a 60 MB output need not be read whole).
inline bool hasMarker(const std::string& path, const std::string& marker)
{
  if (marker.empty()) return true;
  std::ifstream in(path.c_str(), std::ios::binary);
  if (!in) return false;
  const std::streamoff tail = 4 * 1024 * 1024;
  in.seekg(0, std::ios::end);
  std::streamoff size = in.tellg();
  in.seekg(size > tail ? size - tail : 0, std::ios::beg);
  std::ostringstream oss;
  oss << in.rdbuf();
  return oss.str().find(marker) != std::string::npos;
}

inline std::string sizeText(long long n)
{
  char buf[64];
  if (n < 0) return "no file";
  if (n < 10 * 1024) snprintf(buf, sizeof buf, "%lld bytes", n);
  else if (n < 10LL * 1024 * 1024) snprintf(buf, sizeof buf, "%lld KB", n / 1024);
  else snprintf(buf, sizeof buf, "%lld MB", n / (1024 * 1024));
  return buf;
}

// The reason a run's state gives when the output that came back is not the
// whole of what the run wrote.  marker is empty when the code has none.
inline std::string incompleteOutputReason(const std::string& fileName,
                                          const std::string& marker,
                                          long long local, long long host)
{
  std::string r = "The output copied back is incomplete: " + fileName;
  if (local < 0) r += " did not arrive";
  else if (local == 0) r += " arrived empty";
  else if (host > local) r += " has " + sizeText(local) + " of " + sizeText(host);
  else if (!marker.empty()) r += " has no '" + marker + "'";
  if (local <= 0 && host > 0)
    r += " (" + sizeText(host) + " in the run directory)";
  r += ". Use ecce-fetch to copy it from the run directory.";
  return r;
}

// A reason that describes ECCE's contact with the job rather than the job
// itself; the job's own reason always replaces it.
inline bool isContactReason(const std::string& reason)
{
  return reason.rfind("Could not reconnect to the job", 0) == 0;
}

} // namespace OutputCopyCheck

#endif // OUTPUTCOPYCHECK_H
