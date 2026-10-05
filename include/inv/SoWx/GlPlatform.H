#ifndef GLPLATFORM_H
#define GLPLATFORM_H

// The window-system side of the viewer's OpenGL (#232), so that nothing else
// in the viewer includes X11, GLX or EGL headers.  On X11 the context
// queries are GLX and the offscreen contexts EGL; on other wx ports (macOS,
// Windows) wxGLCanvas/wxGLContext draw on screen and Coin's
// SoOffscreenRenderer brings its own CGL/WGL context, so both are no-ops.
//
// ECCE_GL_X11 can be preset to 0 to compile the non-X11 branch on Linux.
#ifndef ECCE_GL_X11
#  if defined(__linux__) || defined(__FreeBSD__)
#    define ECCE_GL_X11 1
#  else
#    define ECCE_GL_X11 0
#  endif
#endif

namespace GlPlatform {

// The window-system context current on this thread (all zero if none).
struct Current {
  void *display;
  void *context;
  unsigned long drawable;
};

Current current();

// Leaves the thread with no window-system context: glvnd refuses
// eglMakeCurrent while a GLX context, such as a wx canvas's, is current.
void release(const Current &c);

// Makes c current again (nothing if c is empty).
void restore(const Current &c);

// Offscreen contexts for SoOffscreenRenderer (Coin builds).  Call once,
// before the first SoOffscreenRenderer; SoWx::init does.  Returns false if
// the platform's own path is left in place because ours is unusable.
bool installOffscreen();

}

#endif
