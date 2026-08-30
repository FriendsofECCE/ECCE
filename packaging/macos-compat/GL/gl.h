/* macOS compatibility shim -- the tree's ~90 call sites all spell this
 * "GL/gl.h" (Linux/X11 convention). Apple's own OpenGL framework
 * headers live under "OpenGL/gl.h" instead. Forwarding here avoids
 * touching every one of those call sites individually. */
#include <OpenGL/gl.h>
