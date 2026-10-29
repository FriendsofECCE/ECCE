#ifndef VIEWERTEXT_H
#define VIEWERTEXT_H

#include <cstring>
#include <string>
#include <GL/gl.h>

#include "util/GLPath.H"
#include "viz/FixedPixelText.H"

// Include "viz/freeglut_font_data.H" (no include guard) before this header.

// Text at the current raster position in the 9x15 font. Drawing glyphs as
// geometry avoids the platform-dependent glBitmap path. `text` is Latin-1
// bytes, as the measurement labels are written (\305 is the angstrom sign).
inline void drawViewerText(GLuint fontOffset, const char *text)
{
   if (!text || !*text) return;
#ifndef __APPLE__
   //  Linux and Windows hardware GL keep the display lists; only macOS
   //  (Retina scaling) and software renderers (#565) draw glyphs as geometry.
   if (!GLPath::software()) {
      glPushAttrib(GL_LIST_BIT);
      glListBase(fontOffset);
      glCallLists(strlen(text), GL_UNSIGNED_BYTE,
                  reinterpret_cast<const GLubyte *>(text));
      glPopAttrib();
      return;
   }
#else
   (void)fontOffset;
#endif
   // drawFixedPixelText reads UTF-8.
   std::string utf8;
   for (const char *p = text; *p; ++p) {
      const unsigned char c = static_cast<unsigned char>(*p);
      if (c < 0x80) utf8 += *p;
      else { utf8 += static_cast<char>(0xC0 | (c >> 6));
             utf8 += static_cast<char>(0x80 | (c & 0x3F)); }
   }
   glPushAttrib(GL_ENABLE_BIT);
   glDisable(GL_LIGHTING);
   glDisable(GL_TEXTURE_2D);
   glDisable(GL_CULL_FACE);
   drawFixedPixelText(Fixed9x15_Character_Map, utf8.c_str(), 9.0,
                      GLPath::contentScaleFactor());
   glPopAttrib();
}

#endif
