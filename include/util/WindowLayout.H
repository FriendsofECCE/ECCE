/**
 * @file
 *
 * Preferences > Windows and tabs (#983 stage 2): which tools open as tabs
 * of the Organizer's window and which in windows of their own.  A preset
 * (Normal, Small, Wide) fills one Tab/Window row per tool; editing a row
 * makes the layout Custom.  Stored in EcceGlobal; no wx dependency, so the
 * rules can be tested and read by scripts without a display.
 */
#ifndef WINDOWLAYOUT_HH
#define WINDOWLAYOUT_HH

#include <map>
#include <string>
#include <vector>

class WindowLayout
{
public:
  enum Preset { Normal, Small, Wide, Custom };

  struct Tool {
    const char *key;     // stored row name, also used by the test hooks
    const char *label;   // shown in the Customize table
  };
  // The tools the Organizer can host as tabs, in table order.
  static const std::vector<Tool>& tools();

  // What a fresh install starts with.
  static Preset defaultPreset();
  static Preset preset();
  static const char *presetName(Preset p);       // "normal", ...
  static bool presetFromName(const std::string& name, Preset& p);

  // Whether preset p puts the tool in a tab (Custom: the stored row).
  static bool presetTab(Preset p, const std::string& key);
  // The current choice for a tool.
  static bool tab(const std::string& key);
  static bool anyTab();
  // Normal: the Calculation Editor's tab is the combined Set-up tab (a
  // detail area for Theory/Runtype Details and the input file beside it).
  static bool combinedSetupTab();

  // Choosing a preset (not Custom) stores it; the rows follow it.
  static void setPreset(Preset p);
  // Stores the rows; the layout is Custom unless they are the current
  // preset's.  Unknown keys are ignored.
  static void setRows(const std::map<std::string, bool>& rows);
  static void setRow(const std::string& key, bool tab);
  static std::map<std::string, bool> rows();

  // "preset=small rows=builder:tab,calced:window,..." for test hooks.
  static std::string describe();
};

#endif
