/* LD_PRELOAD tracer for the fixed-function GL state the viewer sets (#166).
 *   gcc -shared -fPIC -O1 -o gltrace.so tools/coin/gltrace.c -ldl
 *   GLTRACE_OUT=trace.txt LD_PRELOAD=./gltrace.so build-X/viewer-scenes ...
 * Logs enables, lights, materials, stipple, blend, attrib push/pop and the
 * first 4000 colours/normals per frame; frames are separated by glClear.
 * Run it on both builds and diff the last frame of each.  Only calls that go
 * through the PLT are seen (direct glColor3fv etc. are not wrapped). */
#include <stdlib.h>
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <GL/gl.h>
static FILE *F; static int n;
static FILE *f(){ if(!F){F=fopen(getenv("GLTRACE_OUT"),"w");} return F; }
#define NEXT(name) static __typeof__(&name) real; if(!real) real=dlsym(RTLD_NEXT,#name)
void glClear(GLbitfield m){ NEXT(glClear); n=0; fprintf(f(),"=== glClear %x\n",m); real(m);}
void glEnable(GLenum c){ NEXT(glEnable); fprintf(f(),"glEnable %x\n",c); real(c);}
void glDisable(GLenum c){ NEXT(glDisable); fprintf(f(),"glDisable %x\n",c); real(c);}
void glShadeModel(GLenum c){ NEXT(glShadeModel); fprintf(f(),"glShadeModel %x\n",c); real(c);}
void glFrontFace(GLenum c){ NEXT(glFrontFace); fprintf(f(),"glFrontFace %x\n",c); real(c);}
void glCullFace(GLenum c){ NEXT(glCullFace); fprintf(f(),"glCullFace %x\n",c); real(c);}
void glDepthMask(GLboolean c){ NEXT(glDepthMask); fprintf(f(),"glDepthMask %d\n",c); real(c);}
void glBlendFunc(GLenum a,GLenum b){ NEXT(glBlendFunc); fprintf(f(),"glBlendFunc %x %x\n",a,b); real(a,b);}
void glAlphaFunc(GLenum a,GLclampf b){ NEXT(glAlphaFunc); fprintf(f(),"glAlphaFunc %x %g\n",a,b); real(a,b);}
void glColorMaterial(GLenum a,GLenum b){ NEXT(glColorMaterial); fprintf(f(),"glColorMaterial %x %x\n",a,b); real(a,b);}
void glLightModeli(GLenum a,GLint b){ NEXT(glLightModeli); fprintf(f(),"glLightModeli %x %d\n",a,b); real(a,b);}
void glLightModelf(GLenum a,GLfloat b){ NEXT(glLightModelf); fprintf(f(),"glLightModelf %x %g\n",a,b); real(a,b);}
void glLightModelfv(GLenum a,const GLfloat*p){ NEXT(glLightModelfv); fprintf(f(),"glLightModelfv %x %g %g %g %g\n",a,p[0],p[1],p[2],p[3]); real(a,p);}
void glLightfv(GLenum l,GLenum p,const GLfloat*v){ NEXT(glLightfv); fprintf(f(),"glLightfv L%d %x %g %g %g %g\n",l-GL_LIGHT0,p,v[0],v[1],v[2],v[3]); real(l,p,v);}
void glLightf(GLenum l,GLenum p,GLfloat v){ NEXT(glLightf); fprintf(f(),"glLightf L%d %x %g\n",l-GL_LIGHT0,p,v); real(l,p,v);}
void glMaterialfv(GLenum fc,GLenum p,const GLfloat*v){ NEXT(glMaterialfv); fprintf(f(),"glMaterialfv %x %x %g %g %g %g\n",fc,p,v[0],v[1],v[2],p==GL_SHININESS?0:v[3]); real(fc,p,v);}
void glMaterialf(GLenum fc,GLenum p,GLfloat v){ NEXT(glMaterialf); fprintf(f(),"glMaterialf %x %x %g\n",fc,p,v); real(fc,p,v);}
void glColor4ub(GLubyte r,GLubyte g,GLubyte b,GLubyte a){ NEXT(glColor4ub); if(n++<4000)fprintf(f(),"glColor4ub %d %d %d %d\n",r,g,b,a); real(r,g,b,a);}
void glColor4f(GLfloat r,GLfloat g,GLfloat b,GLfloat a){ NEXT(glColor4f); if(n++<4000)fprintf(f(),"glColor4f %g %g %g %g\n",r,g,b,a); real(r,g,b,a);}
void glColor3f(GLfloat r,GLfloat g,GLfloat b){ NEXT(glColor3f); if(n++<4000)fprintf(f(),"glColor3f %g %g %g\n",r,g,b); real(r,g,b);}
void glColor4ubv(const GLubyte*p){ NEXT(glColor4ubv); if(n++<4000)fprintf(f(),"glColor4ubv %d %d %d %d\n",p[0],p[1],p[2],p[3]); real(p);}
void glColorPointer(GLint s,GLenum t,GLsizei st,const void*p){ NEXT(glColorPointer); fprintf(f(),"glColorPointer %d %x %d\n",s,t,st); real(s,t,st,p);}
void glNormalPointer(GLenum t,GLsizei st,const void*p){ NEXT(glNormalPointer); fprintf(f(),"glNormalPointer %x %d\n",t,st); real(t,st,p);}
void glEnableClientState(GLenum c){ NEXT(glEnableClientState); fprintf(f(),"glEnableClientState %x\n",c); real(c);}
void glDisableClientState(GLenum c){ NEXT(glDisableClientState); fprintf(f(),"glDisableClientState %x\n",c); real(c);}
void glBegin(GLenum m){ NEXT(glBegin); fprintf(f(),"glBegin %x\n",m); real(m);}
void glDrawElements(GLenum m,GLsizei c,GLenum t,const void*p){ NEXT(glDrawElements); fprintf(f(),"glDrawElements %x %d\n",m,c); real(m,c,t,p);}
void glDrawArrays(GLenum m,GLint a,GLsizei c){ NEXT(glDrawArrays); fprintf(f(),"glDrawArrays %x %d\n",m,c); real(m,a,c);}
void glMultiDrawElementsEXT(GLenum m,const GLsizei*c,GLenum t,const void*const*p,GLsizei n){ void(*real)(GLenum,const GLsizei*,GLenum,const void*const*,GLsizei)=dlsym(RTLD_NEXT,"glMultiDrawElementsEXT"); fprintf(f(),"glMultiDrawElements %x %d\n",m,n); real(m,c,t,p,n);}
void glNormal3f(GLfloat x,GLfloat y,GLfloat z){ NEXT(glNormal3f); if(n++<4000)fprintf(f(),"glNormal3f %g %g %g\n",x,y,z); real(x,y,z);}
void glNormal3fv(const GLfloat*p){ NEXT(glNormal3fv); if(n++<4000)fprintf(f(),"glNormal3fv %g %g %g\n",p[0],p[1],p[2]); real(p);}
void glPolygonStipple(const GLubyte*m){ NEXT(glPolygonStipple); fprintf(f(),"glPolygonStipple"); for(int i=0;i<128;i++)fprintf(f()," %02x",m[i]); fprintf(f(),"\n"); real(m);}
void glPushAttrib(GLbitfield m){ NEXT(glPushAttrib); fprintf(f(),"glPushAttrib %x\n",m); real(m);}
void glPopAttrib(void){ NEXT(glPopAttrib); fprintf(f(),"glPopAttrib stipple=%d\n",(int)glIsEnabled(GL_POLYGON_STIPPLE)); real();}
