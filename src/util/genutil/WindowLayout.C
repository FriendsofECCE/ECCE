/**
 * @file
 *
 * Preferences > Windows and tabs (#983 stage 2).  See WindowLayout.H.
 */
#include "util/Preferences.H"
#include "util/PreferenceLabels.H"
#include "util/WindowLayout.H"

namespace {
const char *PRESET_KEY = "WindowsAndTabs.Layout";
std::string rowKey(const std::string& key) { return "WindowsAndTabs." + key; }
}


const std::vector<WindowLayout::Tool>& WindowLayout::tools()
{
  static const std::vector<Tool> t = {
    {"builder", "Builder / Viewer"},
    {"calced", "Calculation Editor"},
    {"basistool", "Basis Set Tool"},
    {"mdeditors", "MD editors (Energy, Optimize, Dynamics, GROMACS Prepare)"},
    {"mdprepare", "MD Prepare"},
    {"launcher", "Launcher"},
    {"pertable", "Periodic Table"},
  };
  return t;
}


// Normal: a first-week user gets one window with tabs; Wide stays a choice
// for large or several monitors.
WindowLayout::Preset WindowLayout::defaultPreset()
{
  return Normal;
}


const char *WindowLayout::presetName(Preset p)
{
  switch (p) {
    case Normal: return "normal";
    case Small: return "small";
    case Wide: return "wide";
    default: return "custom";
  }
}


bool WindowLayout::presetFromName(const std::string& name, Preset& p)
{
  for (Preset q : {Normal, Small, Wide, Custom})
    if (name == presetName(q)) { p = q; return true; }
  return false;
}


WindowLayout::Preset WindowLayout::preset()
{
  Preferences pref(PrefLabels::GLOBALPREFFILE);
  std::string name;
  Preset p;
  if (pref.getString(PRESET_KEY, name) && presetFromName(name, p)) return p;
  return defaultPreset();
}


bool WindowLayout::combinedSetupTab()
{
  return preset() == Normal;
}


bool WindowLayout::presetTab(Preset p, const std::string& key)
{
  switch (p) {
    case Wide: return false;
    // Normal: tabs as in Small, the editor's tab is the Set-up tab
    // (combinedSetupTab).
    case Normal:
    case Small: return true;
    default: break;
  }
  Preferences pref(PrefLabels::GLOBALPREFFILE);
  std::string v;
  return pref.getString(rowKey(key), v) && v == "tab";
}


bool WindowLayout::tab(const std::string& key)
{
  return presetTab(preset(), key);
}


bool WindowLayout::anyTab()
{
  for (const Tool& t : tools())
    if (tab(t.key)) return true;
  return false;
}


std::map<std::string, bool> WindowLayout::rows()
{
  std::map<std::string, bool> r;
  Preset p = preset();
  for (const Tool& t : tools()) r[t.key] = presetTab(p, t.key);
  return r;
}


void WindowLayout::setPreset(Preset p)
{
  if (p == Custom) return;
  Preferences pref(PrefLabels::GLOBALPREFFILE);
  pref.setString(PRESET_KEY, presetName(p));
  pref.saveFile();
}


void WindowLayout::setRows(const std::map<std::string, bool>& wanted)
{
  std::map<std::string, bool> r = rows();
  for (const auto& kv : wanted)
    if (r.count(kv.first)) r[kv.first] = kv.second;
  if (r == rows()) return;
  Preferences pref(PrefLabels::GLOBALPREFFILE);
  pref.setString(PRESET_KEY, presetName(Custom));
  for (const auto& kv : r)
    pref.setString(rowKey(kv.first), kv.second ? "tab" : "window");
  pref.saveFile();
}


void WindowLayout::setRow(const std::string& key, bool tab)
{
  setRows({{key, tab}});
}


std::string WindowLayout::describe()
{
  std::string out = std::string("preset=") + presetName(preset()) + " rows=";
  bool first = true;
  std::map<std::string, bool> r = rows();
  for (const Tool& t : tools()) {
    out += std::string(first ? "" : ",") + t.key + ":" +
           (r[t.key] ? "tab" : "window");
    first = false;
  }
  return out;
}
