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
 |	This file defines the SoAntiSquish node class.
 |
 |   Author(s)		: Paul Isaacs
 |
 ______________  S I L I C O N   G R A P H I C S   I N C .  ____________
 _______________________________________________________________________
 */

#ifndef  _SO_ANTI_SQUISH_
#define  _SO_ANTI_SQUISH_

#include "inv/fields/SoFieldData.hpp"
#include "inv/nodes/SoSubNode.hpp"
#include "inv/nodes/SoTransformation.hpp"
#include "inv/fields/SoSFBool.hpp"
#include "inv/fields/SoSFEnum.hpp"

//////////////////////////////////////////////////////////////////////////////
//
//  Class: SoAntiSquish
//
//  undoes any uneven scales in the current transformation matrix.
//  during traversal
//
//////////////////////////////////////////////////////////////////////////////

// C-api: public=sizing
class SoAntiSquish : public SoTransformation {

    SO_NODE_HEADER(SoAntiSquish);

  public:

    // Constructor
    SoAntiSquish();

    enum Sizing {
	X,
	Y,
	Z,
	AVERAGE_DIMENSION,
	BIGGEST_DIMENSION,
	SMALLEST_DIMENSION,
	LONGEST_DIAGONAL
    };

    // Fields
        SoSFEnum		sizing;
	// If TRUE, recalculates every time the ctm changes.
	// If FALSE, recalculates only after the recalc() method is called.
	// Default is TRUE
        SoSFBool		recalcAlways;

    // If recalcAlways field is FALSE, call this method to force a new matrix
    // to be calculated.
    void recalc();

  SoEXTENDER public:
    // Called by actions that need to change the state with a new matrix.
    void		doAction(SoAction *action);

  SoINTERNAL public:
    static void		initClass();

  protected:
    virtual void	callback(SoCallbackAction *action);
    virtual void	GLRender(SoGLRenderAction *action);
    virtual void	getBoundingBox(SoGetBoundingBoxAction *action);
    virtual void	getMatrix(SoGetMatrixAction *action);
    virtual void	pick(SoPickAction *action);

    virtual ~SoAntiSquish();

    SbMatrix getUnsquishingMatrix( SbMatrix squishedMatrix, SbBool doInverse,
				   SbMatrix &inverseAnswer );

  private:
    SbMatrix savedAnswer, savedInverseAnswer;

    SbBool recalcNextTime;
};

#endif /* _SO_ANTI_SQUISH_ */