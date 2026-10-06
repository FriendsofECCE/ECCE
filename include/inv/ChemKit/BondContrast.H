#ifndef BONDCONTRAST_H
#define BONDCONTRAST_H

#include <GL/gl.h>

// Bonds take their atoms' colours, so a white or pale element (H, F, ...)
// vanishes against a light background (#227).  On a light background any
// bond colour brighter than the cap is scaled down to it, keeping its hue;
// dark backgrounds are untouched.  The background is read from the GL clear
// colour, which is what the scene manager just cleared with.
inline void bondContrastColor3fv(const GLfloat *c)
{
  GLfloat bg[4];
  glGetFloatv(GL_COLOR_CLEAR_VALUE, bg);
  const float lb = 0.299f*bg[0] + 0.587f*bg[1] + 0.114f*bg[2];
  const float lc = 0.299f*c[0] + 0.587f*c[1] + 0.114f*c[2];
  const float cap = lb - 0.65f > 0.05f ? lb - 0.65f : 0.05f;
  if (lb > 0.6f && lc > cap && lc > 0.0f) {
    const float s = cap / lc;
    glColor3f(c[0]*s, c[1]*s, c[2]*s);
  } else {
    glColor3fv(c);
  }
}

#endif
