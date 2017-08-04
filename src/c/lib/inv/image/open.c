/*
 *	iopen -
 *
 *				Paul Haeberli - 1984
 *
 */
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<stdarg.h>
#include	"image.h"

void cvtlongs(int buffer[],int n);
void cvtimage(int buffer[]);
void i_seterror(void (*func)());

IMAGE *imgopen(int, char *, char *,unsigned int, unsigned int,
		unsigned int, unsigned int, unsigned int);

IMAGE *iopen(char *file, char *mode, unsigned int type, unsigned int dim,
		unsigned int xsize, unsigned int ysize, unsigned int zsize)
{
    return(imgopen(0, file, mode, type, dim, xsize, ysize, zsize));
}

IMAGE *fiopen(int f, char *mode, unsigned int type, unsigned int dim,
		unsigned int xsize, unsigned int ysize, unsigned int zsize)
{
    return(imgopen(f, 0, mode, type, dim, xsize, ysize, zsize));
}

IMAGE *imgopen(int f, char *file, char *mode,
		unsigned int type, unsigned int dim,
		unsigned int xsize, unsigned int ysize, unsigned int zsize)
{
	register IMAGE 	*image;
	register int rw;
	int tablesize;
	register int i, max;

	image = (IMAGE*)calloc(1,sizeof(IMAGE));
	if(!image ) {
	    i_errhdlr("iopen: error on image struct alloc\n");
	    return NULL;
	}
	rw = mode[1] == '+';
	if(rw) {
	    free(image);
	    i_errhdlr("iopen: read/write mode not supported\n");
	    return NULL;
	}
	if (*mode=='w') {
		if (file) {
		    f = creat(file, 0666);
		    if (rw && f>=0) {
			    close(f);
			    f = open(file, 2);
		    }
		}
		if (f < 0) {
		    free(image);
		    i_errhdlr("iopen: can't open output file %s\n",file);
		    return NULL;
		}
		image->imagic = IMAGIC;
		image->type = type;
		image->xsize = xsize;
		image->ysize = 1;
		image->zsize = 1;
		if (dim>1)
		    image->ysize = ysize;
		if (dim>2)
		    image->zsize = zsize;
		if(image->zsize == 1) {
		    image->dim = 2;
		    if(image->ysize == 1)
			image->dim = 1;
		} else {
		    image->dim = 3;
		}
		image->min = 10000000;
		image->max = 0;
		isetname(image,"no name"); 
		image->wastebytes = 0;
		image->dorev = 0;
		if (write(f,image,sizeof(IMAGE)) != sizeof(IMAGE)) {
		    free(image);
		    close(f);
		    i_errhdlr("iopen: error on write of image header\n");
		    return NULL;
		}
	} else {
		if (file)
		    f = open(file, rw? 2: 0);
		if (f < 0) {
		    free(image);
		    return(NULL);
		}
		if (read(f,image,sizeof(IMAGE)) != sizeof(IMAGE)) {
		    free(image);
		    close(f);
		    i_errhdlr("iopen: error on read of image header\n");
		    return NULL;
		}
		if( ((image->imagic>>8) | ((image->imagic&0xff)<<8)) 
							     == IMAGIC ) {
		    image->dorev = 1;
		    cvtimage((int*)image);
		} else
		    image->dorev = 0;
		if (image->imagic != IMAGIC) {
		    long imagic = image->imagic;
		    free(image);
		    close(f);
		    i_errhdlr("iopen: bad magic in image file %x\n",imagic);
		    return NULL;
		}
	}
	if (rw)
	    image->flags = _IORW;
	else if (*mode != 'r')
	    image->flags = _IOWRT;
	else
	    image->flags = _IOREAD;
	if(ISRLE(image->type)) {
	    tablesize = image->ysize*image->zsize*sizeof(int);
	    image->rowstart = (unsigned long *)malloc(tablesize);
	    image->rowsize = (long *)malloc(tablesize);
	    if( image->rowstart == 0 || image->rowsize == 0 ) {
		free(image);
		close(f);
		i_errhdlr("iopen: error on table alloc\n");
		return NULL;
	    }
	    image->rleend = 512 + 2*tablesize;
	    if (*mode=='w') {
		max = image->ysize*image->zsize;
		for(i=0; i<max; i++) {
		    image->rowstart[i] = 0;
		    image->rowsize[i] = -1;
		}
	    } else {
		tablesize = image->ysize*image->zsize*sizeof(int);
		lseek(f, 512L, 0);
		if (read(f,image->rowstart,tablesize) != tablesize) {
		    free(image);
		    close(f);
		    i_errhdlr("iopen: error on read of rowstart\n");
		    return NULL;
		}
		if(image->dorev)
		    cvtlongs((int *)image->rowstart,tablesize);
		if (read(f,image->rowsize,tablesize) != tablesize) {
		    free(image);
		    close(f);
		    i_errhdlr("iopen: error on read of rowsize\n");
		    return NULL;
		}
		if(image->dorev)
		    cvtlongs((int *)image->rowsize,tablesize);
	    }
	}
	image->cnt = 0;
	image->ptr = 0;
	image->base = 0;
	if( (image->tmpbuf = ibufalloc(image)) == 0 ) {	
	    int xsize = image->xsize;
	    free(image);
	    close(f);
	    i_errhdlr("iopen: error on tmpbuf alloc %d\n",xsize);
	    return NULL;
	}
	image->x = image->y = image->z = 0;
	image->file = f;
	image->offset = 512;			/* set up for img_optseek */
	lseek(image->file, 512L, 0);
	return(image);
}

