/**
 * @file
 *
 * The Launcher's panel inside another tool (the Calculation Editor's Set-up
 * area, #983). The editor's code is linked without the Launcher's, so the
 * Launcher registers how to make one; with none registered the editor opens
 * the Launcher as a tab or window.
 */
#ifndef LAUNCHEMBED_HH
#define LAUNCHEMBED_HH

#include <functional>
#include <string>

class wxWindow;

class LaunchEmbed
{
public:
  virtual ~LaunchEmbed() {}
  virtual wxWindow *window() = 0;
  // Test hook: "info" describes the panel, anything else is the Launcher
  // script's "get" names.
  virtual std::string get(const std::string& what) = 0;
  // Clicks Launch; false when it is off.
  virtual bool go() = 0;

  // parent, the calculation's URL, and what to call when the panel's host
  // must close it or lay it out again, and where its messages go.
  typedef std::function<LaunchEmbed*(wxWindow *, const std::string&,
                                     std::function<void()>,
                                     std::function<void()>,
                                     std::function<void(const std::string&, int)>)> Maker;
  static Maker& maker() { static Maker m; return m; }
};

#endif
