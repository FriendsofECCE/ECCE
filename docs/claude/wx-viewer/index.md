---
type: index
title: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
area: wx-viewer
---
# wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls

Read this before changing any dialog, panel, sizer, grid, ewx control or the Open Inventor viewer. Split out of CLAUDE.md 2026-10-04.

## Entries

### wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls

- [Open Inventor's redraw sensor is a ONE-SHOT that re-arms on render, and with a render callback installed nothing re-arms it](open-inventors-redraw-sensor-is-a-one.md)
- [`ECCE_USE_COIN` (default ON) builds the viewer against Coin3D; `=OFF` keeps the vendored core for one release](ecce-use-coin-builds-the-viewer-against-coin3d.md)
- [Coin SORTED_LAYERS_BLEND is the default lobe transparency (6 passes, alpha canvas); slow scenes fall back to SCREEN_DOOR](coin-sorted-layers-blend-is-the-default-lobe-transparency.md)
- [`GlPlatform` is the viewer's only GLX/EGL code; `tools/nonx11/check.py` compiles the viewer as a non-X11 port (#232)](glplatform-is-the-viewers-only-glx-egl-code.md)
- [`tools/coin/compare.sh` renders a fixed scene set with both viewer builds (#166 stage 2); harness pitfalls](tools-coin-compare-sh-renders-both-viewer-builds.md)
- [Atom labels drew nothing in 9.x: the sh wrappers dropped `FL_FONT_PATH`; flclient now defaults to `$ECCE_HOME/data/client/fonts`](atom-labels-need-the-bundled-font-fl-font-path.md)
- [Builder Reset View (toolbar, Render menu, Home) is camera only; the old home button restored a pre-molecule camera](builder-reset-view-is-camera-only-home-key.md)
- [wx3.2/GTK3 layout reentrancy](wx3-2-gtk3-layout-reentrancy.md)
- [`wxGrid::CreateGrid()`/`SetTable()` synchronously fires `wxEVT_GRID_SELECT_CELL`](wxgrid-creategrid-settable-synchronously-fires-wxevt-grid.md)
- [The `.pjd` (DialogBlocks) files are reference only; never regenerate code from them](the-pjd-dialogblocks-files-are-reference-only.md)
- [`wxEXPAND|wxALIGN_CENTER` on the same sizer item](wxexpand-wxalign-center-on-the-same-sizer.md)
- [`wxFIXED_MINSIZE`](wxfixed-minsize.md)
- [Every drop-down is at least as wide as its widest entry](drop-down-min-width.md)
- [Run-state colours: submitted yellow, created sky blue, pale fills outlined in light themes; checked by tests/look/contrast.py](run-state-colours.md)
- [`std::map`/`unordered_set` iterator invalidation](std-map-unordered-set-iterator-invalidation.md)
- [Uncontrolled format strings](uncontrolled-format-strings.md)
- [`EcceException::what()`](ecceexception-what.md)
- [wx3.2 AUI port dropped the custom "ewxAUI" pane-caption buttons (take focus / pin / options / open) the original app was built against](wx3-2-aui-port-dropped-the-custom.md)
- [`SoWxRenderArea::renderCB` silently drops a redraw](sowxrenderarea-rendercb-silently-drops-a-redraw.md)
- [RESOLVED (#81, fixed `9a3004e`, confirmed live 2026-09-21): the Vibrational Frequencies panel's Animation/Vector radio box did not deliver its click event under wx3.2/GTK3](resolved-81-fixed-9a3004e-confirmed-live-2026.md)
- [ChemDisplay's `glPopAttrib` undoes what Coin's lazy element sent inside it; the first offscreen render drew an ESP surface unlit](chemdisplay-glpopattrib-undoes-lazy-element-sends.md)
- [The unit cell is drawn only while the Periodic Builder panel is open](unit-cell-drawn-only-by-the-periodic-builder.md)
- [The Vibrational Frequencies graph is `SpectrumCanvas` over the wx-free `VibSpectrum` model (#214)](vibrational-spectrum-canvas-214.md)
- [Builder panel layouts (View > Panel layout): every pane stays an AUI pane; the one-column modes hide the inactive tab's panes](builder-panel-layouts-one-column.md)
