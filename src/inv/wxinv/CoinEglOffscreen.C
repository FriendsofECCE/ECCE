#if defined(OIV_COIN) && defined(__linux__)

#define GL_GLEXT_PROTOTYPES 1   // before any GL header: the FBO entry points
#include <cstdio>
#include <cstdlib>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include <Inventor/C/glue/gl.h>

#include "inv/SoWx/CoinEglOffscreen.H"

namespace {

// One context per SoOffscreenRenderer size, drawing into an FBO so that no
// window system surface is needed.
struct Ctx {
  // Coin's cc_glglue_context_max_dimensions hands our handle to its GLX
  // pbuffer query, which reads a flag at +0x40 and a Display/FBConfig at
  // +0x48/+0x50 of it; zeroed padding makes it answer "no pbuffer" instead
  // of dereferencing our members (it crashed under a live X display).
  char coinGlxFields[0x58];
  EGLContext ctx;
  GLuint fbo, color, depth;
  // What was current when we took over, to hand back in reinstate().
  Display *glxDpy; GLXContext glxCtx; GLXDrawable glxDraw;
};

EGLDisplay s_dpy = EGL_NO_DISPLAY;
EGLConfig s_cfg;

bool openDisplay()
{
  if (s_dpy != EGL_NO_DISPLAY) return true;
  // Surfaceless Mesa needs no X server; the plain default display is the
  // fallback for EGL stacks without that extension.
  auto getpd = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
  EGLDisplay d = getpd ? getpd(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, NULL)
                       : EGL_NO_DISPLAY;
  EGLint maj, min;
  if (d == EGL_NO_DISPLAY || !eglInitialize(d, &maj, &min)) {
    d = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (d == EGL_NO_DISPLAY || !eglInitialize(d, &maj, &min)) return false;
  }
  if (!eglBindAPI(EGL_OPENGL_API)) return false;
  const EGLint ca[] = {EGL_SURFACE_TYPE, 0, EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT, EGL_NONE};
  EGLint n = 0;
  if (!eglChooseConfig(d, ca, &s_cfg, 1, &n) || n < 1) return false;
  s_dpy = d;   // never eglTerminate'd: Mesa teardown is not worth the risk
  return true;
}

// glvnd refuses eglMakeCurrent (EGL_BAD_ACCESS) while a GLX context, such as
// the wx canvas's, is current on the thread.
void releaseGlx(Display *dpy, GLXContext ctx)
{
  if (ctx && dpy) glXMakeCurrent(dpy, None, NULL);
}

cc_glglue_offscreen_data create(unsigned int w, unsigned int h)
{
  if (!openDisplay()) return NULL;
  const EGLint xa[] = {EGL_CONTEXT_MAJOR_VERSION, 2, EGL_CONTEXT_MINOR_VERSION, 1,
                       EGL_CONTEXT_OPENGL_PROFILE_MASK,
                       EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT, EGL_NONE};
  Ctx *c = new Ctx();   // value-initialised: coinGlxFields stays zero
  c->glxDpy = 0; c->glxCtx = 0; c->glxDraw = 0;
  c->ctx = eglCreateContext(s_dpy, s_cfg, EGL_NO_CONTEXT, xa);
  if (c->ctx == EGL_NO_CONTEXT) { delete c; return NULL; }

  // Creating the FBO needs the context current; put back what was there.
  Display *pd = glXGetCurrentDisplay();
  GLXContext pc = glXGetCurrentContext();
  GLXDrawable pw = glXGetCurrentDrawable();
  EGLContext pe = eglGetCurrentContext();
  releaseGlx(pd, pc);
  if (!eglMakeCurrent(s_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, c->ctx)) {
    eglDestroyContext(s_dpy, c->ctx); delete c; return NULL;
  }
  glGenFramebuffers(1, &c->fbo);
  glGenRenderbuffers(1, &c->color);
  glGenRenderbuffers(1, &c->depth);
  glBindRenderbuffer(GL_RENDERBUFFER, c->color);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, w, h);
  glBindRenderbuffer(GL_RENDERBUFFER, c->depth);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
  glBindFramebuffer(GL_FRAMEBUFFER, c->fbo);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, c->color);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, c->depth);
  bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  eglMakeCurrent(s_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, pe);
  if (pc && pd) glXMakeCurrent(pd, pw, pc);
  if (!ok) { eglDestroyContext(s_dpy, c->ctx); delete c; return NULL; }
  return c;
}

SbBool makeCurrent(cc_glglue_offscreen_data d)
{
  Ctx *c = (Ctx *)d;
  c->glxDpy = glXGetCurrentDisplay();
  c->glxCtx = glXGetCurrentContext();
  c->glxDraw = glXGetCurrentDrawable();
  releaseGlx(c->glxDpy, c->glxCtx);
  if (!eglMakeCurrent(s_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, c->ctx)) return FALSE;
  glBindFramebuffer(GL_FRAMEBUFFER, c->fbo);
  return TRUE;
}

// A wx canvas's GLX context was current before us; giving it back keeps the
// next window redraw from drawing with no context.
void reinstate(cc_glglue_offscreen_data d)
{
  Ctx *c = (Ctx *)d;
  eglMakeCurrent(s_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  if (c->glxCtx && c->glxDpy) glXMakeCurrent(c->glxDpy, c->glxDraw, c->glxCtx);
}

void destruct(cc_glglue_offscreen_data d)
{
  Ctx *c = (Ctx *)d;
  Display *pd = glXGetCurrentDisplay();
  GLXContext pc = glXGetCurrentContext();
  GLXDrawable pw = glXGetCurrentDrawable();
  releaseGlx(pd, pc);
  eglMakeCurrent(s_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, c->ctx);
  glDeleteFramebuffers(1, &c->fbo);
  glDeleteRenderbuffers(1, &c->color);
  glDeleteRenderbuffers(1, &c->depth);
  eglMakeCurrent(s_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  eglDestroyContext(s_dpy, c->ctx);
  if (pc && pd) glXMakeCurrent(pd, pw, pc);
  delete c;
}

cc_glglue_offscreen_cb_functions s_funcs = {create, makeCurrent, reinstate, destruct};

}  // namespace

bool CoinEglOffscreen::install()
{
  // Coin asserts that a GLX context is current whenever it first queries a
  // new context's GL capabilities, and cannot see an EGL one; the check is
  // only a debugging aid, so switch it off before Coin reads it.
  setenv("COIN_GL_NO_CURRENT_CONTEXT_CHECK", "1", 0);
  if (!openDisplay()) {
    fprintf(stderr, "CoinEglOffscreen: no usable EGL, thumbnails will need GLX\n");
    return false;
  }
  cc_glglue_context_set_offscreen_cb_functions(&s_funcs);
  return true;
}

#endif
