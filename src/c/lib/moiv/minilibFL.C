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

#ifdef __no_libFL
// --> multiplatform fonts
#ifndef __sys_fonts
// <-- multiplatform fonts

/*
	Daniel Soto	2002/02/13
	Simple version of flclient with a unique fixed font.
*/

#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#include <windows.h>
#endif

#include "inv/flclient.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

static GLubyte buffer[12*4];
static GLbitfield default_font[][12] = {
  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }, //
  {  0,  0, 12, 12,  0,  8, 12, 12, 12, 12, 12,  0 }, // !
  {  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 20 }, // "
  {  0,  0, 18, 18, 18, 63, 18, 18, 63, 18, 18,  0 }, // #
  {  0,  8, 28, 42, 10, 10, 12, 24, 40, 42, 28,  8 }, // $
  {  0,  0,  6, 73, 41, 22,  8, 52, 74, 73, 48,  0 }, // %
  {  0, 12, 18, 18, 12, 25, 37, 34, 34, 29,  0,  0 }, // &
  {  0,  0,  0,  0,  0,  0,  0,  0,  0, 24, 12, 12 }, // '
  {  0,  6,  8,  8, 16, 16, 16, 16, 16,  8,  8,  6 }, // (
  {  0, 48,  8,  8,  4,  4,  4,  4,  4,  8,  8, 48 }, // )
  {  0,  0,  0,  0,  0,  0,  8, 42, 20, 42,  8,  0 }, // *
  {  0,  0,  0,  8,  8,  8,127,  8,  8,  8,  0,  0 }, // +
  {  0, 24, 12, 12,  0,  0,  0,  0,  0,  0,  0,  0 }, // ,
  {  0,  0,  0,  0,  0,  0,127,  0,  0,  0,  0,  0 }, // -
  {  0,  0, 24, 24,  0,  0,  0,  0,  0,  0,  0,  0 }, // .
  {  0, 32, 32, 16, 16,  8,  8,  8,  4,  4,  2,  2 }, // /
  {  0,  0, 28, 34, 34, 34, 34, 34, 34, 34, 28,  0 }, // 0
  {  0,  0,  8,  8,  8,  8,  8,  8, 40, 24,  8,  0 }, // 1
  {  0,  0, 62, 32, 16,  8,  4,  2,  2, 34, 28,  0 }, // 2
  {  0,  0, 28, 34,  2,  2, 12,  2,  2, 34, 28,  0 }, // 3
  {  0,  0,  4,  4,  4,126, 68, 36, 20, 12,  4,  0 }, // 4
  {  0,  0, 28, 34,  2,  2,  2, 60, 32, 32, 62,  0 }, // 5
  {  0,  0, 28, 34, 34, 34, 60, 32, 32, 34, 28,  0 }, // 6
  {  0,  0, 16, 16, 16,  8,  8,  4,  2,  2, 62,  0 }, // 7
  {  0,  0, 28, 34, 34, 34, 28, 34, 34, 34, 28,  0 }, // 8
  {  0,  0, 28, 34,  2,  2, 30, 34, 34, 34, 28,  0 }, // 9
  {  0,  0, 24, 24,  0,  0,  0, 24, 24,  0,  0,  0 }, // :
  {  0, 48, 24, 24,  0,  0,  0, 24, 24,  0,  0,  0 }, // ;
  {  0,  0,  0,  2,  4,  8, 16,  8,  4,  2,  0,  0 }, // <
  {  0,  0,  0,  0,  0,127,  0,127,  0,  0,  0,  0 }, // =
  {  0,  0,  0, 16,  8,  4,  2,  4,  8, 16,  0,  0 }, // >
  {  0,  0, 16, 16,  0, 16, 28,  2,  2,  2, 60,  0 }, // ?
  {  0,  0, 28, 32, 73, 86, 82, 82, 78, 34, 28,  0 }, // @
  {  0,  0, 33, 33, 33, 63, 18, 18, 18, 12, 12,  0 }, // A
  {  0,  0, 60, 34, 34, 34, 60, 34, 34, 34, 60,  0 }, // B
  {  0,  0, 14, 16, 32, 32, 32, 32, 32, 18, 14,  0 }, // C
  {  0,  0, 56, 36, 34, 34, 34, 34, 34, 36, 56,  0 }, // D
  {  0,  0, 62, 32, 32, 32, 60, 32, 32, 32, 62,  0 }, // E
  {  0,  0, 16, 16, 16, 16, 30, 16, 16, 16, 30,  0 }, // F
  {  0,  0, 14, 18, 34, 34, 32, 32, 32, 18, 14,  0 }, // G
  {  0,  0, 34, 34, 34, 34, 62, 34, 34, 34, 34,  0 }, // H
  {  0,  0, 62,  8,  8,  8,  8,  8,  8,  8, 62,  0 }, // I
  {  0,  0,112,  8,  8,  8,  8,  8,  8,  8, 62,  0 }, // J
  {  0,  0, 33, 33, 34, 36, 56, 40, 36, 34, 33,  0 }, // K
  {  0,  0, 30, 16, 16, 16, 16, 16, 16, 16, 16,  0 }, // L
  {  0,  0, 33, 33, 33, 45, 45, 45, 51, 51, 33,  0 }, // M
  {  0,  0, 34, 34, 38, 38, 42, 42, 50, 50, 34,  0 }, // N
  {  0,  0, 12, 18, 33, 33, 33, 33, 33, 18, 12,  0 }, // O
  {  0,  0, 32, 32, 32, 60, 34, 34, 34, 34, 60,  0 }, // P
  {  3,  6, 12, 18, 33, 33, 33, 33, 33, 18, 12,  0 }, // Q
  {  0,  0, 34, 34, 34, 36, 60, 34, 34, 34, 60,  0 }, // R
  {  0,  0, 60,  2,  2,  6, 28, 48, 32, 32, 30,  0 }, // S
  {  0,  0,  8,  8,  8,  8,  8,  8,  8,  8,127,  0 }, // T
  {  0,  0, 28, 34, 34, 34, 34, 34, 34, 34, 34,  0 }, // U
  {  0,  0, 12, 12, 18, 18, 18, 33, 33, 33, 33,  0 }, // V
  {  0,  0, 34, 34, 34, 54, 85, 73, 73, 73, 65,  0 }, // W
  {  0,  0, 34, 34, 20, 20,  8, 20, 20, 34, 34,  0 }, // X
  {  0,  0,  8,  8,  8,  8, 20, 20, 34, 34, 34,  0 }, // Y
  {  0,  0, 62, 32, 16, 16,  8,  4,  4,  2, 62,  0 }, // Z
  {  0, 14,  8,  8,  8,  8,  8,  8,  8,  8,  8, 14 }, // [
  {  0,  2,  2,  4,  4,  8,  8,  8, 16, 16, 32, 32 }, // [backslash]
  {  0, 56,  8,  8,  8,  8,  8,  8,  8,  8,  8, 56 }, // ]
  {  0,  0,  0,  0,  0, 34, 34, 20, 20,  8,  8,  0 }, // ^
  {  0,127,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }, // _
  {  0,  0,  0,  0,  0,  0,  0,  0,  0, 24, 24, 12 }, // `
  {  0,  0, 29, 34, 34, 30,  2, 34, 28,  0,  0,  0 }, // a
  {  0,  0, 60, 34, 34, 34, 34, 50, 44, 32, 32, 32 }, // b
  {  0,  0, 14, 16, 32, 32, 32, 16, 14,  0,  0,  0 }, // c
  {  0,  0, 26, 38, 34, 34, 34, 34, 30,  2,  2,  2 }, // d
  {  0,  0, 28, 34, 32, 62, 34, 34, 28,  0,  0,  0 }, // e
  {  0,  0, 16, 16, 16, 16, 16, 16, 62, 16, 16, 14 }, // f
  { 28,  2,  2, 26, 38, 34, 34, 34, 30,  0,  0,  0 }, // g
  {  0,  0, 34, 34, 34, 34, 34, 50, 44, 32, 32, 32 }, // h
  {  0,  0,  8,  8,  8,  8,  8,  8, 56,  0,  8,  8 }, // i
  { 56,  4,  4,  4,  4,  4,  4,  4, 60,  0,  4,  4 }, // j
  {  0,  0, 33, 34, 36, 56, 40, 36, 34, 32, 32, 32 }, // k
  {  0,  0,  8,  8,  8,  8,  8,  8,  8,  8,  8, 56 }, // l
  {  0,  0, 73, 73, 73, 73, 73,109, 82,  0,  0,  0 }, // m
  {  0,  0, 34, 34, 34, 34, 34, 50, 44,  0,  0,  0 }, // n
  {  0,  0, 28, 34, 34, 34, 34, 34, 28,  0,  0,  0 }, // o
  { 32, 32, 60, 34, 34, 34, 34, 50, 44,  0,  0,  0 }, // p
  {  2,  2, 26, 38, 34, 34, 34, 34, 30,  0,  0,  0 }, // q
  {  0,  0, 16, 16, 16, 16, 16, 24, 22,  0,  0,  0 }, // r
  {  0,  0, 60,  2,  2, 28, 32, 32, 30,  0,  0,  0 }, // s
  {  0,  0, 14, 16, 16, 16, 16, 16, 62, 16, 16,  0 }, // t
  {  0,  0, 26, 38, 34, 34, 34, 34, 34,  0,  0,  0 }, // u
  {  0,  0,  8,  8, 20, 20, 34, 34, 34,  0,  0,  0 }, // v
  {  0,  0, 34, 34, 34, 85, 73, 73, 65,  0,  0,  0 }, // w
  {  0,  0, 34, 34, 20,  8, 20, 34, 34,  0,  0,  0 }, // x
  { 48, 16,  8,  8, 20, 20, 34, 34, 34,  0,  0,  0 }, // y
  {  0,  0, 62, 32, 16,  8,  4,  2, 62,  0,  0,  0 }, // z
  {  0,  6,  8,  8,  8,  4, 24,  4,  8,  8,  8,  6 }, // {
  {  0,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8 }, // |
  {  0, 48,  8,  8,  8, 16, 12, 16,  8,  8,  8, 48 }, // }
  {  0,  0,  0,  0,  0,  0, 78, 57,  0,  0,  0,  0 }  // ~
};

