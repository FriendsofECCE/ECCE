---
type: map
title: "The viewer's software/hardware GL path (#566): GL_RENDERER read once, ECCE_GL_PATH and Advanced.GLPath override, text as geometry on software"
area: wx-viewer
paths: [include/util/GLPath.H, src/util/genutil/GLPath.C, include/viz/ViewerText.H, include/viz/FixedPixelText.H, src/inv/wxinv/SoWxRenderArea.C, src/util/genutil/EnvSwitches.C, src/wxgui/wxdialogs/GlobalPrefs.C, tests/glpath/glpath_test.C, tests/apps/gl_path_test.py]
issues: [566, 565, 538]
---
- `SoWxRenderArea::redraw` hands the first context's GL_RENDERER/VENDOR/
  VERSION to `GLPath::record` (wx-free, in `ecceutil`). It classifies the
  renderer, applies the override, prints one `[GL] ...` line on stderr and
  writes the same line to `<ECCE_REALUSERHOME>/.ECCE/gl-renderer`.
- Override order: `ECCE_GL_PATH=software|hardware` (set by hand), else the
  Advanced preference `Advanced.GLPath` (`auto|software|hardware`), which
  `EnvSwitches::apply` turns into `ECCE_GL_PATH` in the process (listed in
  `ECCE_PREF_ENV`, so "preference" can be told from "by hand"). A running
  program keeps its choice.
- Software path: `drawViewerText` (viz/ViewerText.H) draws every label with
  `drawFixedPixelText` instead of `glCallLists`, the font display lists are
  not built, and the render area starts in quick transparency
  (`p_softwareGL`). Hardware path is the old code. `GLPath::software()` is
  false until a context has been seen, so a node that draws before the first
  frame behaves as hardware.
- The Organizer has no viewer: Help > About and the Report a Problem
  `version.txt` show the last line the Viewer wrote, marked "last seen in the
  Viewer".
- `SGLattice` never built its font lists (`#if 000`), so its a/b/c labels
  drew nothing on hardware; on the software path they now draw.