unsigned short *ibufalloc(IMAGE *image)
{
#if 000
fprintf(stderr, "open.c:ibufalloc want to malloc size (%u)\n", IBUFSIZE(image->xsize));
#endif
    return (unsigned short *)malloc(IBUFSIZE(image->xsize));
#if 000
fprintf(stderr, "open.c:ibufalloc malloc succesful\n");
#endif
}

int reverse(lwrd) 
register unsigned int lwrd;
{
    return ((lwrd>>24) 		| 
	   (lwrd>>8 & 0xff00) 	| 
	   (lwrd<<8 & 0xff0000) | 
	   (lwrd<<24) 		);
}

void
cvtshorts( buffer, n)
register unsigned short buffer[];
register int n;
{
    register short i;
    register int nshorts = n>>1;
    register unsigned short swrd;

    for(i=0; i<nshorts; i++) {
	swrd = *buffer;
	*buffer++ = (swrd>>8) | (swrd<<8);
    }
}

void
cvtlongs( buffer, n)
register int buffer[];
register int n;
{
    register short i;
    register int nlongs = n>>2;
    register unsigned int lwrd;

    for(i=0; i<nlongs; i++) {
	lwrd = buffer[i];
	buffer[i] =     ((lwrd>>24) 		| 
	   		(lwrd>>8 & 0xff00) 	| 
	   		(lwrd<<8 & 0xff0000) 	| 
	   		(lwrd<<24) 		);
    }
}

void
cvtimage( buffer )
register int buffer[];
{
    cvtshorts((unsigned short *)buffer,12);
    cvtlongs(buffer+3,12);
    cvtlongs(buffer+26,4);
}

static void (*i_errfunc)();

/*	error handler for the image library.  If the iseterror() routine
	has been called, sprintf's the args into a string and calls the
	error function.  Otherwise calls fprintf with the args and then
	exit.  This allows 'old' programs to assume that no errors
	ever need be worried about, while programs that know how and
	want to can handle the errors themselves.  Olson, 11/88
*/
#if 000
int
i_errhdlr(fmt, a1, a2, a3, a4)	/* most args currently used is 2 */
char *fmt;
{
#else
int i_errhdlr(char *fmt, ...)
{
        /* GDB 2/9/12  Turned this into a proper varargs method so I could create
           a function prototype and eliminate compiler warnings */
        va_list argp;
        va_start(argp, fmt);
#endif

	if(i_errfunc) {
		char ebuf[2048];	/* be generous; if an error includes a
			pathname, the maxlen is 1024, so we shouldn't ever 
			overflow this! */
#if 000
		sprintf(ebuf, fmt, a1, a2, a3, a4);
#else
		sprintf(ebuf, fmt, argp);
#endif
		(*i_errfunc)(ebuf);
		return 0;
	}
#if 000
	fprintf(stderr, fmt, a1, a2, a3, a4);
#else
	fprintf(stderr, fmt, argp);
#endif
	exit(1);
	/*NOTREACHED*/
}

/* this function sets the error handler for i_errhdlr */
void
i_seterror(func)
void (*func)();
{
	i_errfunc = func;
}
