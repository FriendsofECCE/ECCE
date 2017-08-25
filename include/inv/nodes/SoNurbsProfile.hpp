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
 |   Description:
 |	This file defines the SoNurbsProfile node class.
 |
 |   Author(s)		: Thad Beier, Dave Immel, Paul Strauss
 |
 ______________  S I L I C O N   G R A P H I C S   I N C .  ____________
 _______________________________________________________________________
 */

#ifndef  _SO_NURBS_PROFILE_
#define  _SO_NURBS_PROFILE_

#include "inv/fields/SoMFFloat.H"
#include "inv/nodes/SoProfile.H"

//////////////////////////////////////////////////////////////////////////////
//
//  Class: SoNurbsProfile
//
//
//////////////////////////////////////////////////////////////////////////////

// C-api: prefix=SoNProf
// C-api: public=index, linkage, knotVector
class SoNurbsProfile : public SoProfile{

    SO_NODE_HEADER(SoNurbsProfile);

  public:
    // Fields (in addition to those inherited from parent class)
    SoMFFloat		knotVector;

    // Constructor
    SoNurbsProfile();

  SoEXTENDER public:
    // Returns a profile as a trim curve. floatsPerVec will be either
    // 2 or 3 depending on whether the curve is non-rational or
    // rational.
    virtual void	getTrimCurve(SoState *state, int32_t &numPoints,
				     float *&points, int &floatsPerVec,
				     int32_t &numKnots, float *&knotVector);

    // Returns vertices approximating the profile
    virtual void	getVertices(SoState *state, int32_t &nVertices,
				    SbVec2f *&vertices);

  SoINTERNAL public:
    static void		initClass();

  protected:
    virtual ~SoNurbsProfile();
};

#endif /* _SO_NURBS_PROFILE_ */
