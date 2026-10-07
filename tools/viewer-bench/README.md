# viewer-bench

Frame times of the Builder's molecule viewer, with Open Inventor render
caching on (AUTO) and off, to decide whether rewriting moiv's
immediate-mode GL drawing is worth it. Run it once on a machine with a real
GPU, in an X11 session.

## Commands (paste as is, from a checkout of this branch)

```
git fetch origin wip/viewer-bench
git checkout wip/viewer-bench
tools/viewer-bench/run.sh
```

`run.sh` builds `viewer-bench` in `./build-cmake` (the first build compiles
the viewer libraries and takes several minutes; `JOBS=4` limits parallelism),
opens one 800x800 window on `$DISPLAY` for about a minute and a half, prints
the table, and ends with the path of the results file:

```
build-cmake/viewer-bench-<host>-<date>.txt
```

Send that file. Leave the window uncovered while it runs. Optional:
`BENCH_FRAMES=360 BENCH_SECONDS=8 tools/viewer-bench/run.sh` (frames per run,
time cap per run; defaults 180 and 4 s, 10 warm-up frames not counted).
`BENCH_STYLES="CPK,Ball And Stick,Stick,Wireframe,Ball And Wireframe"` times
only the water box, in those styles, with caching AUTO.

## What is timed

The scene is built the way the Builder builds it: `SGContainerManager` ->
`SGContainer` -> `SGFragment` loaded from XYZ, style set through
`CSStyleCmd`, shown in an `SGViewer`. One timed frame is: rotate the camera
about the vertical axis (360 degrees spread over the frames), a synchronous
paint of the real GL canvas, `glFinish`. Caching is switched by setting
`renderCaching` to AUTO or OFF on every `SoSeparator` in the scene.

| system | atoms | source |
|---|---|---|
| water | 3 | built in |
| benzene | 12 | built in (D6h geometry) |
| Cr(CO)6 | 13 | built in (Oh, Cr-C 1.91, C-O 1.14 A) |
| water box | 5184 | `data/client/solvents/water216.xyz` replicated 2x2x2 |
| benzene+MO | 12 | benzene plus an isosurface of a synthetic pi field |

Styles: Ball And Stick and CPK (space-filling) for every system; the MO
case is Ball And Stick only. The repo contains no cube file or finished
calculation, so the isosurface is built from a made-up pi-type field on a
48^3 grid, pushed through the Builder's own `IsoSurfaceCmd` (`ChemIso`).
It exercises the same drawing code but is not a real orbital.

## Reading the result

The header lists `GL_VENDOR/GL_RENDERER/GL_VERSION`; check it is the GPU and
not llvmpipe. `run.sh` turns vsync off (`vblank_mode=0`,
`__GL_SYNC_TO_VBLANK=0`, `glXSwapIntervalEXT(0)`); if every row still reads
about 16.7 ms the compositor or driver is pinning the rate and the numbers
say nothing about drawing cost. On software GL (Xvfb) the numbers are
meaningless and the runs stop at the time cap.
