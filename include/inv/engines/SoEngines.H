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
 |	This file #includes files that define ALL standard Inventor
 |	engine classes. It is provided as a convenience.
 |
 |   Author(s)		: Paul S. Strauss, Gavin Bell, Ronen Barzel
 |
 |   Notes:
 |	Try not to include this file; use only those specific ones
 |	that you actually need.
 |
 ______________  S I L I C O N   G R A P H I C S   I N C .  ____________
 _______________________________________________________________________
 */

#ifndef  _SO_ENGINES_
#define  _SO_ENGINES_

#include "inv/engines/SoBoolOperation.H"
#include "inv/engines/SoCalculator.H"
#include "inv/engines/SoCompose.H"
#include "inv/engines/SoComputeBoundingBox.H"
#include "inv/engines/SoConcatenate.H"
#include "inv/engines/SoCounter.H"
#include "inv/engines/SoElapsedTime.H"
#include "inv/engines/SoFieldConverter.H"
#include "inv/engines/SoGate.H"
#include "inv/engines/SoInterpolate.H"
#include "inv/engines/SoOnOff.H"
#include "inv/engines/SoOneShot.H"
#include "inv/engines/SoSelectOne.H"
#include "inv/engines/SoTimeCounter.H"
#include "inv/engines/SoTransformVec3f.H"
#include "inv/engines/SoTriggerAny.H"

#endif /* _SO_ENGINES_ */
