---
type: index
title: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
area: wx-viewer
---
# wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls

Read this before changing any dialog, panel, sizer, grid, ewx control or the Open Inventor viewer. Split out of CLAUDE.md 2026-10-04.

## Entries

### wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls

- [Every plot is an `ewxPlotCtrl` styled by `PlotStyle.H`, the palette the vibrational spectrum canvas uses; `tests/plots/capture.py` pictures them before and after a change](plots-share-the-spectrum-style.md)
- [`ComputeMoCmd` caches a grid before computing it; a failed Compute must remove it (#239)](mo-grid-cache-keeps-a-failed-compute.md)
- [The redraw sensor must not be drained inside `schedule()`: `SoWxEventHandler::setUpCallbacks()` only wakes idle](open-inventors-redraw-sensor-is-a-one.md)
- [`ECCE_USE_COIN` (default ON) builds the viewer against Coin3D; `=OFF` keeps the vendored core for one release](ecce-use-coin-builds-the-viewer-against-coin3d.md)
- [Coin SORTED_LAYERS_BLEND is the default lobe transparency (6 passes, alpha canvas); slow scenes fall back to SCREEN_DOOR](coin-sorted-layers-blend-is-the-default-lobe-transparency.md)
- [`GlPlatform` is the viewer's only GLX/EGL code; `tools/nonx11/check.py` compiles the viewer as a non-X11 port (#232)](glplatform-is-the-viewers-only-glx-egl-code.md)
- [EPEL's and Fedora's Coin4 are EGL-only: Coin opens an EGL X11 display beside the GLX canvas unless handed ours (#237)](epel-fedora-coin4-is-egl-only.md)
- [`Builder::execute()` paints after every command, so a command sequence shows each intermediate state (#226)](builder-execute-paints-after-every-command.md)
- [Normal-mode arrows start on the drawn sphere, have a fixed thickness, and the longest tip is 1 A out (#228)](normal-mode-arrows-start-on-the-atom-sphere.md)
- [`tools/coin/compare.sh` renders a fixed scene set with both viewer builds (#166 stage 2); harness pitfalls](tools-coin-compare-sh-renders-both-viewer-builds.md)
- [Atom labels drew nothing in 9.x: the sh wrappers dropped `FL_FONT_PATH`; flclient now defaults to `$ECCE_HOME/data/client/fonts`](atom-labels-need-the-bundled-font-fl-font-path.md)
- [Builder Reset View (toolbar, Render menu, Home) is camera only; the old home button restored a pre-molecule camera](builder-reset-view-is-camera-only-home-key.md)
- [`addCovalentBonds` never bonds nubs; lattice Generate/Replicate/Fold left every nub parentless (#243)](nubs-need-a-parent-after-lattice-commands.md)
- [wx3.2/GTK3 layout reentrancy](wx3-2-gtk3-layout-reentrancy.md)
- [`wxGrid::CreateGrid()`/`SetTable()` synchronously fires `wxEVT_GRID_SELECT_CELL`](wxgrid-creategrid-settable-synchronously-fires-wxevt-grid.md)
- [`wxGrid::MakeCellVisible()` before the pane is laid out does nothing; the MOs table opened at the top, not the HOMO](wxgrid-makecellvisible-before-the-pane-is-laid-out.md)
- [The `.pjd` (DialogBlocks) files are reference only; never regenerate code from them](the-pjd-dialogblocks-files-are-reference-only.md)
- [`wxEXPAND|wxALIGN_CENTER` on the same sizer item](wxexpand-wxalign-center-on-the-same-sizer.md)
- [`wxFIXED_MINSIZE`](wxfixed-minsize.md)
- [Every drop-down is at least as wide as its widest entry](drop-down-min-width.md)
- [Run-state colours: submitted yellow, created sky blue, pale fills outlined in light themes; checked by tests/look/contrast.py](run-state-colours.md)
- [GTK :backdrop fades every label to ~2.8:1; `ewxApp::applyBackdropStyle()` undoes it; headless runs need `ECCE_TEST_BACKDROP` (#210)](gtk-backdrop-fades-labels-and-has-no-headless-trigger.md)
- [`std::map`/`unordered_set` iterator invalidation](std-map-unordered-set-iterator-invalidation.md)
- [Uncontrolled format strings](uncontrolled-format-strings.md)
- [`EcceException::what()` returns storage that lives as long as the exception](ecceexception-what.md)
- [The wx3.2 AUI port lost the ewxAUI caption buttons; `EVT_CHILD_FOCUS` on Builder now triggers `receiveFocus()`](wx3-2-aui-port-dropped-the-custom.md)
- [`SoWxRenderArea::renderCB` during a paint defers the redraw through `p_redrawPending`](sowxrenderarea-rendercb-silently-drops-a-redraw.md)
- [A static `EVT_RADIOBOX` entry never reaches an `ewxRadioBox`'s panel; Bind on the widget (#81)](resolved-81-fixed-9a3004e-confirmed-live-2026.md)
- [ChemDisplay's `glPopAttrib` undoes what Coin's lazy element sent inside it; the first offscreen render drew an ESP surface unlit](chemdisplay-glpopattrib-undoes-lazy-element-sends.md)
- [Ball and Stick spent most of its frame in the bond-cap test, O(atoms x bonds) (#224)](bond-cap-test-was-quadratic.md)
- [The unit cell is drawn only while the Periodic Builder panel is open](unit-cell-drawn-only-by-the-periodic-builder.md)
- [The Vibrational Frequencies graph is `SpectrumCanvas` over the wx-free `VibSpectrum` model (#214)](vibrational-spectrum-canvas-214.md)
- [Builder panel layouts (View > Panel layout): every pane stays an AUI pane; the one-column modes hide the inactive tab's panes](builder-panel-layouts-one-column.md)
- [A structure opened from a file is not a calculation: `AbstractPropCalculation::getProperty()` throws](structure-file-calculations-throw-on-getproperty.md)
- [Every `ewxFrame`/`ewxDialog` is capped to its display and scrolls what does not fit (`fitToScreen()`); codereg dialogs scroll in `EccePanel` (#189)](windows-fit-small-screens-via-fittodisplay.md)

- [A failed live property update must not free the cached property; a panel's timer must die with the panel (#217)](property-cache-and-panel-timers-outlive-their-objects.md)
