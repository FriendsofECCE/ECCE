/*
 * ChemBitmapFontCache.h
 *
 *     This file defines the ChemBitmapFontCache class.
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

#ident "$Revision: 22148 $"

#ifndef  __CHEM_BITMAP_FONT_CACHE_H__
#define  __CHEM_BITMAP_FONT_CACHE_H__

/*
	Porting to Windows:
	Daniel Soto	2002/02/13
*/
#ifdef WIN32
#include <windows.h>
#endif

#include "inv/SbBox.H"
#include "inv/actions/SoGLRenderAction.H"
#include "inv/elements/SoViewportRegionElement.H"

#include "inv/flclient.h"


// This very specialized cache class is used to cache bitmaps and GL
// display lists containing bitmaps.  It is strange because it doesn't
// use the normal list of elements used to determine validity, etc,
// and knows exactly which elements it depends on.

class SoGLRenderAction;

class ChemBitmapFontCache
{
  public:
    void ref();
    void unref();

    // Return a font (either a new one or an old one) that is valid
    // for the given state.
    static ChemBitmapFontCache *getFont(SoGLRenderAction *action,
                                        SoState *state,
                                        SbName name, float size,
                                        SbBool forRender);

    // Checks to see if this font is valid
    SbBool      isValid(SoState *state, SbName name, float size) const;
    SbBool      isRenderValid(SoGLRenderAction *action, SoState *state,
                              SbName name, float size) const;

    // Set up for GL rendering:
    void    setupToRender(SoState *state);

    // Returns the amount the current raster position will be advanced
    // after drawing the given character.
    SbVec3f     getCharOffset(char c);

    // Get the pixel-space bounding box of a given character.
    void        getCharBbox(char c, SbBox3f &box);

    // Gets the width (in pixels) of the given string
    float       getWidth(const SbString &str);

    // Gets the height of the font, in pixels
    float       getHeight();

    // Draws the given string
    void        drawString(const SbString &string);

    // Draws the given character (using GL)
    void        drawCharacter(char c);

  protected:
    /* 
        virtual ~ChemBitmapFontCache(); was moved from private mode by fabien
	fontaine the 13/12/2000 to avoid g++ compiler warning. 
    */
    // Destructor
    virtual ~ChemBitmapFontCache();
    
  private:
    int32_t refCount;

    // Constructor.
    ChemBitmapFontCache(SoGLRenderAction *action, SoState *state,
                        SbName name, float size, SbBool forRender);

    
    // Destructor
    /* 
       Moved to protected mode by fabien fontaine the 13/12/2000 to avoid 
       g++ compiler warning
    virtual ~ChemBitmapFontCache();
    */

    // Returns TRUE if this font cache has a display list for the
    // given character.  It will try to build a display list, if it
    // can.
    SbBool  hasDisplayList(const char c);

    // Renders an entire string by using the GL callList() function.
    void    callLists(const SbString &string);

    const FLbitmap *getBitmap(unsigned char c);

    SbName fontName;
    float  fontSize;

    // The display list for the characters in this font
    GLuint  list;
    int32_t context;
    // The number of characters in this font -> always 256 for now
    int32_t numChars;
    // Flag for each character-- have we built GL display list yet?
    SbBool  *listFlags;
    // Cached bitmaps for each character.  NULL if bitmap hasn't been 
    // fetched yet.
    FLbitmap    **bitmaps; 
    
    // This flag will be true if there is another cache open (if
    // building GL display lists for render caching, that means we
    // can't also build display lists).
    SbBool  otherOpen;

    // And some font library stuff:
    static FLcontext    flContext;
    FLfontNumber        fontId;

    // Static list of all fonts.  OPTIMIZATION:  If there turn out to
    // be applications that use lots of fonts, we could change this
    // list into a dictionary keyed off the font name.
    static SbPList  *fonts;
};

#endif /* !__CHEM_BITMAP_FONT_CACHE_H__ */
