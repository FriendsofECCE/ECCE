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

//  -*- C++ -*-

/*
 * Copyright (C) 1990,91   Silicon Graphics, Inc.
 *
 _______________________________________________________________________
 ______________  S I L I C O N   G R A P H I C S   I N C .  ____________
 |
 |   $Revision: 22148 $
 |
 |   Classes:
 |	SoMFString
 |
 |   Author(s)		: Paul S. Strauss
 |
 ______________  S I L I C O N   G R A P H I C S   I N C .  ____________
 _______________________________________________________________________
 */

#ifndef  _SO_MF_STRING_
#define  _SO_MF_STRING_

#include "inv/fields/SoSubField.H"

//////////////////////////////////////////////////////////////////////////////
//
//  SoMFString subclass of SoMField.
//
//////////////////////////////////////////////////////////////////////////////

// C-api: prefix=SoMStr
class SoMFString : public SoMField {
    // Use standard field stuff
    SO_MFIELD_HEADER(SoMFString, SbString, const SbString &);

  public:
    //
    // Some additional convenience functions:
    //

    // Set values from array of character strings
    // C-api: name=SetStrs
    void	setValues(int start, int num, const char *strings[]);

    // Set one value from character string
    // C-api: name=SetStr
    void	setValue(const char *string);

    // Convenience function to delete text from consecutive strings.
    // All text from the given character of one line (fromChar in
    // fromLine) to the given character of another line (toChar in
    // toLine), inclusive, is deleted. Any leftover text on fromLine
    // and toLine is merged into one line.
    void	deleteText(int fromLine, int fromChar, int toLine, int toChar);


  SoINTERNAL public:
    static void		initClass();
};

#endif /* _SO_MF_STRING_ */
