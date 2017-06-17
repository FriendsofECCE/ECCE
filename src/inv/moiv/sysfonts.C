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

#include <string.h>
#include <assert.h>

#include "inv/ChemKit/ChemBitmapFontCache.H"
/*
	WGL/GLX implementation:
	Delete all libfl references, only FLBitmap structure is used
*/

#ifdef __WGL
void CreateSysFont(SbName name, int size, 
									 FLbitmap **bitmaps, SbBool *listFlags, int list)
{
		HDC hdc = wglGetCurrentDC();
		HFONT hOldFont, hFont;

#ifdef UNICODE
   int lstra = lstrlenA(name.getString())+1;
   BSTR unicodestr = SysAllocStringLen(NULL, 2*lstra);
   MultiByteToWideChar(CP_ACP, 0, name.getString(), lstra, unicodestr, lstra);
  	
		hFont = CreateFont(size,							// Our desired HEIGHT of the font
					0,								// The WIDTH (If we leave this zero it will pick the best width depending on the height)
					0,								// The angle of escapement
					0,								// The angle of orientation
					FW_BOLD,						// The font's weight (We want it bold)
					FALSE,							// Italic - We don't want italic
					FALSE,							// Underline - We don't want it underlined
					FALSE,							// Strikeout - We don't want it strikethrough
					ANSI_CHARSET,					// This is the type of character set
					OUT_TT_PRECIS,					// The Output Precision
					CLIP_DEFAULT_PRECIS,			// The Clipping Precision
					ANTIALIASED_QUALITY,			// The quality of the font - We want anitaliased fonts
					FF_DONTCARE|DEFAULT_PITCH,		// The family and pitch of the font.  We don't care.
					(unsigned short*)unicodestr);
		
	 SysFreeString(unicodestr);

#else
		hFont = CreateFont(size,							// Our desired HEIGHT of the font
					0,								// The WIDTH (If we leave this zero it will pick the best width depending on the height)
					0,								// The angle of escapement
					0,								// The angle of orientation
					FW_BOLD,						// The font's weight (We want it bold)
					FALSE,							// Italic - We don't want italic
					FALSE,							// Underline - We don't want it underlined
					FALSE,							// Strikeout - We don't want it strikethrough
					ANSI_CHARSET,					// This is the type of character set
					OUT_TT_PRECIS,					// The Output Precision
					CLIP_DEFAULT_PRECIS,			// The Clipping Precision
					ANTIALIASED_QUALITY,			// The quality of the font - We want anitaliased fonts
					FF_DONTCARE|DEFAULT_PITCH,		// The family and pitch of the font.  We don't care.
					(const char *)name.getString());					// The font name (Like "Arial", "Courier", etc...)
#endif

		if (hFont != NULL)
		{
			// Now that we have created a new font, we need to select that font into our global HDC.
			// We store the old font so we can select it back in when we are done to avoid memory leaks.
			hOldFont = (HFONT)SelectObject(hdc, hFont);

			// build fl bitmap structures

			GLYPHMETRICS metrics;
			memset(&metrics,0,sizeof(GLYPHMETRICS));
			MAT2 mat;
			mat.eM11.fract = 0;mat.eM11.value = 1;
			mat.eM12.fract = 0;mat.eM12.value = 0;
			mat.eM21.fract = 0;mat.eM21.value = 0;
			mat.eM22.fract = 0;mat.eM22.value = 1;
			int i;
			for (i=0; i<256; i++)
			{
				GetGlyphOutline(hdc,i,GGO_METRICS,&metrics,0,NULL,&mat);

				bitmaps[i] = (FLbitmap *)malloc(sizeof(FLbitmap));
				bitmaps[i]->width        = metrics.gmBlackBoxX;
				bitmaps[i]->height       = metrics.gmBlackBoxY;
				bitmaps[i]->xorig        = 0;//metrics.gmptGlyphOrigin.x;
				bitmaps[i]->yorig        = 0;//metrics.gmptGlyphOrigin.y;
				bitmaps[i]->xmove        = metrics.gmCellIncX;
				bitmaps[i]->ymove        = metrics.gmCellIncY;
				bitmaps[i]->bitmap       = NULL;
				listFlags[i] = TRUE;
			}

			// build display lists
			wglUseFontBitmaps(hdc, 0, 255, list);	// Builds 255 bitmap characters

			SelectObject(hdc, hOldFont);//restore the old font

			DeleteObject(hFont); // delete the font
		} //if (hFont != NULL)
}

#endif //__WGL

#ifdef __GLX // linux, unix, etc.
#include <GL/glx.h>
#include <GL/gl.h>
#include <X11/keysym.h>
#include <stdlib.h>

