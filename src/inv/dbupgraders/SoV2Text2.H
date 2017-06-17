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
 |   $Revision: 22147 $
 |
 |   Description:
 |	Upgrade from V2.0 SoText node to V2.1.  Necessary because the
 |      text in 2.0 string field could be European (non-ascii)
 |
 |   Author(s)		: Alan Norton
 |
 ______________  S I L I C O N   G R A P H I C S   I N C .  ____________
 _______________________________________________________________________
 */

#ifndef  _SO_V2TEXT_2_
#define  _SO_V2TEXT_2_

#include "inv/fields/SoMFString.H"
#include "inv/fields/SoSFEnum.H"
#include "inv/fields/SoSFBitMask.H"
#include "inv/fields/SoSFFloat.H"
#include "inv/upgraders/SoUpgrader.H"

class SoV2Text2 : public SoUpgrader {

    SO_NODE_HEADER(SoV2Text2);

  public:
    enum Justification {		// Justification types
	LEFT	= 0x01,
	RIGHT	= 0x02,
	CENTER	= 0x03
    };

    // Fields
    SoMFString		string;		// the strings to display
    SoSFFloat		spacing;	// interval between strings    SoSFBitMask		parts;		// Visible parts of text
    SoSFEnum		justification;

    // Constructor
    SoV2Text2();
    
 
  SoINTERNAL public:
    static void		initClass();
    virtual SoNode	*createNewNode();
    
    //Converter used by both V2Text2 and V2Text3,
    //upgrades european non-ascii to UTF-8, or
    //just makes it ascii (in pre Irix 6.2 versions):
    static SbBool	convertToUTF8(const SbString& inString, 
			    SbString& outString);

  protected:
    virtual ~SoV2Text2();
    
};

#endif /* _SO_V2TEXT_2_ */
