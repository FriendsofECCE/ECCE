#ifndef __CHEM_PER_PIXEL_H__
#define __CHEM_PER_PIXEL_H__

class SoState;

//  Per-pixel (Blinn-Phong) lighting for ChemDisplay's atom spheres and bond
//  cylinders (#210).  Gouraud shading of a tessellated sphere puts the
//  highlight on the vertices, so it comes out polygonal and patchy; the
//  program evaluates the same fixed-function light and material state per
//  fragment instead, so colours, selection and transparency are unchanged.
//
//  Coin builds only.  Without GLSL 1.20, on a compile failure, or with
//  ECCE_GL_PERPIXEL=0 it says so once on stderr and begin() returns false,
//  leaving fixed-function lighting in place.  The vendored build keeps
//  fixed-function lighting.
//
//  Under Coin's sorted-layers transparency the program replaces Coin's
//  peeling fragment program while atoms are drawn.  For opaque atoms that
//  changes nothing: each layer shows the nearest atom again, and an opaque
//  layer hides what lies behind it (identical pictures, tests/look/
//  perpixel.py).  Transparent atoms would need the peel test added here.
class ChemPerPixel {
public:
	//  Below these on-screen radii (ChemDisplay's atomPixelRadius and
	//  bondPixelRadius) triangles cover a few pixels and Gouraud shading
	//  looks the same, so the cheaper fixed-function path is kept.
	static constexpr float minAtomPixels = 12.0f;
	static constexpr float minBondPixels = 2.0f;
#ifdef OIV_COIN
	//  Binds the program for the state's GL context; false if nothing was
	//  bound.  Call with lighting on and GL_COLOR_MATERIAL tracking diffuse.
	static bool begin(SoState *state);
	//  Restores the program bound before begin(); only after begin() == true.
	static void end();
	//  "on", "off" or "unavailable" for the last context begin() saw.
	static const char *status();
#else
	static bool begin(SoState *) { return false; }
	static void end() {}
	static const char *status() { return "off"; }
#endif
};

#endif
