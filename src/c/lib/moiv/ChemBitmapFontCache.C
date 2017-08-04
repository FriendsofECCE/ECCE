/*
 * ChemBitmapFontCache.c++
 *
 *     Classes:  ChemBitmapFontCache
 *
 *
 * Copyright 1996, 1997, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 *
 * UNPUBLISHED -- Rights reserved under the copyright laws of the United
 * States.   Use of a copyright notice is precautionary only and does not
 * imply publication or disclosure.
 *
 * U.S. GOVERNMENT RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions
 * as set forth in FAR 52.227.19(c)(2) or subparagraph (c)(1)(ii) of the Rights
 * in Technical Data and Computer Software clause at DFARS 252.227-7013 and/or
 * in similar or successor clauses in the FAR, or the DOD or NASA FAR
 * Supplement.  Contractor/manufacturer is Silicon Graphics, Inc.,
 * 2011 N. Shoreline Blvd. Mountain View, CA 94039-7311.
 *
 * THE CONTENT OF THIS WORK CONTAINS CONFIDENTIAL AND PROPRIETARY
 * INFORMATION OF SILICON GRAPHICS, INC. ANY DUPLICATION, MODIFICATION,
 * DISTRIBUTION, OR DISCLOSURE IN ANY FORM, IN WHOLE, OR IN PART, IS STRICTLY
 * PROHIBITED WITHOUT THE PRIOR EXPRESS WRITTEN PERMISSION OF SILICON
 * GRAPHICS, INC.
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

// --> multiplatform fonts
#ifndef __sys_fonts
// <-- multiplatform fonts

#ident "$Revision: 22364 $"

#include <assert.h>
#include <ctype.h>
#include <math.h>

#include "inv/elements/SoCacheElement.H"

#include "inv/ChemKit/ChemBitmapFontCache.H"
/*
	Porting to Windows:
	Jamie Doornbos	2001/07/15
	Daniel Soto	2002/02/05
*/
#include "inv/ChemKit/FixFonts.H"

// This very specialized cache class is used to cache bitmaps and GL
// display lists containing bitmaps.  It is strange because it doesn't
// use the normal list of elements used to determine validity, etc,
// and knows exactly which elements it depends on.

