#ifndef ECCE_TEXT_FILE_READER_H
#define ECCE_TEXT_FILE_READER_H
#include <algorithm>
#include <climits>
#include <sstream>
#include <string>
#include "dsm/EDSI.H"

// A bounded tail shared by the window and the Organizer's inline preview.
class TextFileReader {
public:
  static constexpr unsigned long chunk = 65536;
  unsigned long size = 0, start = 0, bytesRead = 0;
  bool binary = false;
  std::string text, error;

  bool tail(EDSI& source) {
    error.clear();
    const auto nextSize = source.getDataSetSize();
    // An empty HTTP resource has no valid byte range to request.
    if (nextSize == 0) {
      if (!source.exists()) {
        error = source.m_msgStack.getMessage();
        if (error.empty()) error = "Unable to stat file.";
        return false;
      }
      size = start = 0; binary = false; text.clear();
      return true;
    }
    std::string probe;
    if (!read(source, 0, std::min(nextSize, 8192UL), probe)) return false;
    const bool nextBinary = probe.find('\0') != std::string::npos;
    // EDSI range offsets are signed ints. Refuse overflow explicitly.
    if (!nextBinary && nextSize > INT_MAX) {
      size = nextSize;
      error = "Text viewing is limited to files smaller than 2 GiB.";
      return false;
    }
    const auto nextStart = nextSize > chunk ? nextSize - chunk : 0;
    std::string nextText;
    if (!nextBinary) {
      if (nextSize <= 8192) nextText = probe;
      else if (!read(source, nextStart, nextSize - nextStart, nextText)) return false;
    }
    size = nextSize; start = nextBinary ? size : nextStart;
    binary = nextBinary; text.swap(nextText);
    return true;
  }
  bool more(EDSI& source) {
    error.clear();
    if (binary || start == 0) return true;
    const auto n = std::min(start, chunk);
    std::string prefix;
    if (!read(source, start - n, n, prefix)) return false;
    start -= n; text.insert(0, prefix);
    return true;
  }
private:
  bool read(EDSI& source, unsigned long at, unsigned long n, std::string& out) {
    std::ostringstream data;
    if (!source.getDataSubSet(data, static_cast<int>(at), static_cast<int>(n))) {
      error = source.m_msgStack.getMessage();
      if (error.empty()) error = "Unable to read file.";
      return false;
    }
    out = data.str(); bytesRead += out.size();
    return true;
  }
};
#endif
