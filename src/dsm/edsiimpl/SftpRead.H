#ifndef ECCE_SFTP_READ_H
#define ECCE_SFTP_READ_H
#include <string>

// Keep the wire data encoded: the interactive login normalises line endings.
namespace SftpRead {
inline std::string quote(const std::string& s)
{
  std::string out = "'";
  for (char c : s) out += c == '\'' ? "'\\''" : std::string(1, c);
  return out + "'";
}
inline std::string rangeScript(const std::string& path, int start, int length)
{
  return "if [ -f " + quote(path) + " ] && [ -r " + quote(path) + " ]; then\n"
    "  { dd if=" + quote(path) + " bs=1 skip=" + std::to_string(start) +
    " count=" + std::to_string(length) + " 2>/dev/null && printf '\\nECCE_RANGE_OK'; } | base64\n"
    "fi\ntrue\n";
}
inline std::string sizeScript(const std::string& path)
{
  return "if [ -f " + quote(path) + " ]; then\n"
    "  stat -c '%s' -- " + quote(path) + " 2>/dev/null || stat -f '%z' " + quote(path) + " 2>/dev/null\n"
    "fi\ntrue\n";
}
}
#endif