int dummy_context = 0;

/* functions provided only to Molecular Inventor */
FLcontext flCreateContext(
    const GLubyte *                fontPath,
    GLint                          fontNamePreference,
    const GLubyte *                fontNameRestriction,
    GLfloat                        pointsPerUMx,
    GLfloat                        pointsPerUMy
) {
	FLcontext ctx = (FLcontext)&dummy_context;
	return ctx;
};

FLfontNumber flCreateFont(
    const GLubyte *                /* fontName */,
    GLfloat [2][2]                 /* mat */, 
    GLint                          /* charNameCount */,
    GLubyte **                     /* charNameVector */
) {	return 1;}

void flDestroyFont(
    FLfontNumber                   /* fn */
){}

void flFreeBitmap(
    FLbitmap *                     /* bitmapPtr */
) {return;};

FLbitmap *flGetBitmap(
    FLfontNumber                   fn, 
    GLuint                         c
) {
	FLbitmap *cbitmap;
	int i;
	
	cbitmap  = (FLbitmap *)malloc(sizeof(FLbitmap));

//	for (i=0;i<12;i++)
//		buffer[i*4] = default_font[c-32][i];

	cbitmap->width        = 8;
	cbitmap->height       = 12;
	cbitmap->xorig        = 0.0;
	cbitmap->yorig        = 0.0;
	cbitmap->xmove        = 8.0;
	cbitmap->ymove        = 0.0;
	cbitmap->bitmap       = (GLubyte*)default_font[c-32];
//	cbitmap->bitmap       = buffer;

	return cbitmap;
};

FLcontext flGetCurrentContext(
    void
) {
	FLcontext ctx = (FLcontext)&dummy_context;
	return ctx;
};

GLboolean flMakeCurrentContext(
    FLcontext                      /* ctx */
) {return true;};

FLfontNumber flGetCurrentFont(
    void
) {return 1;};

GLboolean flMakeCurrentFont(
    FLfontNumber                   /* fn */
) {return true;};

void flSetHint(
    GLuint                         /* hint */, 
    GLfloat                        /* hintValue */
) {return;};

#ifdef __cplusplus
}
#endif /* __cplusplus */

// --> multiplatform fonts
#endif //__sys_fonts
// <-- multiplatform fonts
#endif /* __no_libFL */
