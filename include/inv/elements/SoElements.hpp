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

//-*- C++ -*-

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
 |	element classes. It is provided as a convenience.
 |
 |   Author(s)		: Paul S. Strauss
 |
 |   Notes:
 |	Try not to include this file; use only those specific ones
 |	that you actually need.
 |
 ______________  S I L I C O N   G R A P H I C S   I N C .  ____________
 _______________________________________________________________________
 */

#ifndef  _SO_ELEMENTS_
#define  _SO_ELEMENTS_

#include "inv/elements/SoBBoxModelMatrixElement.H"
#include "inv/elements/SoCacheElement.H"
#include "inv/elements/SoClipPlaneElement.H"
#include "inv/elements/SoComplexityElement.H"
#include "inv/elements/SoComplexityTypeElement.H"
#include "inv/elements/SoCoordinateElement.H"
#include "inv/elements/SoCreaseAngleElement.H"
#include "inv/elements/SoDrawStyleElement.H"
#include "inv/elements/SoFloatElement.H"
#include "inv/elements/SoFocalDistanceElement.H"
#include "inv/elements/SoFontNameElement.H"
#include "inv/elements/SoFontSizeElement.H"
#include "inv/elements/SoGLCacheContextElement.H"
#include "inv/elements/SoGLClipPlaneElement.H"
#include "inv/elements/SoGLCoordinateElement.H"
#include "inv/elements/SoGLDrawStyleElement.H"
#include "inv/elements/SoGLLazyElement.H"
#include "inv/elements/SoGLLightIdElement.H"
#include "inv/elements/SoGLLinePatternElement.H"
#include "inv/elements/SoGLLineWidthElement.H"
#include "inv/elements/SoGLModelMatrixElement.H"
#include "inv/elements/SoGLNormalElement.H"
#include "inv/elements/SoGLPointSizeElement.H"
#include "inv/elements/SoGLProjectionMatrixElement.H"
#include "inv/elements/SoGLRenderPassElement.H"
#include "inv/elements/SoGLShapeHintsElement.H"
#include "inv/elements/SoGLTextureCoordinateElement.H"
#include "inv/elements/SoGLTextureEnabledElement.H"
#include "inv/elements/SoGLTextureImageElement.H"
#include "inv/elements/SoGLTextureMatrixElement.H"
#include "inv/elements/SoGLUpdateAreaElement.H"
#include "inv/elements/SoGLViewingMatrixElement.H"
#include "inv/elements/SoGLViewportRegionElement.H"
#include "inv/elements/SoInt32Element.H"
#include "inv/elements/SoLightAttenuationElement.H"
#include "inv/elements/SoLinePatternElement.H"
#include "inv/elements/SoLineWidthElement.H"
#include "inv/elements/SoLocalBBoxMatrixElement.H"
#include "inv/elements/SoMaterialBindingElement.H"
#include "inv/elements/SoModelMatrixElement.H"
#include "inv/elements/SoNormalBindingElement.H"
#include "inv/elements/SoNormalElement.H"
#include "inv/elements/SoOverrideElement.H"
#include "inv/elements/SoPickRayElement.H"
#include "inv/elements/SoPickStyleElement.H"
#include "inv/elements/SoPointSizeElement.H"
#include "inv/elements/SoProfileCoordinateElement.H"
#include "inv/elements/SoProfileElement.H"
#include "inv/elements/SoProjectionMatrixElement.H"
#include "inv/elements/SoShapeHintsElement.H"
#include "inv/elements/SoShapeStyleElement.H"
#include "inv/elements/SoSwitchElement.H"
#include "inv/elements/SoTextureCoordinateBindingElement.H"
#include "inv/elements/SoTextureCoordinateElement.H"
#include "inv/elements/SoTextureImageElement.H"
#include "inv/elements/SoTextureMatrixElement.H"
#include "inv/elements/SoTextureQualityElement.H"
#include "inv/elements/SoUnitsElement.H"
#include "inv/elements/SoViewVolumeElement.H"
#include "inv/elements/SoViewingMatrixElement.H"
#include "inv/elements/SoViewportRegionElement.H"
#include "inv/elements/SoWindowElement.H"

#endif /* _SO_ELEMENTS_ */
