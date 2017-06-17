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
 * Copyright (C) 1990-95   Silicon Graphics, Inc.
 *
 _______________________________________________________________________
 ______________  S I L I C O N   G R A P H I C S   I N C .  ____________
 |
 |   $Revision: 22147 $
 |
 |   Description:
 |	Upgrade from 1.0 PackedColor format to the 2.0 format
 |
 |   Author(s)		: David Mott
 |
 ______________  S I L I C O N   G R A P H I C S   I N C .  ____________
 _______________________________________________________________________
 */

#ifndef  _SO_V1PACKEDCOLOR_
#define  _SO_V1PACKEDCOLOR_

#include "inv/upgraders/SoUpgrader.H"
#include "inv/fields/SoMFUInt32.H"

//////////////////////////////////////////////////////////////////////////////
//
//  Class: SoV1PackedColor
//
//  V1PackedColor for upgrading 1.0 and 2.0 PackedColors to the 2.1 file format.
//
//////////////////////////////////////////////////////////////////////////////

class SoV1PackedColor : public SoUpgrader {

    SO_NODE_HEADER(SoV1PackedColor);

  public:
    // Fields
    SoMFUInt32		rgba;   

    // Constructor
    SoV1PackedColor();

  SoINTERNAL public:
    static void		initClass();

    virtual SoNode	*createNewNode();

  protected:
    virtual ~SoV1PackedColor();
};

#endif /* _SO_V1PACKEDCOLOR_ */
