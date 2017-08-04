/*
 *
 *  Copyright (C) 2000 Silicon Graphics, Inc.  All Rights Reserved. 
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  Further, this software is distributed without any warranty that it is
 *  free of the rightful claim of any third person regarding infringement
 *  or the like.  Any license provided herein, whether implied or
 *  otherwise, applies only to this software file.  Patent licenses, if
 *  any, provided herein do not apply to combinations of this program with
 *  other software, or any other product whatsoever.
 * 
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Contact information: Silicon Graphics, Inc., 1600 Amphitheatre Pkwy,
 *  Mountain View, CA  94043, or:
 * 
 *  http://www.sgi.com 
 * 
 *  For further information regarding this notice, see: 
 * 
 *  http://oss.sgi.com/projects/GenInfo/NoticeExplan/
 *
 */

#ifndef	__GL_IMAGE_H__
#define	__GL_IMAGE_H__
#ifdef __cplusplus
extern "C" {
#endif


/*
 *	Defines for image files . . . .
 *
 *  			Paul Haeberli - 1984
 *      Look in /usr/people/4Dgifts/iristools/imgtools for example code!
 *
 */

#include <stdio.h>

#define IMAGIC 	0732

/* colormap of images */
#define CM_NORMAL		0	/* file contains rows of values which 
					 * are either RGB values (zsize == 3) 
					 * or greyramp values (zsize == 1) */
#define CM_DITHERED		1
#define CM_SCREEN		2	/* file contains data which is a screen
					 * image; getrow returns buffer which 
					 * can be displayed directly with 
					 * writepixels */
#define CM_COLORMAP		3	/* a colormap file */

#define TYPEMASK		0xff00
#define BPPMASK			0x00ff
#define ITYPE_VERBATIM		0x0000
#define ITYPE_RLE		0x0100
#define ISRLE(type)		(((type) & 0xff00) == ITYPE_RLE)
#define ISVERBATIM(type)	(((type) & 0xff00) == ITYPE_VERBATIM)
#define BPP(type)		((type) & BPPMASK)
#define RLE(bpp)		(ITYPE_RLE | (bpp))
#define VERBATIM(bpp)		(ITYPE_VERBATIM | (bpp))
#define	IBUFSIZE(pixels)	((pixels+(pixels>>6))<<2)
#define	RLE_NOP			0x00

#define	ierror(p)		(((p)->flags&_IOERR)!=0)
#define	ifileno(p)		((p)->file)
#define	getpix(p)		(--(p)->cnt>=0 ? *(p)->ptr++ : ifilbuf(p))
#define putpix(p,x)		(--(p)->cnt>=0 \
				    ? ((int)(*(p)->ptr++=(unsigned)(x))) \
				    : iflsbuf(p,(unsigned)(x)))

typedef struct {
    unsigned short	imagic;		/* stuff saved on disk . . */
    unsigned short 	type;
    unsigned short 	dim;
    unsigned short 	xsize;
    unsigned short 	ysize;
    unsigned short 	zsize;
    unsigned long 	min;
    unsigned long 	max;
    unsigned long	wastebytes;	
    char 		name[80];
    unsigned long	colormap;

    long 		file;		/* stuff used in core only */
    unsigned short 	flags;
    short		dorev;
    short		x;
    short		y;
    short		z;
    short		cnt;
    unsigned short	*ptr;
    unsigned short	*base;
    unsigned short	*tmpbuf;
    unsigned long	offset;
    unsigned long	rleend;		/* for rle images */
    unsigned long	*rowstart;	/* for rle images */
    long		*rowsize;	/* for rle images */
} IMAGE;

IMAGE *icreate();
/*
 * IMAGE *iopen(char *file, char *mode, unsigned int type, unsigned int dim,
 * 		unsigned int xsize, unsigned int ysize, unsigned int zsize);
 * IMAGE *fiopen(int f, char *mode, unsigned int type, unsigned int dim,
 * 		unsigned int xsize, unsigned int ysize, unsigned int zsize);
 *
 * ...while iopen and fiopen can take an extended set of parameters, the 
 * last five are optional, so a more correct prototype would be:
 *
 * IMAGE *iopen(char *file, char *mode, ...);
 * IMAGE *fiopen(int f, char *mode, ...);
 * 
 * unsigned short *ibufalloc(IMAGE *image);
 * int ifilbuf(IMAGE *image);
 * int iflush(IMAGE *image);
 * unsigned int iflsbuf(IMAGE *image, unsigned int c);
 * void isetname(IMAGE *image, char *name);
 * void isetcolormap(IMAGE *image, int colormap);
 * int iclose(IMAGE *image);
 * 
 * int putrow(IMAGE *image, unsigned short *buffer, unsigned int y, unsigned int z);
 * int getrow(IMAGE *image, unsigned short *buffer, unsigned int y, unsigned int z);
 * 
 */

IMAGE *iopen();
IMAGE *icreate();
unsigned short *ibufalloc();

#if 111
/* GDB 2/9/12  Added prototypes to eliminate compiler warnings */
int iflush(IMAGE *image);
unsigned int img_optseek(IMAGE *image, unsigned int offset);
void cvtimage(int buffer[]);
int img_write(IMAGE *image, char *buffer,int count);
void cvtlongs(int buffer[],int n);
void cvtshorts(unsigned short buffer[],int n);
int getrow(IMAGE *image, unsigned short *buffer, unsigned int y, unsigned int z);
int putrow(IMAGE *image, unsigned short *buffer, unsigned int y, unsigned int z);
void isetname(IMAGE *image, char *name);
int ifilbuf(IMAGE *image);
unsigned int iflsbuf(IMAGE *image, unsigned int c);
int img_badrow(IMAGE *image, unsigned int y, unsigned int z);
unsigned int img_seek(IMAGE *image, unsigned int y, unsigned int z);
int img_rle_compact(unsigned short *expbuf, int ibpp, unsigned short *rlebuf, int obpp, int cnt);
void img_rle_expand(unsigned short *rlebuf, int ibpp, unsigned short *expbuf, int obpp);
void img_setrowsize(IMAGE *image, int cnt, int y, int z);
int img_getrowsize(IMAGE *image);
int img_read(IMAGE *image, char *buffer, int count);
int i_errhdlr(char *fmt, ...);
#endif

#define _IOEOF          0020    /* EOF reached on read */
#define _IOERR          0040    /* I/O error from system */
#define _IOREAD         0001    /* currently reading */
#define _IOWRT          0002    /* currently writing */
#define _IORW           0200    /* opened for reading and writing */

#define IMAGEDEF		/* for backwards compatibility */
#ifdef __cplusplus
}
#endif
#endif	/* !__GL_IMAGE_H__ */