// Static variables for ChemBitmapFontCache
SbPList *ChemBitmapFontCache::fonts = NULL;
FLcontext ChemBitmapFontCache::flContext;

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
    if (fonts == NULL) {
        // One-time font library initialization
        fonts = new SbPList;
        flContext = flCreateContext(NULL, FL_FONTNAME, NULL, 1.0, 1.0);
        if (flContext == NULL) {
#ifdef DEBUG
            SoDebugError::post("ChemDisplay::getFont",
                               "flCreateContext returned NULL");
#endif
            return NULL;
        }
        if (flGetCurrentContext() != flContext) {
            flMakeCurrentContext(flContext);
        }
        flSetHint(FL_HINT_MINOUTLINESIZE, 24.0);
    }
    else if (flContext == NULL) {
        return NULL;
    }
    else {
        if (flGetCurrentContext() != flContext) {
            flMakeCurrentContext(flContext);
        }
    }
    
    ChemBitmapFontCache *result = NULL;
    for (int i = 0; i < fonts->getLength() && result == NULL; i++) {
        ChemBitmapFontCache *fc = (ChemBitmapFontCache *)(*fonts)[i];
        if (forRender ? fc->isRenderValid(action, state, name, size) :
                        fc->isValid(state, name, size)) {
            result = fc;
            result->ref();
            if (flGetCurrentFont() != result->fontId) {
                flMakeCurrentFont(result->fontId);
            }
        }           
    }
    if (result == NULL) {
        result = new ChemBitmapFontCache(action, state, name, size, forRender);

        // If error:
        if (result->fontId == 0) {
            delete result;
            return NULL;
        }
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
    if (list == 0) return isValid(state, name, size);
    else
        return ((context == action->getCacheContext()) &&
                (isValid(state, name, size)));
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

    if (result) {
        if (flGetCurrentContext() != flContext) {
            flMakeCurrentContext(flContext);
            flMakeCurrentFont(fontId);
        }
        else if (flGetCurrentFont() != fontId) {
            flMakeCurrentFont(fontId);
        }
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
    GLfloat m[2][2];
    m[0][0] = m[1][1] = fontSize;
    m[0][1] = m[1][0] = 0.0;
    fontId = flCreateFont((const GLubyte *)fontName.getString(), m, 0, NULL);

    if (fontId == 0) {
        // Try Utopia-Regular, unless we just did!
/*
	Porting to Windows:
	Jamie Doornbos	2001/07/15.
	Daniel Soto	2002/02/05.
*/
        if (fontName != SbName(DEFAULT_FONT)) {
#ifdef DEBUG
            SoDebugError::post("ChemDisplay::getFont",
                      "Couldn't find font %s, replacing with Default Font",
                       fontName.getString());
#endif
            fontId = flCreateFont((const GLubyte *)DEFAULT_FONT,
                                  m, 0, NULL);
        }
        if (fontId == 0) {
#ifdef DEBUG
            SoDebugError::post("ChemDisplay::getFont",
                               "Couldn't find default font!");
#endif
            numChars = 0;
            return;
        }
    }   
    flMakeCurrentFont(fontId);
    
    numChars = 256;  // ??? JUST DO ASCII FOR NOW!
    listFlags = new SbBool[numChars];
    bitmaps = new FLbitmap*[numChars];
    for (int i = 0; i < numChars; i++) {
        listFlags[i] = FALSE;
        bitmaps[i] = NULL;
    }

    fonts->append(this);
    if (forRender && (action != NULL)) {
        context = action->getCacheContext();
    }
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
    if (fontId) {
        if (flGetCurrentContext() != flContext) {
            flMakeCurrentContext(flContext);
            flMakeCurrentFont(fontId);
        }
        else if (flGetCurrentFont() != fontId) {
            flMakeCurrentFont(fontId);
        }
	// int i = 0 put out of the for loop by fabien fontaine the 15/12/2000
	// to avoid compiler warning
	int i = 0 ;
        for ( ; i < numChars; i++) {
            if (bitmaps[i] != NULL) flFreeBitmap(bitmaps[i]);
        }

        // Only destroy the font library font if no other font caches
        // are using the same font identifier:
        SbBool otherUsing = FALSE;
        for (i = 0; i < fonts->getLength(); i++) {
            ChemBitmapFontCache *t = (ChemBitmapFontCache *)(*fonts)[i];
            if (t != this && t->fontId == fontId) otherUsing = TRUE;
        }
        if (!otherUsing) {
            flDestroyFont(fontId);
        }
    
        if (list > 0) {
            glDeleteLists(list, numChars);
        }
        delete [] listFlags;
        delete [] bitmaps;

        fonts->remove(fonts->find(this));
    }
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
    otherOpen = SoCacheElement::anyOpen(state);

    if (!otherOpen && (list == 0)) {
        list = glGenLists(numChars);
    }
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
    // If we have one, return TRUE
    if (listFlags[(int)c] == TRUE) return TRUE;
    
    // If we don't and we can't build one, return FALSE.
    if (otherOpen) return FALSE;
    
    // Build one:
    glNewList(list+c, GL_COMPILE);
    drawCharacter(c);
    glEndList();
    listFlags[(int)c] = TRUE;

    return TRUE;
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
        return 0;
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
    const FLbitmap *bmap = getBitmap(c);
    
    if (bmap != NULL) {
        glBitmap(bmap->width, bmap->height, bmap->xorig, bmap->yorig,
             bmap->xmove, bmap->ymove, bmap->bitmap);
    }
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
    SbBool useCallLists = TRUE;

    const char *chars = string.getString();

    // If there aren't any other caches open, build display lists for
    // the characters we can:
    for (int i = 0; i < string.getLength(); i++) {
        // See if the font cache already has (or can build) a display
        // list for this character:
        if (!hasDisplayList(chars[i])) {
            useCallLists = FALSE;
            break;
        }
    }
        
    // if we have display lists for all of the characters, use
    // glCallLists:
    if (useCallLists) {
        callLists(string);
    } else {
        // if we don't, draw the string character-by-character, using the
        // display lists we do have:
        for (int i = 0; i < string.getLength(); i++) {
            if (!hasDisplayList(chars[i])) {
                drawCharacter(chars[i]);
            }
            else {
                glCallList(list+chars[i]);
            }
        }
    }
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
    if (bitmaps[c] == NULL) {
        bitmaps[c] = flGetBitmap(fontId, c);
    }
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

// --> multiplatform fonts
#endif // __sys_fonts
// <-- multiplatform fonts

