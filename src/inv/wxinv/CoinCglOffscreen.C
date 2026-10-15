#if defined(OIV_COIN) && defined(__APPLE__)

#define GL_SILENCE_DEPRECATION 1
#include <cstdio>

#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>

#include <Inventor/C/glue/gl.h>

#include "inv/SoWx/CoinCglOffscreen.H"

namespace {

// A CGL context with no drawable, rendering into its own FBO; the legacy
// (2.1) profile, which Coin's fixed-function drawing needs.
struct Ctx {
  CGLContextObj ctx;
  GLuint fbo, color, depth;
  CGLContextObj prev;   // what was current when we took over
};

CGLPixelFormatObj s_pf = NULL;

bool choosePixelFormat()
{
  if (s_pf) return true;
  const CGLPixelFormatAttribute accel[] = {
    kCGLPFAAccelerated, kCGLPFAAllowOfflineRenderers,
    kCGLPFAColorSize, (CGLPixelFormatAttribute)24,
    kCGLPFAAlphaSize, (CGLPixelFormatAttribute)8,
    kCGLPFADepthSize, (CGLPixelFormatAttribute)24,
    (CGLPixelFormatAttribute)0};
  // Without kCGLPFAAccelerated as a fallback, for a Mac (or VM) whose only
  // renderer is Apple's software one.
  const CGLPixelFormatAttribute any[] = {
    kCGLPFAAllowOfflineRenderers,
    kCGLPFAColorSize, (CGLPixelFormatAttribute)24,
    kCGLPFAAlphaSize, (CGLPixelFormatAttribute)8,
    kCGLPFADepthSize, (CGLPixelFormatAttribute)24,
    (CGLPixelFormatAttribute)0};
  GLint n = 0;
  if (CGLChoosePixelFormat(accel, &s_pf, &n) != kCGLNoError || !s_pf) {
    s_pf = NULL;
    if (CGLChoosePixelFormat(any, &s_pf, &n) != kCGLNoError || !s_pf) {
      s_pf = NULL;
      return false;
    }
  }
  return true;
}

cc_glglue_offscreen_data create(unsigned int w, unsigned int h)
{
  if (!choosePixelFormat()) return NULL;
  Ctx *c = new Ctx();
  if (CGLCreateContext(s_pf, NULL, &c->ctx) != kCGLNoError) {
    delete c;
    return NULL;
  }
  CGLContextObj prev = CGLGetCurrentContext();
  if (CGLSetCurrentContext(c->ctx) != kCGLNoError) {
    CGLDestroyContext(c->ctx); delete c; return NULL;
  }
  glGenFramebuffersEXT(1, &c->fbo);
  glGenRenderbuffersEXT(1, &c->color);
  glGenRenderbuffersEXT(1, &c->depth);
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, c->color);
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA8, w, h);
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, c->depth);
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_EXT, w, h);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, c->fbo);
  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                               GL_RENDERBUFFER_EXT, c->color);
  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                               GL_RENDERBUFFER_EXT, c->depth);
  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT,
                               GL_RENDERBUFFER_EXT, c->depth);
  bool ok = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT;
  CGLSetCurrentContext(prev);
  if (!ok) { CGLDestroyContext(c->ctx); delete c; return NULL; }
  return c;
}

SbBool makeCurrent(cc_glglue_offscreen_data d)
{
  Ctx *c = (Ctx *)d;
  c->prev = CGLGetCurrentContext();
  if (CGLSetCurrentContext(c->ctx) != kCGLNoError) return FALSE;
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, c->fbo);
  // A context with no drawable has no default buffers to read or draw.
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
  return TRUE;
}

// The wx canvas's context was current before us; the next window redraw
// must find it again.
void reinstate(cc_glglue_offscreen_data d)
{
  Ctx *c = (Ctx *)d;
  CGLSetCurrentContext(c->prev);
}

void destruct(cc_glglue_offscreen_data d)
{
  Ctx *c = (Ctx *)d;
  CGLContextObj prev = CGLGetCurrentContext();
  if (CGLSetCurrentContext(c->ctx) == kCGLNoError) {
    glDeleteFramebuffersEXT(1, &c->fbo);
    glDeleteRenderbuffersEXT(1, &c->color);
    glDeleteRenderbuffersEXT(1, &c->depth);
  }
  CGLSetCurrentContext(prev == c->ctx ? NULL : prev);
  CGLDestroyContext(c->ctx);
  delete c;
}

cc_glglue_offscreen_cb_functions s_funcs = {create, makeCurrent, reinstate, destruct};

}  // namespace

bool CoinCglOffscreen::install()
{
  if (!choosePixelFormat()) {
    fprintf(stderr, "CoinCglOffscreen: no CGL pixel format, thumbnails left to Coin\n");
    return false;
  }
  cc_glglue_context_set_offscreen_cb_functions(&s_funcs);
  return true;
}

#endif
