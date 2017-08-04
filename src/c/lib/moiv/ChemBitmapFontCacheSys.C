/*
	WGL/GLX implementation:
Delete all libfl references, only FLBitmap structure is used
*/
/**************************************************************************\
 *
 * OpenMOIV - C++ library for molecular visualization using Inventor.
 * Copyright (C) 2001-2003 Universitat Pompeu Fabra - Barcelona (Spain)
 * 
 * Developers: Interactive Technology Group (GTI)
 * Team: Josep Blat, Eduard Gonzalez, Sergi Gonzalez,
 *       Daniel Soto, Alejandro Ramirez, Oscar Civit.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details (see the file 
 * LICENSE.LGPL at the root directory).
 *
 * REMARK: This library is a derived product.
 *         You need also to accept all other applicable licenses.
 * 
 * Homepage: http://www.tecn.upf.es/openMOIV/
 * Contact:  openmoiv@upf.es
 *
\**************************************************************************/

#ifdef __sys_fonts
#ifndef __no_libFL

#include <assert.h>
#include <ctype.h>
#include <math.h>

#include "ChemBitmapFontCache.h"
#include "inv/elements/SoCacheElement.H"

#include "FixFonts.h"

// This very specialized cache class is used to cache bitmaps and GL
// display lists containing bitmaps.  It is strange because it doesn't
// use the normal list of elements used to determine validity, etc,
// and knows exactly which elements it depends on.

// Static variables for ChemBitmapFontCache
SbPList *ChemBitmapFontCache::fonts = NULL;

void CreateSysFont(SbName name, int size, 
									 FLbitmap **bitmaps, SbBool *listFlags, int list);

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Get a font cache suitable for using with the given state.
//
// Use: static, internal, public

ChemBitmapFontCache *
ChemBitmapFontCache::getFont(SoGLRenderAction *action, SoState *state,
    SbName name, float size, SbBool forRender)
