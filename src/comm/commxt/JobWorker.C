#include "comm/JobWorker.H"

#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

#include "util/Ecce.H"
#include "util/PosixCompat.H"

namespace JobWorker
{
  namespace {
    std::string gRecordPath;
    Record gRecord;
    EventWriter gWriter = 0;
    void *gWriterContext = 0;
    int gLockFd = -1;

    std::string hexEncode(const std::string& value)
    {
      static const char digits[] = "0123456789abcdef";
      std::string out;
      out.reserve(value.size() * 2);
      for (unsigned char c : value) {
        out += digits[c >> 4];
        out += digits[c & 0x0f];
      }
      return out;
    }

    Phase phaseFromName(const std::string& value)
    {
      if (value == "preparing") return PREPARING;
      if (value == "authenticating") return AUTHENTICATING;
      if (value == "submitting") return SUBMITTING;
      if (value == "submitted") return SUBMITTED;
      if (value == "monitoring") return MONITORING;
      if (value == "recovering") return RECOVERING;
      if (value == "needs_reconciliation") return NEEDS_RECONCILIATION;
      if (value == "complete") return COMPLETE;
      if (value == "failed") return FAILED;
      return CREATED;
    }

    bool hexDecode(const std::string& value, std::string& out)
    {
      if (value.size() % 2) return false;
      out.clear();
      for (size_t i = 0; i < value.size(); i += 2) {
        const char *digits = "0123456789abcdef";
        size_t a = std::string(digits).find((char)tolower(value[i]));
        size_t b = std::string(digits).find((char)tolower(value[i + 1]));
        if (a == std::string::npos || b == std::string::npos) return false;
        out += (char)((a << 4) | b);
      }
      return true;
    }

