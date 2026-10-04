#ifndef COINEGLOFFSCREEN_H
#define COINEGLOFFSCREEN_H

// Coin3D only (#166): offscreen GL contexts for SoOffscreenRenderer from Mesa
// EGL instead of GLX, so thumbnails render under Xvfb and with no X server.
// Call once, before the first SoOffscreenRenderer is created (SoWx::init does).
// Returns false, leaving Coin's own GLX path in place, if EGL is unusable.
namespace CoinEglOffscreen {
  bool install();
}

#endif