//
////////////////////////////////////////////////////////////////////////
{
    if (fonts == NULL)
		{
        // One-time font library initialization
        fonts = new SbPList;
		}
        
		ChemBitmapFontCache *result = NULL;
		for (int i = 0; i < fonts->getLength() && result == NULL; i++)
		{
				ChemBitmapFontCache *fc = (ChemBitmapFontCache *)(*fonts)[i];
				if (forRender ? fc->isRenderValid(action, state, name, size) :
												fc->isValid(state, name, size)) {
						result = fc;
						result->ref();
				}           
		}
    
		if (result == NULL)
		{
        result = new ChemBitmapFontCache(action, state, name, size, forRender);
    }    
    return result;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sees if this font is valid.  If it is valid, it also makes it
//    current.
//
// Use: public

SbBool
ChemBitmapFontCache::isRenderValid(SoGLRenderAction *action, SoState *state,
    SbName name, float size) const
//
////////////////////////////////////////////////////////////////////////
{
    return (list > 0) && isValid(state,name,size);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sees if this font is valid.  If it is valid, it also makes it
//    current.
//
// Use: public

SbBool
ChemBitmapFontCache::isValid(SoState *state, SbName name, float size) const
//
////////////////////////////////////////////////////////////////////////
{
    SbBool result = FALSE;

    const SbViewportRegion &vpr = SoViewportRegionElement::get(state);
    
    if ((name == fontName) && (size*vpr.getPixelsPerPoint() == fontSize)) {
        result = TRUE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Construct a bitmap font cache, given the state and a dummy
//    (empty) list of overridden elements (needed only to pass to the
//    SoCache constructor).
//
// Use: internal, private

ChemBitmapFontCache::ChemBitmapFontCache(SoGLRenderAction *action,
    SoState *state, SbName name, float size, SbBool forRender)
//
////////////////////////////////////////////////////////////////////////
{
  refCount = 0;
  ref();
  list = 0;
  context = 0;

  // Grab all the stuff we'll need to determine our validity from
  // the state.
  fontName = name;
  const SbViewportRegion &vpr = SoViewportRegionElement::get(state);
  fontSize = size * vpr.getPixelsPerPoint();

  // Initialize everything
  
  numChars = 256;  // ??? JUST DO ASCII FOR NOW!
  listFlags = new SbBool[numChars];
  bitmaps = new FLbitmap*[numChars];
  for (int i = 0; i < numChars; i++) {
      listFlags[i] = FALSE;
      bitmaps[i] = NULL;
  }

	list = glGenLists(numChars);
	if (list > 0)
	{
		 CreateSysFont(name,(int)size,bitmaps,listFlags,list);
	} //if (list > 0)

    fonts->append(this);
}
    
////////////////////////////////////////////////////////////////////////
//
// Description:
//    Destructor.  Clean up-- GL stuff, font library stuff, and
//    internally generated stuff.
//
// Previous Use: internal, private
// Current Use: protected since changes made by fabien fontaine the 13/12/2000
// to avoid g++ compiler warning

ChemBitmapFontCache::~ChemBitmapFontCache()
//
////////////////////////////////////////////////////////////////////////
{
	int i;
  for (i=0 ; i < numChars; i++)
	{
      if (bitmaps[i] != NULL)
				delete bitmaps[i];
  }

  if (list > 0)
      glDeleteLists(list, numChars);

  delete [] listFlags;
  delete [] bitmaps;

  fonts->remove(fonts->find(this));
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets up for GL rendering.
//
// Use: internal

void
ChemBitmapFontCache::setupToRender(SoState *state)
//
////////////////////////////////////////////////////////////////////////
{
    if (list > 0) {
        // Set correct list base
        glListBase(list);
    }   
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Returns TRUE if a display lists exists for given character.
//    Tries to build a display list, if it can.
//
// Use: internal

SbBool
ChemBitmapFontCache::hasDisplayList(const char c)
//
////////////////////////////////////////////////////////////////////////
{
    return (listFlags[c] == TRUE);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Assuming that there are display lists built for all the
//    characters in given string, render them using the GL's CallLists
//    routine.
//
// Use: internal

void
ChemBitmapFontCache::callLists(const SbString &string)
//
////////////////////////////////////////////////////////////////////////
{
    const char *str = string.getString();

    glCallLists(string.getLength(), GL_UNSIGNED_BYTE, str);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Returns the pixel-space bounding box of given character.
//
// Use: internal, public

void
ChemBitmapFontCache::getCharBbox(char c, SbBox3f &box)
//
////////////////////////////////////////////////////////////////////////
{
    box.makeEmpty();

    const FLbitmap *bmap = getBitmap(c);
    if (bmap == NULL) return;
    
    box.extendBy(SbVec3f(-bmap->xorig, -bmap->yorig, 0));
    box.extendBy(SbVec3f(bmap->width - bmap->xorig,
                         bmap->height - bmap->yorig, 0));
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Returns the amount the current raster position will be advanced
//    after drawing the given character.
//
// Use: internal, public

SbVec3f
ChemBitmapFontCache::getCharOffset(char c)
//
////////////////////////////////////////////////////////////////////////
{
    const FLbitmap *bmap = getBitmap(c);
    if (bmap != NULL) {
        return SbVec3f(bmap->xmove, bmap->ymove, 0);
    }
    else {
        return SbVec3f(0,0,0);
    }
}
    
////////////////////////////////////////////////////////////////////////
//
// Description:
//    Returns the width of given string.
//
// Use: internal, public

float
ChemBitmapFontCache::getWidth(const SbString &str)
//
////////////////////////////////////////////////////////////////////////
{
    float result = 0.0;

    const char *chars = str.getString();
    for (int i = 0; i < str.getLength(); i++) {
        const FLbitmap *bmap = getBitmap(chars[i]);
        if (bmap != NULL) {
            result += bmap->xmove;
        }
    }
    return result;
}
    
////////////////////////////////////////////////////////////////////////
//
// Description:
//    Returns the height of given string.
//
// Use: internal, public

float
ChemBitmapFontCache::getHeight()
//
////////////////////////////////////////////////////////////////////////
{
    const FLbitmap *bmap = getBitmap('M');
    if (bmap != NULL) {
        return bmap->height - bmap->yorig;
    }
    else {
        return 0.0f;
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Draws a bitmap
//
// Use: internal public

void
ChemBitmapFontCache::drawCharacter(char c)
//
////////////////////////////////////////////////////////////////////////
{
	glCallList(list+c);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Draws a whole string.  Tries to build display lists, if it can.
//
// Use: internal public

void
ChemBitmapFontCache::drawString(const SbString &string)
//
////////////////////////////////////////////////////////////////////////
{
	callLists(string);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Returns a bitmap.
//
// Use: private

const FLbitmap *
ChemBitmapFontCache::getBitmap(unsigned char c)
//
////////////////////////////////////////////////////////////////////////
{
    return bitmaps[c];
}

void
ChemBitmapFontCache::ref()
{
    ++refCount;
}

void
ChemBitmapFontCache::unref()
{
    --refCount;
    if (refCount <= 0) {
        delete this;
    }
}
#endif //#ifndef __no_libFL
#endif //#ifndef __sys_fonts
