/*
    #define TYPE to be the data type
    #define CXTYPE to be the chopper type

    externally defined variables:

    long    ix[];           // array integer index
    float   fx[];           // array subsample position

    float   dst;           // destination vector
*/
{
    int     i,j,k;          // counting variables
    TYPE    *v;
    double  round_helper;

    switch (CXTYPE) {
      case LATTICE_DATA_INT8:
      case LATTICE_DATA_INT16:
      case LATTICE_DATA_INT32:
        round_helper = 0.499999;
        break;
      case LATTICE_DATA_FLOAT32:
      case LATTICE_DATA_FLOAT64:
      default:
        round_helper = 0.0;
        break;
    }
        
    /* fill up a buffer */
    int32_t n = 3;
    int32_t np = 1 << n;  // this will always be 8 for n = 3;
    TYPE b[8];            // this should be dimensioned np;

    /* offset to the base sample */
    int32_t offset = 0;
    int32_t stride = 1;
    for ( i = 0; i < n; i++ ) {
        offset += ix[i]*stride;
        stride *= dims[i];
    }

    /* pick out all the surrounding samples */
    for ( i = 0; i < np; i++ ) {
        v = (TYPE *)glob.colorValues +
            (offset + glob.colorDataOffset[i])*numColorVar + glob.colorVar;
        b[i] = *v;
    }

    for ( i = 0; i < n; i++ ) {
        for ( j = 0; j < np; j += 1 << (i + 1) ) {
            k = j + (1 << i);
            b[j] = (TYPE)((1.0 - fx[i])*b[j] 
                               + fx[i]*b[k]
                               + round_helper );
        }
    }

    dst = (float)b[0];
}

#undef TYPE
#undef CXTYPE
