#include "util/GLPath.H"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>

#include "util/EnvSwitches.H"

using std::string;

static bool s_known = false;
static bool s_software = false;
static string s_line;

static string lower(const char* s)
{
  string r = s ? s : "";
  std::transform(r.begin(), r.end(), r.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return r;
}

bool GLPath::isSoftwareRenderer(const char* renderer)
{
  static const char* const names[] = {"llvmpipe", "softpipe", "swrast",
      "svga3d", "virgl", "microsoft basic render driver", "gdi generic",
      "apple software renderer"};
  const string r = lower(renderer);
  for (const char* n : names)
    if (r.find(n) != string::npos) return true;
  return false;
}

GLPath::Choice GLPath::choose(const char* renderer, const char* override,
                              bool fromPreference)
{
  const string o = lower(override);
  if (o == "software" || o == "hardware")
    return {o == "software", fromPreference ? PREFERENCE : ENVIRONMENT};
  return {isSoftwareRenderer(renderer), DETECTED};
}

const char* GLPath::whyText(Why why)
{
  return why == DETECTED ? "detected"
       : why == ENVIRONMENT ? "ECCE_GL_PATH" : "preference";
}

static string stateFile()
{
  //  The thumbnail renderer runs without ECCE_REALUSERHOME.
  const char* home = getenv("ECCE_REALUSERHOME");
  if (!home || !*home) home = getenv("HOME");
  return (home && *home) ? string(home) + "/.ECCE/gl-renderer" : string();
}

void GLPath::record(const char* renderer, const char* vendor,
                    const char* version)
{
  if (s_known) return;
  const char* env = getenv("ECCE_GL_PATH");
  const bool byPref = env && EnvSwitches::viaPreference("ECCE_GL_PATH");
  const Choice c = choose(renderer, env, byPref);
  s_known = true;
  s_software = c.software;
  s_line = string("OpenGL: renderer ") + (renderer ? renderer : "unknown") +
           ", vendor " + (vendor ? vendor : "unknown") + ", version " +
           (version ? version : "unknown") + "; drawing path " +
           (c.software ? "software" : "hardware") + " (" + whyText(c.why) + ")";
  //  Every viewer process (one per thumbnail) gets here: say it only when
  //  the renderer or the choice differs from the last one recorded.
  const string path = stateFile();
  string old;
  if (!path.empty()) {
    std::ifstream in(path.c_str());
    std::getline(in, old);
  }
  if (path.empty() || old != s_line)
    fprintf(stderr, "[GL] %s\n", s_line.c_str());
  if (!path.empty() && old != s_line) {
    std::ofstream out(path.c_str());
    out << s_line << "\n";
  }
}

bool GLPath::known() { return s_known; }

bool GLPath::software() { return s_known && s_software; }

string GLPath::describe() { return s_line; }

string GLPath::describeForDisplay()
{
  if (s_known) return s_line;
  const string path = stateFile();
  if (path.empty()) return "";
  std::ifstream in(path.c_str());
  string line;
  std::getline(in, line);
  return line.empty() ? line : line + " (last seen in the Viewer)";
}