    bool replaceFile(const std::string& from, const std::string& to)
    {
#ifdef _WIN32
      return MoveFileExA(from.c_str(), to.c_str(),
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
      return rename(from.c_str(), to.c_str()) == 0;
#endif
    }

    std::string baseDirectory()
    {
      const std::string root = std::string(Ecce::realUserHome()) + "/.ECCE";
      struct stat st;
      if (stat(root.c_str(), &st) != 0 && mkdir(root.c_str(), 0700) != 0)
        return "";
      std::string path = root + "/jobworker";
      if (stat(path.c_str(), &st) != 0 && mkdir(path.c_str(), 0700) != 0)
        return "";
      return path;
    }

    bool saveActiveRecord(std::string& error)
    {
      if (gRecordPath.empty()) return true; // legacy eccejobstore process
      std::string temp = gRecordPath + ".tmp." + std::to_string((long)getpid());
      std::ofstream out(temp.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
      if (!out) { error = "Cannot create worker state file " + temp; return false; }
      out << "version=1\n"
          << "url=" << hexEncode(gRecord.calculationUrl) << "\n"
          << "phase=" << phaseName(gRecord.phase) << "\n"
          << "job=" << hexEncode(gRecord.schedulerJobId) << "\n"
          << "bookmark=" << hexEncode(gRecord.monitorBookmark) << "\n"
          << "config=" << hexEncode(gRecord.configFile) << "\n"
          << "remote=" << hexEncode(gRecord.remoteDirectory) << "\n"
          << "sequence=" << gRecord.sequence << "\n";
      out.flush();
      if (!out) { error = "Cannot write worker state file " + temp; out.close(); unlink(temp.c_str()); return false; }
      out.close();
      int fd = open(temp.c_str(), O_RDONLY);
      if (fd >= 0) { (void)fsync(fd); close(fd); }
      if (!replaceFile(temp, gRecordPath)) {
        error = "Cannot replace worker state file " + gRecordPath;
        unlink(temp.c_str());
        return false;
      }
      return true;
    }

    bool copyFile(const std::string& from, const std::string& to)
    {
      std::ifstream in(from.c_str(), std::ios::binary);
      std::ofstream out(to.c_str(), std::ios::binary | std::ios::trunc);
      if (!in || !out) return false;
      out << in.rdbuf();
      out.flush();
      return in.good() || in.eof() ? out.good() : false;
    }
  }

  const char *phaseName(Phase phase)
  {
    switch (phase) {
      case CREATED: return "created";
      case PREPARING: return "preparing";
      case AUTHENTICATING: return "authenticating";
      case SUBMITTING: return "submitting";
      case SUBMITTED: return "submitted";
      case MONITORING: return "monitoring";
      case RECOVERING: return "recovering";
      case NEEDS_RECONCILIATION: return "needs_reconciliation";
      case COMPLETE: return "complete";
      case FAILED: return "failed";
    }
    return "unknown";
  }

  const char *eventName(EventType type)
  {
    switch (type) {
      case PHASE_CHANGED: return "phase_changed";
      case AUTHENTICATION_REQUIRED: return "authentication_required";
      case PROGRESS: return "progress";
      case OUTPUT_AVAILABLE: return "output_available";
      case WARNING: return "warning";
      case FAILURE: return "failure";
      case FINISHED: return "finished";
    }
    return "unknown";
  }

  std::string recordPathFor(const std::string& calculationUrl)
  {
    // FNV-1a is used instead of std::hash so the disk name stays stable
    // across process restarts and compiler/library upgrades.
    unsigned long long hash = 14695981039346656037ULL;
    for (unsigned char c : calculationUrl) {
      hash ^= c;
      hash *= 1099511628211ULL;
    }
    std::ostringstream name;
    const std::string base = baseDirectory();
    if (base.empty()) return "";
    name << base << "/job-" << std::hex << hash << ".state";
    return name.str();
  }

  std::string recordDirectoryFor(const std::string& calculationUrl)
  { return recordPathFor(calculationUrl) + ".d"; }

  bool loadRecord(const std::string& calculationUrl, Record& record)
  {
    const std::string path = recordPathFor(calculationUrl);
    if (path.empty()) return false;
    std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
    if (!in) return false;
    Record found;
    std::string line;
    while (std::getline(in, line)) {
      size_t eq = line.find('=');
      if (eq == std::string::npos) continue;
      std::string key = line.substr(0, eq), value = line.substr(eq + 1), decoded;
      if (key == "url" && hexDecode(value, decoded)) found.calculationUrl = decoded;
      else if (key == "phase") found.phase = phaseFromName(value);
      else if (key == "job" && hexDecode(value, decoded)) found.schedulerJobId = decoded;
      else if (key == "bookmark" && hexDecode(value, decoded)) found.monitorBookmark = decoded;
      else if (key == "config" && hexDecode(value, decoded)) found.configFile = decoded;
      else if (key == "remote" && hexDecode(value, decoded)) found.remoteDirectory = decoded;
      else if (key == "sequence") found.sequence = strtoul(value.c_str(), 0, 10);
    }
    if (found.calculationUrl != calculationUrl) return false;
    if (found.monitorBookmark.empty() && !found.configFile.empty()) {
      const size_t slash = found.configFile.find_last_of("/\\");
      if (slash != std::string::npos) {
        std::ifstream bookmark((found.configFile.substr(0, slash + 1) + "bookmark").c_str());
        std::getline(bookmark, found.monitorBookmark);
      }
    }
    record = found;
    return true;
  }

  bool initializeRecord(const std::string& path, const Record& record,
                        std::string& error)
  {
    if (path.empty()) { error = "Cannot locate the per-user worker state directory"; return false; }
    const size_t slash = path.find_last_of("/\\");
    if (slash == std::string::npos) { error = "Worker state path has no directory"; return false; }
    const std::string dir = path.substr(0, slash);
    struct stat st;
    if (stat(dir.c_str(), &st) != 0 && mkdir(dir.c_str(), 0700) != 0) {
      error = "Cannot create worker state directory " + dir;
      return false;
    }
    gRecordPath = path;
    gRecord = record;
    return saveActiveRecord(error);
  }

  bool acquireRecordLock(const std::string& path, std::string& error)
  {
    if (path.empty()) { error = "Cannot locate the per-user worker state directory"; return false; }
    const std::string lockPath = path + ".lock";
    gLockFd = open(lockPath.c_str(), O_CREAT | O_RDWR, 0600);
    if (gLockFd < 0 || flock(gLockFd, LOCK_EX | LOCK_NB) != 0) {
      if (gLockFd >= 0) close(gLockFd);
      gLockFd = -1;
      error = "A worker already owns this calculation's monitor";
      return false;
    }
    return true;
  }

  void releaseRecordLock()
  {
    if (gLockFd >= 0) { (void)flock(gLockFd, LOCK_UN); close(gLockFd); gLockFd = -1; }
  }

  void setActiveRecord(const std::string& path, const Record& record)
  { gRecordPath = path; gRecord = record; }

  Record activeRecord() { return gRecord; }

  bool persistSubmission(const std::string& jobId,
                         const std::string& configFile,
                         const std::string& remoteDirectory,
                         std::string& error)
  {
    if (gRecordPath.empty()) return true;
    const std::string dir = recordDirectoryFor(gRecord.calculationUrl);
    struct stat st;
    if (dir.empty() || (stat(dir.c_str(), &st) != 0 && mkdir(dir.c_str(), 0700) != 0)) {
      error = "Cannot create durable job monitor directory " + dir;
      return false;
    }
    const std::string durableConfig = dir + "/eccejobstore.conf";
    // Save the scheduler ID first. If preserving the monitor config fails,
    // recovery must still know submission happened and avoid a duplicate.
    gRecord.schedulerJobId = jobId;
    gRecord.remoteDirectory = remoteDirectory;
    gRecord.phase = SUBMITTED;
    std::string recordError;
    if (!saveActiveRecord(recordError)) { error = recordError; return false; }
    if (!copyFile(configFile, durableConfig)) {
      gRecord.configFile = configFile;
      (void)saveActiveRecord(recordError);
      error = "Cannot preserve monitor configuration " + configFile;
      return false;
    }
    gRecord.monitorBookmark.clear();
    gRecord.configFile = durableConfig;
    return saveActiveRecord(error);
  }

  void updateBookmark(const std::string& bookmark)
  {
    if (gRecordPath.empty()) return;
    gRecord.monitorBookmark = bookmark;
    std::string error;
    if (!saveActiveRecord(error))
      std::cerr << "eccejobworker: " << error << std::endl;
  }

  bool transition(Phase phase, EventType type, const std::string& message)
  {
    if (gRecordPath.empty()) return false;
    gRecord.phase = phase;
    ++gRecord.sequence;
    std::string error;
    const bool saved = saveActiveRecord(error);
    if (!saved)
      std::cerr << "eccejobworker: " << error << std::endl;
    emitEvent(type, phase, message);
    return saved;
  }

  void setEventWriter(EventWriter writer, void *context)
  { gWriter = writer; gWriterContext = context; }

  void emitEvent(EventType type, Phase phase, const std::string& message)
  {
    static unsigned long fallbackSequence = 0;
    unsigned long sequence = ++fallbackSequence;
    if (!gRecordPath.empty()) sequence = gRecord.sequence;
    std::string frame = "EVT|" + std::string(eventName(type)) + "|" +
      phaseName(phase) + "|" + std::to_string(sequence) + "|" + hexEncode(message) + "\n";
    if (gWriter) {
      if (!gWriter(gWriterContext, frame)) {
        // The GUI can close its pipe after submission. Monitoring remains
        // independent; failed event delivery must not stop the worker.
      }
    } else {
      // The original CLI mode remains useful for diagnostics and non-Windows
      // callers. It uses the same event names, wrapped as JSON lines.
      std::string escaped;
      for (char c : message) {
        if (c == '"' || c == '\\') escaped += '\\';
        if (c == '\n') escaped += "\\n";
        else if (c == '\r') escaped += "\\r";
        else escaped += c;
      }
      std::cout << "{\"event\":\"" << eventName(type)
                << "\",\"phase\":\"" << phaseName(phase)
                << "\",\"sequence\":" << sequence
                << ",\"message\":\"" << escaped << "\"}" << std::endl;
    }
  }
}
