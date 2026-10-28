/**
 * @file
 *
 * Which OpenGL drawing path the 3-D viewer takes (#566).  Software renderers
 * (llvmpipe, VMs, Windows' GDI) get text drawn as geometry and the quick
 * transparency mode at once; hardware renderers are drawn as before.  No GL
 * or wx here: the viewer passes in the strings it read from the context.
 */
#ifndef GLPATH_HH
#define GLPATH_HH

#include <string>

namespace GLPath
{
  enum Why { DETECTED, ENVIRONMENT, PREFERENCE };

  struct Choice
  {
    bool software;
    Why  why;
  };

  /// True when a GL_RENDERER string names a software rasteriser.
  bool isSoftwareRenderer(const char* renderer);

  /// `override` is the value of ECCE_GL_PATH ("software", "hardware",
  /// anything else is ignored); `fromPreference` says it was set by the
  /// Advanced preference rather than by hand.
  Choice choose(const char* renderer, const char* override,
                bool fromPreference);

  /// Called with the strings of the first viewer context; later calls
  /// do nothing.  Reads ECCE_GL_PATH, writes one line to stderr and the
  /// per-user "last seen" file.
  void record(const char* renderer, const char* vendor, const char* version);

  /// False until record() has run in this process.
  bool known();

  /// Current window-to-framebuffer scale, updated by the active render area.
  void setContentScaleFactor(double scale);
  double contentScaleFactor();

  /// The path in use.  Hardware until a renderer is known.
  bool software();

  /// "OpenGL: renderer R, vendor V, version N; drawing path software (detected)".
  std::string describe();

  /// describe() when this process has a viewer context, else the line the
  /// last viewer wrote, marked "last seen in the Viewer"; "" if none.
  std::string describeForDisplay();

  const char* whyText(Why why);
}

#endif
