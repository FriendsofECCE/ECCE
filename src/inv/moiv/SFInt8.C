/*
 * SFInt8.c++
 *
 *     Classes:  SFInt8, SFUInt8
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

#ident "$Revision: 22141 $"

#include "inv/ChemKit/SFInt8.H"

//////////////////////////////////////////////////////////////////
// Class SFInt8

// Defines all required member variables and functions for a
// single-value field
SO_SFIELD_SOURCE(SFInt8, int8_t, int8_t);

//
// This reads the value of a int8_t field from a
// file. It returns FALSE if the value could not be read
// successfully.
//

SbBool
SFInt8::readValue(SoInput *in)
{
   // Read a int8_t from the input
   SbBool rtn;
   short tmpValue;
   if (in->read(tmpValue)) {
      value = (int8_t)tmpValue;
      rtn = TRUE;
   }
   else {
      value = 0;
      rtn = FALSE;
   }
   return rtn;
}

//
// This writes the value of a int8_t field to a
// file.
//

void
SFInt8::writeValue(SoOutput *out) const
{
   // Write a int8_t
   short tmpValue = (short)value;
   out->write(tmpValue);
}



//////////////////////////////////////////////////////////////////
// Class SFUInt8

// Defines all required member variables and functions for a
// single-value field
SO_SFIELD_SOURCE(SFUInt8, uint8_t, uint8_t);

//
// This reads the value of a uint8_t field from a
// file. It returns FALSE if the value could not be read
// successfully.
//

SbBool
SFUInt8::readValue(SoInput *in)
{
   // Read a uint8_t from the input
   SbBool rtn;
   short tmpValue;
   if (in->read(tmpValue)) {
      value = (uint8_t)tmpValue;
      rtn = TRUE;
   }
   else {
      value = 0;
      rtn = FALSE;
   }
   return rtn;
}

//
// This writes the value of a uint8_t field to a
// file.
//

void
SFUInt8::writeValue(SoOutput *out) const
{
   // Write a uint8_t
   short tmpValue = (short)value;
   out->write(tmpValue);
}
