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
 * Copyright (C) 1990,91,92   Silicon Graphics, Inc.
 *
 _______________________________________________________________________
 ______________  S I L I C O N   G R A P H I C S   I N C .  ____________
 |
 |   $Revision: 22148 $
 |
 |   Description:
 |	This file contains the definition of the SbCylinderProjector
 |	class. This is an abstract base class for projectors
 |	that somehow use a cylinder in their projection.
 |
 |	The getRotation methods for an SbCylinder will always
 |	return a rotation that is about the axis of the cylinder.
 |	  
 |   Author(s)		: Howard Look
 |
 ______________  S I L I C O N   G R A P H I C S   I N C .  ____________
 _______________________________________________________________________
 */

#ifndef _SB_CYLINDER_PROJECTOR_
#define _SB_CYLINDER_PROJECTOR_

#include "inv/projectors/SbProjector.H"

// C-api: abstract
// C-api: prefix=SbCylProj
class SbCylinderProjector : public SbProjector
{
  public:
    // Destructor
    ~SbCylinderProjector(){};

    // Apply the projector using the given point, returning the
    // point in three dimensions that it projects to.
    // The point should be normalized from 0-1, with (0,0) at the lower-left.
    // C-api: expose
    virtual SbVec3f	project(const SbVec2f &point) = 0;

    // Same as above, but also get a rotation from the last
    // projected point to this one.
    // C-api: name=projectGetRot
    SbVec3f		projectAndGetRotation(const SbVec2f &point,
					      SbRotation &rot);

    // Get a rotation given two points on this cylinder projector.
    // C-api: expose
    // C-api: name=getRot
    virtual SbRotation	getRotation(const SbVec3f &point1,
				    const SbVec3f &point2) = 0;

    // Set/get the cylinder to use.
    // Default cylinder is centered about Y axis and has radius 1.0.
    // C-api: name=setCyl
    void		setCylinder(const SbCylinder &cyl);

    // C-api: name=getCyl
    const SbCylinder &	getCylinder() const		{ return cylinder; }

    // Set/get whether the projector should always be oriented towards the eye.
    // Set to FALSE if the tolerance should be evaluated in working space.
    void		setOrientToEye(SbBool orientToEye);
    SbBool		isOrientToEye() const		{ return orientToEye; }


    // Set/get whether the projector should intersect the half of the
    // cylinder that faces the eye.
    // Set to FALSE if the projector should intersect with the rear half.
    void                setFront(SbBool isFront);
    SbBool              isFront()       const   { return intersectFront; }
    SbBool              isPointInFront( const SbVec3f &point ) const;


    // Set the transform space to work in.
    virtual void	setWorkingSpace(const SbMatrix &space);
    
  protected:
    // Constructors
    // The default cylinder to is centered about the Y axis and
    // has a radius of 1.0.
    SbCylinderProjector(SbBool orientToEye);
    SbCylinderProjector(const  SbCylinder &cyl,
			SbBool orientToEye);

    // Are intersections done on the front half ( if not, they're done on th e
    // back half) of the cylinder?
    SbBool      intersectFront;

    // Just like cylinder::intersect, except always returns
    // the intersection that is in "front".
    SbBool	intersectCylinderFront(const SbLine &line, SbVec3f &result);

    SbCylinder	cylinder;		// Cylinder for this projector.
    SbBool	orientToEye;		// TRUE if always oriented to eye.
    SbBool	needSetup;		// Set TRUE whenever cylinder,
					// work space or orientation changes.

    // Cached last point on this projector. Subclasses must set
    // this in their project and getRotation methods.
    SbVec3f	lastPoint;
};


#endif /* _SB_CYLINDER_PROJECTOR_ */
