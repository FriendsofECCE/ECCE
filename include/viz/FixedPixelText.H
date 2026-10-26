#ifndef FIXEDPIXELTEXT_H
#define FIXEDPIXELTEXT_H

#include <cstddef>
#include <GL/gl.h>

// Draw the fixed 9x15 font with screen-space pixels.  Drawing each set bit
// as geometry avoids OpenGL's glBitmap path, which can allocate temporary
// textures in software renderers while drawing display lists.
// The font holds Latin-1; text is UTF-8.  Returns the glyph index of the
// character at text[i] and moves i past it: Latin-1 as is, Greek mu as the
// micro sign (same shape), anything else as '?'.
inline unsigned char fixedPixelGlyph(const char *text, std::size_t &i)
{
   const unsigned char c = static_cast<unsigned char>(text[i++]);
   if (c < 0x80) return c;
   unsigned int cp = 0;
   int more = 0;
   if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; more = 1; }
   else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; more = 2; }
   else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; more = 3; }
   else return '?';
   for (; more > 0; --more) {
      const unsigned char d = static_cast<unsigned char>(text[i]);
      if ((d & 0xC0) != 0x80) return '?';
      cp = (cp << 6) | (d & 0x3F);
      ++i;
   }
   if (cp == 0x3BC) return 0xB5;
   return cp <= 0xFF ? static_cast<unsigned char>(cp) : '?';
}

// Characters, not bytes, in a UTF-8 string: its width in glyphs.
inline std::size_t fixedPixelLength(const char *text)
{
   std::size_t n = 0;
   for (std::size_t i = 0; text && text[i] != '\0'; ++n) fixedPixelGlyph(text, i);
   return n;
}

inline void drawFixedPixelText(const GLubyte *const *glyphs,
                               const char *text,
                               GLdouble glyphAdvance)
{
   if (!glyphs || !text || !*text)
      return;

   GLboolean valid = GL_FALSE;
   glGetBooleanv(GL_CURRENT_RASTER_POSITION_VALID, &valid);
   if (!valid)
      return;

   GLfloat raster[4];
   GLint viewport[4];
   glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
   glGetIntegerv(GL_VIEWPORT, viewport);

   if (viewport[2] <= 0 || viewport[3] <= 0)
      return;

   GLint matrixMode;
   glGetIntegerv(GL_MATRIX_MODE, &matrixMode);
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadIdentity();
   glOrtho(viewport[0], viewport[0] + viewport[2],
           viewport[1], viewport[1] + viewport[3], -1.0, 1.0);

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();

   const GLdouble windowZ = 1.0 - 2.0 * raster[2];
   glBegin(GL_QUADS);
   std::size_t index = 0;
   for (std::size_t i = 0; text[i] != '\0'; ++index) {
      const unsigned char code = fixedPixelGlyph(text, i);
      const GLubyte *glyph = glyphs[code];
      if (!glyph) continue;

      // The first byte is the glyph width; the bitmap is stored as two
      // most-significant-bit-first bytes for each of its 16 rows.  The
      // visible font is 9x15, as in the former glBitmap display lists.
      const GLubyte *bitmap = glyph + 1;
      const GLdouble originX = raster[0] + glyphAdvance * index;
      for (int row = 0; row < 15; ++row) {
         const unsigned int bits =
            (static_cast<unsigned int>(bitmap[row * 2]) << 8) |
             static_cast<unsigned int>(bitmap[row * 2 + 1]);
         for (int column = 0; column < 9; ++column) {
            if ((bits & (0x8000u >> column)) == 0) continue;
            const GLdouble x = originX + column;
            const GLdouble y = raster[1] + row;
            glVertex3d(x,     y,     windowZ);
            glVertex3d(x + 1, y,     windowZ);
            glVertex3d(x + 1, y + 1, windowZ);
            glVertex3d(x,     y + 1, windowZ);
         }
      }
   }
   glEnd();

   glPopMatrix();
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glMatrixMode(matrixMode);
}

#endif
