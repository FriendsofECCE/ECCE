#include "inv/SoWx/GlPlatform.H"

#if ECCE_GL_X11
#include <GL/glx.h>
#endif
#if defined(OIV_COIN) && ECCE_GL_X11
#include "inv/SoWx/CoinEglOffscreen.H"
#endif

namespace GlPlatform {

#if ECCE_GL_X11

Current current()
{
  Current c;
  c.display = glXGetCurrentDisplay();
  c.context = glXGetCurrentContext();
  c.drawable = glXGetCurrentDrawable();
  return c;
}

void release(const Current &c)
{
  if (c.context && c.display) glXMakeCurrent((Display *)c.display, None, NULL);
}

void restore(const Current &c)
{
  if (c.context && c.display)
    glXMakeCurrent((Display *)c.display, (GLXDrawable)c.drawable, (GLXContext)c.context);
}

#else

// wxOSX and wxMSW make their own contexts current; nothing here needs to
// step around them.
Current current() { return Current{0, 0, 0}; }
void release(const Current &) {}
void restore(const Current &) {}

#endif

bool installOffscreen()
{
#if defined(OIV_COIN) && ECCE_GL_X11
  return CoinEglOffscreen::install();
#else
  // Coin's SoOffscreenRenderer uses CGL on macOS and WGL on Windows.
  return true;
#endif
}

}
