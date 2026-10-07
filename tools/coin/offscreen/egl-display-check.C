//  Coin build on X11 only (#237): a Coin built for EGL alone (EPEL's and
//  Fedora's Coin4) sets up the glue of every GL context on its global
//  eglglue_display and, while that is unset, opens it with
//  eglGetDisplay(EGL_DEFAULT_DISPLAY): an EGL X11 display, which on an X
//  server offering DRI3 without a device ended in a heap abort inside Mesa.
//  CoinEglOffscreen::install() must hand Coin its surfaceless display.
//
//  This program defines eglglue_display itself, standing in for such a Coin,
//  points DISPLAY at an X server that does not exist and checks that the
//  display Coin would use initialises without one.  Exit 0 and "OK", 1 on
//  failure, 77 (skip) where EGL is unusable altogether.
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <EGL/egl.h>

#include "inv/SoWx/CoinEglOffscreen.H"

EGLDisplay eglglue_display = EGL_NO_DISPLAY;

int main()
{
  setenv("DISPLAY", ":237", 1);
  unsetenv("WAYLAND_DISPLAY");
  if (!CoinEglOffscreen::install()) {
    printf("SKIP: no usable EGL\n");
    return 77;
  }
  if (eglglue_display == EGL_NO_DISPLAY) {
    printf("FAIL: Coin's glue display is unset; an EGL-only Coin would open "
           "an EGL X11 display of its own\n");
    return 1;
  }
  EGLint maj = 0, min = 0;
  if (!eglInitialize(eglglue_display, &maj, &min)) {
    printf("FAIL: the display handed to Coin does not initialise (0x%x)\n",
           eglGetError());
    return 1;
  }
  printf("OK: Coin's glue display is the offscreen one (EGL %d.%d, %s)\n",
         maj, min, eglQueryString(eglglue_display, EGL_VENDOR));
  fflush(stdout);
  _exit(0);   // skip Mesa teardown, as offscreen-check does
}
