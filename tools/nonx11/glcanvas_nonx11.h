// Stand-in for a non-X11 wx port's GL canvas (modelled on wx/osx/glcanvas.h
// of wx 3.2): only the portable wxGLCanvas/wxGLContext API, no GLX.
#ifndef GLCANVAS_NONX11_H
#define GLCANVAS_NONX11_H

#include <GL/gl.h>   // <OpenGL/gl.h> on macOS, via packaging/macos-compat

class WXDLLIMPEXP_GL wxGLContext : public wxGLContextBase
{
public:
    wxGLContext(wxGLCanvas *win, const wxGLContext *other = NULL,
                const wxGLContextAttrs *ctxAttrs = NULL);
    virtual ~wxGLContext();
    virtual bool SetCurrent(const wxGLCanvas& win) const wxOVERRIDE;
};

class WXDLLIMPEXP_GL wxGLCanvas : public wxGLCanvasBase
{
public:
    wxGLCanvas(wxWindow *parent, const wxGLAttributes& dispAttrs,
               wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize, long style = 0,
               const wxString& name = wxGLCanvasName,
               const wxPalette& palette = wxNullPalette);
    explicit wxGLCanvas(wxWindow *parent, wxWindowID id = wxID_ANY,
                        const int *attribList = NULL,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize, long style = 0,
                        const wxString& name = wxGLCanvasName,
                        const wxPalette& palette = wxNullPalette);
    bool Create(wxWindow *parent, wxWindowID id = wxID_ANY,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize, long style = 0,
                const wxString& name = wxGLCanvasName,
                const int *attribList = NULL,
                const wxPalette& palette = wxNullPalette);
    virtual ~wxGLCanvas();
    virtual bool SwapBuffers() wxOVERRIDE;
    static bool IsDisplaySupported(const wxGLAttributes& dispAttrs);
    static bool IsDisplaySupported(const int *attribList);
};

#endif
