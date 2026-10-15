#ifndef COINCGLOFFSCREEN_H
#define COINCGLOFFSCREEN_H

// Coin3D on macOS only (#344): offscreen GL contexts for SoOffscreenRenderer
// as a windowless CGL context drawing into an FBO.  Coin's own CGL path asks
// for a pbuffer and then a kCGLPFAOffScreen pixel format, and current macOS
// refuses both, so every thumbnail failed.  Called through
// GlPlatform::installOffscreen() (SoWx::init).  Returns false, leaving
// Coin's own path in place, if no accelerated pixel format is found.
namespace CoinCglOffscreen {
  bool install();
}

#endif