void CreateSysFont(SbName name, int size,
									 FLbitmap **bitmaps, SbBool *listFlags, int list)
{
	int i;
	Display *display = NULL;
	XFontStruct *font_struct = NULL;

	display = glXGetCurrentDisplay();
	assert(display!=NULL);

	char **listFonts;
	int count = 0;
	char sname[80];
	char sfilter[80];
	sprintf(sname,"%s",name.getString());

	listFonts = XListFonts(display,sname,1,&count);
	//for (i=0; i<count; i++) printf("%s\n",listFonts[i]);

#ifndef NDEBUG
	fprintf(stdout,"Searching font name... %s\n",sname);
#endif

	if (count==1)
	{
		strcpy(sname,listFonts[0]);
		XFreeFontNames(listFonts);
	}
	else
	{
		// build filter and pick up first font name
		sprintf(sname,"-*-%s-*-*-*-*-*-*-*-*-*-*-*-*",name.getString());
#ifndef NDEBUG
		fprintf(stdout,"-Building filter... %s\n",sname);
#endif

		listFonts = XListFonts(display,sname,2,&count);
		if (count > 0)
		{
			strcpy(sname,listFonts[0]);
			XFreeFontNames(listFonts);
#ifndef NDEBUG
			fprintf(stdout,"-Picking first name returned... %s\n",sname);
#endif
		}
		else
		{
			// get the first font avalaible
			listFonts = XListFonts(display,"-*-Utopia-*-*-*-*-*-*-*-*-*-*-*-*",2,&count);
			strcpy(sname,listFonts[0]);
			XFreeFontNames(listFonts);
#ifndef NDEBUG
			fprintf(stdout,"-Search returned 0 names. Picking first Utopia font avalaible..:%s\n",sname);
#endif
		}
	}

	// sname contains a valid string name
	if (count>1)// size is not ok
	{
		// now we must find the font with a proper size
		int pos = 0, pos1;
		int hyphen = 0;
		int len = strlen(sname);
		while (pos < len && hyphen != 7)
		{
			if (sname[pos] == '-') hyphen++;
			pos++;
		}
		// search next hyphen 'cause there are fonts with 8 and fonts with 10
		pos1 = pos+1;
		while (pos1 < len && hyphen !=8)
		{
			if (sname[pos1] == '-') hyphen++;
			pos1++;
		}
		SbString stemp((const char*)sname);
		sprintf(sfilter,"%s*%s",stemp.getSubString(0,pos-1).getString(),stemp.getSubString(pos1-1,-1).getString());

		listFonts = XListFonts(display,sfilter,100,&count);
		if (count > 0)
		{
			int index = 0;
			int properIndex = 0;
			int curDif = 0;
			int dif = 100;
			char ssize[3];
			strcpy(ssize,"00");
#ifndef NDEBUG
			fprintf(stdout,"-Searching nearest font size to %d in %d different names...\n",size,count);
#endif
			while (index < count)
			{
				pos = 0;
				hyphen = 0;
				len = strlen(listFonts[index]);
				while (pos < len && hyphen != 7)
				{
					if (listFonts[index][pos] == '-') hyphen++;
					pos++;
				}

				ssize[0] = listFonts[index][pos];
				if (listFonts[index][pos+1] != '-') ssize[1] = listFonts[index][pos+1];
				else ssize[1] = '\0';
				curDif = abs(atoi(ssize) - size);
#ifndef NDEBUG
				fprintf(stdout,"-Parsing font size from %d:name %s. Size parsed is %s.\n",index,listFonts[index],ssize);
#endif
				if (curDif < dif)
				{
					properIndex = index;
					dif = curDif;
					if (dif == 0) index = count;//leave while
				}
				index++;
			}
			strcpy(sname,listFonts[properIndex]);
			XFreeFontNames(listFonts);
		}
	}

	// build fl bitmap structures
	font_struct = XLoadQueryFont(display,sname);
	if (font_struct == NULL)
	{
		// get the first font avalaible
		listFonts = XListFonts(display,"-*-Utopia-*-*-*-*-*-*-*-*-*-*-*-*",2,&count);
#ifndef NDEBUG
			fprintf(stdout,"-Font %s can't be queried. Picking first Utopia font avalaible..:%s\n",sname);
#endif

		strcpy(sname,listFonts[0]);
		XFreeFontNames(listFonts);
	}
#ifndef NDEBUG
	fprintf(stdout,"Querying font name... %s\n",sname);
#endif

	font_struct = XLoadQueryFont(display,sname);
	if (font_struct == NULL)
	{
		// get the first font avalaible
		listFonts = XListFonts(display,"*",100,&count);
#ifndef NDEBUG
		fprintf(stdout,"-Font %s can't be queried. Picking first font avalaible..:%s\n",sname);
#endif
		for (i=0; i<count;i++)
		{
			strcpy(sname,listFonts[i]);
			font_struct = XLoadQueryFont(display,sname);
			if (font_struct != NULL) break;
#ifndef NDEBUG
			fprintf(stdout,"-Font %s can't be queried. Testing next font avalaible..:%s\n",sname);
#endif
		}
		if (count>0) XFreeFontNames(listFonts);
	}
	if (font_struct != NULL)
	{
		for (i=0; i<256;i++)
		{
				bitmaps[i] = (FLbitmap *)malloc(sizeof(FLbitmap));
				bitmaps[i]->width        = font_struct->per_char[i].width;
				bitmaps[i]->height       = font_struct->per_char[i].ascent + font_struct->per_char[i].descent;
				bitmaps[i]->xorig        = 0;
				bitmaps[i]->yorig        = 0;
				bitmaps[i]->xmove        = font_struct->per_char[i].width;
				bitmaps[i]->ymove        = 0;
				bitmaps[i]->bitmap       = NULL;
				listFlags[i] = TRUE;
		}
		XFreeFont(display,font_struct);
			// build display lists
		Font font;
		font = XLoadFont(display, sname);
		glXUseXFont(font, 0, 255, list);	// Builds 255 bitmap characters
		XUnloadFont(display,font);
	}
	else
	{
#ifndef NDEBUG
			fprintf(stdout,"Font can't be generated.\n");
#endif
	}
}
#endif //__GLX

#ifndef __WGL
#ifndef __GLX
#error "CreateSysFont() must be implemented when using system fonts... "
#endif
#endif

#endif//__no_libFL
#endif//__sys_fonts
