---
type: pitfall
title: "EPEL's and Fedora's Coin4 are built for EGL only: Coin opens an EGL X11 display beside the wx GLX canvas unless it is given ours"
area: wx-viewer
paths: [src/inv/wxinv/CoinEglOffscreen.C, tools/coin/offscreen/egl-display-check.C, CMakeLists.txt]
issues: [237]
---
Debian's and Ubuntu's Coin (4.0.2/4.0.3) are GLX builds. EPEL 9's Coin4
4.0.10 and Fedora's (4.0.7 on F42, 4.0.10 on F44) import `egl*` and no
`glX*` at all (`nm -D /lib64/libCoin.so.80`). Such a Coin runs its glue for
every context, the wx canvas's GLX context included, through `eglglue_init`,
which opens `eglglue_display` with `eglGetDisplay(EGL_DEFAULT_DISPLAY)`: on
X11 a Mesa EGL X11 display, initialised at the canvas's first paint
(`SoGLRenderActionP::isDirectRendering`). That is the only place the viewer
touches EGL on X11; `CoinEglOffscreen` itself uses the surfaceless platform.

On an X server without a usable DRI3 device (Xvfb, VNC, FastX) that init
prints `DRI3 error: Could not get DRI3 device` and retries with zink and
then swrast (Mesa 25.2's second line is "Ensure your X server supports
DRI3", 25.1's "Activate DRI3 at Xorg"). On FastX the viewer then aborted
with `malloc(): unaligned fastbin chunk detected` (#237). The abort did not
reproduce on Xvfb, with or without a shim faking FastX's DRI3, and valgrind
showed no invalid writes there, so the corruption is presumably in Mesa's
fallback on that server; Mesa 25.2 no longer reads `LIBGL_DRI3_DISABLE`.

`CoinEglOffscreen::install()` now writes its own surfaceless display into
Coin's exported `eglglue_display` (found with `dlsym`, so a GLX Coin, which
has no such symbol, is untouched). Coin uses the display only for
`eglInitialize`/`eglQueryString`; it is never `eglTerminate`d because ECCE
never calls `SoDB::finish()`. ctest `coin_egl_display` checks the handover
with `DISPLAY` pointing at no server. A Coin that renames the symbol would
silently bring the X11 display back.

Reproducing: the released el9 RPM in a `rockylinux:9` container under Xvfb
prints the warning; `break eglGetDisplay` in gdb shows Coin's
`eglglue_get_display` as the caller. An `LD_PRELOAD` shim answering the
DRI3 extension query as present and `DRI3Open` as failed stands in for
FastX's half of it. With the handover, a Fedora 42 build prints nothing.
