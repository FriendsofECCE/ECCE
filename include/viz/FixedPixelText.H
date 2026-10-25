#ifndef FIXEDPIXELTEXT_H
#define FIXEDPIXELTEXT_H

#include <GL/gl.h>

// Draw a bitmap string with a fixed screen-space advance between glyphs.
// Drawing glyphs under a window-coordinate projection prevents camera zoom
// from changing the raster advance used by glBitmap.
inline void drawFixedPixelBitmapText(GLuint fontOffset, const char *text,
                                     GLdouble glyphAdvance)
{
   if (!text || !*text)
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
   glPushAttrib(GL_PIXEL_MODE_BIT);
   glPixelZoom(1.0f, 1.0f);

   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadIdentity();
   glOrtho(viewport[0], viewport[0] + viewport[2],
           viewport[1], viewport[1] + viewport[3], -1.0, 1.0);

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();

   const GLdouble windowZ = 1.0 - 2.0 * raster[2];
   for (const unsigned char *character =
           reinterpret_cast<const unsigned char *>(text);
        *character != '\0'; ++character) {
      const GLdouble windowX = raster[0] + glyphAdvance *
         (character - reinterpret_cast<const unsigned char *>(text));
      glRasterPos3d(windowX, raster[1], windowZ);
      glGetBooleanv(GL_CURRENT_RASTER_POSITION_VALID, &valid);
      if (!valid)
         break;
      glCallList(fontOffset + *character);
   }

   glPopMatrix();
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glMatrixMode(matrixMode);
   glPopAttrib();
}

#endif
